//
//  review-multipage-write-enosys.c
//  mulle-vm
//
//  Originally validated REVIEW.md claim: "Missing Multi-Page Writes:
//  _mulle_vm_io explicitly fails with ENOSYS when attempting a write that
//  spans page boundaries."
//
//  After fix: Multi-page writes now work correctly.
//  Test: Create data spanning two pages, then write across the page
//  boundary. Verify that the write succeeds and data is correct.
//

#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int   main( void)
{
   struct mulle_vm   vm;
   void              *result;
   char              write_buf[ 16];
   char              read_buf[ 16];
   const void        *data;
   unsigned int      n_pages;

   mulle_printf( "=== REVIEW: Multi-Page Write Test ===\n\n");

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   // Insert enough data to span multiple pages
   // With DEBUG_PAGE_SIZE=8 and skew=25, we need more than one page
   mulle_vm_insert( &vm, mulle_range_make( 0, 6), "AAAAAA");
   mulle_vm_insert( &vm, mulle_range_make( 6, 6), "BBBBBB");

   // Count pages to confirm we have multiple
   n_pages = 0;
   mulle_vm_for( &vm, page)
   {
      n_pages++;
   }
   mulle_printf( "Pages after two inserts: %u\n", n_pages);

   if( n_pages >= 2)
   {
      // Write across the page boundary - should now succeed
      memset( write_buf, 'X', 8);
      errno  = 0;
      result = mulle_vm_write( &vm, mulle_range_make( 2, 8), write_buf);

      mulle_printf( "Cross-page write (offset=2, len=8) result: %s\n",
                    result ? "non-NULL (success)" : "NULL (failed)");

      if( result)
      {
         // Verify the written data
         data = mulle_vm_read( &vm, mulle_range_make( 0, 12), read_buf);
         mulle_printf( "Content after write: %.*s\n", 12, (char *) data);
         // Expected: AA XXXXXXXX BB -> "AAXXXXXXXXBB"
         if( memcmp( read_buf, "AAXXXXXXXXBB", 12) == 0)
            mulle_printf( "CONFIRMED: Multi-page write works correctly\n");
         else
            mulle_printf( "ERROR: Written data is incorrect\n");
      }
      else
      {
         mulle_printf( "FAILED: Multi-page write still fails (errno=%d)\n", errno);
      }
   }
   else
   {
      mulle_printf( "SKIP: Could not create multi-page scenario with debug allocator\n");
   }

   // Also verify that single-page writes still work
   memset( write_buf, 'Z', 2);
   result = mulle_vm_write( &vm, mulle_range_make( 0, 2), write_buf);
   mulle_printf( "\nSingle-page write result: %s\n", result ? "non-NULL (success)" : "NULL (failed)");

   // Read back to verify
   data = mulle_vm_read( &vm, mulle_range_make( 0, 2), read_buf);
   if( data && memcmp( data, "ZZ", 2) == 0)
      mulle_printf( "Single-page write verified: ZZ\n");

   mulle_printf( "\n=== Multi-Page Write Test Complete ===\n");

   mulle_vm_done( &vm);
   return( 0);
}
