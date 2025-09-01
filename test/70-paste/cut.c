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
   mulle_fprintf(stderr, "%s: %lu vm pages %lu os pages, %lu bytes\n", name,
          (long) _mulle__rbtree_get_count( (struct mulle__rbtree *) &vm->tree), total_pages, info.vm_size);
   
   // Show detailed page information
   i = 0;
   vm_address = 0;
   struct mulle_vm_node *node;
   mulle_vm_tree_for( &vm->tree, node)
   {
      page = node->page;
      mulle_fprintf(stderr, "  Page %u: vm_addr=%lu, size=%lu, used=[%lu,%lu], retain_count=%td\n",
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
      mulle_fprintf(stderr, "%s content: (empty)\n", name);
      return;
   }

   s = mulle_vm_read(vm, mulle_range_make(0, info.vm_size), buffer);
   if (s) {
      fwrite(s, 1, info.vm_size, stdout);
      mulle_fprintf(stderr, " (length: %lu)\n", info.vm_size);
   } else {
      mulle_fprintf(stderr, "%s content: (read failed)\n", name);
   }
}


int main(void)
{
   struct mulle_vm_config  mulle_vm_debug_no_check_config;
   struct mulle_vm         src_vm, dst_vm;
   char test_data[] = "Hello World! This is test data for cut operations.";
   int result;

   mulle_printf("=== VM Cut Test ===\n\n");

   mulle_vm_debug_no_check_config          = mulle_vm_debug_config;
   mulle_vm_debug_no_check_config.shutdown = 0;

   // Initialize VMs
   mulle_vm_init(&src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);

   mulle_printf("Initial state:\n");

   // Add test data to source VM using insert
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);

   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   show_vm_info(&dst_vm, "Dest VM");
   mulle_printf("\n");

#ifdef TEST_FIRST
   // Test 1: Cut first 5 characters
   mulle_printf("Test 1: Cut first 5 characters (0-5)\n");
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(0, 5));
   mulle_printf("Cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (after cut)");
   dump_vm_content(&src_vm, "Source VM (after cut)");
   mulle_printf("\n");

   // Reset VMs for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_reset(&src_vm);
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);
#endif

#ifdef TEST_MIDDLE
   // Test 2: Cut middle section (6-11, "World!")
   mulle_printf("Test 2: Cut middle section (6-11)\n");
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(6, 6));
   mulle_printf("Cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (after cut)");
   dump_vm_content(&src_vm, "Source VM (after cut)");
   mulle_printf("\n");

   // Reset VMs for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_reset(&src_vm);
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);
#endif

#ifdef TEST_ENTIRE
   // Test 3: Cut entire content
   mulle_printf("Test 3: Cut entire content\n");
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make_all());
   mulle_printf("Cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (after cut)");
   dump_vm_content(&src_vm, "Source VM (after cut)");
   mulle_printf("\n");
#endif

#ifdef TEST_END
   // Test 4: Cut from end of string (last 10 characters)
   mulle_printf("Test 4: Cut from end (last 10 characters)\n");
   mulle_vm_done(&dst_vm);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);
   
   size_t src_len = strlen(test_data);
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(src_len - 10, 10));
   mulle_printf("Cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (after cut)");
   dump_vm_content(&src_vm, "Source VM (after cut)");
   mulle_printf("\n");
#endif

#ifdef TEST_MULTIPLE
   // Test 5: Multiple cut operations
   mulle_printf("Test 5: Multiple cut operations\n");
   mulle_vm_reset(&dst_vm);
   mulle_vm_reset(&src_vm);
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);
   
   // Cut "Hello"
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(0, 5));
   mulle_printf("First cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM after first cut");
   dump_vm_content(&dst_vm, "Dest VM after first cut");
   show_vm_info(&src_vm, "Source VM after first cut");
   dump_vm_content(&src_vm, "Source VM after first cut");
   
   // Cut " World!" (now at position 0 since we cut "Hello")
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(0, 7));
   mulle_printf("Second cut result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM after second cut");
   dump_vm_content(&dst_vm, "Dest VM after second cut");
   show_vm_info(&src_vm, "Source VM after second cut");
   dump_vm_content(&src_vm, "Source VM after second cut");
   mulle_printf("\n");
#endif

#ifdef TEST_INVALID
   // Test 6: Cut with invalid range (should fail gracefully)
   mulle_printf("Test 6: Cut with invalid range (beyond source)\n");
   mulle_vm_reset(&dst_vm);
   mulle_vm_reset(&src_vm);
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(test_data)), test_data);
   
   result = mulle_vm_cut(&dst_vm, &src_vm, mulle_range_make(0, strlen(test_data) + 100));
   mulle_printf("Cut result (should fail): %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM");
   mulle_printf("\n");
#endif

   mulle_printf("=== Cut Test Complete ===\n");

   // Cleanup
   mulle_vm_done(&dst_vm);
   mulle_vm_done(&src_vm);

   return 0;
}
