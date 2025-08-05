#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



static void   test_mulle_memchr_basic( void)
{
   char    *test_string;
   void    *result;
   char    *expected;

   printf( "== Test mulle_memchr basic functionality ==\n");

   // Test 1: Simple character found
   test_string = "Hello World";
   result = mulle_memchr( test_string, 'o', strlen( test_string));
   expected = strchr( test_string, 'o');
   printf( "Find 'o' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 2: Character not found
   result = mulle_memchr( test_string, 'z', strlen( test_string));
   expected = strchr( test_string, 'z');
   printf( "Find 'z' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 3: First character
   result = mulle_memchr( test_string, 'H', strlen( test_string));
   expected = strchr( test_string, 'H');
   printf( "Find 'H' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 4: Last character
   result = mulle_memchr( test_string, 'd', strlen( test_string));
   expected = strchr( test_string, 'd');
   printf( "Find 'd' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   printf( "\n");
}


static void   test_mulle_memchr_edge_cases( void)
{
   char    *test_string;
   void    *result;
   char    *expected;

   printf( "== Test mulle_memchr edge cases ==\n");

   // Test 1: Empty string
   test_string = "";
   result = mulle_memchr( test_string, 'a', 0);
   printf( "Find 'a' in empty string: ");
   if( result == NULL)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 2: Null character
   test_string = "Hello\0World";
   result = mulle_memchr( test_string, '\0', 6);
   printf( "Find '\\0' in \"Hello\\0World\": ");
   if( result == (test_string + 5))
      printf( "PASS (found at offset 5)\n");
   else
      printf( "FAIL (expected offset 5, got %p)\n", result);

   // Test 3: Single character string
   test_string = "A";
   result = mulle_memchr( test_string, 'A', 1);
   printf( "Find 'A' in single char \"A\": ");
   if( result == test_string)
      printf( "PASS (found at offset 0)\n");
   else
      printf( "FAIL (expected %p, got %p)\n", test_string, result);

   // Test 4: Length smaller than string
   test_string = "Hello World";
   result = mulle_memchr( test_string, 'W', 5);  // Only search first 5 chars
   printf( "Find 'W' in first 5 chars of \"%s\": ", test_string);
   if( result == NULL)
      printf( "PASS (not found in limited range)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   printf( "\n");
}


static void   test_mulle_memchr_alignment( void)
{
   char    *buffer;
   void    *result;
   int     i;

   printf( "== Test mulle_memchr alignment scenarios ==\n");

   // Create a buffer that tests various alignments
   buffer = malloc( 64);
   memset( buffer, 'X', 64);

   // Test different starting alignments
   for( i = 0; i < 8; i++)
   {
      char *aligned_start = buffer + i;
      aligned_start[16] = 'T';  // Place target character
      
      result = mulle_memchr( aligned_start, 'T', 32);
      printf( "Alignment %d: Find 'T' at offset 16: ", i);
      if( result == (aligned_start + 16))
         printf( "PASS\n");
      else
         printf( "FAIL (expected %p, got %p)\n", aligned_start + 16, result);
      
      aligned_start[16] = 'X';  // Reset for next test
   }

   free( buffer);
   printf( "\n");
}


static void   test_mulle_memchr_large_data( void)
{
   char    *large_buffer;
   void    *result;
   size_t  size = 10000;
   int     i;

   printf( "== Test mulle_memchr with large data ==\n");

   large_buffer = malloc( size);
   memset( large_buffer, 'A', size);

   // Test 1: Character not in large buffer
   result = mulle_memchr( large_buffer, 'Z', size);
   printf( "Find 'Z' in 10000 'A's: ");
   if( result == NULL)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 2: Place character at various positions
   int positions[] = { 0, 7, 8, 15, 16, 1000, 5000, size-1 };
   int num_positions = sizeof(positions) / sizeof(positions[0]);

   for( i = 0; i < num_positions; i++)
   {
      if( positions[i] < size)
      {
         large_buffer[positions[i]] = 'T';
         result = mulle_memchr( large_buffer, 'T', size);
         printf( "Find 'T' at position %d in large buffer: ", positions[i]);
         if( result == (large_buffer + positions[i]))
            printf( "PASS\n");
         else
            printf( "FAIL (expected %p, got %p)\n", large_buffer + positions[i], result);
         large_buffer[positions[i]] = 'A';  // Reset
      }
   }

   free( large_buffer);
   printf( "\n");
}


static void   test_mulle_memchr_vs_standard( void)
{
   char    test_strings[][32] = {
      "Hello World",
      "ABCDEFGHIJKLMNOP",
      "1234567890123456",
      "aaaaaaaaaaaaaaaaa",
      "The quick brown fox",
      ""
   };
   char    test_chars[] = { 'e', 'A', '5', 'a', 'x', 'z', '\0' };
   int     i, j;
   void    *mulle_result, *std_result;

   printf( "== Test mulle_memchr vs standard memchr ==\n");

   for( i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++)
   {
      if( strlen(test_strings[i]) == 0 && i > 0)
         continue;  // Skip empty string for most tests
         
      for( j = 0; j < sizeof(test_chars) / sizeof(test_chars[0]); j++)
      {
         size_t len = strlen(test_strings[i]);
         if( len == 0 && test_chars[j] != '\0')
            continue;
            
         mulle_result = mulle_memchr( test_strings[i], test_chars[j], len);
         std_result = memchr( test_strings[i], test_chars[j], len);
         
         printf( "String \"%s\", char '%c': ", test_strings[i], 
                 test_chars[j] == '\0' ? '0' : test_chars[j]);
         if( mulle_result == std_result)
            printf( "PASS\n");
         else
            printf( "FAIL (mulle: %p, std: %p)\n", mulle_result, std_result);
      }
   }

   printf( "\n");
}


int   main( void)
{
   printf( "Testing mulle_memchr function\n");
   printf( "=============================\n\n");

   test_mulle_memchr_basic();
   test_mulle_memchr_edge_cases();
   test_mulle_memchr_alignment();
   test_mulle_memchr_large_data();
   test_mulle_memchr_vs_standard();

   printf( "All mulle_memchr tests completed.\n");
   return( 0);
}
