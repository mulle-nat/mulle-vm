#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                 total_pages;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   printf( "%lu pages, %lu bytes\n", total_pages, info.vm_size);
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


static void  test_with_range( uintptr_t location, uintptr_t length)
{
   struct mulle_vm      vm;
   struct mulle_range   range;

   range = mulle_range_make( location, length);

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);
   mulle_vm_append( &vm, "mnop", 4);
   mulle_vm_append( &vm, "qrst", 4);
   mulle_vm_append( &vm, "uvw", 3);
   mulle_vm_append( &vm, "xyz", 3);

   printf( "== delete {%td, %td} ==\n", range.location, range.length);
   mulle_vm_delete( &vm, range);
   show_vm( &vm);
   dump_vm( &vm);

   mulle_vm_done( &vm);
}


int   main( void)
{
   test_with_range( 0, 8);
   test_with_range( 1, 8);
   test_with_range( 7, 8);
   test_with_range( 7, 9);
   test_with_range( 7, 10);

   test_with_range( 8, 7);
   test_with_range( 8, 8);
   test_with_range( 8, 9);

   test_with_range( 0, 26);
   test_with_range( 1, 25);
   test_with_range( 0, 25);
   test_with_range( 1, 24);

   return( 0);
}