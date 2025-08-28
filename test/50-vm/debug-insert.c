//
// debug-insert.c
// Simple test to debug insert issue
//
#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>


int main(void)
{
   struct mulle_vm vm;
   char buffer[256];
   
   mulle_printf("Debug: Testing simple insert\n");
   
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
   
   // Test 1: Empty VM prepend
   mulle_printf("Test 1: Insert 'Hello' at position 0\n");
   
   int rval = mulle_vm_insert(&vm, mulle_range_make(0, 5), "Hello");
   if (rval)
   {
      mulle_printf("ERROR: Failed to insert, rval=%d\n", rval);
      return 1;
   }
   
   size_t len = mulle_vm_get_length(&vm);
   mulle_printf("VM length after insert: %zu\n", len);
   
   if (len > 0)
   {
      mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
      buffer[len] = '\0';
      mulle_printf("Content: '%s'\n", buffer);
   }
   
   mulle_vm_done(&vm);
   mulle_printf("Debug test completed\n");
   
   return 0;
}