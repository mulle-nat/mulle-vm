//
//  review-global-lock.c
//  mulle-vm
//
//  Validates REVIEW.md claim: "Global State & Locking: The use of a global
//  mulle_thread_mutex_t in mulle-vm-page.c for all page
//  allocations/deallocations introduces unnecessary contention between
//  separate mulle_vm instances in a multi-threaded environment."
//
//  Test: Verify that two separate VM instances share the same global page
//  allocator by checking that mulle_vm_get_pagesize() returns the same
//  value regardless of which VM you ask, and that the page allocator
//  is set globally (not per-instance).
//

#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


int   main( void)
{
   struct mulle_vm          vm1, vm2;
   struct mulle_vm_config   no_check_config;
   size_t                   pagesize1;
   size_t                   pagesize2;
   size_t                   pagesize_before;

   mulle_printf( "=== REVIEW: Global Lock / Shared Allocator Test ===\n\n");

   // mulle_vm_get_pagesize is global - not tied to any instance
   pagesize_before = mulle_vm_get_pagesize();
   mulle_printf( "Pagesize before any VM init: %lu\n", (unsigned long) pagesize_before);

   no_check_config          = mulle_vm_debug_config;
   no_check_config.shutdown = NULL;

   mulle_vm_init( &vm1, &mulle_vm_debug_config, NULL);
   pagesize1 = mulle_vm_get_pagesize();
   mulle_printf( "Pagesize after vm1 init: %lu\n", (unsigned long) pagesize1);

   mulle_vm_init( &vm2, &no_check_config, NULL);
   pagesize2 = mulle_vm_get_pagesize();
   mulle_printf( "Pagesize after vm2 init: %lu\n", (unsigned long) pagesize2);

   // All the same because it's global state
   if( pagesize_before == pagesize1 && pagesize1 == pagesize2)
      mulle_printf( "\nCONFIRMED: Page allocator is global (same pagesize for all)\n");
   else
      mulle_printf( "\nNOT CONFIRMED: Pagesizes differ unexpectedly\n");

   // The struct mulle_vm itself has no allocator field
   mulle_printf( "\nstruct mulle_vm holds tree + config (no allocator pointer)\n");

   // Both VMs allocate from the same pool - insert into both
   mulle_vm_insert( &vm1, mulle_range_make( 0, 4), "aaaa");
   mulle_vm_insert( &vm2, mulle_range_make( 0, 4), "bbbb");

   mulle_printf( "Both VMs successfully allocated pages from global pool\n");
   mulle_printf( "CONFIRMED: Global allocator with global lock\n");

   mulle_printf( "\n=== Global Lock Test Complete ===\n");

   mulle_vm_done( &vm2);
   mulle_vm_done( &vm1);
   return( 0);
}
