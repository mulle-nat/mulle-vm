#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>

static void show_vm(struct mulle_vm *vm)
{
   struct mulle_vm_statistics statistics;
   size_t total_pages;

   statistics = mulle_vm_get_statistics(vm);
   total_pages = statistics.page_size / mulle_vm_get_pagesize(vm);
   printf("%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}


static void test_mulle_vm_for_reverse_empty(void)
{
   struct mulle_vm vm;
   int count;

   printf("== Test mulle_vm_mapping_for_reverse with empty VM ==\n");
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
   show_vm(&vm);

   struct mulle_vm_statistics statistics = mulle_vm_get_statistics(&vm);
   size_t total_pages = statistics.page_size / mulle_vm_get_pagesize(&vm);

   count = 0;
   mulle_vm_mapping_for_reverse(&vm, info)
   {
      printf("Page %zu: address=%lu, size=%lu, used=[%lu,%lu)\n",
             total_pages - count - 1,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Total pages enumerated: %d\n\n", count);

   mulle_vm_done(&vm);
}


static void test_mulle_vm_for_reverse_single_page(void)
{
   struct mulle_vm vm;
   int count;

   printf("== Test mulle_vm_mapping_for_reverse with single page ==\n");
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);

   mulle_vm_append(&vm, "Hello", 5);
   show_vm(&vm);

   struct mulle_vm_statistics statistics = mulle_vm_get_statistics(&vm);
   size_t total_pages = statistics.page_size / mulle_vm_get_pagesize(&vm);

   count = 0;
   mulle_vm_mapping_for_reverse(&vm, info)
   {
      printf("Page %zu: address=%lu, size=%lu, used=[%lu,%lu)\n",
             total_pages - count - 1,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Total pages enumerated: %d\n\n", count);

   mulle_vm_done(&vm);
}


static void test_mulle_vm_for_reverse_multiple_pages(void)
{
   struct mulle_vm vm;
   int count;

   printf("== Test mulle_vm_mapping_for_reverse with multiple pages ==\n");
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);

   // Add data exceeding page size to create multiple pages
   mulle_vm_append(&vm, "abcd", 4);
   mulle_vm_append(&vm, "efgh", 4);
   mulle_vm_append(&vm, "ijkl", 4);
   mulle_vm_append(&vm, "mnop", 4);
   mulle_vm_append(&vm, "qrst", 4);
   mulle_vm_append(&vm, "uvwx", 4);
   mulle_vm_append(&vm, "yz", 2);
   show_vm(&vm);

   struct mulle_vm_statistics statistics = mulle_vm_get_statistics(&vm);
   size_t total_pages = statistics.page_size / mulle_vm_get_pagesize(&vm);

   count = 0;
   mulle_vm_mapping_for_reverse(&vm, info)
   {
      printf("Page %zu: address=%lu, size=%lu, used=[%lu,%lu)\n",
             total_pages - count - 1,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Total pages enumerated: %d\n\n", count);

   mulle_vm_done(&vm);
}


static void test_mulle_vm_for_reverse_null_vm(void)
{
   struct mulle_vm *vm;
   int count;

   printf("== Test mulle_vm_mapping_for_reverse with NULL VM ==\n");
   vm = NULL;

   // total_pages must be 0 when vm is NULL, so just print index as count for safety
   count = 0;
   mulle_vm_mapping_for_reverse(vm, info)
   {
      printf("Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
             count,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Total pages enumerated: %d\n\n", count);
}


static void test_mulle_vm_for_reverse_detailed_info(void)
{
   struct mulle_vm vm;
   int count;
   char buf[1024];
   const void *s;

   printf("== Test mulle_vm_mapping_for_reverse with detailed page info ==\n");
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);

   // Create content that spans multiple pages
   mulle_vm_append(&vm, "ABC", 3);
   mulle_vm_append(&vm, "DEF", 3);
   mulle_vm_append(&vm, "GHI", 3);
   mulle_vm_append(&vm, "JKL", 3);
   mulle_vm_append(&vm, "MNO", 3);
   show_vm(&vm);

   // Show VM content
   struct mulle_vm_statistics statistics = mulle_vm_get_statistics(&vm);
   s = mulle_vm_read(&vm, mulle_range_make(0, statistics.vm_size), buf);
   if (s)
   {
      printf("VM content: ");
      fwrite(s, 1, statistics.vm_size, stdout);
      printf("\n");
   }

   size_t total_pages = statistics.page_size / mulle_vm_get_pagesize(&vm);

   count = 0;
   mulle_vm_mapping_for_reverse(&vm, info)
   {
      printf("Page %zu: address=%lu, size=%lu, used=[%lu,%lu), holes=[front:%lu, back:%lu]\n",
             total_pages - count - 1,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length,
             info->page->used.location,
             info->page->size - (info->page->used.location + info->page->used.length));
      count++;
   }
   printf("Total pages enumerated: %d\n\n", count);

   mulle_vm_done(&vm);
}


// New test for comparison of forward and reverse iteration with matching indices
static void test_mulle_vm_for_reverse_comparison(void)
{
   struct mulle_vm vm;
   int count;
   char buf[1024];
   const void *s;

   printf("== Test mulle_vm_mapping_for_reverse comparison with forward ==\n");
   mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);

   // Create content that will span multiple pages
   mulle_vm_append(&vm, "AB", 2);
   mulle_vm_append(&vm, "CD", 2);
   mulle_vm_append(&vm, "EF", 2);
   mulle_vm_append(&vm, "GH", 2);
   mulle_vm_append(&vm, "IJ", 2);
   show_vm(&vm);

   struct mulle_vm_statistics vm_info = mulle_vm_get_statistics(&vm);
   s = mulle_vm_read(&vm, mulle_range_make(0, vm_info.vm_size), buf);
   if (s)
   {
      printf("VM content: ");
      fwrite(s, 1, vm_info.vm_size, stdout);
      printf("\n");
   }

   size_t total_pages = vm_info.page_size / mulle_vm_get_pagesize(&vm);

   // Forward iteration
   printf("Forward iteration:\n");
   count = 0;
   mulle_vm_mapping_for(&vm, info)
   {
      printf("  Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
             count,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Forward pages enumerated: %d\n", count);

   // Reverse iteration
   printf("Reverse iteration:\n");
   count = 0;
   mulle_vm_mapping_for_reverse(&vm, info)
   {
      printf("  Page %zu: address=%lu, size=%lu, used=[%lu,%lu)\n",
             total_pages - count - 1,
             info->address,
             info->page->size,
             info->page->used.location,
             info->page->used.location + info->page->used.length);
      count++;
   }
   printf("Reverse pages enumerated: %d\n\n", count);

   mulle_vm_done(&vm);
}


int main(void)
{
   printf("Testing mulle_vm_mapping_for_reverse iterator function\n");
   printf("=====================================\n\n");

   test_mulle_vm_for_reverse_empty();
   test_mulle_vm_for_reverse_single_page();
   test_mulle_vm_for_reverse_multiple_pages();
   test_mulle_vm_for_reverse_null_vm();
   test_mulle_vm_for_reverse_detailed_info();
   test_mulle_vm_for_reverse_comparison();

   printf("All mulle_vm_mapping_for_reverse tests completed.\n");
   return 0;
}
