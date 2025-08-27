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
   
   printf( "mulle-vm Basic Tests\n");
   printf( "==================\n");
   
   // Test 1: VM init/done
   printf( "Test 1: VM init/done\n");
   mulle_vm_init( &vm, NULL, NULL);
   printf( "VM initialized successfully\n");
   
   mulle_vm_done( &vm);
   printf( "VM destroyed successfully\n");
   
   printf( "\nAll basic tests completed!\n");
   
   return( 0);
}
