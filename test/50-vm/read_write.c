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
   printf( "%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}


static void   dump_vm( struct mulle_vm *vm)
{
   char                         buf[ 1024];
   const void                   *s;
   struct mulle_vm_statistics   statistics;
   struct mulle_vm_mapping      *info;
   struct mulle_vm_page         *page;
   unsigned int                 i;
   uintptr_t                    vm_address;

   statistics = mulle_vm_get_statistics( vm);

   s = mulle_vm_read( vm, mulle_range_make( 0, statistics.vm_size), buf);
   if( s)
      fwrite( s, 1, statistics.vm_size, stdout);
   printf( "\n");


   printf( "VM:\n");
   fflush( stdout);
   fprintf( stderr, "adr-end (size-[fhole,ehole])\n");

   i = 0;
   mulle_structarray_for( &vm->mappings, info)
   {
      page       = info->page;
      vm_address = info->address;
      ++i;

      // output vm range, size of pages, and hole in front and back
      fflush( stdout);
      fprintf( stderr, "@%p: ", page);

      printf( "\t%2td-%-2td (%td-[%td,%td])\n",
         vm_address,
         vm_address + page->used.length,
         page->size,
         page->used.location,
         page->size - mulle_range_get_max( page->used));
   }
   printf( "\n");
}


int   main( void)
{
   struct mulle_vm        vm;
   struct mulle_vm_statistics   info;

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   printf( "== after init ==\n");
   show_vm( &vm);

   // build from 0
   printf( "\n== build from 0 ==\n");
   mulle_vm_insert( &vm, mulle_range_make( 0, 12), "abcdefghijkl");
   show_vm( &vm);
   dump_vm( &vm);

   // write in the middle (overwrite)
   printf( "== write in the middle (overwrite) ==\n");
   mulle_vm_write( &vm, mulle_range_make( 6, 4), "ZZZZ");
   show_vm( &vm);
   dump_vm( &vm);

   // write at start (overwrite)
   printf( "== write at start (overwrite) ==\n");
   mulle_vm_write( &vm, mulle_range_make( 0, 4), "XXXX");
   show_vm( &vm);
   dump_vm( &vm);

   // write at end (overwrite)
   printf( "== write at end (overwrite) ==\n");
   info = mulle_vm_get_statistics( &vm);
   mulle_vm_write( &vm, mulle_range_make( info.vm_size - 4, 4), "YYYY");
   show_vm( &vm);
   dump_vm( &vm);

   // write beyond end (extend)
   printf( "== write beyond end (extend) ==\n");
   info = mulle_vm_get_statistics( &vm);
   mulle_vm_write( &vm, mulle_range_make( info.vm_size, 4), "!!!!");
   show_vm( &vm);
   dump_vm( &vm);

   // write in hole (extend)
   printf( "== write in hole (extend) ==\n");
   mulle_vm_write( &vm, mulle_range_make( 20, 4), "HOLE");
   show_vm( &vm);
   dump_vm( &vm);

   mulle_vm_done( &vm);

   return( 0);
}