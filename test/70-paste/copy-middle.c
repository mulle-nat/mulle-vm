#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


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
   struct mulle_vm src_vm, dst_vm;
   char test_data1[] = "Hello World!";
   char test_data2[] = " - ";
   char test_data3[] = "This is test data for copy operations.";
   int result;

   // Initialize VMs
   mulle_vm_init(&src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init(&dst_vm, NULL, NULL);

   // Add test data to source VM using insert
   mulle_vm_append(&src_vm, test_data1, strlen( test_data1));
   mulle_vm_append(&src_vm, test_data2, strlen( test_data2));
   mulle_vm_append(&src_vm, test_data3, strlen( test_data3));

   mulle_printf("Initial state after adding test data to source:\n");
   show_vm_info(&src_vm, "Source VM");
   dump_vm_content(&src_vm, "Source VM");

   // Test 2: Copy middle section (6-11, "World!")
   mulle_printf("\nTest 2: Copy middle section (6-11)\n");
   result = mulle_vm_copy(&dst_vm, &src_vm, mulle_range_make(6, 6));
   mulle_printf("Copy result: %d\n", result);
   show_vm_info(&dst_vm, "Dest VM");
   dump_vm_content(&dst_vm, "Dest VM");

   // Cleanup
   mulle_printf("Cleanup Dest VM\n");
   mulle_vm_done(&dst_vm);
   mulle_printf("Cleanup Source VM\n");
   mulle_vm_done(&src_vm);

   return 0;
}

