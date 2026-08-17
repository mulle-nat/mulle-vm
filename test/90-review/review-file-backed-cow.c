//
//  review-file-backed-cow.c
//  mulle-vm
//
//  Validates REVIEW.md claim: "File-Backed Write Crash: When initializing
//  a VM from a file (mulle_vm_init_with_contents_of_file), pages are added
//  with a retain_count of 0. Since they are not marked as 'shared,'
//  _mulle_vm_will_write_page will not trigger COW when a write occurs.
//  This will lead to a SIGSEGV if the underlying file mapping is read-only."
//
//  Test: Load a file into a VM, check that pages have retain_count == 0,
//  and verify they are NOT considered "shared" (mulle_vm_page_is_shared
//  returns false). This confirms the preconditions for the bug.
//  We do NOT actually trigger the SIGSEGV (that would crash the test).
//

#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-file.h>
#include <stdio.h>
#include <string.h>


int   main( void)
{
   struct mulle_vm          vm;
   struct mulle_vm_page     *page;
   intptr_t                 retain;
   int                      rval;
   int                      is_shared;
   unsigned int             n_pages;

   mulle_printf( "=== REVIEW: File-Backed COW Bug Test ===\n\n");

   rval = mulle_vm_init_with_contents_of_file( &vm, "test_data.txt", NULL);
   if( rval)
   {
      mulle_printf( "Could not load test_data.txt (rval=%d)\n", rval);
      mulle_printf( "SKIP: No test data file available\n");
      return( 0);
   }

   mulle_printf( "File loaded successfully\n");

   // Check pages
   n_pages = 0;
   mulle_vm_for( &vm, current_page)
   {
      page      = current_page;
      retain    = (intptr_t) _mulle_atomic_pointer_read_nonatomic( &page->retain_count);
      is_shared = mulle_vm_page_is_shared( page);

      mulle_printf( "Page %u: retain_count=%td, is_shared=%d\n",
                    n_pages, retain, is_shared);
      n_pages++;
   }

   mulle_printf( "\nTotal pages from file: %u\n", n_pages);

   if( n_pages > 0)
   {
      // Get the first page again for the final check
      mulle_vm_for( &vm, first_page)
      {
         page = first_page;
         break;
      }

      retain    = (intptr_t) _mulle_atomic_pointer_read_nonatomic( &page->retain_count);
      is_shared = mulle_vm_page_is_shared( page);

      if( retain == 0 && ! is_shared)
      {
         mulle_printf( "\nCONFIRMED: File-backed pages have retain_count=0 and are NOT shared\n");
         mulle_printf( "This means _mulle_vm_will_write_page will NOT trigger COW.\n");
         mulle_printf( "Writing to a read-only mmap'd page would SIGSEGV.\n");
      }
      else if( is_shared)
      {
         mulle_printf( "\nNOT CONFIRMED: Pages are marked as shared (bug may be fixed)\n");
      }
      else
      {
         mulle_printf( "\nUNCLEAR: retain_count=%td, is_shared=%d\n", retain, is_shared);
      }
   }

   mulle_printf( "\n=== File-Backed COW Bug Test Complete ===\n");

   mulle_vm_done( &vm);
   return( 0);
}
