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
   struct mulle_vm_node         *node;
   struct mulle_vm_page         *page;
   unsigned int                 i;
   uintptr_t                    vm_address;

   statistics = mulle_vm_get_statistics( vm);

   s = mulle_vm_read( vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
      fwrite( s, 1, statistics.vm_size, stdout);
   mulle_printf( "\n");


   mulle_printf( "VM:\n");
   fflush( stdout);
   mulle_fprintf( stderr, "adr-end (size-[fhole,ehole])\n");

   i = 0;
   vm_address = 0;
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




int   main( void)
{
   struct mulle_vm        vm;
   struct mulle_vm_statistics   info;

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   mulle_printf( "== after init ==\n");
   show_vm( &vm);

   // build from 0
   mulle_printf( "\n== build from 0 ==\n");
   // create multiple pages
   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);
   mulle_vm_append( &vm, "mnop", 4);
   mulle_vm_append( &vm, "qrst", 4);
   mulle_vm_append( &vm, "uvw", 3);
   mulle_vm_append( &vm, "xyz", 3);
   show_vm( &vm);
   dump_vm( &vm);

   // insert in the middle
   mulle_printf( "== delete 'f' in the middle ==\n");
   mulle_vm_delete( &vm, mulle_range_make( 5, 1));
   show_vm( &vm);
   dump_vm( &vm);

   // delete 'a' in front
   mulle_printf( "== delete 'a' in front ==\n");
   mulle_vm_delete( &vm, mulle_range_make( 0, 1));
   show_vm( &vm);
   dump_vm( &vm);

   mulle_printf( "== delete 'z' in back ==\n");
   info = mulle_vm_get_statistics( &vm);
   mulle_vm_delete( &vm, mulle_range_make( info.vm_size - 1, 1));
   show_vm( &vm);
   dump_vm( &vm);

   mulle_printf( "== delete { 2, 12} (more than a page size) ==\n");
   mulle_vm_delete( &vm, mulle_range_make( 2, 8 + 2 + 2));
   show_vm( &vm);
   dump_vm( &vm);

   mulle_vm_done( &vm);

   return( 0);
}
