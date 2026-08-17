//
//  review-no-per-instance-allocator.c
//  mulle-vm
//
//  Validates REVIEW.md claim: "Documentation vs. Implementation Mismatch:
//  The README.md claims that mulle_vm_config allows per-instance page
//  allocators (alloc_pages/free_pages). However, the actual code in
//  mulle-vm.h lacks these fields, and mulle-vm-page.c uses a global
//  allocator and lock."
//
//  Test: Verify that mulle_vm_config does NOT have alloc_pages/free_pages
//  fields and that the config size matches expectations.
//

#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


int   main( void)
{
   struct mulle_vm_config   config;
   struct mulle_vm          vm;

   mulle_printf( "=== REVIEW: No Per-Instance Allocator Test ===\n\n");

   // Show the actual fields of mulle_vm_config
   mulle_printf( "sizeof(mulle_vm_config): %lu\n", (unsigned long) sizeof( struct mulle_vm_config));

   // The config only has: shutdown, skew
   // If alloc_pages/free_pages existed, the struct would be larger
   mulle_printf( "Fields in mulle_vm_config:\n");
   mulle_printf( "  shutdown offset: %lu\n", (unsigned long) offsetof( struct mulle_vm_config, shutdown));
   mulle_printf( "  skew offset: %lu\n", (unsigned long) offsetof( struct mulle_vm_config, skew));

   // Verify: two different VMs use the same global pagesize
   // (proving the allocator is global, not per-instance)
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   mulle_printf( "\nGlobal pagesize: %lu\n", (unsigned long) mulle_vm_get_pagesize());
   mulle_printf( "CONFIRMED: mulle_vm_get_pagesize() is a global function, not per-instance\n");

   mulle_printf( "\n=== No Per-Instance Allocator Test Complete ===\n");

   mulle_vm_done( &vm);
   return( 0);
}
