//
// random-insert-delete-test.c
// mulle-vm
//
// Random insert/delete stress test (may trigger coalescing incidentally)
//

#include "include.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static void  show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   statistics;
   size_t                       total_pages;

   statistics  = mulle_vm_get_statistics( vm);
   total_pages = statistics.page_size / mulle_vm_get_pagesize();
   mulle_printf( "%lu pages, %lu bytes\n", total_pages, statistics.vm_size);
}


int   main( void)
{
   struct mulle_vm   vm;
   size_t            target_size = 2 * 1024 * 1024;  // 2 MB
   char              buffer[ 16];
   
   srand( 1848); // time( NULL));
   
   mulle_printf( "=== Random Insert/Delete Stress Test ===\n");
   mulle_printf( "Target size: %zu bytes (2 MB)\n", target_size);
   
   // Phase 1: Add random strings until 2MB
   mulle_printf( "\n--- Phase 1: Adding random strings ---\n");
   mulle_vm_init( &vm, NULL, NULL);
   
   while( mulle_vm_get_length( &vm) < target_size)
   {
      size_t size = (rand() % 16) + 1;  // 1-16 bytes
      size_t remaining = target_size - mulle_vm_get_length( &vm);
      
      if( size > remaining)
         size = remaining;
         
      // Fill buffer with random characters
      for( size_t i = 0; i < size; i++)
         buffer[i] = 'A' + (rand() % 26);
         
      mulle_vm_append( &vm, buffer, size);
      
      if( mulle_vm_get_length( &vm) % (target_size / 10) == 0 || 
          mulle_vm_get_length( &vm) == target_size)
      {
         mulle_fprintf( stderr, "Progress: %zu/%zu bytes (%.1f%%) - ",
                mulle_vm_get_length( &vm), target_size,
                (double)mulle_vm_get_length( &vm) / target_size * 100.0);
         show_vm( &vm);
      }
   }
   
   mulle_printf( "\nFinal state after adding:\n");
   show_vm( &vm);
   
   // Phase 2: Remove random ranges until empty
   mulle_printf( "\n--- Phase 2: Removing random ranges ---\n");
   
   while( mulle_vm_get_length( &vm) > 0)
   {
      size_t current_size = mulle_vm_get_length( &vm);
      size_t remove_size = (rand() % 16) + 1;  // 1-16 bytes
      
      if( remove_size > current_size)
         remove_size = current_size;
         
      size_t start_pos = 0;
      if( current_size > remove_size)
         start_pos = rand() % (current_size - remove_size + 1);
      
      mulle_vm_delete( &vm, mulle_range_make( start_pos, remove_size));
      
      if( current_size % (target_size / 10) == 0 || mulle_vm_get_length( &vm) == 0)
      {
         mulle_fprintf( stderr, "Progress: %zu/%zu bytes remaining (%.1f%%) - ",
                mulle_vm_get_length( &vm), target_size,
                (double)mulle_vm_get_length( &vm) / target_size * 100.0);
         show_vm( &vm);
      }
   }
   
   mulle_printf( "\nFinal state after removal:\n");
   show_vm( &vm);
   
   mulle_vm_done( &vm);
   
   mulle_printf( "\n=== Random insert/delete test completed ===\n");
   
   return( 0);
}