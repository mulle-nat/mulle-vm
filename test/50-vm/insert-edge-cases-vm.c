//
//  insert-edge-cases-vm.c
//  mulle-vm
//
//  Advanced edge case tests for mulle_vm_insert functionality
//  Tests: zero-length, boundary conditions, memory pressure, and error cases
//

#include "include.h"

int main(void)
{
   struct mulle_vm vm;
   int rval;
   char buffer[1024];
   
   mulle_printf("mulle-vm Insert Edge Cases Tests\n");
   mulle_printf("=================================\n");
    
   // Test 1: Zero-length insert (should be no-op)
   mulle_printf("\nTest 1: Zero-length insert\n");
   mulle_vm_init(&vm, NULL, NULL);
   
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 0), "");
   if (rval == 0 && mulle_vm_get_length(&vm) == 0)
   {
      mulle_printf("SUCCESS: Zero-length insert is no-op\n");
   }
   else
   {
      mulle_printf("ERROR: Zero-length insert failed\n");
      return 1;
   }
    
   // Test 2: Insert beyond current length (should append)
   mulle_printf("\nTest 2: Insert beyond current length\n");
   mulle_vm_append(&vm, "Hello", 5);
   
   rval = mulle_vm_insert(&vm, mulle_range_make(100, 5), "World");
   if (rval == 0)
   {
      size_t length = mulle_vm_get_length(&vm);
      if (length == 10)   // Hello + World
      {
         mulle_printf("SUCCESS: Beyond-length insert appends\n");
      }
      else
      {
         mulle_printf("ERROR: Expected length 10, got %zu\n", length);
         return 1;
      }
   }
   else
   {
      mulle_printf("ERROR: Beyond-length insert failed\n");
      return 1;
   }
   
   mulle_vm_reset(&vm);
    
   // Test 3: Insert at exact page boundaries
   mulle_printf("\nTest 3: Insert at page boundaries\n");
   
   // Create content that spans multiple small pages
   struct mulle_vm_config config = { 0 };
   config.pagesize = 8; // Small pages for testing
   config.skew = 50;
   
   mulle_vm_init(&vm, &config, NULL);
   
   // Add three pages of data
   mulle_vm_append(&vm, "Page1XXX", 8);
   mulle_vm_append(&vm, "Page2YYY", 8);
   mulle_vm_append(&vm, "Page3ZZZ", 8);
   
   // Insert at exact boundaries
   // At position 8 (between page 1 and 2)
   rval = mulle_vm_insert(&vm, mulle_range_make(8, 8), "BOUNDARY");
   if (rval)
   {
      mulle_printf("ERROR: Failed boundary insertion at 8\n");
      return 1;
   }
   
   // At position 16 (between page 2 and 3)
   rval = mulle_vm_insert(&vm, mulle_range_make(16, 6), "MIDDLE");
   if (rval)
   {
      mulle_printf("ERROR: Failed boundary insertion at 16\n");
      return 1;
   }
   
   size_t expected_length = 8 + 8 + 8 + 8 + 6; // 3 pages + 2 inserts
   size_t actual_length = mulle_vm_get_length(&vm);
   if (actual_length == expected_length)
   {
      mulle_printf("SUCCESS: Boundary insertions - %zu bytes\n", actual_length);
   }
   else
   {
      mulle_printf("ERROR: Expected %zu bytes, got %zu\n", expected_length, actual_length);
      return 1;
   }
    
   // Test 4: Insert causing page split
   mulle_printf("\nTest 4: Insert causing page split\n");
   
   mulle_vm_reset(&vm);
   
   // Add content that will be split
   mulle_vm_append(&vm, "ABCDEFGHIJKLMNOP", 16);
   
   // Insert in the middle to force split
   rval = mulle_vm_insert(&vm, mulle_range_make(8, 5), "SPLIT");
   if (rval)
   {
      mulle_printf("ERROR: Failed split insertion\n");
      return 1;
   }
   
   mulle_vm_read(&vm, mulle_range_make(0, 21), buffer);
   buffer[21] = '\0';
   if (strcmp(buffer, "ABCDEFGHSPLITIJKLMNOP") == 0)
   {
      mulle_printf("SUCCESS: Page split insertion\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'ABCDEFGHSPLITIJKLMNOP', got '%s'\n", buffer);
      return 1;
   }
   
   // Test 5: Very large insertions
   mulle_printf("\nTest 5: Very large insertions\n");
   
   mulle_vm_reset(&vm);
   mulle_vm_append(&vm, "Start", 5);
   
   // Create large insertion
   char *large_insert = malloc(1000);
   if (large_insert)
   {
      memset(large_insert, 'X', 1000);
      
      rval = mulle_vm_insert(&vm, mulle_range_make(2, 1000), large_insert);
      if (rval == 0)
      {
         size_t length = mulle_vm_get_length(&vm);
         if (length == 1005)
         {
            mulle_printf("SUCCESS: Large insertion (1000 bytes)\n");
         }
         else
         {
            mulle_printf("ERROR: Expected 1005 bytes, got %zu\n", length);
         }
      }
      else
      {
         mulle_printf("ERROR: Large insertion failed\n");
      }
      free(large_insert);
   }
    
   // Test 6: Multiple consecutive insertions
   mulle_printf("\nTest 6: Multiple consecutive insertions\n");
   
   mulle_vm_reset(&vm);
   mulle_vm_append(&vm, "A", 1);
   
   // Insert at same position multiple times
   for (int i = 0; i < 5; i++)
   {
      char insert[2] = { '0' + i, '\0' };
      rval = mulle_vm_insert(&vm, mulle_range_make(1, 1), insert);
      if (rval)
      {
         mulle_printf("ERROR: Failed consecutive insertion %d\n", i);
         return 1;
      }
   }
   
   mulle_vm_read(&vm, mulle_range_make(0, 6), buffer);
   buffer[6] = '\0';
   if (strcmp(buffer, "A43210") == 0)
   {
      mulle_printf("SUCCESS: Multiple consecutive insertions\n");
   }
   else
   {
      mulle_printf("ERROR: Expected 'A43210', got '%s'\n", buffer);
      return 1;
   }
    
   // Test 7: Insert with null data pointer
   mulle_printf("\nTest 7: Null data pointer\n");
   
   mulle_vm_reset(&vm);
   
   rval = mulle_vm_insert(&vm, mulle_range_make(0, 0), NULL);
   if (rval != EINVAL)
   {
      mulle_printf("ERROR: Null data pointer should return EINVAL\n");
      return 1;
   }
   else
   {
      mulle_printf("SUCCESS: Null data pointer handled correctly\n");
   }
    
   // Test 8: Insert at negative or invalid positions
   mulle_printf("\nTest 8: Invalid position handling\n");
   
   rval = mulle_vm_insert(
      &vm, mulle_range_make((uintptr_t)-1, 0), "test");
   if (rval != EINVAL)
   {
      mulle_printf("ERROR: Negative position should return EINVAL\n");
      return 1;
   }
   else
   {
      mulle_printf("SUCCESS: Invalid position handled correctly\n");
   }
    
   // Test 9: Insert at exact end of VM
   mulle_printf("\nTest 9: Insert at exact end\n");
   
   mulle_vm_reset(&vm);
   mulle_vm_append(&vm, "Test", 4);
   
   rval = mulle_vm_insert(
      &vm, mulle_range_make(4, 3), "End");
   if (rval == 0)
   {
      mulle_vm_read(
         &vm, mulle_range_make(0, 7), buffer);
      buffer[7] = '\0';
      if (strcmp(buffer, "TestEnd") == 0)
      {
         mulle_printf("SUCCESS: Insert at exact end\n");
      }
      else
      {
         mulle_printf("ERROR: Expected 'TestEnd', got '%s'\n", buffer);
         return 1;
      }
   }
   else
   {
      mulle_printf("ERROR: Insert at exact end failed\n");
      return 1;
   }
    
   // Test 10: Performance test with many small insertions
   mulle_printf("\nTest 10: Performance test\n");
   
   mulle_vm_reset(&vm);
   
   clock_t start = clock();
   for (int i = 0; i < 100; i++)
   {
      char insert[16];
      snprintf(insert, sizeof(insert), "%d", i);
      size_t pos = (i * 3) % (mulle_vm_get_length(&vm) + 1);
      rval = mulle_vm_insert(
         &vm, mulle_range_make(pos, strlen(insert)), insert);
      if (rval)
      {
         mulle_printf("ERROR: Performance test failed at iteration %d\n", i);
         return 1;
      }
   }
   clock_t end = clock();
   
   mulle_printf("SUCCESS: Performance test - 100 insertions in %.2f ms\n", 
      (double)(end - start) * 1000.0 / CLOCKS_PER_SEC);
   
   mulle_vm_done(&vm);
   
   mulle_printf("\nAll edge cases tests completed successfully!\n");
   return 0;
}