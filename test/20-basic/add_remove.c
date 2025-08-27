#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>


MULLE__VM_GLOBAL
struct mulle_vm_page *
   _mulle_vm_new_page_with_bytes( struct mulle_vm *vm, void *bytes, size_t length);


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                       total_pages;
   unsigned int                 i;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   printf( "%lu OS pages, %lu bytes\n", total_pages, info.vm_size);

   i = 0;
   mulle_vm_for( vm, page)
   {
      printf( "   %u: rc=%ld\n", i, mulle_vm_page_get_retaincount( page));
   }
}


int   main( void)
{
   struct mulle_vm         vm;
   struct mulle_vm_page    *page;
   char                    buf[ 1024];
   char                    text[ 32];

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   show_vm( &vm);

   strcpy( text, "Hello World!");
   page = _mulle_vm_new_page_with_bytes( &vm, text, 32);
   mulle_vm_tree_add_page( &vm.tree, page);
   mulle_vm_page_release( page);

   show_vm( &vm);

   mulle_vm_tree_remove_page( &vm.tree, page);
   show_vm( &vm);

   mulle_vm_done( &vm);

   return( 0);
}