//
// debug-insert2.c
// Debug the specific position 5 insertion issue
//
#include "include.h"
//#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
   struct mulle_vm vm;
   char buffer[256];
   int rval;
   
   mulle_printf("Debug: Testing position 5 insertion\n");
   
   mulle_vm_init(&vm, NULL, NULL); // &mulle_vm_debug_config, NULL);
   
   // Reproduce the exact sequence from comprehensive test
   mulle_printf("Test 1: Insert 'Hello' at position 0\n");
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 5), "Hello");
   if (rval) {
      mulle_printf("ERROR: Test 1 failed, rval=%d\n", rval);
      return 1;
   }
   
   size_t len = mulle_vm_get_length(&vm);
   mulle_printf("Length after Test 1: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Content: '%s'\n", buffer);
   
   mulle_printf("Test 2: Insert 'World ' at position 0\n");
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 6), "World ");
   if (rval) {
      mulle_printf("ERROR: Test 2 failed, rval=%d\n", rval);
      return 1;
   }
   
   len = mulle_vm_get_length(&vm);
   mulle_printf("Length after Test 2: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Content: '%s'\n", buffer);
   
   mulle_printf("Test 3: Insert '!' at position 11\n");
   rval = mulle_vm_insert(&vm, mulle_range_make(11, 1), "!");
   if (rval) {
      mulle_printf("ERROR: Test 3 failed, rval=%d\n", rval);
      return 1;
   }
   
   len = mulle_vm_get_length(&vm);
   mulle_printf("Length after Test 3: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Content: '%s'\n", buffer);
   
   mulle_printf("Test 4: Insert ' Beautiful' at position 11\n");
   rval = mulle_vm_insert(&vm, mulle_range_make(11, 10), " Beautiful");
   if (rval) {
      mulle_printf("ERROR: Test 4 failed, rval=%d\n", rval);
      return 1;
   }
   
   len = mulle_vm_get_length(&vm);
   mulle_printf("Length after Test 4: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Content: '%s'\n", buffer);
   
   mulle_printf("Test 5: Insert '-' at position 5\n");
   mulle_printf("Current length: %zu\n", mulle_vm_get_length(&vm));
   rval = mulle_vm_insert(&vm, mulle_range_make(5, 1), "-");
   if (rval) {
      mulle_printf("ERROR: Test 5 failed, rval=%d\n", rval);
      return 1;
   }
   
   len = mulle_vm_get_length(&vm);
   mulle_printf("Length after Test 5: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Final content: '%s'\n", buffer);
   
   mulle_vm_done(&vm);
   mulle_printf("Debug test completed successfully\n");
   
   return 0;
}