//
//  review-mid-insert-shared-page.c
//  mulle-vm
//
//  Regression test: inserting into the middle of a VM whose page is shared
//  (copy-on-write) must work.
//
//  _mulle_vm_insert() cached the page it got from _mulle_vm_tree_search() and
//  kept using that pointer after calling _mulle_vm_split(). Splitting a shared
//  page goes through _mulle_vm_will_write_page(), which copies the page and
//  updates only result.node->page - so the cached page was stale, and the code
//  below passed its pre-split used.length to _mulle_vm_page_insert_bytes() as
//  the insert offset. That offset lies beyond the end of the new, shorter page,
//  so the insert fails with ERANGE and trips an assertion in
//  _mulle_vm_insert_into_large_enough_node.
//
//  Found via mulle-editor: mulle_editor_load_file() uses mulle_vm_paste(),
//  which shares pages, so the first middle insert after a load took this path.
//
//  Test: share a page with mulle_vm_copy and keep the source VM alive (so the
//  page really stays shared), then insert into the middle of the destination
//  and verify both VMs.
//

#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TEXT_LENGTH       24
#define INSERTED_LENGTH   9

static char   text[]         = "Hello World\nSecond Line\n";
static char   expected_dst[] = "Hello INSERTED World\nSecond Line\n";


static void   print_text( const char *label, const void *bytes, size_t length)
{
   char     c;
   size_t   i;

   mulle_printf( "%s\"", label);
   for( i = 0; i < length; i++)
   {
      c = ((char *) bytes)[ i];
      if( c == '\n')
         mulle_printf( "\\n");
      else
         mulle_printf( "%c", c);
   }
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

   mulle_printf( "=== REVIEW: Middle Insert Into Shared Page Test ===\n\n");

   mulle_vm_init( &src_vm, NULL, NULL);
   mulle_vm_init( &dst_vm, NULL, NULL);

   mulle_vm_insert( &src_vm, mulle_range_make( 0, TEXT_LENGTH), text);

   data = mulle_vm_read_copy( &src_vm, mulle_range_make( 0, TEXT_LENGTH), buffer);
   print_text( "src content: ", data, TEXT_LENGTH);

   // Share the pages with dst. src_vm stays alive, so the page keeps its
   // retain count and the write below has to go through COW.
   rval = mulle_vm_copy( &dst_vm, &src_vm, mulle_range_make( 0, TEXT_LENGTH));
   mulle_printf( "copy (sharing) result: %d\n", rval);
   if( rval)
   {
      mulle_printf( "FAILED: mulle_vm_copy returned %d\n", rval);
      ++failures;
   }

   // The insert below only exercises the COW path if the page really is
   // shared. Check it, so the test fails loudly instead of passing vacuously
   // if page sharing ever stops happening.
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

   // The regression itself: a middle insert into that shared page.
   errno = 0;
   rval  = mulle_vm_insert( &dst_vm,
                            mulle_range_make( 5, INSERTED_LENGTH),
                            (void *) " INSERTED");
   mulle_printf( "mid insert into shared page: %d (errno=%d)\n", rval, errno);
   if( rval)
   {
      mulle_printf( "FAILED: middle insert into a shared page returned %d\n", rval);
      ++failures;
   }

   mulle_printf( "dst length: %lu (expected %lu)\n",
                 (unsigned long) mulle_vm_get_length( &dst_vm),
                 (unsigned long) (TEXT_LENGTH + INSERTED_LENGTH));
   if( mulle_vm_get_length( &dst_vm) != TEXT_LENGTH + INSERTED_LENGTH)
      ++failures;

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
      if( memcmp( data, expected_dst, TEXT_LENGTH + INSERTED_LENGTH) != 0)
      {
         print_text( "FAILED, expected: ", expected_dst, TEXT_LENGTH + INSERTED_LENGTH);
         ++failures;
      }
   }

   // COW: the source must have come through the insert untouched.
   memset( buffer, 0, sizeof( buffer));
   data = mulle_vm_read_copy( &src_vm, mulle_range_make( 0, TEXT_LENGTH), buffer);
   if( ! data || memcmp( data, text, TEXT_LENGTH) != 0)
   {
      mulle_printf( "FAILED: COW did not preserve the source\n");
      ++failures;
   }
   else
      print_text( "src after insert: ", data, TEXT_LENGTH);

   if( ! failures)
      mulle_printf( "CONFIRMED: middle insert into a shared page works, source unchanged\n");
   else
      mulle_printf( "ERROR: %d check(s) failed\n", failures);

   mulle_printf( "\n=== Middle Insert Into Shared Page Test Complete ===\n");

   mulle_vm_done( &dst_vm);
   mulle_vm_done( &src_vm);

   return( failures ? 1 : 0);
}
