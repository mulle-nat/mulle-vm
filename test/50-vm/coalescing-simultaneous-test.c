//
// coalescing-simultaneous-test.c
// mulle-vm
//
// Tests for simultaneous predecessor and successor coalescing in single call
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
   
   mulle_printf( "=== Testing simultaneous predecessor and successor coalescing ===\n");
   mulle_printf( "Page size: %zu bytes\n", pagesize);

   // Test 1: Middle page deletion triggers both predecessor and successor coalescing
   mulle_printf( "\n=== Test 1: Simultaneous predecessor + successor coalescing ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create left page with available space at back
   for( int i = 0; i < pagesize / 2; i++)
      mulle_vm_append( &vm, "A", 1);
   
   // Create middle page with small data
   for( int i = 0; i < 2; i++)
      mulle_vm_append( &vm, "B", 1);
   
   // Create right page with available space at front
   for( int i = 0; i < pagesize / 2; i++)
      mulle_vm_append( &vm, "C", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   dump_vm( &vm);
   
   // Delete middle page completely to trigger coalescing
   mulle_vm_delete( &vm, mulle_range_make( pagesize / 2, 2));
   
   mulle_printf( "After middle page deletion: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 2: Three consecutive pages - delete middle page
   mulle_printf( "\n=== Test 2: Three consecutive pages - middle deletion ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Left page
   mulle_vm_append( &vm, "AAAA", 4);
   
   // Middle page (will be deleted)
   mulle_vm_append( &vm, "BB", 2);
   
   // Right page
   mulle_vm_append( &vm, "CCCC", 4);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   dump_vm( &vm);
   
   // Delete the middle page data
   mulle_vm_delete( &vm, mulle_range_make( 4, 2));
   
   mulle_printf( "After middle deletion: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 3: Complex scenario with multiple small pages
   mulle_printf( "\n=== Test 3: Complex multi-page scenario ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create 5 pages with specific data patterns
   // Page 1: small data
   mulle_vm_append( &vm, "L1", 2);
   
   // Page 2: small data (middle)
   mulle_vm_append( &vm, "M1", 2);
   
   // Page 3: small data (middle)
   mulle_vm_append( &vm, "M2", 2);
   
   // Page 4: small data (middle)
   mulle_vm_append( &vm, "M3", 2);
   
   // Page 5: small data
   mulle_vm_append( &vm, "R1", 2);
   
   mulle_printf( "Before (5 pages): ");
   show_vm( &vm);
   dump_vm( &vm);
   
   // Delete middle pages to trigger cascading coalescing
   mulle_vm_delete( &vm, mulle_range_make( 2, 6));
   
   mulle_printf( "After deleting middle pages: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 4: Edge case - single character between two pages
   mulle_printf( "\n=== Test 4: Single character bridging ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Left page with space
   for( int i = 0; i < pagesize - 1; i++)
      mulle_vm_append( &vm, "L", 1);
   
   // Single character bridge
   mulle_vm_append( &vm, "X", 1);
   
   // Right page with space
   for( int i = 0; i < pagesize - 1; i++)
      mulle_vm_append( &vm, "R", 1);
   
   mulle_printf( "Before: ");
   show_vm( &vm);
   dump_vm( &vm);
   
   // Delete the bridge character
   mulle_vm_delete( &vm, mulle_range_make( pagesize - 1, 1));
   
   mulle_printf( "After bridge deletion: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   mulle_printf( "\n=== Simultaneous coalescing tests completed ===\n");
   
   return( 0);
}