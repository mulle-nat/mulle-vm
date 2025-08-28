//
//  simple-test.c
//  mulle-vm
//
//  Simple test to debug the crash
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>

int main( void)
{
   struct mulle_vm vm;
   char buffer[ 64];
   
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Test simple append
   mulle_vm_append( &vm, "abc", 3);
   
   // Test reading back
   const void *result = mulle_vm_read( &vm, mulle_range_make( 0, 3), buffer);
   if( !result)
   {
      printf( "Read failed\n");
      return( 1);
   }
   
   printf( "Success: %.*s\n", 3, (char *)result);
   
   mulle_vm_done( &vm);
   return( 0);
}