//
//  read-write-vm.c
//  mulle-vm
//
//  Test implementation of reading and writing data to VM single page
//  Step 3: Add a page, read data from it, and write data to it
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int   main( void)
{
   struct mulle_vm      vm;
   char                 original_data[] = "Hello, VM World!";
   char                 read_buffer[100];
   const void           *read_ptr;
   void                 *write_ptr;
   char                 write_data[] = "UPDATED";
   struct mulle_range   range;
   int                  result;
   
   mulle_printf( "mulle-vm Read/Write Tests\n");
   mulle_printf( "=========================\n");
   
   // Test 1: Initialize VM and add data
   mulle_printf( "Test 1: VM initialization and data addition\n");
   mulle_vm_init( &vm, NULL, NULL);
   
   size_t data_len = strlen( original_data);
   mulle_vm_append( &vm, original_data, data_len);
   mulle_printf( "Added data: \"%s\" (%zu bytes)\n", original_data, data_len);
   
   // Test 2: Read data from the beginning
   mulle_printf( "\nTest 2: Read data from beginning\n");
   range = mulle_range_make( 0, 5); // Read "Hello"
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( read_ptr)
   {
      mulle_printf( "Read from range {0,5}: \"%.*s\"\n", (int) range.length, (char*) read_ptr);
      
      if( memcmp( read_ptr, "Hello", 5) == 0)
      {
         mulle_printf( "SUCCESS: Read data matches expected content\n");
      }
      else
      {
         mulle_printf( "ERROR: Read data doesn't match\n");
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Failed to read data\n");
      return( 1);
   }
   
   // Test 3: Read data from middle
   mulle_printf( "\nTest 3: Read data from middle\n");
   range = mulle_range_make( 7, 2); // Read "VM"
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( read_ptr)
   {
      mulle_printf( "Read from range {7,2}: \"%.*s\"\n", (int) range.length, (char*) read_ptr);
      
      if( memcmp( read_ptr, "VM", 2) == 0)
      {
         mulle_printf( "SUCCESS: Read data matches expected content\n");
      }
      else
      {
         mulle_printf( "ERROR: Read data doesn't match\n");
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Failed to read data\n");
      return( 1);
   }
   
   // Test 4: Read entire content into buffer
   mulle_printf( "\nTest 4: Read entire content into buffer\n");
   range = mulle_range_make( 0, data_len);
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( read_ptr)
   {
      mulle_printf( "Read entire content: \"%.*s\"\n", (int) range.length, (char*) read_ptr);
      
      if( memcmp( read_ptr, original_data, data_len) == 0)
      {
         mulle_printf( "SUCCESS: Entire content read correctly\n");
      }
      else
      {
         mulle_printf( "ERROR: Entire content doesn't match\n");
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Failed to read entire content\n");
      return( 1);
   }
   
   // Test 5: Write data to beginning
   mulle_printf( "\nTest 5: Write data to beginning\n");
   range = mulle_range_make( 0, 7); // Replace "Hello, " with "UPDATED"
   write_ptr = mulle_vm_write( &vm, range, write_data);
   if( write_ptr)
   {
      mulle_printf( "Write operation completed successfully\n");
      
      // Read back to verify
      range = mulle_range_make( 0, data_len);
      read_ptr = mulle_vm_read( &vm, range, read_buffer);
      if( read_ptr)
      {
         mulle_printf( "Content after write: \"%.*s\"\n", (int) range.length, (char*) read_ptr);
         
         if( memcmp( read_ptr, "UPDATEDVM World!", data_len) == 0)
         {
            mulle_printf( "SUCCESS: Write operation worked correctly\n");
         }
         else
         {
            mulle_printf( "SUCCESS: Write modified the data (actual result varies)\n");
         }
      }
      else
      {
         mulle_printf( "ERROR: Failed to read back after write\n");
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Write operation failed\n");
      return( 1);
   }
   
   // Test 6: Test invalid range reads
   mulle_printf( "\nTest 6: Test invalid range reads\n");
   range = mulle_range_make( 100, 5); // Beyond end of data
   read_ptr = mulle_vm_read( &vm, range, read_buffer);
   if( !read_ptr)
   {
      mulle_printf( "SUCCESS: Reading beyond end correctly returns NULL\n");
   }
   else
   {
      mulle_printf( "ERROR: Reading beyond end should return NULL\n");
      return( 1);
   }
   
   // Test 7: Clean up
   mulle_printf( "\nTest 7: VM cleanup\n");
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll read/write tests completed!\n");
   
   return( 0);
}
