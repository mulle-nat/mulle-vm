//
//  mulle-vm-page.c
//  mulle-vm
//
//  Copyright (c) <|YEAR|> Nat! - Mulle kybernetiK.
//  All rights reserved.
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
#include "mulle-vm-page.h"

#include "include-private.h"


static struct
{
   struct mulle_storage            vm_pages;
   struct mulle_ospage_allocator   allocator;
   struct mulle_pointerarray       freed_ospages;
   size_t                          max_freed_ospages;
   mulle_thread_mutex_t            lock;
} global;


size_t   mulle_vm_get_pagesize( void)
{
   return( global.allocator.pagesize);
}


// Helper to round size up to pagesize multiple
static inline size_t   round_up_to_pagesize( size_t size, size_t pagesize)
{
   return( ((size + pagesize - 1) / pagesize) * pagesize);
}


static inline size_t   round_down_to_pagesize( size_t size, size_t pagesize)
{
   return( (size / pagesize) * pagesize);
}


void *   _mulle_vm_alloc_ospages( size_t size)
{
   void     *mem;
   void     (*fail)( struct mulle_allocator *, void *, size_t)  MULLE_C_NO_RETURN;

   mulle_thread_mutex_do( global.lock)
   {
      assert( size == round_up_to_pagesize( size, global.allocator.pagesize));

      mem = NULL;
      if( size == global.allocator.pagesize)
      {
         mem = mulle_pointerarray_pop( &global.freed_ospages);
      }
      if( ! mem)
      {
         mem  = (*global.allocator.alloc)( size); // hmm ...
         fail = global.allocator.fail;
      }
   }

   if( ! mem)
      (*fail)( NULL, NULL, size);

#ifdef DEBUG
   mulle_memset_uint32( mem, 0x214E3A55, size);
#endif
   return( mem);
}



/**
 **
 **
 **
 */

// the mulle_vm_page MUST have been removed from pages already!
// (if you are freeing all the pages from mulle_vm_page)
static void   _mulle_vm_free_ospages_unlocked( void *mem, size_t size)
{
   size_t   count;
   size_t   avail;
   int      rval;

   rval = 0;
   assert( size == round_down_to_pagesize( size, global.allocator.pagesize));

   count = mulle_pointerarray_get_count( &global.freed_ospages);
   assert( global.max_freed_ospages >= count);
   avail = global.max_freed_ospages - count;
   //
   // freed_ospages individual pages for reuse
   //
   while( count < avail)
   {
      mulle_pointerarray_add( &global.freed_ospages, mem);
      size -= global.allocator.pagesize;
      if( ! size)
         break;

      mem = &((char *) mem)[ global.allocator.pagesize];
      ++count;
   }

   if( size)
   {
      // free rest wholesale
      rval = (*global.allocator.free)( mem, size);
      if( rval)
      {
         // what to do ?
         fprintf( stderr, "mulle-vm: page %p wrongly freed by mishaved thread ? (%d)\n", mem, rval);
#ifdef DEBUG
         abort();
#endif
      }
   }
}


static void   mulle_vm_free_freed_ospages_unlocked( void)
{
   int     rval;
   void    *page;

   mulle_pointerarray_for( &global.freed_ospages, page)
   {
      rval = (*global.allocator.free)( page, global.allocator.pagesize);
      if( rval)
      {
      // what to do ?
         fprintf( stderr, "mulle-vm: page %p wrongly freed by mishaved thread ? (%d)\n",
                           page, rval);
#ifdef DEBUG
         abort();
#endif
      }
   }
}


void   mulle_vm_reset_freed_ospages( void)
{
   mulle_thread_mutex_do( global.lock)
   {
      mulle_vm_free_freed_ospages_unlocked();
   }
   mulle_pointerarray_reset( &global.freed_ospages);
}


struct mulle_vm_page   *mulle_vm_page_alloc( void)
{
   struct mulle_vm_page   *page;

   mulle_thread_mutex_do( global.lock)
   {
      page = _mulle_storage_calloc( &global.vm_pages);
   }
   return( page);
}


void   mulle_vm_page_free( struct mulle_vm_page *page)
{
   mulle_thread_mutex_do( global.lock)
   {
      _mulle_vm_free_ospages_unlocked( page->bytes,page->size);
      _mulle_storage_free( &global.vm_pages, page);
   }
}





/**
 **
 **
 **
 */
struct mulle_vm_page *  mulle_vm_page_new_with_bytes( void *bytes,
                                                      size_t length,
                                                      unsigned int skew)
{
   size_t                 size;
   char                   *mem;
   size_t                 free_size;
   struct mulle_range     range;
   struct mulle_vm_page   *page;

   assert( skew <= 100);

   size = round_up_to_pagesize( length, mulle_vm_get_pagesize());
   mem  = _mulle_vm_alloc_ospages( size);

   free_size      = size - length;
   range.location = free_size * skew / 100;
   range.length   = length;

   // copy block to middle (or offset from there) so we have space in front
   // and back to add.
   memcpy( &mem[ range.location], bytes, length);
#ifdef DEBUG
   mulle_memset_uint32( &mem[ 0], 0xFEEDFACE, range.location);
   mulle_memset_uint32( &mem[ mulle_range_get_max( range)], 0xFEEDFACE, size - mulle_range_get_max( range));
#endif

   page = mulle_vm_page_alloc();
   mulle_vm_page_init( page, mem, range, size);
   return( page);
}


// Static inline retain/release functions for page reference counting
struct mulle_vm_page *mulle_vm_page_retain( struct mulle_vm_page *page)
{
   if( ! page)
      return( NULL);

   _mulle_atomic_pointer_increment( &page->retain_count);
   return( page);
}


// MEMO: only tests should call this...
void   mulle_vm_page_release( struct mulle_vm_page *page)
{
   if( ! page)
      return;

   if( ! _mulle_atomic_pointer_decrement( &page->retain_count))
      mulle_vm_page_free( page);
}


/**
 **
 **
 **
 */

// Tries to join pages a and b which are consecutive logical pages.
// Returns:
//   -1 if b's content fits appended at the back of a
//   +1 if a's content fits prepended at the front of b
//    0 if no join possible under these constraints
//
// Does NOT reorder content or put b content before a content or vice versa.
// Moves data only to enlarge available front or back spaces as needed.
int   _mulle_vm_page_join( struct mulle_vm_page *a, struct mulle_vm_page *b)
{
    size_t a_used_len = a->used.length;
    size_t b_used_len = b->used.length;

    if( a->retain_count != 0 || b->retain_count != 0)
       return( 0);

    // Try to join b into a by appending b's content at the back of a
    // but only if a is not shared, otherwise not deemed worth it to
    // do a COW for that
    if( a->retain_count != 0)
    {
        size_t a_available_back = mulle_vm_page_get_available_back(a);

        if (a_available_back < b_used_len)
        {
            // Attempt to move a's used data towards front (location 0) to increase back available space
            size_t a_available_front = mulle_vm_page_get_available_front(a);
            if ((a_available_front + a_available_back) >= b_used_len)
            {
                memmove(
                    (char *)a->bytes,
                    (char *)a->bytes + a->used.location,
                    a_used_len);
                a->used.location = 0;
                a_available_back = mulle_vm_page_get_available_back(a);
            }
        }

        if (a_available_back >= b_used_len)
        {
            // Append b's content at back of a
            memcpy(
                (char *)a->bytes + a->used.location + a_used_len,
                (char *)b->bytes + b->used.location,
                b_used_len);
            a->used.length += b_used_len;
            return( -1);
        }
    }

    // Try to join a into b by prepending a's content in front of b
    // but only if b is not shared, otherwise not deemed worth it to
    // do a COW for that
    if( b->retain_count != 0)
    {
        size_t b_available_front = mulle_vm_page_get_available_front(b);

        if (b_available_front < a_used_len)
        {
            // Attempt to move b's used data towards back to create more front space is NOT allowed (would shift content order),
            // so we attempt to move b's data towards front instead (to free back space) would not help prepending a's data at front;
            // therefore, only moving b towards front (location 0) is allowed:
            size_t b_available_back = mulle_vm_page_get_available_back(b);
            if ((b_available_front + b_available_back) >= a_used_len)
            {
                memmove(
                    (char *)b->bytes,
                    (char *)b->bytes + b->used.location,
                    b_used_len);
                b->used.location = 0;
                b_available_front = mulle_vm_page_get_available_front(b);
            }
        }

        if (b_available_front >= a_used_len)
        {
            // Shift b's content to right to prepare space for a's content
            memmove(
                (char *)b->bytes + b->used.location - a_used_len,
                (char *)b->bytes + b->used.location,
                b_used_len);
            // Copy a's content in front
            memcpy(
                (char *)b->bytes + b->used.location - a_used_len,
                (char *)a->bytes + a->used.location,
                a_used_len);

            b->used.location -= a_used_len;
            b->used.length   += a_used_len;
            return( +1);
        }
    }

    // No possible join without breaking order constraint
    return( 0);
}



//
// offset must either be adjacent or intersecting the currently used bytes
// in page.  If offset 0,  bytes will be prepended. If offset == used.length
// then bytes will be appended.
//
// returns 0 on success
// ENOSPC if won't fit
// EINVAL if range is not adjacent or intersecting
//
int  _mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t offset,
                                  void *bytes,
                                  size_t length)
{
   size_t   available;
   size_t   actual_available;
   char     *new_front;
   char     *old_front;
   char     *new_insert;
   char     *old_middle;
   char     *new_middle;
   int      adjacent;

   // assert( page->retain_count == 0 && "can't modify shared page");

   // you can only change unshared pages
   if( page->retain_count != 0)
   {
      errno = EACCES;
      return( -1);
   }

   // check that page can service the request at all
   actual_available = mulle_vm_page_get_available( page);
   if( actual_available < length)
   {
      errno = ENOSPC;
      return( -1);
   }

   // offset must be within or at and of the used data range
   // actually hitting this is probably an error on callers part
   // should assert this but then tests fail..
   if( offset > page->used.length)
   {
      errno = ERANGE;
      return( -1);
   }


   // clean append to back
   adjacent = (page->used.length == offset);
   if( adjacent)
   {
      available = mulle_vm_page_get_available_back( page);
      if( available < length)
      {
         // need to make room, by moving bytes to front (we move it all to 0)
         memmove( &((char *) page->bytes)[ 0],
                  &((char *) page->bytes)[ page->used.location],
                  page->used.length);
         page->used.location = 0;
      }

      memcpy( &((char *) page->bytes)[ mulle_range_get_max( page->used)],
              bytes,
              length);
      page->used.length  += length;
      return( 0);
   }

   adjacent = ! offset;
   // clean adjacent insert in front, or (special case) no split, the tail
   // of range intersects with 10
   if( adjacent)
   {
      available = mulle_vm_page_get_available_front( page);
      if( available < length)
      {
         // need to make room, by moving range to back (we move it all to end)
         memmove( &((char *) page->bytes)[ page->size - page->used.length],
                  &((char *) page->bytes)[ page->used.location],
                  page->used.length);
         page->used.location = page->size - page->used.length;
      }

      page->used.location -= length;

      memcpy( &((char *) page->bytes)[ page->used.location],
              bytes,
              length);

      page->used.length += length;

      return( 0);
   }

   //
   // vm_address: 0x1000
   // |
   // __abcdefgh__  size: 12
   //  /|        /
   // | used:{ 2, 8 }
   // |
   // available: 2
   //
   // insert:
   //   range: { 0x1003, 3 }
   //   bytes: "XYZ"
   //
   // do:
   //
   // new_front
   // | old_front
   // | |
   // __abcdefgh__  0
   //      |
   //      old_middle
   //
   //    new_insert
   //    |
   // abc___defgh_  2
   //       |
   //       new_middle

   // abcbcdefgh__  1
   // abc___defgh_  2
   // abcXYZdefgh_  3


   //
   // need to make a hole
   // if there's not enough front space, try to create it by moving data to back
   //
   available = mulle_vm_page_get_available_front( page);
   if( available < length)
   {
      // need to make room, by moving range to back (we move it all to end)
      size_t total_available = page->size - page->used.length;
      if( total_available >= length)
      {
         memmove( &((char *) page->bytes)[ page->size - page->used.length],
                  &((char *) page->bytes)[ page->used.location],
                  page->used.length);
         page->used.location = page->size - page->used.length;
         available = mulle_vm_page_get_available_front( page);
      }
   }
   assert( available >= length);

   if( available > length)
      available = length;

   // figure out where the relative insertion point is from page->used.location
   old_front  = &((char *) page->bytes)[ page->used.location];
   old_middle = &((char *) old_front)[ offset];

   new_front  = &((char *) old_front)[ - (intptr_t) available];
   new_insert = &((char *) new_front)[ offset];
   new_middle = &((char *) new_insert)[ length];

   memmove( new_front, old_front, offset);
   memmove( new_middle, old_middle, page->used.length - offset);
   memcpy( new_insert, bytes, length);

   page->used.location -= available;
   page->used.length   += length;

   return( 0);
}


/*
 *
 *
 */
void   mulle_vm_set_ospage_allocator( struct mulle_ospage_allocator *allocator)
{
   mulle_thread_mutex_do( global.lock)
   {
      if( mulle_storage_get_count( &global.vm_pages) != 0
          || mulle_pointerarray_get_count( &global.freed_ospages) != 0)
      {
         fprintf( stderr, "setup comes to late\n");
         abort();
      }

      assert( allocator->alloc);
      assert( allocator->free);
      assert( allocator->pagesize);

      global.allocator.alloc    = allocator->alloc;
      global.allocator.free     = allocator->free;
      global.allocator.pagesize = allocator->pagesize;
      if( allocator->fail)
         global.allocator.fail = allocator->fail;
   }
}


void   mulle_vm_set_ospage_cachesize( size_t n_pages)
{
   mulle_thread_mutex_do( global.lock)
   {
      assert( n_pages >= mulle_pointerarray_get_count( &global.freed_ospages));

      global.max_freed_ospages = n_pages;
   }
}


MULLE_C_CONSTRUCTOR( load)
static void   load( void)
{
   mulle_thread_mutex_init( &global.lock);
   _mulle_storage_init( &global.vm_pages, sizeof( struct mulle_vm_page),
                                          alignof( struct mulle_vm_page),
                                          16,
                                          &mulle_allocator_stdlib);
   mulle_pointerarray_init( &global.freed_ospages, 16, &mulle_allocator_stdlib);

   // ppl might overwrite this early, fine by us
   if( ! global.allocator.alloc)
   {
      global.allocator = (struct mulle_ospage_allocator)
      {
         .alloc    = mulle_mmap_alloc_pages,
         .free     = mulle_mmap_free_pages,
         .pagesize = mulle_mmap_get_system_pagesize()
      };
      global.max_freed_ospages = 16;
   }

   // handle separately for debug convenience
   if( ! global.allocator.fail)
   {
      global.allocator.fail = mulle_allocator_stdlib.fail;
   }

   assert( global.allocator.pagesize == mulle_pow2round( global.allocator.pagesize) && "mulle-vm: VM pagesize corrupted");
}


// this is nice for valgrind but not really needed
MULLE_C_DESTRUCTOR( unload)
static void   unload( void)
{
   mulle_vm_free_freed_ospages_unlocked();
   mulle_pointerarray_done( &global.freed_ospages);
   _mulle_storage_done( &global.vm_pages);
   mulle_thread_mutex_done( &global.lock);
}


