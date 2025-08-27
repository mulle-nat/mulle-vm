//
//  delete-simple.c
//  mulle-vm
//
//  Simple debug test for mulle_vm_delete
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

int main(void)
{
    struct mulle_vm vm;
    char data[] = "ABCDEFGH";  // Exactly 8 bytes
    const void *read_ptr;
    struct mulle_range range;
    
    mulle_printf("=== Simple Delete Test ===\n");
    
    // Initialize VM with debug config (8-byte pages)
    mulle_vm_init(&vm, &mulle_vm_debug_config, NULL);
    
    // Add one page of data
    mulle_vm_append(&vm, data, strlen(data));
    
    size_t initial_length = mulle_vm_get_length(&vm);
    mulle_printf("Initial VM length: %zu bytes\n", initial_length);
    
    // Try to delete the entire page
    struct mulle_range delete_range = { .location = 0, .length = 8 };
    mulle_printf("Deleting range {%zu, %zu}\n", delete_range.location, delete_range.length);
    
    mulle_vm_delete(&vm, delete_range);
    
    size_t final_length = mulle_vm_get_length(&vm);
    mulle_printf("Final VM length: %zu bytes\n", final_length);
    
    if(final_length == 0) {
        mulle_printf("SUCCESS: Page deleted completely\n");
    } else {
        mulle_printf("ERROR: Page not deleted, length still %zu\n", final_length);
    }
    
    mulle_vm_done(&vm);
    return final_length == 0 ? 0 : 1;
}
