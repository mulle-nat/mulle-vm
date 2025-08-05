#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>


MULLE__VM_GLOBAL
struct mulle_vm_page *
   _mulle_vm_new_page( struct mulle_vm *vm, size_t length);

// used by some testing code
static inline void   mulle_vm_add_page( struct mulle_vm *vm,
                                        struct mulle_vm_page *page)
{
   MULLE__VM_GLOBAL
   MULLE_C_NONNULL_FIRST
   void   _mulle_vm_add_page( struct mulle_vm *vm,
                              struct mulle_vm_page *page);
   if( ! vm)
      return;
   _mulle_vm_add_page( vm, page);
}


MULLE__VM_GLOBAL
void   _mulle_vm_remove_mapping_at_index( struct mulle_vm *vm,
                                          uintptr_t index);


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                 total_pages;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize( vm);
   printf( "%lu pages, %lu bytes\n", total_pages, info.vm_size);
}


int   main( void)
{
   struct mulle_vm         vm;
   struct mulle_vm_page    *page;
   char                    buf[ 1024];

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   show_vm( &vm);

   page = _mulle_vm_new_page( &vm, 32);
   mulle_vm_add_page( &vm, page);
   show_vm( &vm);

   _mulle_vm_remove_mapping_at_index( &vm, 0);
   show_vm( &vm);

   mulle_vm_done( &vm);

   return( 0);
}