//
//  edge-cases-tree.c
//  mulle-vm
//
//  Test implementation of mulle_vm_tree edge cases and more API functions
//  Tests: empty tree enumeration, search, reset, reverse enumeration, etc.
//

#include "include.h"

int   main( void)
{
   struct mulle_vm_tree             tree;
   struct mulle_vm_page             *page1, *page2, *page3;
   struct mulle_vm_node             *node;
   struct mulle_vm_tree_search_result search_result;
   struct mulle_allocator           *allocator;
   char                             data1[] = "Hello";
   char                             data2[] = "World";
   char                             data3[] = "!";
   int                              page_count;
   uintptr_t                        search_offset;
   struct mulle_vm_page             *found_page;
   
   mulle_printf( "mulle-vm-tree Edge Cases Tests\n");
   mulle_printf( "==============================\n");
   
   // Test 1: Empty tree enumeration
   mulle_printf( "Test 1: Empty tree enumeration\n");
   mulle_vm_tree_init( &tree, NULL);
   
   page_count = 0;
   mulle_vm_tree_for( &tree, node)
   {
      page_count++;
      mulle_printf( "ERROR: Found node in empty tree!\n");
   }
   
   if( page_count == 0)
   {
      mulle_printf( "SUCCESS: Empty tree enumeration works correctly\n");
   }
   else
   {
      mulle_printf( "ERROR: Empty tree returned %d nodes\n", page_count);
      return( 1);
   }
   
   // Test 2: Empty tree total length
   mulle_printf( "\nTest 2: Empty tree total length\n");
   size_t empty_length = mulle_vm_tree_get_total_length( &tree);
   if( empty_length == 0)
   {
      mulle_printf( "SUCCESS: Empty tree has total length 0\n");
   }
   else
   {
      mulle_printf( "ERROR: Empty tree has total length %zu\n", empty_length);
      return( 1);
   }
   
   // Test 3: Empty tree search
   mulle_printf( "\nTest 3: Empty tree search\n");
   search_result = _mulle_vm_tree_search( &tree, 0);
   if( !mulle_vm_tree_search_result_is_valid( search_result))
   {
      mulle_printf( "SUCCESS: Search on empty tree returns invalid result\n");
   }
   else
   {
      mulle_printf( "ERROR: Search on empty tree returned valid result\n");
      return( 1);
   }
   
   // Test 4: Get allocator
   mulle_printf( "\nTest 4: Get allocator\n");
   allocator = mulle_vm_tree_get_allocator( &tree);
   if( allocator != NULL)
   {
      mulle_printf( "SUCCESS: Got allocator from tree\n");
   }
   else
   {
      mulle_printf( "ERROR: Failed to get allocator from tree\n");
      return( 1);
   }
   
   // Test 5: Add some pages for further testing
   mulle_printf( "\nTest 5: Add pages for search testing\n");
   
   // Add page1 "Hello" (5 bytes)
   page1 = mulle_vm_page_alloc();
   size_t len1 = strlen( data1);
   void *memory1 = mulle_malloc( len1);
   memcpy( memory1, data1, len1);
   mulle_vm_page_init( page1, memory1, mulle_range_make( 0, len1), len1);
   mulle_vm_tree_add_page( &tree, page1);
   
   // Add page2 "World" (5 bytes) 
   page2 = mulle_vm_page_alloc();
   size_t len2 = strlen( data2);
   void *memory2 = mulle_malloc( len2);
   memcpy( memory2, data2, len2);
   mulle_vm_page_init( page2, memory2, mulle_range_make( 0, len2), len2);
   mulle_vm_tree_add_page( &tree, page2);
   
   // Add page3 "!" (1 byte)
   page3 = mulle_vm_page_alloc();
   size_t len3 = strlen( data3);
   void *memory3 = mulle_malloc( len3);
   memcpy( memory3, data3, len3);
   mulle_vm_page_init( page3, memory3, mulle_range_make( 0, len3), len3);
   mulle_vm_tree_add_page( &tree, page3);
   
   mulle_printf( "Added 3 pages: \\\"Hello\\\" + \\\"World\\\" + \\\"!\\\" = 11 bytes total\n");
   
   // Test 6: Search at various positions
   mulle_printf( "\nTest 6: Position-based search\n");
   
   // Search at position 0 (should find "Hello", offset 0)
   found_page = mulle_vm_tree_search( &tree, 0, &search_offset);
   if( found_page == page1 && search_offset == 0)
   {
      mulle_printf( "SUCCESS: Search at position 0 found first page, offset 0\n");
   }
   else
   {
      mulle_printf( "ERROR: Search at position 0 failed\n");
      return( 1);
   }
   
   // Search at position 3 (should find "Hello", offset 3 = 'l')
   found_page = mulle_vm_tree_search( &tree, 3, &search_offset);
   if( found_page == page1 && search_offset == 3)
   {
      mulle_printf( "SUCCESS: Search at position 3 found first page, offset 3\n");
   }
   else
   {
      mulle_printf( "ERROR: Search at position 3 failed\n");
      return( 1);
   }
   
   // Search at position 5 (should find "World", offset 0)
   found_page = mulle_vm_tree_search( &tree, 5, &search_offset);
   if( found_page == page2 && search_offset == 0)
   {
      mulle_printf( "SUCCESS: Search at position 5 found second page, offset 0\n");
   }
   else
   {
      mulle_printf( "ERROR: Search at position 5 failed\n");
      return( 1);
   }
   
   // Search at position 10 (should find "!", offset 0)
   found_page = mulle_vm_tree_search( &tree, 10, &search_offset);
   if( found_page == page3 && search_offset == 0)
   {
      mulle_printf( "SUCCESS: Search at position 10 found third page, offset 0\n");
   }
   else
   {
      mulle_printf( "ERROR: Search at position 10 failed\n");
      return( 1);
   }
   
   // Search beyond end (should return NULL)
   found_page = mulle_vm_tree_search( &tree, 15, &search_offset);
   if( found_page == NULL)
   {
      mulle_printf( "SUCCESS: Search beyond end returned NULL\n");
   }
   else
   {
      mulle_printf( "ERROR: Search beyond end returned a page\n");
      return( 1);
   }
   
   // Test 7: Reverse enumeration
   mulle_printf( "\nTest 7: Reverse enumeration\n");
   page_count = 0;
   mulle_vm_tree_for_reverse( &tree, node)
   {
      page_count++;
      struct mulle_vm_page *page = node->page;
      mulle_printf( "Reverse Page %d: \\\"%.*s\\\"\n", 
                   page_count, 
                   (int) page->used.length, 
                   (char*) page->bytes + page->used.location);
   }
   
   if( page_count == 3)
   {
      mulle_printf( "SUCCESS: Reverse enumeration found 3 pages\n");
   }
   else
   {
      mulle_printf( "ERROR: Reverse enumeration found %d pages\n", page_count);
      return( 1);
   }
   
   // Test 8: Search result helper functions
   mulle_printf( "\nTest 8: Search result helper functions\n");
   search_result = _mulle_vm_tree_search( &tree, 7); // Should be in "World" at offset 2
   if( mulle_vm_tree_search_result_is_valid( search_result))
   {
      struct mulle_vm_page *result_page = mulle_vm_tree_search_result_get_page( search_result);
      uintptr_t result_offset = mulle_vm_tree_search_result_get_offset( search_result);
      
      if( result_page == page2 && result_offset == 2)
      {
         mulle_printf( "SUCCESS: Search result helpers work correctly\n");
      }
      else
      {
         mulle_printf( "ERROR: Search result helpers returned wrong values\n");
         return( 1);
      }
   }
   else
   {
      mulle_printf( "ERROR: Search result should be valid\n");
      return( 1);
   }
   
   // Test 9: Tree reset
   mulle_printf( "\nTest 9: Tree reset\n");
   size_t length_before_reset = mulle_vm_tree_get_total_length( &tree);
   mulle_printf( "Length before reset: %zu\n", length_before_reset);
   
   mulle_free( memory1);
   mulle_free( memory2);
   mulle_free( memory3);
   mulle_vm_tree_reset( &tree);
   
   size_t length_after_reset = mulle_vm_tree_get_total_length( &tree);
   if( length_after_reset == 0)
   {
      mulle_printf( "SUCCESS: Tree reset cleared all pages (length = %zu)\n", length_after_reset);
   }
   else
   {
      mulle_printf( "ERROR: Tree reset failed, length = %zu\n", length_after_reset);
      return( 1);
   }
   
   // Test 10: Enumeration after reset should be empty
   mulle_printf( "\nTest 10: Enumeration after reset\n");
   page_count = 0;
   mulle_vm_tree_for( &tree, node)
   {
      page_count++;
   }
   
   if( page_count == 0)
   {
      mulle_printf( "SUCCESS: Enumeration after reset is empty\n");
   }
   else
   {
      mulle_printf( "ERROR: Enumeration after reset found %d nodes\n", page_count);
      return( 1);
   }
   
   // Test 11: NULL tree handling
   mulle_printf( "\nTest 11: NULL tree handling\n");
   size_t null_length = mulle_vm_tree_get_total_length( NULL);
   struct mulle_allocator *null_allocator = mulle_vm_tree_get_allocator( NULL);
   
   if( null_length == 0 && null_allocator == NULL)
   {
      mulle_printf( "SUCCESS: NULL tree handling works correctly\n");
   }
   else
   {
      mulle_printf( "ERROR: NULL tree handling failed\n");
      return( 1);
   }
   
   // Test 12: Clean up
   mulle_printf( "\nTest 12: Tree cleanup\n");
   mulle_vm_tree_done( &tree);
   mulle_printf( "Tree destroyed successfully\n");
   
   mulle_printf( "\nAll edge cases tests completed!\n");
   
   return( 0);
}
