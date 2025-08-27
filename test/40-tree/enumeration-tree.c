//
//  enumeration-tree.c
//  mulle-vm
//
//  Test implementation of mulle-vm-tree enumeration functionality
//  Simple test: Add three pages and enumerate them using mulle_vm_tree_for
//

#include "include.h"

int   main( void)
{
   struct mulle_vm_tree   tree;
   struct mulle_vm_page   *page1, *page2, *page3;
   struct mulle_vm_node   *node;
   char                   data1[] = "First";
   char                   data2[] = "Second";
   char                   data3[] = "Third";
   int                    page_count;
   
   mulle_printf( "mulle-vm-tree Enumeration Tests\n");
   mulle_printf( "===============================\n");
   
   // Test 1: Initialize tree
   mulle_printf( "Test 1: Tree initialization\n");
   mulle_vm_tree_init( &tree, NULL);
   mulle_printf( "Tree initialized successfully\n");
   
   // Test 2: Create and add first page
   mulle_printf( "\nTest 2: Create and add pages\n");
   page1 = mulle_vm_page_alloc();
   if( !page1)
   {
      mulle_printf( "ERROR: Failed to allocate page1\n");
      return( 1);
   }
   
   size_t len1 = strlen( data1);
   void *memory1 = malloc( len1);
   if( !memory1)
   {
      mulle_printf( "ERROR: Failed to allocate memory1\n");
      mulle_vm_page_free( page1);
      return( 1);
   }
   memcpy( memory1, data1, len1);
   mulle_vm_page_init( page1, memory1, mulle_range_make( 0, len1), len1);
   mulle_vm_tree_add_page( &tree, page1);
   mulle_printf( "Added page1: \"%.*s\"\n", (int) len1, data1);
   
   // Create and add second page
   page2 = mulle_vm_page_alloc();
   if( !page2)
   {
      mulle_printf( "ERROR: Failed to allocate page2\n");
      return( 1);
   }
   
   size_t len2 = strlen( data2);
   void *memory2 = malloc( len2);
   if( !memory2)
   {
      mulle_printf( "ERROR: Failed to allocate memory2\n");
      mulle_vm_page_free( page2);
      return( 1);
   }
   memcpy( memory2, data2, len2);
   mulle_vm_page_init( page2, memory2, mulle_range_make( 0, len2), len2);
   mulle_vm_tree_add_page( &tree, page2);
   mulle_printf( "Added page2: \"%.*s\"\n", (int) len2, data2);
   
   // Create and add third page
   page3 = mulle_vm_page_alloc();
   if( !page3)
   {
      mulle_printf( "ERROR: Failed to allocate page3\n");
      return( 1);
   }
   
   size_t len3 = strlen( data3);
   void *memory3 = malloc( len3);
   if( !memory3)
   {
      mulle_printf( "ERROR: Failed to allocate memory3\n");
      mulle_vm_page_free( page3);
      return( 1);
   }
   memcpy( memory3, data3, len3);
   mulle_vm_page_init( page3, memory3, mulle_range_make( 0, len3), len3);
   mulle_vm_tree_add_page( &tree, page3);
   mulle_printf( "Added page3: \"%.*s\"\n", (int) len3, data3);
   
   // Test 3: Check tree total length
   mulle_printf( "\nTest 3: Check tree total length\n");
   size_t total_length = mulle_vm_tree_get_total_length( &tree);
   size_t expected_length = len1 + len2 + len3;
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
   
   // Test 4: Enumerate all pages using mulle_vm_tree_for
   mulle_printf( "\nTest 4: Enumerate pages using mulle_vm_tree_for\n");
   page_count = 0;
   mulle_vm_tree_for( &tree, node)
   {
      page_count++;
      struct mulle_vm_page *page = node->page;
      mulle_printf( "Page %d: \"%.*s\" (length: %zu, total: %zu)\n", 
                   page_count, 
                   (int) page->used.length, 
                   (char*) page->bytes + page->used.location, 
                   page->used.length,
                   node->total);
   }
   
   if( page_count == 3)
   {
      mulle_printf( "SUCCESS: Enumerated expected 3 pages\n");
   }
   else
   {
      mulle_printf( "ERROR: Expected 3 pages, found %d\n", page_count);
      return( 1);
   }
   
   // Test 5: Clean up
   mulle_printf( "\nTest 5: Tree cleanup\n");
   mulle_vm_tree_done( &tree);
   mulle_printf( "Tree destroyed successfully\n");
   
   mulle_printf( "\nAll enumeration tests completed!\n");
   
   return( 0);
}
