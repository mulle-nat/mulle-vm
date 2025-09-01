//
//  basic-vm.c
//  mulle-vm
//
//  Test implementation of basic mulle-vm initialization and cleanup
//  Step 1: Initialize VM and tear it down
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>

int   main( void)
{
   struct mulle_vm   vm;
   
   mulle_printf( "mulle-vm Basic Tests\n");
   mulle_printf( "==================\n");
   
   // Test 1: VM init/done
   mulle_printf( "Test 1: VM init/done\n");
   mulle_vm_init( &vm, NULL, NULL);
   mulle_printf( "VM initialized successfully\n");
   
   mulle_vm_done( &vm);
   mulle_printf( "VM destroyed successfully\n");
   
   mulle_printf( "\nAll basic tests completed!\n");
   
   return( 0);
}
