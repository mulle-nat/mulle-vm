//
//  mulle-memmem.c
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
#include "mulle-memmem.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "include-private.h"


// Public: initialize state with needle
void  mulle_memmem_init( struct mulle_memmem_state *state,
                         const void *needle,
                         size_t needle_len)
{
    if( ! state)
       return;

    memset( state, 0, sizeof(*state));

    // Try to use static buffer for LPS if small enough
    if( needle_len <= MULLE_MEMMEM_BUFFER_SIZE)
       state->lps = (size_t *) state->buffer;
    else
       state->lps = mulle_malloc( needle_len * sizeof(size_t));

    state->needle      = needle;
    state->needle_len  = needle_len;

    size_t *lps = (size_t *)state->lps;
    lps[0] = 0;
    size_t j = 0;

    for (size_t i = 1; i < needle_len; i++) {
        while (j > 0 && ((const uint8_t *)needle)[i] != ((const uint8_t *)needle)[j]) {
            assert(j > 0);
            assert(j - 1 < needle_len);
            j = lps[j - 1];
        }
        if (((const uint8_t *)needle)[i] == ((const uint8_t *)needle)[j]) {
            j++;
        }
        assert(i < needle_len);
        lps[i] = j;
    }
}

// Public: clean up
void   mulle_memmem_done(struct mulle_memmem_state *state)
{
    if (!state)
      return;
    if (state->buffer != state->lps)
        mulle_free( state->lps);

#ifdef DEBUG
    mulle_memset_uint32( state, 0xDEADFACE, sizeof(*state));
#endif
}


// Public: feed data and search for needle in streaming manner
// Returns: absolute offset in total stream where match starts,
// or mulle_not_found_e (e.g. SIZE_MAX) if not found.
size_t mulle_memmem( const void *data,
                     size_t data_len,
                     struct mulle_memmem_state *state)
{
    if( ! state || ! data || data_len == 0)
        return mulle_not_found_e;
    
    // Handle empty needle case
    if( state->needle_len == 0)
        return mulle_not_found_e;

    const uint8_t *bytes = (const uint8_t *)data;
    const uint8_t *needle = (const uint8_t *)state->needle;
    size_t j = state->match_index;  // Current partial match length
    size_t needle_len = state->needle_len;

    for (size_t i = 0; i < data_len; i++) {
        // While mismatch and j>0, fallback in prefix table
        while (j > 0 && bytes[i] != needle[j]) {
            assert(j > 0);
            assert(j - 1 < needle_len);
            j = state->lps[j - 1];
        }

        // If match, advance j
        if (bytes[i] == needle[j]) {
            j++;
        }

        // If full match found
        if (j == needle_len)
        {
            // Absolute offset where match starts in the stream
            size_t absolute_offset = state->total_bytes + i - needle_len + 1;

            // Reset match state for next search instructions or keep as-is for continued search
            state->match_index = 0;

            return absolute_offset;
        }
    }

    // Keep track how much of the needle matched so far
    state->match_index = j;

    // Update total bytes seen so far
    state->total_bytes += data_len;

    return mulle_not_found_e;
}



// Init for backward streaming KMP without reversing needle.
// Computes LPS backward on needle as-is.
void   mulle_memrmem_init(struct mulle_memrmem_state *state,
                          const void *needle,
                          size_t needle_len)
{
    if( ! state)
        return;

    memset( state, 0, sizeof(*state));

    if( ! needle || needle_len == 0)
      return;

    // Allocate LPS storage or use buffer optimization
    if (needle_len <= MULLE_MEMMEM_BUFFER_SIZE)
        state->lps = (size_t *) state->buffer;
    else
        state->lps = (size_t *) mulle_malloc(needle_len * sizeof(size_t));

    state->needle     = needle;
    state->needle_len = needle_len;


    size_t *lps = state->lps;
    const uint8_t *needle_bytes = (const uint8_t *)needle;

    // Compute LPS backward:
    // lps[i] = length of longest proper prefix of suffix needle[i..end]
    // that is also suffix ending at needle[end].
    //
    // We walk needle indices from end to start.
    // Note: careful handling when i=0 (size_t underflow).
    lps[needle_len - 1] = 0;
    size_t j = 0;  // length of previous longest prefix suffix

    // i goes backward from second-to-last down to 0
    for (size_t idx = needle_len - 1; idx > 0; idx--) {
        size_t i = idx - 1;
        while (j > 0 && needle_bytes[i] != needle_bytes[needle_len - 1 - j]) {
            assert(j > 0);
            assert(needle_len - 1 - j < needle_len);
            j = lps[needle_len - 1 - j];
        }
        if (needle_bytes[i] == needle_bytes[needle_len - 1 - j]) {
            j++;
        }
        assert(i < needle_len);
        lps[i] = j;
    }
}

// Cleanup function frees memory if needed
void   mulle_memrmem_done(struct mulle_memrmem_state *state)
{
    if( ! state)
        return;

    if( state->lps != state->buffer)
    {
        mulle_free( state->lps);
    }

#ifdef DEBUG
    memset(state, 0xDE, sizeof(*state));
#endif
}

// Streaming reverse search over input chunk (without internal buffer).
// Input chunk is processed backward.
// Returns absolute offset in forward stream where match starts,
// or mulle_not_found_e if no match.
size_t   mulle_memrmem(const void *data, size_t data_len, struct mulle_memrmem_state *state)
{
    if( ! state || ! data || data_len == 0)
        return mulle_not_found_e;
    if( ! state->needle_len)
        return mulle_not_found_e;

    const uint8_t *bytes = (const uint8_t *)data;
    const uint8_t *needle = (const uint8_t *)state->needle;
    size_t needle_len = state->needle_len;
    size_t j = state->match_index;
    size_t *lps = state->lps;

    // Process the slice backward (from end to front)
    for (ssize_t i = (ssize_t)data_len - 1; i >= 0; i--) {
        // While mismatch and j>0, fallback lps
        while (j > 0 && bytes[i] != needle[needle_len - 1 - j]) {
            assert(j > 0);
            assert(needle_len - 1 - j < needle_len);
            j = lps[needle_len - 1 - j];
        }

        // If match at current position, advance j
        if (bytes[i] == needle[needle_len - 1 - j])
        {
            j++;
        }

        // Full match found
        if (j == needle_len)
        {
            // Calculate absolute match offset in forward stream
            // When we find a match at position i, the match starts at position i
            // since we're processing backwards and i is the start of the match
            size_t abs_offset = state->total_bytes + i;

            // Reset match_index to allow for next search continuation (or you can keep j=0 for exact one-shot)
            state->match_index = 0;

            return abs_offset;
        }
    }

    // No full match found yet, save partial match progress
    state->match_index = j;

    // Advance total_bytes by current data length
    state->total_bytes += data_len;

    return mulle_not_found_e;
}
