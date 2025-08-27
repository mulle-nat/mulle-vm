//
//  comprehensive-insert-vm.c
//  mulle-vm
//
//  Comprehensive test for mulle_vm_insert functionality
//  Tests all insertion scenarios: prepend, append, middle, split, boundary, and edge cases
//

#include "include.h"

static void dump_vm_content(struct mulle_vm *vm)
{
    size_t length = mulle_vm_get_length(vm);
    if (length == 0) {
        mulle_printf("(empty)");
        return;
    }
    
    char *buffer = malloc(length + 1);
    if (!buffer) {
        mulle_printf("(error)");
        return;
    }
    
    if (mulle_vm_read(vm, mulle_range_make(0, length), buffer)) {
        buffer[length] = '\0';
        mulle_printf("\"%.*s\"", (int)length, buffer);
    } else {
        mulle_printf("(read error)");
    }
    
    free(buffer);
}

int main(void)
{
   struct mulle_vm vm;
   char buffer[256];
   
   mulle_printf("mulle-vm Comprehensive Insert Tests\n");
   mulle_printf("===================================\n");
    
   // Test 1: Empty VM prepend
   mulle_printf("\nTest 1: Empty VM prepend\n");
   mulle_vm_init(&vm, NULL, NULL);
   
   mulle_vm_insert(&vm, mulle_range_make(0, 5), "Hello");
   
   mulle_vm_read(&vm, mulle_range_make(0, 5), buffer);
   buffer[5] = '\0';
   if (strcmp(buffer, "Hello") == 0)
   {
      mulle_printf("SUCCESS: Empty VM prepend - \"Hello\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'Hello', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 2: Prepend to existing content
   mulle_printf("\nTest 2: Prepend to existing content\n");
   mulle_vm_insert(&vm, mulle_range_make(0, 6), "World ");
   
   mulle_vm_read(&vm, mulle_range_make(0, 11), buffer);
   buffer[11] = '\0';
   if (strcmp(buffer, "World Hello") == 0)
   {
      mulle_printf("SUCCESS: Prepend - \"World Hello\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'World Hello', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 3: Append to existing content
   mulle_printf("\nTest 3: Append to existing content\n");
   mulle_vm_insert(&vm, mulle_range_make(11, 1), "!");
   
   mulle_vm_read(&vm, mulle_range_make(0, 12), buffer);
   buffer[12] = '\0';
   if (strcmp(buffer, "World Hello!") == 0)
   {
      mulle_printf("SUCCESS: Append - \"World Hello!\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'World Hello!', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 4: Middle insertion within page
   mulle_printf("\nTest 4: Middle insertion within page\n");
   mulle_vm_insert(&vm, mulle_range_make(11, 10), " Beautiful");
   
   mulle_vm_read(&vm, mulle_range_make(0, 22), buffer);
   buffer[22] = '\0';
   if (strcmp(buffer, "World Hello Beautiful!") == 0)
   {
      mulle_printf("SUCCESS: Middle insert - \"World Hello Beautiful!\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'World Hello Beautiful!', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 5: Insert at specific positions
   mulle_printf("\nTest 5: Insert at specific positions\n");
   
   // Insert at position 5
   mulle_vm_insert(&vm, mulle_range_make(5, 1), "-");
   
   // Insert at position 12
   mulle_vm_insert(&vm, mulle_range_make(12, 1), "-");
   
   mulle_vm_read(&vm, mulle_range_make(0, 24), buffer);
   buffer[24] = '\0';
   if (strcmp(buffer, "World- Hello- Beautiful!") == 0)
   {
      mulle_printf("SUCCESS: Position insertions - \"World- Hello- Beautiful!\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'World- Hello- Beautiful!', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 6: Large data insertion (forces new page)
   mulle_printf("\nTest 6: Large data insertion (forces new page)\n");
   
   // Reset VM for this test
   mulle_vm_reset(&vm);
   mulle_vm_append(&vm, "Start", 5);
   mulle_vm_append(&vm, "End", 3);
   
   // Insert large data in middle
   char *large_data = "[THIS IS A VERY LARGE INSERTION THAT WILL FORCE A NEW PAGE TO BE CREATED]";
   int rval = mulle_vm_insert(&vm, mulle_range_make(5, strlen(large_data)), large_data);
   if (rval)
   {
      mulle_printf("ERROR: Failed large data insertion\n");
      return 1;
   }
   
   size_t expected_length = 5 + strlen(large_data) + 3;
   size_t actual_length = mulle_vm_get_length(&vm);
   if (actual_length == expected_length)
   {
      mulle_printf("SUCCESS: Large data insertion - %zu bytes\n", actual_length);
   }
   else
   {
      mulle_printf("ERROR: Expected %zu bytes, got %zu\n", expected_length, actual_length);
      return 1;
   }
    
   // Test 7: Boundary conditions
   mulle_printf("\nTest 7: Boundary conditions\n");
   
   mulle_vm_reset(&vm);
   
   // Insert at position 0 in empty VM
   mulle_vm_insert(&vm, mulle_range_make(0, 8), "Boundary");
   
   // Insert at end
   size_t current_length = mulle_vm_get_length(&vm);
   mulle_vm_insert(&vm, mulle_range_make(current_length, 4), "Test");
   
   // Verify
   mulle_vm_read(&vm, mulle_range_make(0, 12), buffer);
   buffer[12] = '\0';
   if (strcmp(buffer, "BoundaryTest") == 0)
   {
      mulle_printf("SUCCESS: Boundary insertions - \"BoundaryTest\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'BoundaryTest', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 8: Multiple small insertions
   mulle_printf("\nTest 8: Multiple small insertions\n");
   
   mulle_vm_reset(&vm);
   mulle_vm_append(&vm, "Base", 4);
   
   // Insert multiple small pieces
   mulle_vm_insert(&vm, mulle_range_make(0, 1), "1");
   mulle_vm_insert(&vm, mulle_range_make(2, 1), "2");
   mulle_vm_insert(&vm, mulle_range_make(4, 1), "3");
   mulle_vm_insert(&vm, mulle_range_make(6, 1), "4");
   
   mulle_vm_read(&vm, mulle_range_make(0, 8), buffer);
   buffer[8] = '\0';
   if (strcmp(buffer, "1B2a3s4e") == 0)
   {
      mulle_printf("SUCCESS: Multiple small insertions - \"1B2a3s4e\"\n");
   }
   else
   {
      mulle_printf("ERROR: Expected '1B2a3s4e', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 9: Empty string insertion (should be no-op)
   mulle_printf("\nTest 9: Empty string insertion\n");
   
   size_t length_before = mulle_vm_get_length(&vm);
   mulle_vm_insert(&vm, mulle_range_make(4, 0), "");
   size_t length_after = mulle_vm_get_length(&vm);
   
   if (length_before == length_after)
   {
      mulle_printf("SUCCESS: Empty string insertion (no-op)\n");
   }
   else
   {
      mulle_printf("ERROR: Empty string insertion failed\n");
      return 1;
   }
    
   // Test 10: Statistics verification
   mulle_printf("\nTest 10: Statistics verification\n");
   
   struct mulle_vm_statistics stats = mulle_vm_get_statistics(&vm);
   if (stats.vm_size == 8)
   {
      mulle_printf("SUCCESS: Statistics match - vm_size=%zu\n", stats.vm_size);
   }
   else
   {
      mulle_printf("ERROR: Expected vm_size=8, got %zu\n", stats.vm_size);
      return 1;
   }
   
   mulle_printf("\nAll comprehensive insert tests completed successfully!\n");
   mulle_vm_done(&vm);
   return 0;
}