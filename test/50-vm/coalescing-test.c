//
// coalescing-test.c
// mulle-vm
//
// Test for node coalescing scenarios
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>


static void  print_vm( struct mulle_vm *vm)
{
   size_t       len;
   const char   *s;
   char         buf[ 1024];

   len = mulle_vm_get_length( vm);
   assert( len < sizeof( buf));
   s   = mulle_vm_read( vm, mulle_range_make( 0, len), buf);
   mulle_printf( "%.*s\n", (int) len, s ? s : "");
}


int   main( void)
{
   struct mulle_vm   vm;

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Test 1: Basic coalescing with predecessor
   mulle_printf( "Test 1: Predecessor coalescing\n");
   mulle_vm_append( &vm, "AAAA", 4);
   mulle_vm_append( &vm, "BBBB", 4);
   mulle_printf( "Before: ");
   print_vm( &vm);
   
   // Delete from first page to trigger coalescing
   mulle_vm_delete( &vm, mulle_range_make( 1, 1));
   mulle_printf( "After: ");
   print_vm( &vm);

   mulle_vm_done( &vm);

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Test 2: Successor coalescing
   mulle_printf( "Test 2: Successor coalescing\n");
   mulle_vm_append( &vm, "XXXX", 4);
   mulle_vm_append( &vm, "YYYY", 4);
   mulle_printf( "Before: ");
   print_vm( &vm);
   
   // Delete from second page to trigger coalescing
   mulle_vm_delete( &vm, mulle_range_make( 5, 1));
   mulle_printf( "After: ");
   print_vm( &vm);

   mulle_vm_done( &vm);

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Test 3: Edge case - empty page removal
   mulle_printf( "Test 3: Empty page removal\n");
   mulle_vm_append( &vm, "A", 1);
   mulle_vm_append( &vm, "B", 1);
   mulle_vm_append( &vm, "C", 1);
   mulle_printf( "Before: ");
   print_vm( &vm);
   
   // Remove middle character - should coalesce and remove empty page
   mulle_vm_delete( &vm, mulle_range_make( 1, 1));
   mulle_printf( "After: ");
   print_vm( &vm);

   mulle_vm_done( &vm);

   return( 0);
}