//
//  delete-complete-page.c
//  mulle-vm
//
//  Test for deleting a complete page - the simplest delete case
//  This tests the scenario where we delete a range that exactly matches
//  a complete page, allowing the page to be removed entirely.
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
    struct mulle_vm vm;
    char page1_data[] = "ABCDEFGH";  // Exactly 8 bytes for debug pagesize
    char page2_data[] = "12345678";  // Exactly 8 bytes for debug pagesize
    char page3_data[] = "WXYZ";      // 4 bytes
    const void *read_ptr;
    struct mulle_range range;
    int result;
    
    mulle_printf("=== Testing mulle_vm_delete complete page scenario ===\n");
    
    // Initialize VM with debug config (8-byte pages)
    mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
    
    mulle_printf("VM initialized with pagesize=%zu, skew=%zu\n", 
           mulle_vm_debug_config.pagesize, mulle_vm_debug_config.skew);
    
    // Step 1: Create three pages
    mulle_printf("\nStep 1: Creating three pages...\n");
    
    mulle_vm_append( &vm, initial_data, initial_len);
    if(result != 0) {
        mulle_printf("ERROR: Failed to append page1 data: %d\n", result);
        return 1;
    }
    
    mulle_vm_append( &vm, initial_data, initial_len);
    if(result != 0) {
        mulle_printf("ERROR: Failed to append page2 data: %d\n", result);
        return 1;
    }
    
    mulle_vm_append( &vm, initial_data, initial_len);
    if(result != 0) {
        mulle_printf("ERROR: Failed to append page3 data: %d\n", result);
        return 1;
    }
    
    size_t initial_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length with three pages: %zu bytes\n", initial_length);
    
    // Verify initial content
    char expected_initial[] = "ABCDEFGH12345678WXYZ";
    range = mulle_range_make(0, initial_length);
    char read_buffer[32];
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Initial content: \"%.*s\"\n", (int)initial_length, (char*)read_ptr);
    }
    
    // Step 2: Delete middle page (page2) completely
    // This should delete exactly 8 bytes starting at offset 8
    mulle_printf("\nStep 2: Deleting complete middle page (8 bytes at offset 8)...\n");
    struct mulle_range delete_range = { .location = 8, .length = 8 };
    
    mulle_vm_delete(&vm, delete_range);
    
    size_t new_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after delete: %zu bytes\n", new_length);
    
    if(new_length == initial_length - 8) {
        mulle_printf("SUCCESS: VM length decreased by exactly 8 bytes\n");
    } else {
        mulle_printf("ERROR: Expected length %zu, got %zu\n", initial_length - 8, new_length);
        return 1;
    }
    
    // Step 3: Verify content after deletion
    mulle_printf("\nStep 3: Verifying content after deletion...\n");
    char expected_after[] = "ABCDEFGHWXYZ";
    range = mulle_range_make(0, new_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    
    if(read_ptr) {
        mulle_printf("Content after delete: \"%.*s\"\n", (int)new_length, (char*)read_ptr);
        
        if(memcmp(read_ptr, expected_after, new_length) == 0) {
            mulle_printf("SUCCESS: Content matches expected result\n");
        } else {
            mulle_printf("ERROR: Content doesn't match expected \"%s\"\n", expected_after);
            return 1;
        }
    } else {
        mulle_printf("ERROR: Failed to read content after delete\n");
        return 1;
    }
    
    // Step 4: Delete first page completely
    mulle_printf("\nStep 4: Deleting first page completely (8 bytes at offset 0)...\n");
    struct mulle_range delete_first = { .location = 0, .length = 8 };
    
    mulle_vm_delete(&vm, delete_first);
    
    size_t final_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after deleting first page: %zu bytes\n", final_length);
    
    // Step 5: Verify final content
    mulle_printf("\nStep 5: Verifying final content...\n");
    char expected_final[] = "WXYZ";
    range = mulle_range_make(0, final_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    
    if(read_ptr) {
        mulle_printf("Final content: \"%.*s\"\n", (int)final_length, (char*)read_ptr);
        
        if(memcmp(read_ptr, expected_final, final_length) == 0) {
            mulle_printf("SUCCESS: Final content matches expected result\n");
        } else {
            mulle_printf("ERROR: Final content doesn't match expected \"%s\"\n", expected_final);
            return 1;
        }
    } else {
        mulle_printf("ERROR: Failed to read final content\n");
        return 1;
    }
    
    mulle_vm_done(&vm);
    mulle_printf("\n=== Test completed successfully ===\n");
    
    return 0;
}
