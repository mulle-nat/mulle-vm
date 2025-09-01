#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                       total_pages;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   mulle_fprintf( stderr, "%lu pages, %lu bytes\n", total_pages, info.vm_size);
}


static void   dump_vm_content( struct mulle_vm *vm)
{
   char         buf[1024];
   const void   *s;
   struct mulle_vm_statistics   info;

   info = mulle_vm_get_statistics( vm);
   s    = mulle_vm_read( vm, mulle_range_make( 0, info.vm_size), buf);
   if( s)
   {
      mulle_fprintf( stderr, "VM content: \"");
      fwrite( s, 1, info.vm_size, stdout);
      mulle_fprintf( stderr, "\"\n");
   }
}


static void   test_mulle_vm_memrchr_empty( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memrchr with empty VM ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, &mulle_default_allocator);
   show_vm( &vm);

   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a' in empty VM: ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %lu)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_single_page( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memrchr with single page ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   mulle_vm_append( &vm, "Hello", 5);
   show_vm( &vm);
   dump_vm_content( &vm);

   // Test 1: Find last occurrence of 'l'
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'l');
   mulle_printf( "Find last 'l': ");
   if( result == 3)  // Last 'l' is at position 3 in "Hello"
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 3, got %lu)\n", result);

   // Test 2: Find first character (which is also last)
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'H');
   mulle_printf( "Find 'H': ");
   if( result == 0)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 0, got %lu)\n", result);

   // Test 3: Find last character 'o'
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'o');
   mulle_printf( "Find 'o': ");
   if( result == 4)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 4, got %lu)\n", result);

   // Test 4: Character not found
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'z');
   mulle_printf( "Find 'z': ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %lu)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_multiple_pages( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memrchr with multiple pages ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create multiple pages with repeating characters
   mulle_vm_append( &vm, "abcd", 4);  // Page 1: positions 0-3
   mulle_vm_append( &vm, "efgh", 4);  // Page 2: positions 4-7
   mulle_vm_append( &vm, "ijkl", 4);  // Page 3: positions 8-11
   mulle_vm_append( &vm, "mnop", 4);  // Page 4: positions 12-15
   mulle_vm_append( &vm, "qrst", 4);  // Page 5: positions 16-19
   mulle_vm_append( &vm, "uvwa", 4);  // Page 6: positions 20-23 (note 'a' at end)
   mulle_vm_append( &vm, "xyza", 4);  // Page 7: positions 24-27 (note 'a' at end)
   
   show_vm( &vm);
   dump_vm_content( &vm);

   // Test 1: Find last occurrence of 'a' (should be at position 27)
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'a');
   mulle_printf( "Find last 'a': ");
   if( result == 27)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 27, got %lu)\n", result);

   // Test 2: Find character in middle page
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'k');
   mulle_printf( "Find 'k' (middle page): ");
   if( result == 10)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 10, got %lu)\n", result);

   // Test 3: Find character in first page
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'b');
   mulle_printf( "Find 'b' (first page): ");
   if( result == 1)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 1, got %lu)\n", result);

   // Test 4: Character not found
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'A');
   mulle_printf( "Find 'A' (not found): ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %lu)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_page_boundaries( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memrchr at page boundaries ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Set up data where target characters are at page boundaries
   mulle_vm_append( &vm, "abcdefgT", 8);  // 'T' at end of first page (pos 7)
   mulle_vm_append( &vm, "Tijklmno", 8);  // 'T' at start of second page (pos 8)
   mulle_vm_append( &vm, "pqrsTuvw", 8);  // 'T' in middle of third page (pos 20)
   
   show_vm( &vm);
   dump_vm_content( &vm);

   // Should find the last 'T' at position 20 (reverse search)
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'T');
   mulle_printf( "Find last 'T': ");
   if( result == 20)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 20, got %lu)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_null_vm( void)
{
   uintptr_t   result;

   mulle_printf( "== Test mulle_vm_memrchr with NULL VM ==\n");
   
   result = mulle_vm_memrchr( NULL, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a' in NULL VM: ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %lu)\n", result);

   mulle_printf( "\n");
}


static void   test_mulle_vm_memrchr_multiple_occurrences( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memrchr with multiple occurrences ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create pattern with repeated characters across pages
   mulle_vm_append( &vm, "XabcXdef", 8);  // 'X' at pos 0 and 4
   mulle_vm_append( &vm, "XghiXjkl", 8);  // 'X' at pos 8 and 12
   mulle_vm_append( &vm, "XmnoXpqr", 8);  // 'X' at pos 16 and 20
   
   show_vm( &vm);
   dump_vm_content( &vm);

   // Should find the last 'X' at position 20
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'X');
   mulle_printf( "Find last 'X': ");
   if( result == 20)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 20, got %lu)\n", result);

   // Test with other repeated characters
   result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a': ");
   if( result == 1)
      mulle_printf( "PASS (found at %lu)\n", result);
   else
      mulle_printf( "FAIL (expected 1, got %lu)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_comprehensive( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;
   char              test_chars[] = {'a', 'e', 'i', 'm', 'q', 'u', 'x', 'Z'};
   uintptr_t         expected[] = {24, 4, 8, 14, 17, 21, 25, mulle_not_found_e};
   int               i;

   mulle_printf( "== Test mulle_vm_memrchr comprehensive ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create pattern where some characters appear multiple times
   mulle_vm_append( &vm, "abcd", 4);    // positions 0-3
   mulle_vm_append( &vm, "efgh", 4);    // positions 4-7
   mulle_vm_append( &vm, "ijkl", 4);    // positions 8-11
   mulle_vm_append( &vm, "mnmop", 5);   // positions 12-16 ('m' at 12 and 14)
   mulle_vm_append( &vm, "qrst", 4);    // positions 17-20
   mulle_vm_append( &vm, "uvwa", 4);    // positions 21-24 ('a' at 24)
   mulle_vm_append( &vm, "xyz", 3);     // positions 25-27
   
   show_vm( &vm);
   dump_vm_content( &vm);

   for( i = 0; i < sizeof(test_chars); i++)
   {
      result = mulle_vm_memrchr( &vm, mulle_range_make_all(), test_chars[i]);
      mulle_printf( "Find last '%c': ", test_chars[i]);
      if( result == expected[i])
         mulle_printf( "PASS");
      else
         mulle_printf( "FAIL (expected %lu, got %lu)", expected[i], result);
      
      if( result != mulle_not_found_e)
         mulle_printf( " (found at %lu)", result);
      else
         mulle_printf( " (not found)");
      mulle_printf( "\n");
   }

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memrchr_vs_memchr( void)
{
   struct mulle_vm   vm;
   uintptr_t         memchr_result;
   uintptr_t         memrchr_result;

   mulle_printf( "== Test mulle_vm_memrchr vs mulle_vm_memchr ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create pattern with multiple occurrences to show the difference
   mulle_vm_append( &vm, "TestTesT", 8);  // 'T' at positions 0, 4, 7
   
   show_vm( &vm);
   dump_vm_content( &vm);

   memchr_result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'T');
   memrchr_result = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'T');
   
   mulle_printf( "Forward search for 'T': ");
   if( memchr_result == 0)
      mulle_printf( "PASS (found at %lu)\n", memchr_result);
   else
      mulle_printf( "FAIL (expected 0, got %lu)\n", memchr_result);
      
   mulle_printf( "Reverse search for 'T': ");
   if( memrchr_result == 7)
      mulle_printf( "PASS (found at %lu)\n", memrchr_result);
   else
      mulle_printf( "FAIL (expected 7, got %lu)\n", memrchr_result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


int   main( void)
{
   mulle_printf( "Testing mulle_vm_memrchr function\n");
   mulle_printf( "=================================\n\n");

   test_mulle_vm_memrchr_empty();
   test_mulle_vm_memrchr_single_page();
   test_mulle_vm_memrchr_multiple_pages();
   test_mulle_vm_memrchr_page_boundaries();
   test_mulle_vm_memrchr_null_vm();
   test_mulle_vm_memrchr_multiple_occurrences();
   test_mulle_vm_memrchr_comprehensive();
   test_mulle_vm_memrchr_vs_memchr();

   mulle_printf( "All mulle_vm_memrchr tests completed.\n");
   return( 0);
}
