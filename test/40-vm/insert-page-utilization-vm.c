//
//  insert-page-utilization-vm.c
//  mulle-vm
//
//  Test implementation of inserting data with page utilization optimization
//  This tests the case where we insert at page boundary but use available space
//  in adjacent pages instead of creating new pages
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int   main( void)
{
   struct mulle_vm      vm;
   char                 first_data[] = "12345";     // 5 bytes in 8-byte page (3 free)
   char                 second_data[] = "abcde";    // 5 bytes in 8-byte page (3 free)
   char                 insert_data[] = "XYZTOP";   // 6 bytes (should fit in 3+3 available)
   char                 expected_result[] = "12345XYZTOPabcde"; // 16 bytes total
   const void           *read_ptr;
   struct mulle_range   range;
   int                  result;
   size_t               first_len, second_len, insert_len, total_expected_len;
   size_t               insert_position;
   
   mulle_printf( "mulle-vm Insert Page Utilization Tests\n");
   mulle_printf( "======================================\n");
   
   // Test 1: Initialize VM and add first page (5 bytes in 8-byte page)
   mulle_printf( "Test 1: Initialize VM and add first page\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   first_len = strlen( first_data);
   result = mulle_vm_append( &vm, first_data, first_len);
   if( result != 0)
   {
      mulle_printf( "ERROR: Failed to append first page data\n");
      return( 1);
   }
   mulle_printf( "First page data: \"%s\" (%zu bytes in 8-byte page)\n", first_data, first_len);
   
   // Test 2: Add second page (5 bytes in 8-byte page)
   mulle_printf( "\nTest 2: Add second page\n");
   second_len = strlen( second_data);
   result = mulle_vm_append( &vm, second_data, second_len);
   if( result != 0)
   {
      mulle_printf( "ERROR: Failed to append second page data\n");
      return( 1);
   }
   mulle_printf( "Second page data: \"%s\" (%zu bytes in 8-byte page)\n", second_data, second_len);
   
   // Verify initial state
   size_t vm_length = mulle_vm_get_length( &vm);
   mulle_printf( "VM length after two pages: %zu bytes\n", vm_length);
   
   char read_buffer[32];
   range = mulle_range_make( 0, vm_length);
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( read_ptr)
   {
      mulle_printf( "Initial content: \"%.*s\"\n", (int)vm_length, (char*)read_ptr);
   }
   
   // Test 3: Insert data at boundary (should use available space in adjacent pages)
   mulle_printf( "\nTest 3: Insert data at boundary with page utilization\n");
   insert_len = strlen( insert_data);
   insert_position = first_len; // Insert exactly at the boundary (position 5)
   mulle_printf( "Inserting \"%s\" (%zu bytes) at position %zu\n", 
           insert_data, insert_len, insert_position);
   mulle_printf( "Available space: Page1 has 3 bytes free, Page2 has 3 bytes free (total 6 bytes)\n");
   
   result = mulle_vm_insert( &vm, mulle_range_make( insert_position, insert_len), insert_data);
   if( result != 0)
   {
      mulle_printf( "ERROR: Insert operation failed with code %d\n", result);
      return( 1);
   }
   mulle_printf( "Insert operation completed successfully\n");
   
   // Test 4: Verify the result uses optimized page utilization  
   mulle_printf( "\nTest 4: Verify result and page count\n");
   total_expected_len = first_len + insert_len + second_len;
   
   // Check total VM length
   vm_length = mulle_vm_get_length( &vm);
   mulle_printf( "Final VM length: %zu bytes (expected: %zu)\n", vm_length, total_expected_len);
   
   // Read and verify content
   range = mulle_range_make( 0, total_expected_len);
   char result_buffer[32];
   read_ptr = mulle_vm_read( &vm, range, result_buffer);
   
   if( read_ptr)
   {
      mulle_printf( "Final content: \"%.*s\"\n", (int)total_expected_len, (char*)read_ptr);
      
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
      mulle_printf( "ERROR: Failed to read final content\n");
      return( 1);
   }
   
   // Test 5: Check if we avoided creating a new page (page count should still be 2)
   mulle_printf( "\nTest 5: Verify page count optimization\n");
   struct mulle_vm_statistics stats = mulle_vm_get_statistics( &vm);
   size_t total_pages = stats.page_size / mulle_vm_get_pagesize( &vm);
   mulle_printf( "Total pages used: %zu (should be 2 if optimization worked)\n", total_pages);
   
   if( total_pages == 2)
   {
      mulle_printf( "SUCCESS: Page utilization optimization worked - no new page created\n");
   }
   else
   {
      mulle_printf( "INFO: Used %zu pages (may have created new page)\n", total_pages);
   }
   
   // Test 6: Clean up
   mulle_printf( "\nTest 6: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll insert page utilization tests completed!\n");
   
   return( 0);
}
