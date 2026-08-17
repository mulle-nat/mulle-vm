//
//  review-cow-sharing.c
//  mulle-vm
//
//  Validates REVIEW.md claim: "Copy-on-Write (COW): The implementation of
//  shared pages with reference counting and COW semantics is a standout
//  feature. This allows for extremely fast copy/paste operations between VM
//  instances without duplicating memory unless modifications occur."
//
//  Test: After mulle_vm_copy, pages should be shared (retain_count > 0).
//  Writing to the copy should trigger COW, leaving original unchanged.
//

#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


int   main( void)
{
   struct mulle_vm              src_vm, dst_vm;
   struct mulle_vm_config       no_check_config;
   struct mulle_vm_page         *src_page;
   struct mulle_vm_page         *dst_page;
   char                         buffer[ 64];
   const void                   *data;
   int                          result;
   intptr_t                     retain_before;
   intptr_t                     retain_after;

   mulle_printf( "=== REVIEW: COW Page Sharing Test ===\n\n");

   no_check_config          = mulle_vm_debug_config;
   no_check_config.shutdown = NULL;

   mulle_vm_init( &src_vm, &mulle_vm_debug_config, NULL);
   mulle_vm_init( &dst_vm, &no_check_config, NULL);

   // Insert data into source
   mulle_vm_insert( &src_vm, mulle_range_make( 0, 5), "Hello");

   // Get source page and check its retain count before copy
   src_page      = NULL;
   mulle_vm_for( &src_vm, page)
   {
      src_page = page;
      break;
   }

   retain_before = (intptr_t) _mulle_atomic_pointer_read_nonatomic( &src_page->retain_count);
   mulle_printf( "Source page retain_count before copy: %td\n", retain_before);

   // Copy range from src to dst - should share the page
   result = mulle_vm_copy( &dst_vm, &src_vm, mulle_range_make( 0, 5));
   mulle_printf( "Copy result: %d\n", result);

   // Check retain count increased (page is now shared)
   retain_after = (intptr_t) _mulle_atomic_pointer_read_nonatomic( &src_page->retain_count);
   mulle_printf( "Source page retain_count after copy: %td\n", retain_after);

   if( retain_after > retain_before)
      mulle_printf( "CONFIRMED: Page sharing via retain_count increase\n");
   else
      mulle_printf( "NOT CONFIRMED: retain_count did not increase\n");

   // Verify data in dst is correct
   data = mulle_vm_read( &dst_vm, mulle_range_make( 0, 5), buffer);
   mulle_printf( "Dst content: %.*s\n", 5, (char *) data);

   // Write to dst - should trigger COW
   mulle_vm_write( &dst_vm, mulle_range_make( 0, 5), "World");

   // Verify source is unchanged (COW preserved original)
   data = mulle_vm_read( &src_vm, mulle_range_make( 0, 5), buffer);
   mulle_printf( "Source after dst write: %.*s\n", 5, (char *) data);

   if( memcmp( data, "Hello", 5) == 0)
      mulle_printf( "CONFIRMED: COW preserved original data\n");
   else
      mulle_printf( "NOT CONFIRMED: Original data was modified\n");

   // Verify dst has new data
   data = mulle_vm_read( &dst_vm, mulle_range_make( 0, 5), buffer);
   mulle_printf( "Dst after write: %.*s\n", 5, (char *) data);

   mulle_printf( "\n=== COW Test Complete ===\n");

   mulle_vm_done( &dst_vm);
   mulle_vm_done( &src_vm);

   return( 0);
}
