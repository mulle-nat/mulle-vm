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
#ifndef mulle_vm_page_h__
#define mulle_vm_page_h__

#include "include.h"

#include <stddef.h>


typedef void   mulle_vm_ospage;


struct mulle_ospage_allocator
{
   size_t   pagesize;
   void     *(*alloc)( size_t);
   int      (*free)( void *, size_t);
   void     (*fail)( struct mulle_allocator *, void *, size_t)  MULLE_C_NO_RETURN;
};


MULLE__VM_GLOBAL
void   mulle_vm_set_ospage_allocator( struct mulle_ospage_allocator *allocator);

MULLE__VM_GLOBAL
void   mulle_vm_set_ospage_cachesize( size_t n_pages);

MULLE__VM_GLOBAL
mulle_vm_ospage *   _mulle_vm_alloc_ospages( size_t size);


MULLE__VM_GLOBAL
void                _mulle_vm_free_ospages( mulle_vm_ospage *mem, size_t size);



/**
 * Get the page size of a VM instance.
 *
 * Returns the configured page size for the given VM. If vm is NULL,
 * returns the default page size (MULLE__VM_PAGESIZE_DEFAULT).
 *
 * @param vm Pointer to the VM structure, or NULL for default
 * @return The page size in bytes
 */
MULLE__VM_GLOBAL
size_t   mulle_vm_get_pagesize( void);



//
// used by "copy", we share pages with another mulle_vm_page (i.e.) two
// references. If we ever write to the copied page, we need to copy the
// bytes. If we want to model "empty" space, we could allow bytes to be
// NULL but so far we don't.
//
struct mulle_vm_page
{
   mulle_vm_ospage         *bytes;         // mapped page start
   struct mulle_range      used;           // used range [location, location+length)
   size_t                  size;           // total mapped size
   mulle_atomic_pointer_t  retain_count;
};


static inline void   mulle_vm_page_init( struct mulle_vm_page *page,
                                         mulle_vm_ospage *bytes,
                                         struct mulle_range used,
                                         size_t size)
{
   used = mulle_range_validate_against_length( used, size);

   page->bytes        = bytes;
   page->used         = used;
   page->size         = size;
   page->retain_count = 0;
}


static inline uintptr_t
   _mulle_vm_page_offset( struct mulle_vm_page *page, uintptr_t offset)
{
   if( ! page)
   {
      fprintf( stderr, "SIGSEGV: offset %td with no page in mulle_vm", offset);
      abort();
   }

   if( offset >= page->used.length)
   {
      fprintf( stderr, "SIGSEGV: offset %td out of bounds in mulle_vm", offset);
      abort();
   }

   return( page->used.location + offset);
}


static inline void *
   _mulle_vm_page_address( struct mulle_vm_page *page, uintptr_t relative)
{
   uintptr_t   offset;

   offset = _mulle_vm_page_offset( page, relative);
   return( &((char *) page->bytes)[ offset]);
}


static inline size_t
   _mulle_vm_page_get_length( struct mulle_vm_page *page)
{
   return( page->used.length);
}


static inline void *
   mulle_vm_page_address( struct mulle_vm_page *page, uintptr_t relative)
{
   if( ! page)
      return( NULL);
   return(_mulle_vm_page_address( page, relative));
}


static inline size_t
   mulle_vm_page_get_length( struct mulle_vm_page *page)
{
   return( page ? page->used.length : 0);
}


static inline struct mulle_data
   mulle_vm_page_get_data( struct mulle_vm_page *page)
{
   return( page ? mulle_data_make( _mulle_vm_page_address( page, 0),
                                   _mulle_vm_page_get_length( page))
                : mulle_data_make_invalid());
}


static inline size_t
   mulle_vm_page_get_available( struct mulle_vm_page *page)
{
   return( page ? page->size - page->used.length : 0);
}


static inline size_t
   mulle_vm_page_get_available_front( struct mulle_vm_page *page)
{
   return( page ? page->used.location : 0);
}


static inline size_t
   mulle_vm_page_get_available_back( struct mulle_vm_page *page)
{
   return( page ? page->size - mulle_range_get_max( page->used) : 0);
}



static inline size_t
   mulle_vm_page_get_used_length_first_ospage( struct mulle_vm_page *page,
                                               size_t pagesize)
{
   size_t   length;

   if( page->used.location >= pagesize)
      return( 0);

   length = pagesize - page->used.location;
   if( mulle_range_get_max( page->used) < pagesize)
      length = page->used.length;
   return( length);
}


static inline size_t
   mulle_vm_page_get_used_length_last_ospage( struct mulle_vm_page *page,
                                              size_t pagesize)
{
   size_t   end;
   size_t   length;

   end = mulle_range_get_max( page->used);
   if( end <= page->size - pagesize)
      return( 0);

   length = page->size - end;
   if( page->used.location > page->size - pagesize)
      length = page->used.length;
   return( length);
}


MULLE__VM_GLOBAL
struct mulle_vm_page   *mulle_vm_page_alloc( void);

MULLE__VM_GLOBAL
void   mulle_vm_page_free( struct mulle_vm_page *page);

MULLE__VM_GLOBAL
struct mulle_vm_page *  mulle_vm_page_new_with_bytes( void *bytes,
                                                      size_t length,
                                                      unsigned int skew);

//
// Offset is can be from 0 to page->length.used (0: prepend,
// page->length.used: append, other: split)
// If page is shared or does not have enough room returns -1
//
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_THIRD
int  _mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t offset,
                                  void *bytes,
                                  size_t length);
static inline
MULLE_C_NONNULL_THIRD
int  mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t offset,
                                  void *bytes,
                                  size_t length)
{
   if( ! page)
      return( 0);
   return( _mulle_vm_page_insert_bytes( page, offset, bytes, length));
}


MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_SECOND
int   _mulle_vm_page_join( struct mulle_vm_page *a, struct mulle_vm_page *b);


MULLE__VM_GLOBAL
struct mulle_vm_page *mulle_vm_page_retain( struct mulle_vm_page *page);

// MEMO: only tests should call this...
MULLE__VM_GLOBAL
void   mulle_vm_page_release( struct mulle_vm_page *page);


// MEMO: only tests should call this...
static inline intptr_t
   mulle_vm_page_get_retaincount( struct mulle_vm_page *page)
{
   intptr_t    rc;

   if( ! page)
      return( 0);

   rc = (intptr_t) _mulle_atomic_pointer_read( &page->retain_count);
   return( rc);
}


MULLE__VM_GLOBAL
void   mulle_vm_page_free_unused_ospages( struct mulle_vm_page *page);

MULLE__VM_GLOBAL
void   mulle_vm_reset_freed_ospages( void);


#endif
