//
//  simple-insert-test.c
//  mulle-vm
//
//  Simple test for insert functionality
//

#include "include.h"
#include <errno.h>

int main(void)
{
   struct mulle_vm vm;
   int rval;
   char buffer[64];
   
   mulle_printf("Simple Insert Test\n");
   
   mulle_vm_init(&vm, NULL, NULL);
   
   // Test 1: Basic insert
   errno = 0;
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 0), "Hello");
   mulle_printf("Test 1: rval=%d errno=%d\n", rval, errno);
   
   // Test 2: Null data
   errno = 0;
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 5), NULL);
   mulle_printf("Test 2: rval=%d errno=%d\n", rval, errno);
   
   mulle_vm_done(&vm);
   return 0;
}