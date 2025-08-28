//
//  basic-tree.c
//  mulle-vm
//
//  Test implementation of mulle-vm-tree functionality
//  Step 1: Just allocate and delete a tree
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
   s   = mulle_vm_read( vm, mulle_range_make_all(), buf);
   mulle_printf( "%.*s\n", (int) len, s ? s : "");
}


int   main( void)
{
   struct mulle_vm   vm;
   
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   mulle_vm_append( &vm, "abcd", 4);
   print_vm( &vm);
   mulle_vm_append( &vm, "efgh", 4);
   print_vm( &vm);
   mulle_vm_append( &vm, "ijkl", 4);
   print_vm( &vm);
   mulle_vm_append( &vm, "mnopq", 5);
   print_vm( &vm);
   mulle_vm_append( &vm, "rstu", 4);
   print_vm( &vm);
   mulle_vm_append( &vm, "vwxyz", 5);
   print_vm( &vm);
   mulle_vm_done( &vm);
   
   return( 0);
}
