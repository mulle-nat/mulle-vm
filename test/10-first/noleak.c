#include <mulle-vm/mulle-vm.h>
#include <mulle-allocator/mulle-allocator.h>
#include <stdio.h>

int   main( void)
{
   struct mulle_vm   vm;

   mulle_vm_init( &vm, NULL, NULL);
   mulle_printf( "create\n");
   mulle_printf( "destroy\n");
   mulle_vm_done( &vm);

   return( 0);
}