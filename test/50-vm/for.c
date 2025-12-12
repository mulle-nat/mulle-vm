#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   statistics;
   size_t                       total_pages;

   statistics  = mulle_vm_get_statistics( vm);
   total_pages = statistics.page_size / mulle_vm_get_pagesize();
   mulle_printf( "%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}


static void   dump_vm( struct mulle_vm *vm)
{
   char                         buf[ 1024];
   const void                   *s;
   struct mulle_vm_statistics   statistics;
   struct mulle_vm_page         *page;
   struct mulle_vm_node         *node;
   unsigned int                 i;
   uintptr_t                    vm_address = 0;

   statistics = mulle_vm_get_statistics( vm);

   s = mulle_vm_read( vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
      fwrite( s, 1, statistics.vm_size, stdout);
   mulle_printf( "\n");


   mulle_printf( "VM:\n");
   fflush( stdout);
   mulle_fprintf( stderr, "adr-end (size-[fhole,ehole])\n");

   i = 0;
   mulle_vm_tree_for( &vm->tree, node)
   {
      page       = node->page;
      ++i;

      // output vm range, size of pages, and hole in front and back
      fflush( stdout);
      mulle_fprintf( stderr, "@%p: ", page);

      mulle_printf( "\t%2td-%-2td (%td-[%td,%td])\n",
         vm_address,
         vm_address + page->used.length,
         page->size,
         page->used.location,
         page->size - mulle_range_get_max( page->used));

      vm_address += page->used.length;
   }
   mulle_printf( "\n");
}


static void   test_mulle_vm_for_empty( void)
{
   struct mulle_vm   vm;
   int               count;
   uintptr_t         vm_address;
   struct mulle_vm_node *node;

   mulle_printf( "== Test mulle_vm_mapping_for with empty VM ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   show_vm( &vm);

   count = 0;
   vm_address = 0;
   mulle_vm_tree_for( &vm.tree, node)
   {
      mulle_printf( "Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
              count,
              vm_address,
              node->page->size,
              node->page->used.location,
              node->page->used.location + node->page->used.length);
      vm_address += node->page->used.length;
      count++;
   }
   mulle_printf( "Total pages enumerated: %d\n\n", count);

   mulle_vm_done( &vm);
}


static void   test_mulle_vm_for_single_page( void)
{
   struct mulle_vm   vm;
   int               count;
   uintptr_t         vm_address;
   struct mulle_vm_node *node;

   mulle_printf( "== Test mulle_vm_mapping_for with single page ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   mulle_vm_append( &vm, "Hello", 5);
   show_vm( &vm);

   count = 0;
   vm_address = 0;
   mulle_vm_tree_for( &vm.tree, node)
   {
      mulle_printf( "Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
              count,
              vm_address,
              node->page->size,
              node->page->used.location,
              node->page->used.location + node->page->used.length);
      vm_address += node->page->used.length;
      count++;
   }
   mulle_printf( "Total pages enumerated: %d\n\n", count);

   mulle_vm_done( &vm);
}


static void   test_mulle_vm_for_multiple_pages( void)
{
   struct mulle_vm   vm;
   int               count;
   uintptr_t         vm_address;
   struct mulle_vm_node *node;

   mulle_printf( "== Test mulle_vm_mapping_for with multiple pages ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Create multiple pages by adding data that exceeds page size
   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);
   mulle_vm_append( &vm, "mnop", 4);
   mulle_vm_append( &vm, "qrst", 4);
   mulle_vm_append( &vm, "uvwx", 4);
   mulle_vm_append( &vm, "yz", 2);
   show_vm( &vm);

   count = 0;
   vm_address = 0;
   mulle_vm_tree_for( &vm.tree, node)
   {
      mulle_printf( "Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
              count,
              vm_address,
              node->page->size,
              node->page->used.location,
              node->page->used.location + node->page->used.length);
      vm_address += node->page->used.length;
      count++;
   }
   mulle_printf( "Total pages enumerated: %d\n\n", count);

   mulle_vm_done( &vm);
}


static void   test_mulle_vm_for_null_vm( void)
{
   struct mulle_vm_tree   *vm;
   int                    count;
   uintptr_t              vm_address;
   struct mulle_vm_node   *node;

   mulle_printf( "== Test mulle_vm_mapping_for with NULL VM ==\n");
   vm = NULL;

   count = 0;
   vm_address = 0;
   mulle_vm_tree_for( vm, node)
   {
      mulle_printf( "Page %d: address=%lu, size=%lu, used=[%lu,%lu)\n",
              count,
              vm_address,
              node->page->size,
              node->page->used.location,
              node->page->used.location + node->page->used.length);
      vm_address += node->page->used.length;
      count++;
   }
   mulle_printf( "Total pages enumerated: %d\n\n", count);
}


static void   test_mulle_vm_for_detailed_info( void)
{
   struct mulle_vm   vm;
   int               count;
   char              buf[1024];
   const void        *s;
   uintptr_t         vm_address;
   struct mulle_vm_node *node;

   mulle_printf( "== Test mulle_vm_mapping_for with detailed page info ==\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Create content that will span multiple pages
   mulle_vm_append( &vm, "ABC", 3);
   mulle_vm_append( &vm, "DEF", 3);
   mulle_vm_append( &vm, "GHI", 3);
   mulle_vm_append( &vm, "JKL", 3);
   mulle_vm_append( &vm, "MNO", 3);
   show_vm( &vm);

   // Show the VM content
   struct mulle_vm_statistics statistics = mulle_vm_get_statistics( &vm);
   s = mulle_vm_read( &vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
   {
      mulle_printf( "VM content: ");
      fwrite( s, 1, statistics.vm_size, stdout);
      mulle_printf( "\n");
   }

   count = 0;
   vm_address = 0;
   mulle_vm_tree_for( &vm.tree, node)
   {
      mulle_printf( "Page %d: address=%lu, size=%lu, used=[%lu,%lu), holes=[front:%lu, back:%lu]\n",
              count,
              vm_address,
              node->page->size,
              node->page->used.location,
              node->page->used.location + node->page->used.length,
              node->page->used.location,
              node->page->size - (node->page->used.location + node->page->used.length));
      vm_address += node->page->used.length;
      count++;
   }
   mulle_printf( "Total pages enumerated: %d\n\n", count);

   mulle_vm_done( &vm);
}


int   main( void)
{
   mulle_printf( "Testing mulle_vm_mapping_for iterator function\n");
   mulle_printf( "=====================================\n\n");

   test_mulle_vm_for_empty();
   test_mulle_vm_for_single_page();
   test_mulle_vm_for_multiple_pages();
   test_mulle_vm_for_null_vm();
   test_mulle_vm_for_detailed_info();

   mulle_printf( "All mulle_vm_mapping_for tests completed.\n");
   return( 0);
}
