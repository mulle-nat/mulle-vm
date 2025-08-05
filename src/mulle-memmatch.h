#ifndef mulle_memmatch_h__
#define mulle_memmatch_h__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * mulle_memmatch - Find first occurrence of a byte matching the set in a buffer.
 *
 * @buffer: pointer to the start of the buffer (must not be NULL)
 * @length: number of bytes in the buffer (must be >= 0)
 * @match : 256-byte lookup table; non-zero entries indicate bytes to match
 *
 * Scans the buffer from the beginning (left to right) using a single pointer,
 * returning the address of the first byte in the buffer that is present in the match set.
 * The scanning loop uses only pointer arithmetic and comparison to a sentinel,
 * with no loop variable or index arithmetic.
 *
 * The sentinel is computed once as `buffer + length`, and the loop terminates
 * when the pointer reaches or exceeds it.
 *
 * Returns:
 *   A pointer to the first byte in the buffer that matches the set, or NULL
 *   if no matching byte is found or if length is zero.
 *
 * Example:
 *   char data[] = "hello world";
 *   uint8_t match[256] = {0};
 *   match['w'] = 1;
 *   match['r'] = 1;
 *   void *result = mulle_memmatch(data, 11, match);
 *   // Returns pointer to 'w' in "world"
 *
 * Note:
 *   The match table must be valid and contain 256 entries.
 *   No bounds checking is performed on the match table.
 *   The function is safe for zero-length buffers.
 */
static inline void *mulle_memmatch(const void *buffer, size_t length, const uint8_t match[256])
{
    const uint8_t *p = (const uint8_t *)buffer;
    const uint8_t *const end = p + length;

    while (p < end)
    {
        if (match[*p])
            return (void *)p;
        p++;
    }

    return NULL;
}


/**
 * mulle_memrmatch - Find last occurrence of a byte matching the set in a buffer.
 *
 * @buffer: pointer to the start of the buffer (must not be NULL)
 * @length: number of bytes in the buffer (must be >= 0)
 * @match : 256-byte lookup table; non-zero entries indicate bytes to match
 *
 * Scans the buffer from the end (right to left) using a single pointer,
 * returning the address of the last byte in the buffer that is present in the match set.
 * The scanning loop uses only pointer arithmetic and comparison to a sentinel,
 * with no loop variable or index arithmetic.
 *
 * The sentinel is computed once as `buffer + length - 1`, and the loop terminates
 * when the pointer is no longer greater than it.
 *
 * Returns:
 *   A pointer to the last byte in the buffer that matches the set, or NULL
 *   if no matching byte is found or if length is zero.
 *
 * Example:
 *   char data[] = "hello world";
 *   uint8_t match[256] = {0};
 *   match['w'] = 1;
 *   match['r'] = 1;
 *   void *result = mulle_memrmatch(data, 11, match);
 *   // Returns pointer to 'r' in "world"
 *
 * Note:
 *   The match table must be valid and contain 256 entries.
 *   No bounds checking is performed on the match table.
 *   The function is safe for zero-length buffers.
 *   If length is zero, returns NULL.
 */
static inline void *mulle_memrmatch(const void *buffer, size_t length, const uint8_t match[256])
{
    const uint8_t *p = (const uint8_t *)buffer + length;
    const uint8_t *const sentinel = p - 1;  // start at last byte

    while (p > sentinel)
    {
        if (match[*p])
            return (void *)p;
        p--;
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
