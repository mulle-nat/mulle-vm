#include <mulle-vm/mulle-vm.h>
#include <mulle-allocator/mulle-allocator.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// memrchr might not be available on all systems, so provide a reference implementation
static void   *reference_memrchr( const void *s, int c, size_t n)
{
   const unsigned char *p = (const unsigned char *)s;
   unsigned char ch = (unsigned char)c;
   
   if( n == 0)
      return( NULL);
      
   p += n;
   while( n--)
   {
      --p;
      if( *p == ch)
         return( (void *)p);
   }
   return( NULL);
}


static void   test_mulle_memrchr_basic( void)
{
   char    *test_string;
   void    *result;
   void    *expected;

   printf( "== Test mulle_memrchr basic functionality ==\n");

   // Test 1: Find last occurrence of character
   test_string = "Hello World";
   result = mulle_memrchr( test_string, 'l', strlen( test_string));
   expected = reference_memrchr( test_string, 'l', strlen( test_string));
   printf( "Find last 'l' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 2: Character not found
   result = mulle_memrchr( test_string, 'z', strlen( test_string));
   expected = reference_memrchr( test_string, 'z', strlen( test_string));
   printf( "Find 'z' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 3: First character (which is also last occurrence)
   result = mulle_memrchr( test_string, 'H', strlen( test_string));
   expected = reference_memrchr( test_string, 'H', strlen( test_string));
   printf( "Find 'H' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 4: Last character
   result = mulle_memrchr( test_string, 'd', strlen( test_string));
   expected = reference_memrchr( test_string, 'd', strlen( test_string));
   printf( "Find 'd' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   printf( "\n");
}


static void   test_mulle_memrchr_edge_cases( void)
{
   char    *test_string;
   void    *result;
   void    *expected;

   printf( "== Test mulle_memrchr edge cases ==\n");

   // Test 1: Empty string
   test_string = "";
   result = mulle_memrchr( test_string, 'a', 0);
   printf( "Find 'a' in empty string: ");
   if( result == NULL)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 2: Null character
   test_string = "Hello\0World";
   result = mulle_memrchr( test_string, '\0', 6);
   expected = reference_memrchr( test_string, '\0', 6);
   printf( "Find '\\0' in \"Hello\\0World\": ");
   if( result == expected)
      printf( "PASS (found at offset 5)\n");
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 3: Single character string
   test_string = "A";
   result = mulle_memrchr( test_string, 'A', 1);
   expected = reference_memrchr( test_string, 'A', 1);
   printf( "Find 'A' in single char \"A\": ");
   if( result == expected)
      printf( "PASS (found at offset 0)\n");
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 4: Length smaller than string
   test_string = "Hello World";
   result = mulle_memrchr( test_string, 'W', 5);  // Only search first 5 chars
   expected = reference_memrchr( test_string, 'W', 5);
   printf( "Find 'W' in first 5 chars of \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (not found in limited range)\n");
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   printf( "\n");
}


static void   test_mulle_memrchr_multiple_occurrences( void)
{
   char    *test_string;
   void    *result;
   void    *expected;

   printf( "== Test mulle_memrchr with multiple occurrences ==\n");

   // Test string with multiple occurrences
   test_string = "abcabcabc";
   
   // Test 1: Find last 'a'
   result = mulle_memrchr( test_string, 'a', strlen( test_string));
   expected = reference_memrchr( test_string, 'a', strlen( test_string));
   printf( "Find last 'a' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 2: Find last 'b'
   result = mulle_memrchr( test_string, 'b', strlen( test_string));
   expected = reference_memrchr( test_string, 'b', strlen( test_string));
   printf( "Find last 'b' in \"%s\": ", test_string);  
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   // Test 3: Find last 'c'
   result = mulle_memrchr( test_string, 'c', strlen( test_string));
   expected = reference_memrchr( test_string, 'c', strlen( test_string));
   printf( "Find last 'c' in \"%s\": ", test_string);
   if( result == expected)
      printf( "PASS (found at offset %ld)\n", (char*)result - test_string);
   else
      printf( "FAIL (expected %p, got %p)\n", expected, result);

   printf( "\n");
}


static void   test_mulle_memrchr_alignment( void)
{
   char    *buffer;
   void    *result;
   void    *expected;
   int     i;

   printf( "== Test mulle_memrchr alignment scenarios ==\n");

   // Create a buffer that tests various alignments
   buffer = malloc( 64);
   memset( buffer, 'X', 64);

   // Test different ending alignments
   for( i = 0; i < 8; i++)
   {
      char *aligned_start = buffer + i;
      aligned_start[16] = 'T';  // Place target character
      aligned_start[32] = 'T';  // Place another target character
      
      result = mulle_memrchr( aligned_start, 'T', 48);
      expected = reference_memrchr( aligned_start, 'T', 48);
      printf( "Alignment %d: Find last 'T' in 48 bytes: ", i);
      if( result == expected)
         printf( "PASS\n");
      else
         printf( "FAIL (expected %p, got %p)\n", expected, result);
      
      aligned_start[16] = 'X';  // Reset
      aligned_start[32] = 'X';  // Reset
   }

   free( buffer);
   printf( "\n");
}


static void   test_mulle_memrchr_large_data( void)
{
   char    *large_buffer;
   void    *result;
   void    *expected;
   size_t  size = 10000;
   int     i;

   printf( "== Test mulle_memrchr with large data ==\n");

   large_buffer = malloc( size);
   memset( large_buffer, 'A', size);

   // Test 1: Character not in large buffer
   result = mulle_memrchr( large_buffer, 'Z', size);
   printf( "Find 'Z' in 10000 'A's: ");
   if( result == NULL)
      printf( "PASS (not found)\n");
   else
      printf( "FAIL (expected NULL, got %p)\n", result);

   // Test 2: Place character at various positions and find the last one
   int positions[] = { 0, 7, 8, 15, 16, 1000, 5000, size-1 };
   int num_positions = sizeof(positions) / sizeof(positions[0]);

   for( i = 0; i < num_positions; i++)
   {
      if( positions[i] < size)
      {
         // Place 'T' at multiple positions
         large_buffer[100] = 'T';        // Early position
         large_buffer[positions[i]] = 'T';  // Test position
         
         result = mulle_memrchr( large_buffer, 'T', size);
         expected = reference_memrchr( large_buffer, 'T', size);
         printf( "Find last 'T' with one at position %d: ", positions[i]);
         if( result == expected)
            printf( "PASS\n");
         else
            printf( "FAIL (expected %p, got %p)\n", expected, result);
            
         large_buffer[100] = 'A';        // Reset
         large_buffer[positions[i]] = 'A';  // Reset
      }
   }

   free( large_buffer);
   printf( "\n");
}


static void   test_mulle_memrchr_vs_reference( void)
{
   char    test_strings[][32] = {
      "Hello World",
      "ABCDEFGHIJKLMNOP",
      "1234567890123456",
      "aaaaaaaaaaaaaaaaa",
      "The quick brown fox",
      "abcabcabcabc",
      "XYXYXYXYXYXY"
   };
   char    test_chars[] = { 'e', 'A', '5', 'a', 'x', 'z', 'X', 'c' };
   int     i, j;
   void    *mulle_result, *ref_result;

   printf( "== Test mulle_memrchr vs reference implementation ==\n");

   for( i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++)
   {
      for( j = 0; j < sizeof(test_chars) / sizeof(test_chars[0]); j++)
      {
         size_t len = strlen(test_strings[i]);
         if( len == 0)
            continue;
            
         mulle_result = mulle_memrchr( test_strings[i], test_chars[j], len);
         ref_result = reference_memrchr( test_strings[i], test_chars[j], len);
         
         printf( "String \"%s\", char '%c': ", test_strings[i], test_chars[j]);
         if( mulle_result == ref_result)
            printf( "PASS\n");
         else
            printf( "FAIL (mulle: %p, ref: %p)\n", mulle_result, ref_result);
      }
   }

   printf( "\n");
}


int   main( void)
{
   printf( "Testing mulle_memrchr function\n");
   printf( "==============================\n\n");

   test_mulle_memrchr_basic();
   test_mulle_memrchr_edge_cases();
   test_mulle_memrchr_multiple_occurrences();
   test_mulle_memrchr_alignment();
   test_mulle_memrchr_large_data();
   test_mulle_memrchr_vs_reference();

   printf( "All mulle_memrchr tests completed.\n");
   return( 0);
}
