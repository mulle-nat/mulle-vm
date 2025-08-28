//
//  insert-middle-page-vm.c
//  mulle-vm
//
//  Test implementation of inserting data in the middle of a single page
//  This tests the case where we insert data within a page using memmove
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int   main( void)
{
   struct mulle_vm      vm;
   char                 initial_data[] = "ABCD";  // 4 bytes initial data
   char                 insert_data[] = "1234";   // 4 bytes to insert in middle
   char                 expected_result[] = "AB1234CD"; // 8 bytes total (fills debug page)
   const void           *read_ptr;
   struct mulle_range   range;
   int                  result;
   size_t               initial_len, insert_len, total_expected_len;
   size_t               insert_position;
   auto char            buf[ 256];

   mulle_printf( "mulle-vm Insert Middle Page Tests\n");
   mulle_printf( "==================================\n");
   
   // Test 1: Initialize VM and add initial data (single page)
   mulle_printf( "Test 1: Initialize VM and add initial data\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   initial_len = strlen( initial_data);
   mulle_vm_append( &vm, initial_data, initial_len);
   mulle_printf( "Initial data: \"%s\" (%zu bytes)\n", initial_data, initial_len);
   
   // Verify initial state
   size_t vm_length = mulle_vm_get_length( &vm);
   mulle_printf( "VM length after initial data: %zu bytes\n", vm_length);
   
   // Read complete content to verify initial state
   range    = mulle_range_make( 0, vm_length);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr)
   {
      mulle_printf( "Initial content: \"%.*s\"\n", (int)vm_length, (char*)read_ptr);
   }
   else
   {
      if( mulle_vm_get_pagesize() != 8)
         mulle_printf( "ERROR: Need to adapt test to pagesize %ld\n",  mulle_vm_get_pagesize());
      else
         mulle_printf( "ERROR: Failed to read initial content\n");
      return( 1);
   }
   
   // Test 2: Insert data in the middle of the page (after position 2, between "AB" and "CD")
   mulle_printf( "\nTest 2: Insert data in the middle of the page\n");
   insert_len = strlen( insert_data);
   insert_position = 2; // Insert after "AB", before "CD"
   mulle_printf( "Inserting \"%s\" (%zu bytes) at position %zu (middle of page)\n", 
           insert_data, insert_len, insert_position);
   mulle_printf( "Expected result: \"%s\"\n", expected_result);
   
   result = mulle_vm_insert( &vm, mulle_range_make( insert_position, insert_len), insert_data);
   if( result != 0)
   {
      mulle_printf( "ERROR: Insert operation failed with code %d\n", result);
      return( 1);
   }
   mulle_printf( "Insert operation completed successfully\n");
   
   // Test 3: Verify the complete result
   mulle_printf( "\nTest 3: Verify complete result\n");
   total_expected_len = initial_len + insert_len;
   range = mulle_range_make( 0, total_expected_len);
   read_ptr = mulle_vm_read( &vm, range, buf);
   
   if( read_ptr)
   {
      mulle_printf( "Complete content: \"%.*s\" (%zu bytes)\n", 
              (int)total_expected_len, (char*)read_ptr, total_expected_len);
      
      if( memcmp( read_ptr, expected_result, total_expected_len) == 0)
      {
         mulle_printf( "SUCCESS: Content matches expected result\n");
      }
      else
      {
         mulle_printf( "ERROR: Content doesn't match expected \"%s\"\n", expected_result);
         mulle_printf( "Got: \"%.*s\"\n", (int)total_expected_len, (char*)read_ptr);
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Failed to read complete content\n");
      return( 1);
   }
   
   // Test 4: Verify individual parts
   mulle_printf( "\nTest 4: Verify individual parts\n");
   
   // Check first part ("AB")
   range = mulle_range_make( 0, 2);
   read_ptr = mulle_vm_read( &vm, range, buf);
   if( read_ptr && memcmp( read_ptr, "AB", 2) == 0)
   {
      mulle_printf( "SUCCESS: First part correct: \"%.*s\"\n", 2, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: First part incorrect\n");
      return( 1);
   }
   
   // Check inserted part ("1234")
   range = mulle_range_make( 2, insert_len);
   read_ptr = mulle_vm_read( &vm, range, buf);
   if( read_ptr && memcmp( read_ptr, insert_data, insert_len) == 0)
   {
      mulle_printf( "SUCCESS: Inserted part correct: \"%.*s\"\n", (int)insert_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Inserted part incorrect\n");
      return( 1);
   }
   
   // Check last part ("CD")
   range = mulle_range_make( 2 + insert_len, 2);
   read_ptr = mulle_vm_read( &vm, range, buf);
   if( read_ptr && memcmp( read_ptr, "CD", 2) == 0)
   {
      mulle_printf( "SUCCESS: Last part correct: \"%.*s\"\n", 2, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Last part incorrect\n");
      return( 1);
   }
   
   // Test 5: Verify VM total length
   mulle_printf( "\nTest 5: Verify VM total length\n");
   vm_length = mulle_vm_get_length( &vm);
   if( vm_length == total_expected_len)
   {
      mulle_printf( "SUCCESS: VM length is correct: %zu bytes\n", vm_length);
   }
   else
   {
      mulle_printf( "ERROR: VM length incorrect: got %zu, expected %zu\n", vm_length, total_expected_len);
      return( 1);
   }
   
   // Test 6: Clean up
   mulle_printf( "\nTest 6: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll insert-middle-page tests completed!\n");
   
   return( 0);
}