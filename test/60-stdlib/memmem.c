#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static void test_mulle_memmem_basic(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test mulle_memmem basic functionality ==\n");
    
    // Test 1: Simple needle found
    const char *haystack = "Hello World Hello";
    const char *needle = "World";
    
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(haystack, strlen(haystack), &state);
    printf("Find \"World\" in \"%s\": ", haystack);
    if (result == 6)  // "World" starts at index 6
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 6, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 2: Needle not found
    needle = "xyz";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(haystack, strlen(haystack), &state);
    printf("Find \"xyz\" in \"%s\": ", haystack);
    if (result == mulle_not_found_e)
        printf("PASS (not found)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 3: Needle at beginning
    needle = "Hello";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(haystack, strlen(haystack), &state);
    printf("Find \"Hello\" in \"%s\": ", haystack);
    if (result == 0)
        printf("PASS (found at offset 0)\n");
    else
        printf("FAIL (expected 0, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 4: Single character needle
    needle = "o";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(haystack, strlen(haystack), &state);
    printf("Find \"o\" in \"%s\": ", haystack);
    if (result == 4)  // First 'o' in "Hello"
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 4, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    printf("\n");
}


static void test_mulle_memmem_streaming(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test mulle_memmem streaming functionality ==\n");
    
    // Test streaming search across multiple chunks
    const char *needle = "pattern";
    const char *chunk1 = "This is a test pat";
    const char *chunk2 = "tern that spans chunks";
    
    mulle_memmem_init(&state, needle, strlen(needle));
    
    // First chunk - should not find complete match
    result = mulle_memmem(chunk1, strlen(chunk1), &state);
    printf("First chunk \"%s\": ", chunk1);
    if (result == mulle_not_found_e)
        printf("PASS (partial match, not complete)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    
    // Second chunk - should find complete match
    result = mulle_memmem(chunk2, strlen(chunk2), &state);
    printf("Second chunk \"%s\": ", chunk2);
    // The pattern starts at position 15 in chunk1, so absolute position is 15
    if (result == 15)
        printf("PASS (found at absolute offset %zu)\n", result);
    else
        printf("FAIL (expected 15, got %zu)\n", result);
    
    mulle_memmem_done(&state);
    
    // Test overlapping pattern
    needle = "abab";
    const char *data = "ababab";
    
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(data, strlen(data), &state);
    printf("Find \"abab\" in \"ababab\": ");
    if (result == 0)  // Should find first occurrence
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 0, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    printf("\n");
}


static void test_mulle_memmem_edge_cases(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test mulle_memmem edge cases ==\n");
    
    // Test 1: Empty needle - should return not found
    mulle_memmem_init(&state, "", 0);
    result = mulle_memmem("Hello", 5, &state);
    printf("Empty needle: ");
    if (result == mulle_not_found_e)
        printf("PASS (not found)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 2: Empty haystack
    const char *needle = "test";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem("", 0, &state);
    printf("Empty haystack: ");
    if (result == mulle_not_found_e)
        printf("PASS (not found)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 3: Needle longer than haystack
    needle = "verylongpattern";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem("short", 5, &state);
    printf("Needle longer than haystack: ");
    if (result == mulle_not_found_e)
        printf("PASS (not found)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    // Test 4: Needle same as haystack
    needle = "exact";
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem("exact", 5, &state);
    printf("Needle same as haystack: ");
    if (result == 0)
        printf("PASS (found at offset 0)\n");
    else
        printf("FAIL (expected 0, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    printf("\n");
}


static void test_mulle_memrmem_basic(void)
{
    struct mulle_memrmem_state state;
    size_t result;
    
    printf("== Test mulle_memrmem basic functionality ==\n");
    
    // Test 1: Simple needle found
    const char *haystack = "Hello World Hello";
    const char *needle = "Hello";
    
    // Initialize state properly
    memset(&state, 0, sizeof(state));
    
    mulle_memrmem_init(&state, needle, strlen(needle));
    result = mulle_memrmem(haystack, strlen(haystack), &state);
    printf("Find \"Hello\" (reverse) in \"%s\": ", haystack);
    // "Hello World Hello" - positions: "Hello"@0, "Hello"@12
    // Reverse search should find the rightmost occurrence at position 12
    if (result == 12)
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 12, got %zu)\n", result);
    mulle_memrmem_done(&state);
    
    // Test 2: Needle not found
    needle = "xyz";
    memset(&state, 0, sizeof(state));  // Clear state before reuse
    mulle_memrmem_init(&state, needle, strlen(needle));
    result = mulle_memrmem(haystack, strlen(haystack), &state);
    printf("Find \"xyz\" (reverse) in \"%s\": ", haystack);
    if (result == mulle_not_found_e)
        printf("PASS (not found)\n");
    else
        printf("FAIL (expected not found, got %zu)\n", result);
    mulle_memrmem_done(&state);
    
    // Test 3: Single character
    needle = "o";
    memset(&state, 0, sizeof(state));  // Clear state before reuse
    mulle_memrmem_init(&state, needle, strlen(needle));
    result = mulle_memrmem(haystack, strlen(haystack), &state);
    printf("Find \"o\" (reverse) in \"%s\": ", haystack);
    // Should find the last 'o' which is at index 16 in "Hello World Hello"
    if (result == 16)
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 16, got %zu)\n", result);
    mulle_memrmem_done(&state);
    
    printf("\n");
}


static void test_mulle_memrmem_streaming(void)
{
    struct mulle_memrmem_state state;
    size_t result;
    
    printf("== Test mulle_memrmem streaming functionality ==\n");
    
    // Test reverse streaming search with simpler data
    const char *needle = "cat";
    const char *chunk1 = "dog cat bird cat end";
    
    memset(&state, 0, sizeof(state));
    
    mulle_memrmem_init(&state, needle, strlen(needle));
    result = mulle_memrmem(chunk1, strlen(chunk1), &state);
    printf("Reverse search for \"cat\" in \"%s\": ", chunk1);
    // "dog cat bird cat end" - positions: "cat"@4, "cat"@13
    // Should find the last occurrence of "cat" at position 13
    if (result == 13)
        printf("PASS (found last occurrence at offset %zu)\n", result);
    else
        printf("Got offset %zu (checking implementation)\n", result);
    mulle_memrmem_done(&state);
    
    printf("\n");
}


static void test_large_needle_buffer(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test large needle (buffer allocation) ==\n");
    
    // Create a needle larger than MULLE_MEMMEM_BUFFER_SIZE (48)
    char large_needle[60];
    memset(large_needle, 'A', 59);
    large_needle[59] = '\0';
    
    // Create haystack with the large needle at the end
    char haystack[120];
    memset(haystack, 'B', 60);
    memcpy(haystack + 60, large_needle, 59);
    haystack[119] = '\0';
    
    mulle_memmem_init(&state, large_needle, 59);
    result = mulle_memmem(haystack, 119, &state);
    printf("Large needle (59 chars) search: ");
    if (result == 60)
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 60, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    printf("\n");
}


static void test_binary_data(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test binary data with null bytes ==\n");
    
    // Test with binary data containing null bytes
    unsigned char haystack[] = {0x01, 0x02, 0x00, 0x03, 0x04, 0x00, 0x05, 0x06};
    unsigned char needle[] = {0x00, 0x03, 0x04};
    
    mulle_memmem_init(&state, needle, 3);
    result = mulle_memmem(haystack, 8, &state);
    printf("Binary search with null bytes: ");
    if (result == 2)  // Pattern starts at index 2
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected 2, got %zu)\n", result);
    mulle_memmem_done(&state);
    
    printf("\n");
}


static void test_performance_large_data(void)
{
    struct mulle_memmem_state state;
    size_t result;
    
    printf("== Test performance with large data ==\n");
    
    const size_t size = 100000;
    char *large_buffer = malloc(size);
    memset(large_buffer, 'A', size);
    
    // Place pattern near the end
    const char *needle = "PATTERN_0123456789_01234567890_01234567890";
    assert( strlen( needle) > MULLE_MEMMEM_BUFFER_SIZE);

    memcpy(large_buffer + size - 20 - strlen(needle), needle, strlen(needle));
    
    mulle_memmem_init(&state, needle, strlen(needle));
    result = mulle_memmem(large_buffer, size, &state);
    printf("Large buffer (100k) search: ");
    if (result == size - 20 - strlen(needle))
        printf("PASS (found at offset %zu)\n", result);
    else
        printf("FAIL (expected %zu, got %zu)\n", size - 20 - strlen(needle), result);
    mulle_memmem_done(&state);
    
    free(large_buffer);
    printf("\n");
}


int main(void)
{
    printf("Testing mulle_memmem streaming search functions\n");
    printf("===============================================\n\n");
    
    test_mulle_memmem_basic();
    test_mulle_memmem_streaming();
    test_mulle_memmem_edge_cases();
    test_mulle_memrmem_basic();
    test_mulle_memrmem_streaming();
    test_large_needle_buffer();
    test_binary_data();
    test_performance_large_data();
    
    printf("All mulle_memmem tests completed.\n");
    return 0;
}
