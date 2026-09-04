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
   struct mulle_vm_tree            tree;
   struct mulle_vm_page            *middle_page, *before_page, *after_page;
   struct mulle_vm_node            *middle_node, *node;
   struct mulle_vm_treeenumerator  rover;
   size_t                          total_length;
   size_t                          expected_length;
   size_t                          middle_len, before_len, after_len;
   void                            *middle_memory, *before_memory, *after_memory;
   char                            middle_data[] = "Middle";
   char                            before_data[] = "Before";
   char                            after_data[] = "After";
   int                             page_count;

   mulle_printf( "mulle-vm-tree Insert Before/After Tests\n");
   mulle_printf( "=======================================\n");

   // Test 1: Initialize tree
   mulle_printf( "Test 1: Tree initialization\n");
   mulle_vm_tree_init( &tree, NULL);
   mulle_printf( "Tree initialized successfully\n");

   // Test 2: Create and add middle page first
   mulle_printf( "\nTest 2: Create and add middle page\n");
   middle_page   = mulle_vm_page_alloc();
   middle_len    = strlen( middle_data);
   middle_memory = mulle_malloc( middle_len);
   memcpy( middle_memory, middle_data, middle_len);
   mulle_vm_page_init( middle_page, middle_memory, mulle_range_make( 0, middle_len), middle_len);
   mulle_vm_tree_add_page( &tree, middle_page);
   mulle_printf( "Middle page added: \"%.*s\" (length: %zu)\n", (int) middle_len, (char *) middle_memory, middle_len);

   // Test 3: Get reference to the middle node for insertions
   mulle_printf( "\nTest 3: Get middle node reference\n");
   rover = mulle_vm_tree_enumerate( &tree);
   if( ! mulle_vm_treeenumerator_next( &rover, &middle_node))
   {
      mulle_printf( "ERROR: Failed to get middle node\n");
      return( 1);
   }
   mulle_vm_treeenumerator_done( &rover);
   mulle_printf( "Got middle node reference successfully\n");

   // Test 4: Create and insert before page
   mulle_printf( "\nTest 4: Create and insert before page\n");
   before_page   = mulle_vm_page_alloc();
   before_len    = strlen( before_data);
   before_memory = mulle_malloc( before_len);
   memcpy( before_memory, before_data, before_len);
   mulle_vm_page_init( before_page, before_memory, mulle_range_make( 0, before_len), before_len);
   mulle_vm_tree_insert_page_before_node( &tree, before_page, middle_node);
   mulle_printf( "Before page inserted: \"%.*s\" (length: %zu)\n", (int) before_len, (char *) before_memory, before_len);

   // Test 5: Create and insert after page
   mulle_printf( "\nTest 5: Create and insert after page\n");
   after_page   = mulle_vm_page_alloc();
   after_len    = strlen( after_data);
   after_memory = mulle_malloc( after_len);
   memcpy( after_memory, after_data, after_len);
   mulle_vm_page_init( after_page, after_memory, mulle_range_make( 0, after_len), after_len);
   mulle_vm_tree_insert_page_after_node( &tree, after_page, middle_node);
   mulle_printf( "After page inserted: \"%.*s\" (length: %zu)\n", (int) after_len, (char *) after_memory, after_len);

   // Test 6: Check total length
   mulle_printf( "\nTest 6: Check tree total length\n");
   total_length    = mulle_vm_tree_get_total_length( &tree);
   expected_length = before_len + middle_len + after_len;
   mulle_printf( "Tree total length: %zu (expected: %zu)\n", total_length, expected_length);

   if( total_length == expected_length)
      mulle_printf( "SUCCESS: Tree length matches expected total\n");
   else
   {
      mulle_printf( "ERROR: Tree length mismatch\n");
      return( 1);
   }

   // Test 7: Enumerate all pages to verify order
   mulle_printf( "\nTest 7: Enumerate pages to verify order\n");
   page_count = 0;
   mulle_vm_tree_for( &tree, node)
   {
      page_count++;
      struct mulle_vm_page *page = node->page;
      mulle_printf( "Page %d: \"%.*s\" (length: %zu)\n",
                   page_count,
                   (int) page->used.length,
                   (char *) page->bytes + page->used.location,
                   page->used.length);
   }

   if( page_count == 3)
      mulle_printf( "SUCCESS: Found expected 3 pages\n");
   else
   {
      mulle_printf( "ERROR: Expected 3 pages, found %d\n", page_count);
      return( 1);
   }

   // Test 8: Clean up
   mulle_printf( "\nTest 8: Tree cleanup\n");
   mulle_vm_tree_done( &tree);
   mulle_free( middle_memory);
   mulle_free( before_memory);
   mulle_free( after_memory);
   mulle_printf( "Tree destroyed successfully\n");

   mulle_printf( "\nAll insert before/after tests completed!\n");

   return( 0);
}
