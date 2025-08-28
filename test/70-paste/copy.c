#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>

#define TEST_FIRST
#define TEST_MIDDLE
#define TEST_END
#define TEST_MULTIPLE
#define TEST_INVALID
#define TEST_ENTIRE

static void show_vm_info(struct mulle_vm *vm, const char *name)
{
   struct mulle_vm_statistics info;
   size_t total_pages;
   struct mulle_vm_page *page;
   unsigned int i;
   uintptr_t vm_address;

   info = mulle_vm_get_statistics(vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   printf("%s: %lu vm pages %lu os pages, %lu bytes\n", name,
          (long) _mulle__rbtree_get_count( (struct mulle__rbtree *) &vm->tree), total_pages, info.vm_size);
   
   // Show detailed page information
   i = 0;
   vm_address = 0;
   struct mulle_vm_node *node;
   mulle_vm_tree_for( &vm->tree, node)
   {
      page = node->page;
      printf("  Page %u: vm_addr=%lu, size=%lu, used=[%lu,%lu], retain_count=%td\n",
             i, vm_address, page->size, page->used.location, page->used.length,
             (intptr_t) _mulle_atomic_pointer_read_nonatomic( &page->retain_count));
      vm_address += page->used.length;
      ++i;
   }
}


static void dump_vm_content(struct mulle_vm *vm, const char *name)
{
   char buffer[256];
   struct mulle_vm_statistics info;
   const void *s;

   info = mulle_vm_get_statistics(vm);
   if (info.vm_size == 0) {
      printf("%s content: (empty)\n", name);
      return;
   }

   s = mulle_vm_read(vm, mulle_range_make(0, info.vm_size), buffer);
   if (s) {
      fwrite(s, 1, info.vm_size, stdout);
      printf(" (length: %lu)\n", info.vm_size);
   } else {
      printf("%s content: (read failed)\n", name);
   }
}


int main(void)
{
   struct mulle_vm_config  mulle_vm_debug_no_check_config;
   struct mulle_vm         src_vm, dst_vm;
   char test_data[] = "Hello World! This is test data for copy operations.";
   int result;

   printf("=== VM Copy Test ===\n\n");

   mulle_vm_debug_no_check_config          = mulle_vm_debug_config;
   mulle_vm_debug_no_check_config.shutdown = 0;

   // Initialize VMs
   mulle_vm_init(&src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);

   printf("Initial state:\n");

   // Add test data to source VM using insert
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);

   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   show_vm_info(&dst_vm, "Dest VM");
   printf("\n");

#ifdef TEST_FIRST
   // Test 1: Copy first 5 characters
   printf("Test 1: Copy first 5 characters (0-5)\n");
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(0, 5));
   printf("Copy result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
#endif

#ifdef TEST_MIDDLE
   // Test 2: Copy middle section (6-11, "World!")
   printf("Test 2: Copy middle section (6-11)\n");
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(6, 6));
   printf("Copy result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
#endif

#ifdef TEST_ENTIRE
   // Test 3: Copy entire content
   printf("Test 3: Copy entire content\n");
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make_all());
   printf("Copy result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");
#endif

#ifdef TEST_END
   // Test 4: Copy from end of string (last 10 characters)
   printf("Test 4: Copy from end (last 10 characters)\n");
   mulle_vm_done(&dst_vm);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);
   
   size_t src_len = strlen(test_data);
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(src_len - 10, 10));
   printf("Copy result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");
#endif

#ifdef TEST_MULTIPLE
   // Test 5: Multiple copy operations (append)
   printf("Test 5: Multiple copy operations\n");
   mulle_vm_reset(&dst_vm);
   
   // Copy "Hello"
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(0, 5));
   printf("First copy result: %d\n", result);
   
   // Copy " World!"
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(5, 7));
   printf("Second copy result: %d\n", result);
   
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");
#endif

#ifdef TEST_INVALID
   // Test 6: Copy with invalid range (should fail gracefully)
   printf("Test 6: Copy with invalid range (beyond source)\n");
   mulle_vm_reset(&dst_vm);
   
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(0, strlen(test_data) + 100));
   printf("Copy result (should fail): %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   printf("\n");
#endif

   // Verify source VM is unchanged
   printf("Verify source VM unchanged:\n");
   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   printf("\n");

   printf("=== Copy Test Complete ===\n");

   // Cleanup
   mulle_vm_done(&dst_vm);
   mulle_vm_done(&src_vm);

   return 0;
}
