//
// debug-insert3.c
// Debug the specific position 5 insertion issue in Test 5
//
#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
   struct mulle_vm vm;
   char buffer[256];
   int rval;
   
   mulle_printf("Debug: Testing position 5 insertion\n");
   
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
   
   // Build up to the exact state at Test 5
   mulle_printf("Building state...\n");
   
   // Test 1: Insert 'Hello' at position 0
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 5), "Hello");
   if (rval) {
      mulle_printf("Test 1 failed: %d\n", rval);
      return 1;
   }
   
   // Test 2: Insert 'World ' at position 0
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 6), "World ");
   if (rval) {
      mulle_printf("Test 2 failed: %d\n", rval);
      return 1;
   }
   
   // Test 3: Insert '!' at position 11
   rval = mulle_vm_insert(&vm, mulle_range_make(11, 1), "!");
   if (rval) {
      mulle_printf("Test 3 failed: %d\n", rval);
      return 1;
   }
   
   // Test 4: Insert ' Beautiful' at position 11
   rval = mulle_vm_insert(&vm, mulle_range_make(11, 10), " Beautiful");
   if (rval) {
      mulle_printf("Test 4 failed: %d\n", rval);
      return 1;
   }
   
   size_t len = mulle_vm_get_length(&vm);
   mulle_printf("Current length: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("Current content: '%s'\n", buffer);
   
   // Now test position 5 insertion
   mulle_printf("Testing insertion at position 5...\n");
   rval = mulle_vm_insert(&vm, mulle_range_make(5, 1), "-");
   if (rval) {
      mulle_printf("ERROR: Position 5 insertion failed, rval=%d\n", rval);
      mulle_printf("errno=%d\n", errno);
      return 1;
   }
   
   len = mulle_vm_get_length(&vm);
   mulle_printf("New length: %zu\n", len);
   
   mulle_vm_read(&vm, mulle_range_make(0, len), buffer);
   buffer[len] = '\0';
   mulle_printf("New content: '%s'\n", buffer);
   
   mulle_vm_done(&vm);
   mulle_printf("Debug test completed successfully\n");
   
   return 0;
}