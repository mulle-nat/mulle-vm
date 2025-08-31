//
// coalescing-final-test.c
// mulle-vm
//
// Final comprehensive test for node coalescing scenarios
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>


static void  show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   statistics;
   size_t                       total_pages;

   statistics  = mulle_vm_get_statistics( vm);
   total_pages = statistics.page_size / mulle_vm_get_pagesize();
   printf( "%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}


static void  dump_vm( struct mulle_vm *vm)
{
   char                         buf[ 1024];
   const void                   *s;
   struct mulle_vm_statistics   statistics;

   statistics = mulle_vm_get_statistics( vm);
   s = mulle_vm_read( vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
      fwrite( s, 1, statistics.vm_size, stdout);
   printf( "\n");
}


int   main( void)
{
   struct mulle_vm   vm;

   // Test 1: Basic predecessor coalescing
   printf( "=== Test 1: Predecessor coalescing ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   mulle_vm_append( &vm, "AAAA", 4);
   mulle_vm_append( &vm, "BBBB", 4);
   
   printf( "Before: ");
   show_vm( &vm);
   
   mulle_vm_delete( &vm, mulle_range_make( 1, 2));
   
   printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 2: Successor coalescing
   printf( "\n=== Test 2: Successor coalescing ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   mulle_vm_append( &vm, "XXXXXX", 6);
   mulle_vm_append( &vm, "YYYY", 4);
   
   printf( "Before: ");
   show_vm( &vm);
   
   mulle_vm_delete( &vm, mulle_range_make( 7, 2));
   
   printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 3: Boundary conditions
   printf( "\n=== Test 3: Boundary conditions ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   mulle_vm_append( &vm, "A", 1);
   mulle_vm_append( &vm, "B", 1);
   mulle_vm_append( &vm, "C", 1);
   
   printf( "Before: ");
   show_vm( &vm);
   
   mulle_vm_delete( &vm, mulle_range_make( 1, 1));
   
   printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   // Test 4: Empty page removal
   printf( "\n=== Test 4: Empty page removal ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   mulle_vm_append( &vm, "AAA", 3);
   mulle_vm_append( &vm, "BBB", 3);
   mulle_vm_append( &vm, "CCC", 3);
   
   printf( "Before: ");
   show_vm( &vm);
   
   mulle_vm_delete( &vm, mulle_range_make( 3, 3));
   
   printf( "After: ");
   show_vm( &vm);
   dump_vm( &vm);
   mulle_vm_done( &vm);

   return( 0);
}