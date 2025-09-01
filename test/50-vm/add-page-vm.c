//
//  add-page-vm.c
//  mulle-vm
//
//  Test implementation of adding a page to VM and basic functionality  
//  Step 2: Add a page and check statistics, then clean up
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int   main( void)
{
   struct mulle_vm             vm;
   struct mulle_vm_statistics  stats;
   char                        test_data[] = "Hello, VM!";
   int                         result;
   
   mulle_printf( "mulle-vm Add Page Tests\n");
   mulle_printf( "======================\n");
   
   // Test 1: Initialize VM
   mulle_printf( "Test 1: VM initialization\n");
   mulle_vm_init( &vm, NULL, NULL);
   mulle_printf( "VM initialized successfully\n");
   
   // Test 2: Check empty VM statistics  
   mulle_printf( "\nTest 2: Empty VM statistics\n");
   stats = mulle_vm_get_statistics( &vm);
   mulle_printf( "Empty VM: vm_size=%zu, page_size=%zu\n", stats.vm_size, stats.page_size);
   
   if( stats.vm_size == 0 && stats.page_size == 0)
   {
      mulle_printf( "SUCCESS: Empty VM has zero size\n");
   }
   else
   {
      mulle_printf( "ERROR: Empty VM should have zero size\n");
      return( 1);
   }
   
   // Test 3: Add data to VM using append 
   mulle_printf( "\nTest 3: Add data to VM\n");
   size_t data_len = strlen( test_data);
   mulle_vm_append( &vm, test_data, data_len);
   mulle_printf( "Data appended successfully: \"%s\" (%zu bytes)\n", test_data, data_len);
   
   // Test 4: Check VM statistics after adding data
   mulle_printf( "\nTest 4: VM statistics after adding data\n");
   stats = mulle_vm_get_statistics( &vm);
   mulle_printf( "VM after append: vm_size=%zu, page_size=%zu\n", stats.vm_size, stats.page_size);
   
   if( stats.vm_size == data_len)
   {
      mulle_printf( "SUCCESS: VM size matches data length\n");
   }
   else
   {
      mulle_printf( "ERROR: VM size (%zu) doesn't match data length (%zu)\n", stats.vm_size, data_len);
      return( 1);
   }
   
   if( stats.page_size > 0)
   {
      mulle_printf( "SUCCESS: VM allocated pages (total page size: %zu)\n", stats.page_size);
   }
   else
   {
      mulle_printf( "ERROR: VM should have allocated pages\n");
      return( 1);  
   }
   
   // Test 5: Check total VM length
   mulle_printf( "\nTest 5: Check VM length\n");
   size_t vm_length = mulle_vm_get_length( &vm);
   mulle_printf( "VM length: %zu\n", vm_length);
   
   if( vm_length == data_len)
   {
      mulle_printf( "SUCCESS: VM length matches expected data length\n");
   }
   else
   {
      mulle_printf( "ERROR: VM length (%zu) doesn't match expected (%zu)\n", vm_length, data_len);
      return( 1);
   }
   
   // Test 6: Clean up
   mulle_printf( "\nTest 6: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll add page tests completed!\n");
   
   return( 0);
}
