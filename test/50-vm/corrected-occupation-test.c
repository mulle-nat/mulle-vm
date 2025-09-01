//
// corrected-occupation-test.c
// mulle-vm
//
// Test corrected occupation calculations
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>

int main(void)
{
   struct mulle_vm   vm;
   size_t            pagesize = mulle_vm_get_pagesize();
   
   mulle_printf("=== Corrected Occupation Test ===\n");
   mulle_printf("pagesize = %zu\n", pagesize);
   
   // Test the exact case: page.used.location = 1, page.size = pagesize, page.used.length = pagesize - 2
   mulle_printf("\n=== Edge Case Test ===\n");
   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);
   
   // Create the problematic scenario
   mulle_vm_append( &vm, "X", 1);  // used.location = 1, used.length = 1
   for( int i = 0; i < pagesize - 3; i++)
      mulle_vm_append( &vm, "A", 1);  // used.length = pagesize - 2
   
   // Now we have: used.location = 1, used.length = pagesize - 2
   
   mulle_printf("Before deletion: ");
   struct mulle_vm_statistics stats = mulle_vm_get_statistics( &vm);
   mulle_printf("%lu pages, %lu bytes\n", stats.page_size / pagesize, stats.vm_size);
   
   // This should trigger the corrected calculation
   mulle_vm_delete( &vm, mulle_range_make( 1, 2));
   
   mulle_printf("After deletion: ");
   stats = mulle_vm_get_statistics( &vm);
   mulle_printf("%lu pages, %lu bytes\n", stats.page_size / pagesize, stats.vm_size);
   
   mulle_vm_done( &vm);
   
   return 0;
}

// Corrected calculations:
// node_front_occupied = page->used.location (always correct)
// node_back_occupied = page->used.length (always correct)
// 
// The original formulas were trying to calculate "how much data is at front/back"
// but they should simply use the actual data boundaries:
// - Front occupied = offset from start (used.location)
// - Back occupied = actual data length (used.length)
//
// For coalescing decisions, we need:
// - Available front = used.location (space before data)
// - Available back = page->size - (used.location + used.length) (space after data)
// - Occupied front = used.location (data offset)
// - Occupied back = used.length (actual data size)

// The fix is to replace:
// node_front_occupied = used.location
// node_back_occupied = used.length
// 
// And change coalescing conditions to use actual data sizes instead of calculated ones.