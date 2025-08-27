#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TEST_1
#define TEST_2
#define TEST_3


static void show_vm_info(struct mulle_vm *vm, const char *name)
{
   struct mulle_vm_statistics info;
   size_t total_pages;
   struct mulle_vm_page *page;
   unsigned int i;

   info = mulle_vm_get_statistics(vm);
   total_pages = info.page_size / mulle_vm_get_pagesize(vm);
   printf("%s: %lu vm pages %lu os pages, %lu bytes\n", name,
          (long) mulle_structarray_get_count( &vm->mappings), total_pages, info.vm_size);
   
   // Show detailed page information
   i = 0;
   mulle_vm_mapping_for( vm, mapping)
   {
      page = mapping->page;
      printf("  Page %u: vm_addr=%lu, size=%lu, used=[%lu,%lu], retain_count=%td\n",
             i, mapping->address, page->size, page->used.location, page->used.length,
             (intptr_t) _mulle_atomic_pointer_read_nonatomic( &page->retain_count));
      ++i;
   }
}


static void dump_vm_content(struct mulle_vm *vm, const char *name)
{
   char buffer[1024];  // Larger buffer for content preview
   struct mulle_vm_statistics info;
   const void *s;
   size_t preview_size;

   info = mulle_vm_get_statistics(vm);
   if (info.vm_size == 0) {
      printf("%s content: (empty)\n", name);
      return;
   }

   // Only preview first 100 bytes for large data
   preview_size = info.vm_size > 100 ? 100 : info.vm_size;
   s = mulle_vm_read(vm, mulle_range_make(0, preview_size), buffer);
   if (s) {
      printf("%s content preview: ", name);
      fwrite(s, 1, preview_size, stdout);
      if (info.vm_size > 100) {
         printf("... (total length: %lu)\n", info.vm_size);
      } else {
         printf(" (length: %lu)\n", info.vm_size);
      }
   } else {
      printf("%s content: (read failed)\n", name);
   }
}

int main(void)
{
   struct mulle_vm_config  mulle_vm_debug_no_check_config;
   struct mulle_vm         src_vm, dst_vm;
   char test_data[] = "STRESS TEST DATA - This is a moderately sized test string for paste operations.";
   size_t data_size = strlen(test_data);
   int result;

   printf("=== VM Paste Stress Test ===\n\n");

   mulle_vm_debug_no_check_config          = mulle_vm_debug_config;
   mulle_vm_debug_no_check_config.shutdown = 0;

   // Initialize VMs
   mulle_vm_init(&src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init(&dst_vm, &mulle_vm_debug_no_check_config, NULL);

   printf("Initial state:\n");

   // Add test data to source VM
   mulle_vm_insert(&src_vm, mulle_range_make(0, data_size), test_data);

   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");
   printf("\n");

#ifdef TEST_1
   // First paste operation - paste entire source at beginning
   printf("Test 1: First paste operation (paste at beginning)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(0, 0), &src_vm);
   printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM after first paste");
   dump_vm_content(&dst_vm, "Dest VM after first paste");
   printf("\n");
#endif

#ifdef TEST_2
   // Second paste operation - paste source at end (append)
   printf("Test 2: Second paste operation (paste at end - append)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(data_size, 0), &src_vm);
   printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM after second paste");
   dump_vm_content(&dst_vm, "Dest VM after second paste");
   printf("\n");
#endif

#ifdef TEST_3
   // Third paste operation - paste source in the middle (replace some content)
   printf("Test 3: Third paste operation (paste in middle - replace)\n");
   result = mulle_vm_paste(&dst_vm, mulle_range_make(data_size / 2, data_size / 4), &src_vm);
   printf("Paste result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM after third paste");
   dump_vm_content(&dst_vm, "Dest VM after third paste");
   printf("\n");
#endif

   // Verify source VM is unchanged
   printf("Verify source VM unchanged:\n");
   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");
   printf("\n");

   printf("=== Paste Stress Test Complete ===\n");

   // Cleanup
   mulle_vm_done(&dst_vm);
   mulle_vm_done(&src_vm);

   return 0;
}
