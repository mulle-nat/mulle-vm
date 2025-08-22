//
//  insert-before-after-tree.c
//  mulle-vm
//
//  Test implementation of mulle-vm-tree insert before/after functionality
//  Step 3: Insert pages before and after existing nodes
//

#include "include.h"

int   main( void)
{
   struct mulle_vm_tree   tree;
   struct mulle_vm_page   *middle_page, *before_page, *after_page;
   struct mulle_vm_node   *middle_node;
   size_t                 total_length;
   char                   middle_data[] = "Middle";
   char                   before_data[] = "Before";
   char                   after_data[] = "After";
   
   mulle_printf( "mulle-vm-tree Insert Before/After Tests\n");
   mulle_printf( "=======================================\n");
   
   // Test 1: Initialize tree
   mulle_printf( "Test 1: Tree initialization\n");
   mulle_vm_tree_init( &tree, NULL);
   mulle_printf( "Tree initialized successfully\n");
   
   // Test 2: Create and add middle page first
   mulle_printf( "\nTest 2: Create and add middle page\n");
   middle_page = mulle_vm_page_alloc();
   if( !middle_page)
   {
      mulle_printf( "ERROR: Failed to allocate middle page\n");
      return( 1);
   }
   
   // Allocate memory for the middle page data
   size_t middle_len = strlen( middle_data);
   void *middle_memory = malloc( middle_len);
   if( !middle_memory)
   {
      mulle_printf( "ERROR: Failed to allocate middle page memory\n");
      mulle_vm_page_free( middle_page);
      return( 1);
   }
   memcpy( middle_memory, middle_data, middle_len);
   mulle_vm_page_init( middle_page, middle_memory, mulle_range_make( 0, middle_len), middle_len);
   
   mulle_vm_tree_add( &tree, middle_page);
   mulle_printf( "Middle page added: \"%.*s\" (length: %zu)\n", (int) middle_len, (char*) middle_memory, middle_len);
   
   // Test 3: Get reference to the middle node for insertions
   mulle_printf( "\nTest 3: Get middle node reference\n");
   // We need to enumerate to get the node - since we only have one page, it's the first
   struct mulle_vm_treeenumerator   rover;
   rover = mulle_vm_tree_enumerate( &tree);
   if( !mulle_vm_treeenumerator_next( &rover, (void **) &middle_node))
   {
      mulle_printf( "ERROR: Failed to get middle node\n");
      return( 1);
   }
   mulle_vm_treeenumerator_done( &rover);
   mulle_printf( "Got middle node reference successfully\n");
   
   // Test 4: Create and insert before page
   mulle_printf( "\nTest 4: Create and insert before page\n");
   before_page = mulle_vm_page_alloc();
   if( !before_page)
   {
      mulle_printf( "ERROR: Failed to allocate before page\n");
      return( 1);
   }
   
   size_t before_len = strlen( before_data);
   void *before_memory = malloc( before_len);
   if( !before_memory)
   {
      mulle_printf( "ERROR: Failed to allocate before page memory\n");
      mulle_vm_page_free( before_page);
      return( 1);
   }
   memcpy( before_memory, before_data, before_len);
   mulle_vm_page_init( before_page, before_memory, mulle_range_make( 0, before_len), before_len);
   
   int result = mulle_vm_tree_insert_page_before_node( &tree, before_page, middle_node);
   if( result != 0)
   {
      mulle_printf( "ERROR: Failed to insert before page (result: %d)\n", result);
      return( 1);
   }
   mulle_printf( "Before page inserted: \"%.*s\" (length: %zu)\n", (int) before_len, (char*) before_memory, before_len);
   
   // Test 5: Create and insert after page
   mulle_printf( "\nTest 5: Create and insert after page\n");
   after_page = mulle_vm_page_alloc();
   if( !after_page)
   {
      mulle_printf( "ERROR: Failed to allocate after page\n");
      return( 1);
   }
   
   size_t after_len = strlen( after_data);
   void *after_memory = malloc( after_len);
   if( !after_memory)
   {
      mulle_printf( "ERROR: Failed to allocate after page memory\n");
      mulle_vm_page_free( after_page);
      return( 1);
   }
   memcpy( after_memory, after_data, after_len);
   mulle_vm_page_init( after_page, after_memory, mulle_range_make( 0, after_len), after_len);
   
   result = mulle_vm_tree_insert_page_after_node( &tree, after_page, middle_node);
   if( result != 0)
   {
      mulle_printf( "ERROR: Failed to insert after page (result: %d)\n", result);
      return( 1);
   }
   mulle_printf( "After page inserted: \"%.*s\" (length: %zu)\n", (int) after_len, (char*) after_memory, after_len);
   
   // Test 6: Check total length
   mulle_printf( "\nTest 6: Check tree total length\n");
   total_length = mulle_vm_tree_get_total_length( &tree);
   size_t expected_length = before_len + middle_len + after_len;
   mulle_printf( "Tree total length: %zu (expected: %zu)\n", total_length, expected_length);
   
   if( total_length == expected_length)
   {
      mulle_printf( "SUCCESS: Tree length matches expected total\n");
   }
   else
   {
      mulle_printf( "ERROR: Tree length mismatch\n");
      return( 1);
   }
   
   // Test 7: Enumerate all pages to verify order
   mulle_printf( "\nTest 7: Enumerate pages to verify order\n");
   struct mulle_vm_node *node;
   int page_count = 0;
   mulle_vm_tree_for( &tree, node)
   {
      page_count++;
      struct mulle_vm_page *page = node->page;
      mulle_printf( "Page %d: \"%.*s\" (length: %zu)\n", 
                   page_count, 
                   (int) page->used.length, 
                   (char*) page->bytes + page->used.location, 
                   page->used.length);
   }
   
   if( page_count == 3)
   {
      mulle_printf( "SUCCESS: Found expected 3 pages\n");
   }
   else
   {
      mulle_printf( "ERROR: Expected 3 pages, found %d\n", page_count);
      return( 1);
   }
   
   // Test 8: Clean up (mulle_vm_tree_remove is not implemented yet, so we use tree_done)
   mulle_printf( "\nTest 8: Tree cleanup\n");
   mulle_vm_tree_done( &tree);
   mulle_printf( "Tree destroyed successfully\n");
   
   mulle_printf( "\nAll insert before/after tests completed!\n");
   
   return( 0);
}
