//
//  delete-partial.c
//  mulle-vm
//
//  Test for partial page deletions - removing bytes from front and back
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
    struct mulle_vm vm;
    char page_data[] = "ABCDEFGH";  // 8 bytes for debug pagesize
    const void *read_ptr;
    struct mulle_range range;
    int result;
    
    mulle_printf("=== Testing mulle_vm_delete partial scenarios ===\n");
    
    // Initialize VM with debug config (8-byte pages)
    mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
    mulle_printf("VM initialized with debug config\n");
    
    // Test 1: Delete from front of page
    mulle_printf("\n=== Test 1: Delete 3 bytes from front ===\n");
    
    mulle_vm_append( &vm, page_data, strlen(page_data));
    
    size_t initial_length = mulle_vm_get_length(&vm);
    mulle_printf("Initial VM length: %zu bytes\n", initial_length);
    
    // Verify initial content
    char read_buffer[32];
    range = mulle_range_make(0, initial_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Initial content: \"%.*s\"\n", (int)initial_length, (char*)read_ptr);
    }
    
    // Delete first 3 bytes ("ABC")
    struct mulle_range delete_front = { .location = 0, .length = 3 };
    mulle_printf("Deleting range {%zu, %zu} from front\n", delete_front.location, delete_front.length);
    
    mulle_vm_delete(&vm, delete_front);
    
    size_t new_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after front delete: %zu bytes\n", new_length);
    
    if(new_length == initial_length - 3) {
        mulle_printf("SUCCESS: VM length decreased by 3 bytes\n");
    } else {
        mulle_printf("ERROR: Expected length %zu, got %zu\n", initial_length - 3, new_length);
        return 1;
    }
    
    // Verify content after front deletion
    range = mulle_range_make(0, new_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    char expected_after_front[] = "DEFGH";
    
    if(read_ptr) {
        mulle_printf("Content after front delete: \"%.*s\"\n", (int)new_length, (char*)read_ptr);
        
        if(memcmp(read_ptr, expected_after_front, new_length) == 0) {
            mulle_printf("SUCCESS: Content matches expected result\n");
        } else {
            mulle_printf("ERROR: Content doesn't match expected \"%s\"\n", expected_after_front);
            return 1;
        }
    } else {
        mulle_printf("ERROR: Failed to read content after front delete\n");
        return 1;
    }
    
    // Test 2: Delete from back of page
    mulle_printf("\n=== Test 2: Delete 2 bytes from back ===\n");
    
    // Current content is "DEFGH" (5 bytes)
    // Delete last 2 bytes ("GH")
    struct mulle_range delete_back = { .location = 3, .length = 2 };
    mulle_printf("Deleting range {%zu, %zu} from back\n", delete_back.location, delete_back.length);
    
    mulle_vm_delete(&vm, delete_back);
    
    size_t final_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after back delete: %zu bytes\n", final_length);
    
    if(final_length == new_length - 2) {
        mulle_printf("SUCCESS: VM length decreased by 2 bytes\n");
    } else {
        mulle_printf("ERROR: Expected length %zu, got %zu\n", new_length - 2, final_length);
        return 1;
    }
    
    // Verify final content
    range = mulle_range_make(0, final_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    char expected_final[] = "DEF";
    
    if(read_ptr) {
        mulle_printf("Final content after back delete: \"%.*s\"\n", (int)final_length, (char*)read_ptr);
        
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
    
    // Test 3: Fresh page - delete from middle
    mulle_printf("\n=== Test 3: Delete from middle of fresh page ===\n");
    
    mulle_vm_reset(&vm);
    char middle_data[] = "ABCDEFGH";
    
    mulle_vm_append( &vm, middle_data, strlen(middle_data));
    
    mulle_printf("Fresh page content: \"%s\"\n", middle_data);
    
    // Delete middle 3 bytes ("CDE") - from position 2, length 3
    struct mulle_range delete_middle = { .location = 2, .length = 3 };
    mulle_printf("Deleting range {%zu, %zu} from middle\n", delete_middle.location, delete_middle.length);
    
    mulle_vm_delete(&vm, delete_middle);
    
    size_t middle_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after middle delete: %zu bytes\n", middle_length);
    
    if(middle_length == 8 - 3) {
        mulle_printf("SUCCESS: VM length decreased by 3 bytes\n");
    } else {
        mulle_printf("ERROR: Expected length %zu, got %zu\n", 8 - 3, middle_length);
        return 1;
    }
    
    // Verify content after middle deletion - should be "AB" + "FGH" = "ABFGH"
    range = mulle_range_make(0, middle_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    char expected_middle[] = "ABFGH";
    
    if(read_ptr) {
        mulle_printf("Content after middle delete: \"%.*s\"\n", (int)middle_length, (char*)read_ptr);
        
        if(memcmp(read_ptr, expected_middle, middle_length) == 0) {
            mulle_printf("SUCCESS: Middle delete content matches expected result\n");
        } else {
            mulle_printf("ERROR: Content doesn't match expected \"%s\"\n", expected_middle);
            return 1;
        }
    } else {
        mulle_printf("ERROR: Failed to read content after middle delete\n");
        return 1;
    }
    
    mulle_vm_done(&vm);
    mulle_printf("\n=== All partial deletion tests completed successfully ===\n");
    
    return 0;
}
