#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                 total_pages;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   mulle_printf( "%ld pages, %ld bytes\n", total_pages, info.vm_size);
}


static void   dump_vm_content( struct mulle_vm *vm)
{
   char                         buf[1024];
   const void                   *s;
   struct mulle_vm_statistics   info;

   info = mulle_vm_get_statistics( vm);
   s = mulle_vm_read( vm, mulle_range_make( 0, info.vm_size), buf);
   if( s)
   {
      mulle_printf( "VM content: \"");
      fwrite( s, 1, info.vm_size, stdout);
      mulle_printf( "\"\n");
   }
}


static void   test_mulle_vm_memchr_empty( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memchr with empty VM ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   show_vm( &vm);

   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a' in empty VM: ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %ld)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memchr_single_page( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memchr with single page ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, &mulle_default_allocator);
   
   mulle_vm_append( &vm, "Hello World", 11);
   show_vm( &vm);
   dump_vm_content( &vm);

   // Test 1: Find first occurrence
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'o');
   mulle_printf( "Find 'o': ");
   if( result == 4)  // First 'o' is at position 4
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 4, got %ld)\n", result);

   // Test 2: Find first character
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'H');
   mulle_printf( "Find 'H': ");
   if( result == 0)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 0, got %ld)\n", result);

   // Test 3: Find last character
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'd');
   mulle_printf( "Find 'd': ");
   if( result == 10)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 10, got %ld)\n", result);

   // Test 4: Character not found
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'z');
   mulle_printf( "Find 'z': ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %ld)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memchr_multiple_pages( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memchr with multiple pages ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create multiple pages similar to delete-all.c
   mulle_vm_append( &vm, "abcd", 4);  // Page 1: positions 0-3
   mulle_vm_append( &vm, "efgh", 4);  // Page 2: positions 4-7
   mulle_vm_append( &vm, "ijkl", 4);  // Page 3: positions 8-11
   mulle_vm_append( &vm, "mnop", 4);  // Page 4: positions 12-15
   mulle_vm_append( &vm, "qrst", 4);  // Page 5: positions 16-19
   mulle_vm_append( &vm, "uvw", 3);   // Page 6: positions 20-22
   mulle_vm_append( &vm, "xyz", 3);   // Page 7: positions 23-25
   
   show_vm( &vm);
   dump_vm_content( &vm);

   // Test 1: Find character in first page
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a' (first page): ");
   if( result == 0)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 0, got %ld)\n", result);

   // Test 2: Find character in middle page
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'k');
   mulle_printf( "Find 'k' (middle page): ");
   if( result == 10)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 10, got %ld)\n", result);

   // Test 3: Find character in last page
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'z');
   mulle_printf( "Find 'z' (last page): ");
   if( result == 25)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 25, got %ld)\n", result);

   // Test 4: Find character that appears multiple times (should find first)
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'd');
   mulle_printf( "Find 'd' (first occurrence): ");
   if( result == 3)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 3, got %ld)\n", result);

   // Test 5: Character not found
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'A');
   mulle_printf( "Find 'A' (not found): ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %ld)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memchr_page_boundaries( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;

   mulle_printf( "== Test mulle_vm_memchr at page boundaries ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Set up data where target characters are at page boundaries
   mulle_vm_append( &vm, "abcdefgT", 8);  // 'T' at end of first page (pos 7)
   mulle_vm_append( &vm, "Tijklmno", 8);  // 'T' at start of second page (pos 8)
   mulle_vm_append( &vm, "pqrsTuvw", 8);  // 'T' in middle of third page (pos 20)
   
   show_vm( &vm);
   dump_vm_content( &vm);

   // Should find the first 'T' at position 7
   result = mulle_vm_memchr( &vm, mulle_range_make_all(), 'T');
   mulle_printf( "Find first 'T': ");
   if( result == 7)
      mulle_printf( "PASS (found at %ld)\n", result);
   else
      mulle_printf( "FAIL (expected 7, got %ld)\n", result);

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_mulle_vm_memchr_null_vm( void)
{
   uintptr_t   result;

   mulle_printf( "== Test mulle_vm_memchr with NULL VM ==\n");
   
   result = mulle_vm_memchr( NULL, mulle_range_make_all(), 'a');
   mulle_printf( "Find 'a' in NULL VM: ");
   if( result == mulle_not_found_e)
      mulle_printf( "PASS (not found)\n");
   else
      mulle_printf( "FAIL (expected not found, got %ld)\n", result);

   mulle_printf( "\n");
}


static void   test_mulle_vm_memchr_comprehensive( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;
   char              test_chars[] = {'a', 'e', 'i', 'm', 'q', 'u', 'x', 'Z'};
   uintptr_t         expected[] = {0, 4, 8, 12, 16, 20, 23, mulle_not_found_e};
   int               i;

   mulle_printf( "== Test mulle_vm_memchr comprehensive ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create the same pattern as multiple pages test
   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);
   mulle_vm_append( &vm, "mnop", 4);
   mulle_vm_append( &vm, "qrst", 4);
   mulle_vm_append( &vm, "uvw", 3);
   mulle_vm_append( &vm, "xyz", 3);
   
   show_vm( &vm);
   dump_vm_content( &vm);

   for( i = 0; i < sizeof(test_chars); i++)
   {
      result = mulle_vm_memchr( &vm, mulle_range_make_all(), test_chars[i]);
      mulle_printf( "Find '%c': ", test_chars[i]);
      if( result == expected[i])
         mulle_printf( "PASS");
      else
         mulle_printf( "FAIL (expected %ld, got %ld)", expected[i], result);
      
      if( result != mulle_not_found_e)
         mulle_printf( " (found at %ld)", result);
      else
         mulle_printf( " (not found)");
      mulle_printf( "\n");
   }

   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


int   main( void)
{
   mulle_printf( "Testing mulle_vm_memchr function\n");
   mulle_printf( "================================\n\n");

   test_mulle_vm_memchr_empty();
   test_mulle_vm_memchr_single_page();
   test_mulle_vm_memchr_multiple_pages();
   test_mulle_vm_memchr_page_boundaries();
   test_mulle_vm_memchr_null_vm();
   test_mulle_vm_memchr_comprehensive();

   mulle_printf( "All mulle_vm_memchr tests completed.\n");
   return( 0);
}
