//
//  mulle-memmem.h
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
#ifndef mulle_memmem_h__
#define mulle_memmem_h__

#include "include.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
// Maximum size of the internal buffer (avoids malloc)
#ifndef MULLE_MEMMEM_BUFFER_SIZE
# define MULLE_MEMMEM_BUFFER_SIZE  24
#endif


// Internal state
struct mulle_memmem_state
{
   const void    *needle;
   size_t        needle_len;
   size_t        *lps;
   size_t        buffer[ MULLE_MEMMEM_BUFFER_SIZE]; // Internal buffer

   size_t        match_index;   // how many matched so far (KMP state)
   size_t        total_bytes;
};


// Public API
MULLE__VM_GLOBAL
void   mulle_memmem_init( struct mulle_memmem_state *state,
                          const void *needle,
                          size_t needle_len);

MULLE__VM_GLOBAL
void   mulle_memmem_done( struct mulle_memmem_state *state);

// Returns:
//    mulle_not_found_e
//    else: absolute offset where match was found
// this is at the beginning of the needle
MULLE__VM_GLOBAL
size_t   mulle_memmem( const void *data,
                       size_t data_len,
                       struct mulle_memmem_state *state);

/*
 *
 */
// State struct
struct mulle_memrmem_state
{
   const void    *needle;
   size_t        needle_len;
   size_t        *lps;
   size_t        buffer[ MULLE_MEMMEM_BUFFER_SIZE]; // Internal buffer

   size_t        match_index;   // how many matched so far (KMP state)
   size_t        total_bytes;
};


MULLE__VM_GLOBAL
void   mulle_memrmem_init( struct mulle_memrmem_state *state,
                         const void *needle,
                         size_t needle_len);

MULLE__VM_GLOBAL
void   mulle_memrmem_done( struct mulle_memrmem_state *state);




// Returns:
//    mulle_not_found_e
//    else: absolute offset where match was found
//    this is at the end of the needle
MULLE__VM_GLOBAL
size_t   mulle_memrmem( const void *data,
                        size_t data_len,
                        struct mulle_memrmem_state *state);

#ifdef __cplusplus
}
#endif

#endif
