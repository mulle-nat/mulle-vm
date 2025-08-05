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
