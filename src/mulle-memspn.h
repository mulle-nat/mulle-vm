//
//  mulle-memspn.h
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
#ifndef mulle_memspn_h__
#define mulle_memspn_h__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>


/**
 * memspn - Count the number of initial bytes in buffer that
 *          belong to the given match set.
 *
 * @buffer: input buffer to scan
 * @length: buffer length in bytes
 * @match : 256-byte lookup table, where match[b] != 0 means b is matched
 *
 * Returns the number of bytes at the start of buffer where match[b] != 0.
 */
static inline size_t mulle_memspn(const void *buffer, size_t length, const uint8_t match[256])
{
    const uint8_t *start = (const uint8_t *)buffer;
    const uint8_t *ptr = start;

    // Advance pointer while bytes belong to the match set
    while (ptr < start + length && match[*ptr])
    {
        ptr++;
    }

    return (size_t)(ptr - start);
}


/**
 * memrspn - Count trailing bytes in buffer belonging to match set,
 *           scanning backward using a single pointer only.
 *
 * @buffer: pointer to the start of the buffer
 * @length: number of bytes in the buffer
 * @match : 256-byte lookup table, where non-zero means byte is matched
 *
 * Returns count of trailing matching bytes.
 */
static inline size_t mulle_memrspn(const void *buffer, size_t length, const uint8_t match[256])
{
    const uint8_t *start = (const uint8_t *)buffer;
    const uint8_t *ptr = start + length;  // points one past last byte

    // Move ptr backward while last byte matches the set
    while (ptr > start && match[*(ptr - 1)])
    {
        ptr--;
    }

    // Count of trailing matching bytes is difference between end and ptr
    return (size_t)((start + length) - ptr);
}


#ifdef __cplusplus
}
#endif

#endif
