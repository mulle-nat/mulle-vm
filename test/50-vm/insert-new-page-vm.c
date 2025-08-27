//
//  insert-new-page-vm.c
//  mulle-vm
//
//  Test implementation of inserting data that requires creating a new intervening page
//  This tests the case where we insert more than 6 bytes at the boundary between pages,
//  forcing the library to create a new page to hold the inserted data
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int   main( void)
{
   struct mulle_vm      vm;
   char                 first_data[] = "Page1XYZ";  // Exactly 8 bytes for debug pagesize
   char                 second_data[] = "Page2ABC"; // Exactly 8 bytes for debug pagesize
   char                 insert_data[] = "BIGINSERT123456789"; // 18 bytes - much more than 6, forces new page
   char                 expected_result[] = "Page1XYZBIGINSERT123456789Page2ABC"; // 34 bytes total
   const void           *read_ptr;
   struct mulle_range   range;
   int                  result;
   size_t               first_len, second_len, insert_len, total_expected_len;
   size_t               insert_position;
   
   mulle_printf( "mulle-vm Insert New Page Tests\n");
   mulle_printf( "===============================\n");
   
   // Test 1: Initialize VM and add first page
   mulle_printf( "Test 1: Initialize VM and add first page\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   first_len = strlen( first_data);
   mulle_vm_append( &vm, first_data, first_len);
   mulle_printf( "First page data: \"%s\" (%zu bytes)\n", first_data, first_len);
   
   // Test 2: Add second page
   mulle_printf( "\nTest 2: Add second page\n");
   second_len = strlen( second_data);
   mulle_vm_append( &vm, second_data, second_len);
   mulle_printf( "Second page data: \"%s\" (%zu bytes)\n", second_data, second_len);
   
   // Verify we have two separate pages
   size_t vm_length = mulle_vm_get_length( &vm);
   mulle_printf( "VM length after two pages: %zu bytes\n", vm_length);
   
   // Read complete content to verify both pages are there (need buffer for multi-page read)
   range = mulle_range_make( 0, vm_length);
   char read_buffer[64]; // Large enough for our test data
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( read_ptr)
   {
      mulle_printf( "Complete content: \"%.*s\"\n", (int)vm_length, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Failed to read complete content\n");
      return( 1);
   }
   
   // Test 3: Insert large data at the boundary between pages (forces new page creation)
   mulle_printf( "\nTest 3: Insert large data between the two pages (forces new page)\n");
   insert_len = strlen( insert_data);
   insert_position = first_len; // Insert exactly at the boundary
   mulle_printf( "Inserting \"%s\" (%zu bytes) at position %zu (between pages)\n", 
           insert_data, insert_len, insert_position);
   mulle_printf( "Note: %zu bytes is much more than 6, so this should create a new intervening page\n", insert_len);
   
   result = mulle_vm_insert( &vm, mulle_range_make( insert_position, insert_len), insert_data);
   if( result != 0)
   {
      mulle_printf( "ERROR: Insert operation failed with code %d\n", result);
      return( 1);
   }
   mulle_printf( "Insert operation completed successfully\n");
   
   // Test 4: Verify the complete result
   mulle_printf( "\nTest 4: Verify complete result\n");
   total_expected_len = first_len + insert_len + second_len;
   range = mulle_range_make( 0, total_expected_len);
   char result_buffer[64]; // Large enough for our test data
   read_ptr = mulle_vm_read( &vm, range, result_buffer);
   
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
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Failed to read complete content\n");
      return( 1);
   }
   
   // Test 5: Verify individual parts
   mulle_printf( "\nTest 5: Verify individual parts\n");
   
   // Check first part
   range = mulle_range_make( 0, first_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr && memcmp( read_ptr, first_data, first_len) == 0)
   {
      mulle_printf( "SUCCESS: First part correct: \"%.*s\"\n", (int)first_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: First part incorrect\n");
      return( 1);
   }
   
   // Check inserted part
   range = mulle_range_make( first_len, insert_len);
   char insert_buffer[32]; // Buffer for the large insert
   read_ptr = mulle_vm_read( &vm, range, insert_buffer);
   if( read_ptr && memcmp( read_ptr, insert_data, insert_len) == 0)
   {
      mulle_printf( "SUCCESS: Inserted part correct: \"%.*s\"\n", (int)insert_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Inserted part incorrect\n");
      return( 1);
   }
   
   // Check second part (now at offset first_len + insert_len)
   range = mulle_range_make( first_len + insert_len, second_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr && memcmp( read_ptr, second_data, second_len) == 0)
   {
      mulle_printf( "SUCCESS: Second part correct: \"%.*s\"\n", (int)second_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Second part incorrect\n");
      return( 1);
   }
   
   // Test 6: Verify VM total length
   mulle_printf( "\nTest 6: Verify VM total length\n");
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
   
   // Test 7: Clean up
   mulle_printf( "\nTest 7: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll insert-new-page tests completed!\n");
   
   return( 0);
}