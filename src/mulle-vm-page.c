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
   struct mulle_storage   vm_pages;
   mulle_thread_mutex_t   lock;
} global;


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
      _mulle_storage_free( &global.vm_pages, page);
   }
}


// we don't really care to leak check this, so use &mulle_allocator_stlib
MULLE_C_CONSTRUCTOR( load)
static void   load( void)
{
   mulle_thread_mutex_init( &global.lock);
   _mulle_storage_init( &global.vm_pages, sizeof( struct mulle_vm_page),
                                          alignof( struct mulle_vm_page),
                                          16,
                                          &mulle_allocator_stdlib);
}


// this is nice for valgrind but not really needed
MULLE_C_DESTRUCTOR( unload)
static void   unload( void)
{
   _mulle_storage_done( &global.vm_pages);
   mulle_thread_mutex_done( &global.lock);
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


// range must either be adjacent or intersecting the currently used bytes
// in page.
//
// range: range is vm_address, rnage
// bytes:
//
// returns 0 on success
// ENOSPC if won't fit
// EINVAL if range is not adjacent or intersecting
//
int  _mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t vm_address,
                                  struct mulle_range range,
                                  void *bytes)
{
   struct mulle_range   vm_used;
   struct mulle_range   intersection;
   size_t               available;
   size_t               offset;
   char                 *new_front;
   char                 *old_front;
   char                 *new_insert;
   char                 *old_middle;
   char                 *new_middle;
   int                  adjacent;

   assert( page->retain_count == 0 && "can't modify shared page");
   // you can only change unshared pages
   if( page->retain_count != 0)
      return( EACCES);

   if( page->size - page->used.length < range.length)
      return( ENOSPC);

   vm_used  = mulle_range_make( vm_address, page->used.length);
   // clean append to back
   adjacent = mulle_range_get_max( vm_used) == range.location;
   if( adjacent)
   {
      available = mulle_vm_page_get_available_back( page);
      if( available < range.length)
      {
         // need to make room, by moving bytes to front (we move it all to 0)
         memmove( &((char *) page->bytes)[ 0],
                  &((char *) page->bytes)[ page->used.location],
                  page->used.length);
         page->used.location = 0;
      }

      memcpy( &((char *) page->bytes)[ mulle_range_get_max( page->used)],
              bytes,
              range.length);
      page->used.length  += range.length;
      return( 0);
   }

   adjacent = mulle_range_get_max( range) == vm_used.location;
   // clean adjacent insert in front, or (special case) no split, the tail
   // of range intersects with 10
   if( adjacent || range.location == vm_used.location)
   {
      available = mulle_vm_page_get_available_front( page);
      if( available < range.length)
      {
         // need to make room, by moving range to back (we move it all to end)
         memmove( &((char *) page->bytes)[ page->size - page->used.length],
                  &((char *) page->bytes)[ page->used.location],
                  page->used.length);
         page->used.location = page->size - page->used.length;
      }

      page->used.location -= range.length;

      memcpy( &((char *) page->bytes)[ page->used.location],
              bytes,
              range.length);

      page->used.length += range.length;

      return( 0);
   }


   // so we gots to have an intersection now
   intersection = mulle_range_intersection( vm_used, range);
   if( ! intersection.length)
      return( EINVAL);

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
   // figure out, the amount we can move ahead
   //
   available = mulle_vm_page_get_available_front( page);
   if( available > range.length)
      available = range.length;

   // figure out where the relative insertion point is from page->used.location
   offset     = range.location - vm_address;

   old_front  = &((char *) page->bytes)[ page->used.location];
   old_middle = &((char *) old_front)[ offset];

   new_front  = &((char *) old_front)[ - (intptr_t) available];
   new_insert = &((char *) new_front)[ offset];
   new_middle = &((char *) new_insert)[ range.length];

   memmove( new_front, old_front, offset);
   memmove( new_middle, old_middle, page->used.length - offset);
   memcpy( new_insert, bytes, range.length);

   page->used.location -= available;
   page->used.length   += range.length;

   return( 0);
}

