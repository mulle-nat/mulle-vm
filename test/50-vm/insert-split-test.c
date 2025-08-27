//
//  insert-split-test.c
//  mulle-vm
//
//  Test to trigger page split scenario in mulle_vm_insert
//  This test creates a scenario where we try to insert data in the middle
//  of a page that doesn't have enough space, triggering the ENOSPC error.
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
    struct mulle_vm vm;
    char initial_data[6];     // Fill most of an 8-byte debug page
    char insert_data[10];     // Data that won't fit in remaining space
    int result;
    
    mulle_printf("=== Testing mulle_vm_insert page split scenario ===\n");
    
    // Initialize VM with debug config (8-byte pages)
    mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
    
    
    mulle_printf("VM initialized with pagesize=%zu, skew=%zu\n", 
           mulle_vm_debug_config.pagesize, mulle_vm_debug_config.skew);
    
    // Fill initial data with recognizable pattern (5 chars + null terminator = 6 bytes)
    strcpy(initial_data, "ABCDE");
    
    // Fill insert data with different pattern (9 chars + null terminator = 10 bytes)
    strcpy(insert_data, "123456789");
    
    
    // First, append the initial data to create a nearly full page
    // With 8-byte pages and 25% skew, 6 bytes should nearly fill one page
    mulle_printf("\nStep 1: Appending %zu bytes of initial data...\n", strlen(initial_data));
    mulle_vm_append(&vm, initial_data, strlen(initial_data));
    
    mulle_printf("VM length after append: %zu bytes\n", (size_t)mulle_vm_get_length(&vm));
    // Calculate available space in the page
    // With 25% skew, in an 8-byte page with 5 bytes data:
    // - Page has 8 bytes total
    // - Used: 5 bytes + skew offset
    // - Remaining space should be very limited
    
    // Now try to insert large data in the middle (offset 2)
    // This should exceed available space and trigger the split case
    mulle_printf("\nStep 2: Attempting to insert %zu bytes at offset 2...\n", strlen(insert_data));
    struct mulle_range insert_range = { .location = 2, .length = strlen(insert_data) };
    
    result = mulle_vm_insert(&vm, insert_range, insert_data);
    if(result == -1 && errno == ENOSPC) {
        mulle_printf("SUCCESS: Got expected ENOSPC error (page split not implemented)\n");
        mulle_printf("errno = %d (ENOSPC)\n", errno);
    } else if(result == 0) {
        mulle_printf("SUCCESS: Insert split the page as expected\n");
        mulle_printf("New VM length: %zu bytes\n", (size_t)mulle_vm_get_length(&vm));
    } else {
        mulle_printf("ERROR: Got unexpected error result=%d, errno=%d\n", result, errno);
        return 1;
    }
    
    // Test a smaller insert that should fit
    mulle_printf("\nStep 3: Testing smaller insert that should fit...\n");
    char small_data[] = "X";
    struct mulle_range small_range = { .location = 1, .length = strlen(small_data) };
    
    result = mulle_vm_insert(&vm, small_range, small_data);
    if(result == 0) {
        mulle_printf("SUCCESS: Small insert worked as expected\n");
        mulle_printf("Final VM length: %zu bytes\n", (size_t)mulle_vm_get_length(&vm));
    } else {
        mulle_printf("ERROR: Small insert failed: result=%d, errno=%d\n", result, errno);
    }
    
    mulle_vm_done(&vm);
    mulle_printf("\n=== Test completed ===\n");
    
    return 0;
}
