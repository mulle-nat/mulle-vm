//
//  mulle-memchr.h
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
#ifndef mulle_memchr_h__
#define mulle_memchr_h__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Portable handrolled unaligned 64-bit load.
 * Reads 8 bytes from ptr, byte-by-byte, returns as uint64_t.
 */
static inline uint64_t load_u64_unaligned_portable(const void *ptr)
{
    const uint8_t *src = (const uint8_t *)ptr;
    uint64_t val = 0;

    val  = (uint64_t)src[0];
    val |= (uint64_t)src[1] << 8;
    val |= (uint64_t)src[2] << 16;
    val |= (uint64_t)src[3] << 24;
    val |= (uint64_t)src[4] << 32;
    val |= (uint64_t)src[5] << 40;
    val |= (uint64_t)src[6] << 48;
    val |= (uint64_t)src[7] << 56;

    return val;
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
/**
 * On x86 and x86_64, unaligned loads of uint64_t are safe and fast,
 * so just directly cast and dereference.
 */
static inline uint64_t load_u64_unaligned_x86(const void *ptr)
{
    return *(const uint64_t *)ptr;
}
#define load_u64_unaligned(ptr) load_u64_unaligned_x86(ptr)

#elif defined(__aarch64__) || defined(__arm__)
/**
 * ARM usually requires strict alignment, use portable bytewise load.
 */
#define load_u64_unaligned load_u64_unaligned_portable

#else
/**
 * Default fallback: portable handrolled bytewise load.
 */
#define load_u64_unaligned load_u64_unaligned_portable
#endif

static inline void   *mulle_memchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s;
    uint8_t needle = (uint8_t)c;

    // Align pointer forward to 8-byte boundary for block scanning
    while (n && ((uintptr_t)p & 7)) {
        if (*p == needle)
            return (void *)p;
        p++;
        n--;
    }

    // 64-bit repeated pattern for comparison
    uint64_t pattern = 0x0101010101010101ULL * needle;

    while (n >= 8) {
        uint64_t block = load_u64_unaligned((const void *)p);

        // XOR block and pattern: zero byte means match
        uint64_t x = block ^ pattern;
        uint64_t has_zero = (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;

        if (has_zero) {
            // at least one match -> check bytes low to high
            for (int i = 0; i < 8; ++i) {
                if (p[i] == needle)
                    return (void *)(p + i);
            }
        }

        p += 8;
        n -= 8;
    }

    // Check remaining bytes
    while (n--) {
        if (*p == needle)
            return (void *)p;
        p++;
    }

    return NULL;
}


static inline void  *mulle_memrchr(const void *s, int c, size_t n)
{
    const uint8_t *p = (const uint8_t *)s;
    uint8_t needle = (uint8_t)c;
    const uint8_t *end = p + n;

    // Step backwards for unaligned bytes at the end before hitting 8-byte aligned boundary
    while (n && ((uintptr_t)(end) & 7)) {
        --end;
        --n;
        if (*end == needle)
            return (void *)end;
    }

    // 64-bit repeated pattern for comparison
    uint64_t pattern = 0x0101010101010101ULL * needle;

    while (n >= 8) {
        end -= 8;
        n -= 8;

        uint64_t block = load_u64_unaligned((const void *)end);

        // XOR block and pattern: zero byte means match
        uint64_t x = block ^ pattern;
        uint64_t has_zero = (x - 0x0101010101010101ULL) & ~x & 0x8080808080808080ULL;

        if (has_zero) {
            // at least one match -> check bytes from high to low order (reverse)
            for (int i = 7; i >= 0; --i) {
                if (end[i] == needle)
                    return (void *)(end + i);
            }
        }
    }

    // Check remaining bytes
    while (n) {
        --end;
        --n;
        if (*end == needle)
            return (void *)end;
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
