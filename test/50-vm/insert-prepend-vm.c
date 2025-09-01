//
//  insert-prepend-vm.c
//  mulle-vm
//
//  Test implementation of inserting data at address 0 (prepending to VM)
//  Step 4: Test insertion at the beginning of the VM address space
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int   main( void)
{
   struct mulle_vm      vm;
   char                 initial_data[] = "World!";
   char                 prepend_data[] = "Hello, ";
   char                 expected_result[] = "Hello, World!";
   const void           *read_ptr;
   struct mulle_range   range;
   int                  result;
   
   mulle_printf( "mulle-vm Insert/Prepend Tests\n");
   mulle_printf( "=============================\n");
   
   // Test 1: Initialize VM and add initial data
   mulle_printf( "Test 1: VM initialization and initial data\n");
   mulle_vm_init( &vm, NULL, NULL);
   
   size_t initial_len = strlen( initial_data);
   mulle_vm_append( &vm, initial_data, initial_len);
   mulle_printf( "Initial data: \"%s\" (%zu bytes)\n", initial_data, initial_len);
   
   // Verify initial state
   range = mulle_range_make( 0, initial_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr && memcmp( read_ptr, initial_data, initial_len) == 0)
   {
      mulle_printf( "SUCCESS: Initial data verified: \"%.*s\"\n", (int)initial_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Initial data verification failed\n");
      return( 1);
   }
   
   // Test 2: Insert data at address 0 (prepend)
   mulle_printf( "\nTest 2: Insert data at address 0 (prepending)\n");
   size_t prepend_len = strlen( prepend_data);
   mulle_printf( "Inserting \"%s\" (%zu bytes) at address 0\n", prepend_data, prepend_len);
   
   result = mulle_vm_insert( &vm, mulle_range_make( 0, prepend_len), prepend_data);
   if( result != 0)
   {
      mulle_printf( "ERROR: Insert operation failed with code %d\n", result);
      return( 1);
   }
   mulle_printf( "Insert operation completed successfully\n");
   
   // Test 3: Verify the complete result
   mulle_printf( "\nTest 3: Verify complete result\n");
   size_t total_len = prepend_len + initial_len;
   range = mulle_range_make( 0, total_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   
   if( read_ptr)
   {
      mulle_printf( "Complete content: \"%.*s\" (%zu bytes)\n", (int)total_len, (char*)read_ptr, total_len);
      
      if( memcmp( read_ptr, expected_result, total_len) == 0)
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
   
   // Test 4: Verify individual parts
   mulle_printf( "\nTest 4: Verify individual parts\n");
   
   // Check prepended part
   range = mulle_range_make( 0, prepend_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr && memcmp( read_ptr, prepend_data, prepend_len) == 0)
   {
      mulle_printf( "SUCCESS: Prepended part correct: \"%.*s\"\n", (int)prepend_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Prepended part incorrect\n");
      return( 1);
   }
   
   // Check original part (now at offset prepend_len)
   range = mulle_range_make( prepend_len, initial_len);
   read_ptr = mulle_vm_read( &vm, range, NULL);
   if( read_ptr && memcmp( read_ptr, initial_data, initial_len) == 0)
   {
      mulle_printf( "SUCCESS: Original part correct: \"%.*s\"\n", (int)initial_len, (char*)read_ptr);
   }
   else
   {
      mulle_printf( "ERROR: Original part incorrect\n");
      return( 1);
   }
   
   // Test 5: Verify VM length
   mulle_printf( "\nTest 5: Verify VM total length\n");
   size_t vm_length = mulle_vm_get_length( &vm);
   if( vm_length == total_len)
   {
      mulle_printf( "SUCCESS: VM length is correct: %zu bytes\n", vm_length);
   }
   else
   {
      mulle_printf( "ERROR: VM length incorrect: got %zu, expected %zu\n", vm_length, total_len);
      return( 1);
   }
   
   // Test 6: Clean up
   mulle_printf( "\nTest 6: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll insert/prepend tests completed!\n");
   
   return( 0);
}
