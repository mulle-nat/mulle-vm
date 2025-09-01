#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>

// Highlight specific paste scenario causing overlapping address bug
#define TEST_BUG_DEMO

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
   char dst_data[] = "The quick brown fox jumps over the lazy dog.";
   char src_data[] = "INSERTED TEXT";
   int result;

   mulle_printf("=== VM Paste Test ===\n\n");

   mulle_vm_debug_no_check_config          = mulle_vm_debug_config;
   mulle_vm_debug_no_check_config.shutdown = 0;

   // Initialize VMs
   mulle_vm_init(&src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);

   mulle_printf("Initial state:\n");

   // Add test data to both VMs
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
   mulle_vm_insert(&src_vm, mulle_range_make(0, strlen(src_data)), src_data);

   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   mulle_printf("\n");

#ifdef TEST_BEGINNING
   // Test 1: Paste at beginning (replace first 3 characters)
   mulle_printf("Test 1: Paste at beginning (replace 'The' with source data)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(0, 3), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
#endif

#ifdef TEST_MIDDLE
   // Test 2: Paste in middle (replace 'brown fox' with source data)
   mulle_printf("Test 2: Paste in middle (replace 'brown fox' with source data)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(10, 9), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
#endif

#ifdef TEST_END
   // Test 3: Paste at end (replace 'dog.' with source data)
   mulle_printf("Test 3: Paste at end (replace 'dog.' with source data)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(40, 4), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
#endif

#ifdef TEST_INSERT
   // Test 4: Insert without replacement (zero-length range in middle)
   mulle_printf("Test 4: Insert without replacement (zero-length range after 'quick')\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(9, 0), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
#endif

#ifdef TEST_REPLACE
   // Test 5: Replace entire content
   mulle_printf("Test 5: Replace entire content with source data\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make_all(), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");

   // Reset destination VM for next test
   mulle_vm_reset(&dst_vm);
   mulle_vm_insert(&dst_vm, mulle_range_make(0, strlen(dst_data)), dst_data);
#endif

#ifdef TEST_BUG_DEMO
   // Test: Insert without replacement (zero-length range)
   mulle_printf("Test: Insert without replacement (zero-length range after 'quick')\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(9, 0), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (should be unchanged)");
   dump_vm_content(&src_vm, "Source VM (should be unchanged)");
   mulle_printf("\n");
#endif

#ifdef TEST_EMPTY_SRC
   // Test 6: Paste with empty source (should just delete the range)
   mulle_printf("Test 6: Paste with empty source (should delete 'brown fox')\n");
   mulle_vm_reset(&src_vm);  // Make source empty
   result = mulle_vm_paste(&dst_vm, mulle_range_make(10, 9), &src_vm);
   mulle_printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   show_vm_info(&src_vm, "Source VM (empty)");
   dump_vm_content(&src_vm, "Source VM (empty)");
   mulle_printf("\n");
#endif

   mulle_printf("=== Paste Test Complete ===\n");

   // Cleanup
   mulle_vm_done(&dst_vm);
   mulle_vm_done(&src_vm);

   return 0;
}
