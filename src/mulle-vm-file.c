//
//  mulle-vm-file.c
//  mulle-vm
//
//  Copyright (c) 2025 Nat! - Mulle kybernetiK.
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
#include "mulle-vm-file.h"

#include "include-private.h"

#include "mulle-vm-page.h"
#include "mulle-vm.h"


// could be API but isnt yet
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_SECOND
void   _mulle_vm_append_ospages( struct mulle_vm *vm,
                                 void *pages,
                                 size_t length);

int   mulle_vm_init_with_contents_of_file( struct mulle_vm *vm,
                                           char *path,
                                           struct mulle_allocator *allocator)
{
   struct mulle_mmap      map;
   void                   *pages;
   int                    rval;
   size_t                 length;

    // Grab pages (and keep alive after mulle_mmap_done)
   mulle_mmap_init( &map, mulle_mmap_read | mulle_mmap_no_unmap);
   rval   = mulle_mmap_map_file( &map, path);
   pages  = mulle_mmap_get_bytes( &map);
   length = mulle_mmap_get_length( &map);
   mulle_mmap_done( &map);

   // delayed error return
   if( rval)
   {
      memset( vm, 0, sizeof( *vm));
      return( rval);
   }

   // need default mmap based config here
   mulle_vm_init( vm, NULL, allocator);
   _mulle_vm_append_ospages( vm, pages, length);
   return( 0);
}



int   mulle_vm_writev_contents( struct mulle_vm *vm, int fd)
{
   unsigned int   n;
   unsigned int   i;
   int            rval;

   n = _mulle__rbtree_get_count( (struct mulle__rbtree *) &vm->tree);
   mulle_alloca_do( ios, struct mulle_iovec, n)
   {
      i = 0;
      mulle_vm_tree_for( &vm->tree, node)
      {
         ios[ i].iov_base = mulle_vm_page_address( node->page, 0);
         ios[ i].iov_len  = mulle_vm_page_get_length( node->page);
         i++;
      }

      rval = mulle_writev( fd, ios, n);
   }
   return( rval);
}

