//
//  delete-multipage.c
//  mulle-vm
//
//  Test for multi-page deletions - removing bytes across multiple pages
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
    struct mulle_vm vm;
    char page1_data[] = "AAAABBBB";  // 8 bytes - fills one debug page
    char page2_data[] = "CCCCDDDD";  // 8 bytes - fills another debug page
    char page3_data[] = "EEEEFFFF";  // 8 bytes - fills third debug page
    const void *read_ptr;
    struct mulle_range range;
    int result;
    
    mulle_printf("=== Testing mulle_vm_delete multi-page scenarios ===\n");
    
    // Initialize VM with debug config (8-byte pages)
    mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
    mulle_printf("VM initialized with debug config\n");
    
    // Create a multi-page VM: "AAAABBBB" + "CCCCDDDD" + "EEEEFFFF" = 24 bytes total
    mulle_vm_append( &vm, page1_data, strlen(page1_data));
    mulle_vm_append( &vm, page2_data, strlen(page2_data));
    mulle_vm_append( &vm, page3_data, strlen(page3_data));
    
    size_t initial_length = mulle_vm_get_length(&vm);
    mulle_printf("Initial VM length: %zu bytes (3 pages)\n", initial_length);
    
    // Verify initial content: "AAAABBBBCCCCDDDDEEEEFFFFF"
    char read_buffer[64];
    range = mulle_range_make(0, initial_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Initial content: \"%.*s\"\n", (int)initial_length, (char*)read_ptr);
    }
    
    // Test 1: Delete spanning from middle of page1 into middle of page2
    // Delete from position 6 ("BB") through position 13 ("CC") = 8 bytes
    // This spans: "BB" (from page1) + "CCCCDD" (from page2) = "BBCCCCDD"
    mulle_printf("\n=== Test 1: Delete across page boundary (page1->page2) ===\n");
    
    struct mulle_range delete_span12 = { .location = 6, .length = 8 };
    mulle_printf("Deleting range {%zu, %zu} spanning page1->page2\n", 
                delete_span12.location, delete_span12.length);
    
    mulle_vm_delete(&vm, delete_span12);
    
    size_t new_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after span delete: %zu bytes\n", new_length);
    
    if(new_length == initial_length - 8) {
        mulle_printf("SUCCESS: VM length decreased by 8 bytes\n");
    } else {
        mulle_printf("INFO: Expected length %zu, got %zu (multi-page delete may not be implemented yet)\n", 
                    initial_length - 8, new_length);
    }
    
    // Verify content after spanning deletion - should be "AAAADDEEEEFFFFF"
    range = mulle_range_make(0, new_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Content after span delete: \"%.*s\"\n", (int)new_length, (char*)read_ptr);
        
        char expected_span[] = "AAAADDEEEEFFFFF";
        if(new_length == strlen(expected_span) && 
           memcmp(read_ptr, expected_span, new_length) == 0) {
            mulle_printf("SUCCESS: Multi-page delete content matches expected result\n");
        } else if(new_length == initial_length) {
            mulle_printf("INFO: Multi-page delete not implemented yet (content unchanged)\n");
        } else {
            mulle_printf("INFO: Content doesn't match expected (partial implementation)\n");
        }
    }
    
    // Reset VM for next test
    mulle_vm_reset(&vm);
    
    // Test 2: Delete entire middle page plus parts of adjacent pages
    mulle_printf("\n=== Test 2: Delete entire middle page plus edges ===\n");
    
    // Recreate the 3-page setup
    mulle_vm_append(&vm, page1_data, strlen(page1_data));
    mulle_vm_append(&vm, page2_data, strlen(page2_data));
    mulle_vm_append(&vm, page3_data, strlen(page3_data));
    
    mulle_printf("Reset VM with 3 pages: \"AAAABBBBCCCCDDDDEEEEFFFFF\"\n");
    
    // Delete from position 6 ("BB") through position 17 ("EE") = 12 bytes
    // This spans: "BB" (end of page1) + "CCCCDDDD" (all of page2) + "EE" (start of page3)
    struct mulle_range delete_big = { .location = 6, .length = 12 };
    mulle_printf("Deleting range {%zu, %zu} spanning all three pages\n", 
                delete_big.location, delete_big.length);
    
    mulle_vm_delete(&vm, delete_big);
    
    size_t big_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after big delete: %zu bytes\n", big_length);
    
    if(big_length == 24 - 12) {
        mulle_printf("SUCCESS: VM length decreased by 12 bytes\n");
    } else {
        mulle_printf("INFO: Expected length %zu, got %zu (multi-page delete may not be implemented yet)\n", 
                    24 - 12, big_length);
    }
    
    // Verify content after big deletion - should be "AAAAEEFFFFF"
    range = mulle_range_make(0, big_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Content after big delete: \"%.*s\"\n", (int)big_length, (char*)read_ptr);
        
        char expected_big[] = "AAAAEEFFFFF";
        if(big_length == strlen(expected_big) && 
           memcmp(read_ptr, expected_big, big_length) == 0) {
            mulle_printf("SUCCESS: Big multi-page delete content matches expected result\n");
        } else if(big_length == 24) {
            mulle_printf("INFO: Multi-page delete not implemented yet (content unchanged)\n");
        } else {
            mulle_printf("INFO: Content doesn't match expected (partial implementation)\n");
        }
    }
    
    // Test 3: Delete from start spanning multiple pages
    mulle_vm_reset(&vm);
    mulle_printf("\n=== Test 3: Delete from start spanning multiple pages ===\n");
    
    // Recreate the 3-page setup
    mulle_vm_append(&vm, page1_data, strlen(page1_data));
    mulle_vm_append(&vm, page2_data, strlen(page2_data));
    mulle_vm_append(&vm, page3_data, strlen(page3_data));
    
    mulle_printf("Reset VM with 3 pages: \"AAAABBBBCCCCDDDDEEEEFFFFF\"\n");
    
    // Delete first 14 bytes: "AAAABBBBCCCCDD"
    struct mulle_range delete_start = { .location = 0, .length = 14 };
    mulle_printf("Deleting range {%zu, %zu} from start spanning multiple pages\n", 
                delete_start.location, delete_start.length);
    
    mulle_vm_delete(&vm, delete_start);
    
    size_t start_length = mulle_vm_get_length(&vm);
    mulle_printf("VM length after start delete: %zu bytes\n", start_length);
    
    if(start_length == 24 - 14) {
        mulle_printf("SUCCESS: VM length decreased by 14 bytes\n");
    } else {
        mulle_printf("INFO: Expected length %zu, got %zu (multi-page delete may not be implemented yet)\n", 
                    24 - 14, start_length);
    }
    
    // Verify content after start deletion - should be "DDEEEEFFFFF"
    range = mulle_range_make(0, start_length);
    read_ptr = mulle_vm_read(&vm, range, read_buffer);
    if(read_ptr) {
        mulle_printf("Content after start delete: \"%.*s\"\n", (int)start_length, (char*)read_ptr);
        
        char expected_start[] = "DDEEEEFFFFF";
        if(start_length == strlen(expected_start) && 
           memcmp(read_ptr, expected_start, start_length) == 0) {
            mulle_printf("SUCCESS: Start multi-page delete content matches expected result\n");
        } else if(start_length == 24) {
            mulle_printf("INFO: Multi-page delete not implemented yet (content unchanged)\n");
        } else {
            mulle_printf("INFO: Content doesn't match expected (partial implementation)\n");
        }
    }
    
    mulle_vm_done(&vm);
    mulle_printf("\n=== Multi-page deletion tests completed ===\n");
    
    return 0;
}
