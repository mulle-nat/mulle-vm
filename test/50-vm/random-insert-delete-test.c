//
// random-insert-delete-test.c
// mulle-vm
//
// Random insert/delete stress test (may trigger coalescing incidentally).
//
// This test uses its own portable PRNG instead of the platform rand(), so
// the command sequence is identical on every platform. It does not print the
// intermediate state (page counts vary with page size / allocator anyway).
// Instead it checks invariants and returns a non-zero exit code on failure.
// The test framework only needs the exit code / stable output line.
//

#include "include.h"
#include <stdio.h>
#include <stdint.h>


//
// Portable xorshift32 PRNG. Unlike rand(), this yields the same sequence on
// every platform, so the test is reproducible cross-platform.
//
static uint32_t   random_state;

static void   portable_srand( uint32_t seed)
{
   random_state = seed ? seed : 1;
}

static uint32_t   portable_rand( void)
{
   uint32_t   x;

   x = random_state;
   x ^= x << 13;
   x ^= x >> 17;
   x ^= x << 5;
   random_state = x;
   return( x);
}


int   main( void)
{
   struct mulle_vm   vm;
   size_t            target_size = 2 * 1024 * 1024;  // 2 MB
   size_t            size;
   size_t            remaining;
   size_t            current_size;
   size_t            remove_size;
   size_t            start_pos;
   size_t            i;
   char              buffer[ 16];
   int               rval;

#ifdef MULLE_TEST_VALGRIND
   target_size = 4 * 1024;  // 4 KB under valgrind
#endif

   rval = 0;
   portable_srand( 1848);

   mulle_vm_init( &vm, NULL, NULL);

   // Phase 1: Append random strings until target_size
   while( mulle_vm_get_length( &vm) < target_size)
   {
      size      = (portable_rand() % 16) + 1;  // 1-16 bytes
      remaining = target_size - mulle_vm_get_length( &vm);
      if( size > remaining)
         size = remaining;

      for( i = 0; i < size; i++)
         buffer[ i] = 'A' + (portable_rand() % 26);

      mulle_vm_append( &vm, buffer, size);
   }

   // Invariant: after filling, length must be exactly target_size
   if( mulle_vm_get_length( &vm) != target_size)
   {
      mulle_fprintf( stderr, "FAIL: expected length %zu after fill, got %zu\n",
                     target_size, mulle_vm_get_length( &vm));
      rval = 1;
   }

   // Phase 2: Delete random ranges until empty
   while( rval == 0 && mulle_vm_get_length( &vm) > 0)
   {
      current_size = mulle_vm_get_length( &vm);
      remove_size  = (portable_rand() % 16) + 1;  // 1-16 bytes
      if( remove_size > current_size)
         remove_size = current_size;

      start_pos = 0;
      if( current_size > remove_size)
         start_pos = portable_rand() % (current_size - remove_size + 1);

      mulle_vm_delete( &vm, mulle_range_make( start_pos, remove_size));

      // Invariant: length must shrink by exactly remove_size each step
      if( mulle_vm_get_length( &vm) != current_size - remove_size)
      {
         mulle_fprintf( stderr, "FAIL: expected length %zu after delete, got %zu\n",
                        current_size - remove_size, mulle_vm_get_length( &vm));
         rval = 1;
         break;
      }
   }

   // Invariant: VM must be empty at the end
   if( rval == 0 && mulle_vm_get_length( &vm) != 0)
   {
      mulle_fprintf( stderr, "FAIL: expected empty VM, got length %zu\n",
                     mulle_vm_get_length( &vm));
      rval = 1;
   }

   mulle_vm_done( &vm);

   if( rval == 0)
      mulle_printf( "random insert/delete test passed\n");

   return( rval);
}
