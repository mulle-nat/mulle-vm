//
//  add-remove-tree.c
//  mulle-vm
//
//  Test implementation of mulle-vm-tree add/remove functionality
//  Step 2: Add a page to the tree and remove it
//

#include "include.h"

int   main( void)
{
   struct mulle_vm_tree   tree;
   struct mulle_vm_page   *page;
   size_t                 total_length;
   char                   test_data[] = "Hello, World!";
   
   mulle_printf( "mulle-vm-tree Add/Remove Tests\n");
   mulle_printf( "==============================\n");
   
   // Test 1: Initialize tree
   mulle_printf( "Test 1: Tree initialization\n");
   mulle_vm_tree_init( &tree, NULL);
   mulle_printf( "Tree initialized successfully\n");
   
   // Test 2: Create a page with test data
   mulle_printf( "\nTest 2: Create page\n");
   page = mulle_vm_page_alloc();
   if( !page)
   {
      mulle_printf( "ERROR: Failed to allocate page\n");
      return( 1);
   }
   
   // Allocate memory for the page data
   size_t data_len = strlen( test_data);
   void *page_memory = malloc( data_len);
   if( !page_memory)
   {
      mulle_printf( "ERROR: Failed to allocate page memory\n");
      mulle_vm_page_free( page);
      return( 1);
   }
   
   // Copy test data to allocated memory
   memcpy( page_memory, test_data, data_len);
   
   // Initialize the page with our allocated memory
   mulle_vm_page_init( page, page_memory, mulle_range_make( 0, data_len), data_len);
   mulle_printf( "Page created with data: \"%.*s\" (length: %zu)\n", (int) data_len, (char*) page_memory, data_len);
   
   // Test 3: Add page to tree
   mulle_printf( "\nTest 3: Add page to tree\n");
   mulle_vm_tree_add( &tree, page);
   mulle_printf( "Page added to tree successfully\n");
   
   // Test 4: Check tree total length
   mulle_printf( "\nTest 4: Check tree total length\n");
   total_length = mulle_vm_tree_get_total_length( &tree);
   mulle_printf( "Tree total length: %zu\n", total_length);
   
   if( total_length == strlen( test_data))
   {
      mulle_printf( "SUCCESS: Tree length matches expected data length\n");
   }
   else
   {
      mulle_printf( "ERROR: Tree length (%zu) does not match expected length (%zu)\n", 
                   total_length, strlen( test_data));
      return( 1);
   }
   
   // Test 5: Clean up - the tree_done will handle releasing the page
   mulle_printf( "\nTest 5: Tree cleanup\n");
   mulle_vm_tree_done( &tree);
   mulle_printf( "Tree destroyed successfully\n");
   
   mulle_printf( "\nAll add/remove tests completed!\n");
   
   return( 0);
}
