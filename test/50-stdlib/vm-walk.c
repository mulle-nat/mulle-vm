#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


// Forward declaration
static void   test_with_walk( void);

// Helper: callback for mulle_vm_walk
static int   walk_callback( struct mulle_data data,
                            struct mulle_range range,
                            void *context)
{
   printf( "walk: {%td, %td} -> '%.*s'\n",
               range.location, range.location + range.length,
               (int) data.length, (char *) data.bytes);

   return 0;  // continue walking
}

// Test function: walks the entire VM and checks content
static void   test_with_walk( void)
{
   struct mulle_vm        vm;
   struct mulle_vm_statistics   info;
   struct mulle_range     range;

   printf( "=== Testing mulle_vm_walk ===\n");

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Append data in chunks
   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);
   mulle_vm_append( &vm, "mnop", 4);
   mulle_vm_append( &vm, "qrst", 4);
   mulle_vm_append( &vm, "uvw", 3);
   mulle_vm_append( &vm, "xyz", 3);

   info = mulle_vm_get_statistics( &vm );

   // Walk the entire VM
   printf( "Walking VM from 0 to %td...\n", info.vm_size );
   mulle_vm_walk( &vm, mulle_range_make( 0, info.vm_size), walk_callback, NULL );

   printf( "=== Walk completed successfully ===\n");

   printf( "Walking VM from %td to 0...\n", info.vm_size);

   mulle_vm_walk_reverse( &vm, mulle_range_make( 0, info.vm_size), walk_callback, NULL );

   printf( "=== Walk completed successfully ===\n");

   mulle_vm_done( &vm );
}

// Main entry point
int   main( void)
{
   test_with_walk();

   return( 0 );
}
