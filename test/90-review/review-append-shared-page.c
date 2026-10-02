//
//  review-append-shared-page.c
//  mulle-vm
//
//  Regression test: appending to a VM whose page is shared (copy-on-write)
//  must actually append, and must not corrupt the page.
//
//  _mulle_vm_insert_into_large_enough_node() did
//
//      page = _mulle_vm_node_get_page( node);
//      page = _mulle_vm_will_write_page( vm, page);
//
//  and never wrote that page back into node. On an unshared page
//  _mulle_vm_will_write_page() returns its argument unchanged, so the missing
//  store was invisible. On a shared page it copies on write, releases the page
//  it copied from, and returns the new one - so the bytes were inserted into a
//  page nothing referenced, the node kept pointing at the released page, and
//  the copy leaked. The insert then reported success while the document was
//  unchanged, and a later mulle_vm_done() walked the freed page.
//
//  Note appending is the interesting case: inserting in the middle goes
//  through _mulle_vm_split(), which does update the node.
//
//  Found via mulle-editor, whose undo snapshots share a page on every edit, so
//  the very next keystroke took this path.
//
//  Test: share a page with mulle_vm_copy, keep the source alive so the page
//  really stays shared, then append and check both VMs.
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TEXT_LENGTH       6
#define INSERTED_LENGTH   3


static void   print_text( const char *label, const void *bytes, size_t length)
{
   size_t   i;

   mulle_printf( "%s\"", label);
   for( i = 0; i < length; i++)
      mulle_printf( "%c", ((char *) bytes)[ i]);
   mulle_printf( "\"\n");
}


int   main( void)
{
   struct mulle_vm          src_vm;
   struct mulle_vm          dst_vm;
   struct mulle_vm_page     *page;
   void                     *data;
   char                     buffer[ 64];
   int                      is_shared;
   int                      rval;
   int                      failures;

   failures = 0;

   mulle_printf( "=== REVIEW: Append Into Shared Page Test ===\n\n");

   mulle_vm_init( &src_vm, NULL, NULL);
   mulle_vm_init( &dst_vm, NULL, NULL);

   mulle_vm_insert( &src_vm, mulle_range_make( 0, TEXT_LENGTH), (void *) "abcdef");

   data = mulle_vm_read_copy( &src_vm, mulle_range_make( 0, TEXT_LENGTH), buffer);
   print_text( "src content: ", data, TEXT_LENGTH);

   // Share the pages with dst. src_vm stays alive, so the page keeps its
   // retain count and the writes below have to go through COW.
   rval = mulle_vm_copy( &dst_vm, &src_vm, mulle_range_make( 0, TEXT_LENGTH));
   mulle_printf( "copy (sharing) result: %d\n", rval);
   if( rval)
   {
      mulle_printf( "FAILED: mulle_vm_copy returned %d\n", rval);
      ++failures;
   }

   // The append below only exercises the COW path if the page really is
   // shared, so check that instead of passing vacuously if sharing stops.
   is_shared = 0;
   mulle_vm_for( &dst_vm, page)
   {
      if( mulle_vm_page_is_shared( page))
      {
         is_shared = 1;
         break;
      }
   }

   mulle_printf( "dst page shared after copy: %s\n", is_shared ? "YES" : "NO");
   if( ! is_shared)
   {
      mulle_printf( "FAILED: page is not shared, test precondition not met\n");
      ++failures;
   }

   // The regression itself: append to the end of that shared page.
   errno = 0;
   rval  = mulle_vm_insert( &dst_vm, mulle_range_make( TEXT_LENGTH, INSERTED_LENGTH), (void *) "ghi");
   mulle_printf( "append into shared page: %d (errno=%d)\n", rval, errno);
   if( rval)
   {
      mulle_printf( "FAILED: append into a shared page returned %d\n", rval);
      ++failures;
   }

   mulle_printf( "dst length: %lu (expected %lu)\n",
                 (unsigned long) mulle_vm_get_length( &dst_vm),
                 (unsigned long) (TEXT_LENGTH + INSERTED_LENGTH));
   if( mulle_vm_get_length( &dst_vm) != TEXT_LENGTH + INSERTED_LENGTH)
   {
      mulle_printf( "FAILED: the append was lost, the length did not change\n");
      ++failures;
   }

   memset( buffer, 0, sizeof( buffer));
   data = mulle_vm_read_copy( &dst_vm,
                              mulle_range_make( 0, TEXT_LENGTH + INSERTED_LENGTH),
                              buffer);
   if( ! data)
   {
      mulle_printf( "FAILED: could not read back dst\n");
      ++failures;
   }
   else
   {
      print_text( "dst content: ", data, TEXT_LENGTH + INSERTED_LENGTH);
      if( memcmp( data, "abcdefghi", TEXT_LENGTH + INSERTED_LENGTH) != 0)
      {
         mulle_printf( "FAILED: expected \"abcdefghi\"\n");
         ++failures;
      }
   }

   // COW: the source must have come through the append untouched.
   memset( buffer, 0, sizeof( buffer));
   data = mulle_vm_read_copy( &src_vm, mulle_range_make( 0, TEXT_LENGTH), buffer);
   if( ! data || memcmp( data, "abcdef", TEXT_LENGTH) != 0)
   {
      mulle_printf( "FAILED: COW did not preserve the source\n");
      ++failures;
   }
   else
      print_text( "src after append: ", data, TEXT_LENGTH);

   // A second append, to catch state left damaged by the first.
   rval = mulle_vm_insert( &dst_vm, mulle_range_make( TEXT_LENGTH + INSERTED_LENGTH, INSERTED_LENGTH), (void *) "jkl");
   mulle_printf( "append again into shared page: %d (errno=%d)\n", rval, errno);
   mulle_printf( "dst length: %lu (expected %lu)\n",
                 (unsigned long) mulle_vm_get_length( &dst_vm),
                 (unsigned long) (TEXT_LENGTH + 2 * INSERTED_LENGTH));
   if( rval || mulle_vm_get_length( &dst_vm) != TEXT_LENGTH + 2 * INSERTED_LENGTH)
      ++failures;

   memset( buffer, 0, sizeof( buffer));
   data = mulle_vm_read_copy( &dst_vm,
                              mulle_range_make( 0, TEXT_LENGTH + 2 * INSERTED_LENGTH),
                              buffer);
   if( ! data)
      ++failures;
   else
   {
      print_text( "dst content: ", data, TEXT_LENGTH + 2 * INSERTED_LENGTH);
      if( memcmp( data, "abcdefghijkl", TEXT_LENGTH + 2 * INSERTED_LENGTH) != 0)
      {
         mulle_printf( "FAILED: expected \"abcdefghijkl\"\n");
         ++failures;
      }
   }

   if( ! failures)
      mulle_printf( "CONFIRMED: append into a shared page works, source unchanged\n");
   else
      mulle_printf( "ERROR: %d check(s) failed\n", failures);

   mulle_printf( "\n=== Append Into Shared Page Test Complete ===\n");

   // Doing this without a crash is itself part of the regression: the buggy
   // version released a page the node still referenced.
   mulle_vm_done( &dst_vm);
   mulle_vm_done( &src_vm);

   return( failures ? 1 : 0);
}
