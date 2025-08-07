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


static void   walk( struct mulle_vm  *vm, struct mulle_range range)
{
   int   rval;

   printf( "Walking VM from %td to %td...\n",
               range.location,
               mulle_range_get_last_location( range));

   rval = mulle_vm_walk( vm, range, walk_callback, NULL );
   if( rval >= 0)
      printf( "=== Walk completed successfully ===\n");
}

static void   walk_reverse( struct mulle_vm  *vm, struct mulle_range range)
{
   int   rval;

   printf( "Walking VM from %td to %td...\n",
               mulle_range_get_last_location( range),
               range.location);

   rval = mulle_vm_walk_reverse( vm, range, walk_callback, NULL );
   if( rval >= 0)
      printf( "=== Walk completed successfully ===\n");
}



// Test function: walks the entire VM and checks content
static void   test_with_walk( void)
{
   struct mulle_vm              vm;
   struct mulle_vm_statistics   info;
   struct mulle_range           range;

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
   range = mulle_range_make( 0, info.vm_size);
   walk( &vm, range);
   walk_reverse( &vm, range);

   range = mulle_range_make( info.vm_size / 4, info.vm_size / 2);
   walk( &vm, range);
   walk_reverse( &vm, range);

   mulle_vm_done( &vm );
}

// Main entry point
int   main( void)
{
   test_with_walk();

   return( 0 );
}
