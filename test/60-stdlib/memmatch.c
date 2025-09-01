#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


// Helper: build match set from a list of bytes
static void build_match_set(uint8_t match[256], const uint8_t *bytes, size_t count)
{
    memset(match, 0, 256);
    for (size_t i = 0; i < count; i++) {
        match[bytes[i]] = 1;
    }
}

// Test: Basic functionality
static void test_basic(void)
{
    mulle_printf("== Test: Basic functionality ==\n");

    uint8_t buffer[] = "Hello, World!";
    uint8_t match[256];

    // Test 1: Match 'H' at start
    build_match_set(match, (uint8_t[]){ 'H' }, 1);
    void *result = mulle_memmatch(buffer, strlen(buffer), match);
    mulle_printf("Find 'H': %s\n", result == buffer ? "PASS" : "FAIL");

    // Test 2: Match '!' at end
    build_match_set(match, (uint8_t[]){ '!' }, 1);
    result = mulle_memmatch(buffer, strlen(buffer), match);
    mulle_printf("Find '!': %s\n", result == buffer + 12 ? "PASS" : "FAIL");

    // Test 3: Match 'l' (first occurrence)
    build_match_set(match, (uint8_t[]){ 'l' }, 1);
    result = mulle_memmatch(buffer, strlen(buffer), match);
    mulle_printf("Find 'l' (first): %s\n", result == buffer + 2 ? "PASS" : "FAIL");

    // Test 4: No match
    build_match_set(match, (uint8_t[]){ 'X' }, 1);
    result = mulle_memmatch(buffer, strlen(buffer), match);
    mulle_printf("Find 'X' (not present): %s\n", result == NULL ? "PASS" : "FAIL");

    mulle_printf("\n");
}

// Test: Edge cases
static void test_edge_cases(void)
{
    mulle_printf("== Test: Edge cases ==\n");

    uint8_t match[256];
    memset(match, 0, 256);

    // 1. Empty buffer
    void *result = mulle_memmatch("", 0, match);
    mulle_printf("Empty buffer: %s\n", result == NULL ? "PASS" : "FAIL");

    // 2. Null byte in buffer
    uint8_t buf[] = "Hello\0World";
    build_match_set(match, (uint8_t[]){ '\0' }, 1);
    result = mulle_memmatch(buf, 11, match);
    mulle_printf("Find '\\0' in \"Hello\\0World\": %s\n", result == buf + 5 ? "PASS" : "FAIL");

    // 3. Single byte
    uint8_t single[] = "A";
    build_match_set(match, (uint8_t[]){ 'A' }, 1);
    result = mulle_memmatch(single, 1, match);
    mulle_printf("Single char 'A' match: %s\n", result == single ? "PASS" : "FAIL");

    // 4. No match in buffer
    build_match_set(match, (uint8_t[]){ 'Z' }, 1);
    result = mulle_memmatch("Hello", 5, match);
    mulle_printf("Find 'Z' in \"Hello\": %s\n", result == NULL ? "PASS" : "FAIL");

    mulle_printf("\n");
}

// Test: Alignment (from any offset)
static void test_alignment(void)
{
    mulle_printf("== Test: Alignment (from any offset) ==\n");

    uint8_t *buffer = malloc(64);
    memset(buffer, 'X', 64);
    buffer[16] = 'T';  // place 'T' at offset 16

    uint8_t match[256];
    build_match_set(match, (uint8_t[]){ 'T' }, 1);

    for (int i = 0; i < 8; i++) {
        void *result = mulle_memmatch(buffer + i, 32, match);
        mulle_printf("Alignment %d: ", i);
        
        // Check if 'T' should be found in this search range
        if (16 >= i && 16 < i + 32) {
            // 'T' should be found
            if (result == buffer + 16) {
                mulle_printf("PASS (found 'T' at offset %d)\n", 16 - i);
            } else {
                mulle_printf("FAIL (expected 'T' at offset %d, got %p)\n", 16 - i, result);
            }
        } else {
            // 'T' should not be found
            if (result == NULL) {
                mulle_printf("PASS (no 'T' in range)\n");
            } else {
                mulle_printf("FAIL (expected NULL, got %p)\n", result);
            }
        }
    }

    free(buffer);
    mulle_printf("\n");
}

// Test: Large buffer (performance + correctness)
static void test_large(void)
{
    mulle_printf("== Test: Large buffer (10k) ==\n");

    const size_t size = 10000;
    uint8_t *buf = malloc(size);
    memset(buf, 'A', size);

    uint8_t match[256];
    build_match_set(match, (uint8_t[]){ 'T' }, 1);

    // No 'T' → should return NULL
    void *result = mulle_memmatch(buf, size, match);
    mulle_printf("Find 'T' in all 'A's: %s\n", result == NULL ? "PASS" : "FAIL");

    // Insert 'T' at various positions
    int positions[] = { 0, 100, 5000, 9999 };
    for (int i = 0; i < 4; i++) {
        size_t pos = positions[i];
        buf[pos] = 'T';

        result = mulle_memmatch(buf, size, match);
        mulle_printf("Find 'T' at pos %zu: %s\n", pos, result == buf + pos ? "PASS" : "FAIL");

        buf[pos] = 'A'; // reset
    }

    free(buf);
    mulle_printf("\n");
}

// Main: Run all tests
int main(void)
{
    mulle_printf("Running mulle_memmatch tests...\n\n");

    test_basic();
    test_edge_cases();
    test_alignment();
    test_large();

    mulle_printf("🎉 All tests complete. No extra code, no self-reference. Just output.\n");
    return 0;
}
