//
// coalescing-conditions-test.c
// mulle-vm
//
// Targeted tests for specific coalescing conditions
// Tests the exact conditions in _mulle_vm_try_coalesce
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>

static void show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   statistics;
   size_t                       total_pages;

   statistics  = mulle_vm_get_statistics( vm);
   total_pages = statistics.page_size / mulle_vm_get_pagesize();
   mulle_printf( "%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}

static void dump_vm( struct mulle_vm *vm)
{
   char                         buf[ 1024];
   const void                   *s;
   struct mulle_vm_statistics   statistics;

   statistics = mulle_vm_get_statistics( vm);
   s = mulle_vm_read( vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
      fwrite( s, 1, statistics.vm_size, stdout);
   mulle_printf( "\n");
}

int   main( void)
{
   struct mulle_vm   vm;
   size_t            pagesize = mulle_vm_get_pagesize();
   
   mulle_printf( "=== Testing specific coalescing conditions ===\n");
   mulle_printf( "Page size: %zu bytes\n", pagesize);

   // Test 1: Predecessor coalescing - left_back_available >= node_front_occupied
   mulle_printf( "\n=== Test 1: Predecessor coalescing - left_back_available >= node_front_occupied ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Fill first page almost completely, leaving space at back
   for( int i = 0; i < pagesize / 2; i++)
      mulle_vm_append( &vm, "A", 1);
   
   // Add second page with small data
   mulle_vm_append( &vm, "B", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   
   // Delete to trigger predecessor coalescing
   mulle_vm_delete( &vm, mulle_range_make( pagesize / 2 - 1, 1));
   
   mulle_printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 2: Predecessor coalescing - (left_available >= node_front_occupied && lpage->size == pagesize)
   mulle_printf( "\n=== Test 2: Predecessor coalescing - exact page size condition ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create page with exactly pagesize bytes
   for( int i = 0; i < pagesize; i++)
      mulle_vm_append( &vm, "A", 1);
   
   // Add small data in next page
   mulle_vm_append( &vm, "B", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   
   // Delete to trigger coalescing via exact page size condition
   mulle_vm_delete( &vm, mulle_range_make( pagesize - 2, 2));
   
   mulle_printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 3: Successor coalescing - right_front_available >= node_back_occupied
   mulle_printf( "\n=== Test 3: Successor coalescing - right_front_available >= node_back_occupied ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Add first page with small data
   mulle_vm_append( &vm, "A", 1);
   
   // Fill second page from front
   for( int i = 0; i < pagesize / 2; i++)
      mulle_vm_append( &vm, "B", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   
   // Delete to trigger successor coalescing
   mulle_vm_delete( &vm, mulle_range_make( 1, 1));
   
   mulle_printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 4: Successor coalescing - (right_available >= node_back_occupied && rpage->size == pagesize)
   mulle_printf( "\n=== Test 4: Successor coalescing - exact page size condition ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Add small data in first page
   mulle_vm_append( &vm, "A", 1);
   
   // Create second page with exactly pagesize bytes
   for( int i = 0; i < pagesize; i++)
      mulle_vm_append( &vm, "B", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   
   // Delete to trigger coalescing via exact page size condition
   mulle_vm_delete( &vm, mulle_range_make( 0, 1));
   
   mulle_printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 5: Edge case - both conditions false (should not coalesce)
   mulle_printf( "\n=== Test 5: Edge case - should NOT coalesce ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create pages with specific sizes to prevent coalescing
   for( int i = 0; i < pagesize / 4; i++)
      mulle_vm_append( &vm, "A", 1);
   for( int i = 0; i < pagesize / 4; i++)
      mulle_vm_append( &vm, "B", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   
   // Delete small amount - should not trigger coalescing
   mulle_vm_delete( &vm, mulle_range_make( pagesize / 8, 1));
   
   mulle_printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   mulle_printf( "\n=== Coalescing conditions test completed ===\n");
   
   return( 0);
}