//
//  mulle-vm-stdlib.c
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
#include "mulle-vm-stdlib.h"

#include "include-private.h"

#include "mulle-memchr.h"
#include "mulle-memmatch.h"
#include "mulle-memspn.h"
#include "mulle-vm-page.h"
#include "mulle-vm.h"
#include <errno.h>



struct memchr_info
{
   uintptr_t   vm_address;
   int         c;
};


static int  mulle_memchr_callback( struct mulle_data data,
                                   struct mulle_range vm_range,
                                   void *userinfo)
{
   struct memchr_info  *info = userinfo;
   char                *found;

   found = mulle_memchr( data.bytes, info->c, data.length);
   if( ! found)
      return( 0);

   info->vm_address = vm_range.location + (found - (char *) data.bytes);
   return( 1);
}


static int  mulle_memrchr_callback( struct mulle_data data,
                                    struct mulle_range vm_range,
                                    void *userinfo)
{
   struct memchr_info  *info = userinfo;
   char                *found;

   found = mulle_memrchr( data.bytes, info->c, data.length);
   if( ! found)
      return( 0);

   info->vm_address = vm_range.location + (found - (char *) data.bytes);
   return( 1);
}


// Search for a character in mulle_vm pages
uintptr_t   mulle_vm_memchr( struct mulle_vm *vm,
                             struct mulle_range range,
                             int c)
{
   struct memchr_info  info = { .c = c, .vm_address = mulle_not_found_e };

   mulle_vm_walk( vm, range, mulle_memchr_callback, &info);
   return( info.vm_address);
}


uintptr_t   mulle_vm_memrchr( struct mulle_vm *vm,
                              struct mulle_range range,
                              int c)
{
   struct memchr_info  info = { .c = c, .vm_address = mulle_not_found_e };

   mulle_vm_walk_reverse( vm, range, mulle_memrchr_callback, &info);
   return( info.vm_address);
}




struct memmatch_info
{
   uintptr_t       vm_address;
   const uint8_t   *match;
};


static int  mulle_memmatch_callback( struct mulle_data data,
                                     struct mulle_range vm_range,
                                     void *userinfo)
{
   struct memmatch_info  *info = userinfo;
   char                  *found;

   found = mulle_memmatch( data.bytes, data.length, info->match);
   if( ! found)
      return( 0);

   info->vm_address = vm_range.location + (found - (char *) data.bytes);
   return( 1);
}


static int  mulle_memrmatch_callback( struct mulle_data data,
                                    struct mulle_range vm_range,
                                    void *userinfo)
{
   struct memmatch_info  *info = userinfo;
   char                  *found;

   found = mulle_memrmatch( data.bytes, data.length, info->match);
   if( ! found)
      return( 0);

   info->vm_address = vm_range.location + (found - (char *) data.bytes);
   return( 1);
}


// Search for a character in mulle_vm pages
uintptr_t   mulle_vm_memmatch( struct mulle_vm *vm,
                               struct mulle_range range,
                               const uint8_t match[256])
{
   struct memmatch_info  info = { .match = match, .vm_address = mulle_not_found_e };

   mulle_vm_walk( vm, range, mulle_memmatch_callback, &info);
   return( info.vm_address);
}


uintptr_t   mulle_vm_memrmatch( struct mulle_vm *vm,
                                struct mulle_range range,
                                const uint8_t match[256])
{
   struct memmatch_info  info = { .match = match, .vm_address = mulle_not_found_e };

   mulle_vm_walk_reverse( vm, range, mulle_memrmatch_callback, &info);
   return( info.vm_address);
}



/* * * */

struct memspn_info
{
   size_t    span;
   const uint8_t   *match;
};


static int  mulle_memspn_callback( struct mulle_data data,
                                    struct mulle_range vm_range,
                                    void *userinfo)
{
   struct memspn_info  *info = userinfo;
   size_t              count;

   count       = mulle_memspn( data.bytes, data.length, info->match);
   info->span += count;

   if( count == data.length)
      return( 0);

   // stoppeth
   return( 1);
}


static int  mulle_memrspn_callback( struct mulle_data data,
                                    struct mulle_range vm_range,
                                    void *userinfo)
{
   struct memspn_info  *info = userinfo;
   size_t              count;

   count       = mulle_memrspn( data.bytes, data.length, info->match);
   info->span += count;

   if( count == data.length)
      return( 0);

   // stoppeth
   return( 1);
}



uintptr_t   mulle_vm_memspn( struct mulle_vm *vm,
                             struct mulle_range range,
                             const uint8_t match[256])
{
   struct memspn_info  info = { .match = match };

   mulle_vm_walk( vm, range, mulle_memspn_callback, &info);
   return( info.span);
}


uintptr_t   mulle_vm_memrspn( struct mulle_vm *vm,
                              struct mulle_range range,
                              const uint8_t match[256])
{
   struct memspn_info  info = { .match = match };

   mulle_vm_walk( vm, range, mulle_memrspn_callback, &info);
   return( info.span);
}

