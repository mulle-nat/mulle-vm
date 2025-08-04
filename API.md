# mulle-vm API

This document outlines the plan for documenting the mulle-vm library functions.

## Documentation Process

1. Use mulle-sde symbol list to figure out the public API (if available)
2. Create a todo list of functions to document
3. Pick a function from the todo list
4. Look at the function source code
5. Create doc header for this function in the respective header file
6. Mark this function on the todo list
7. Proceed until done

## Public API Functions to Document

### Version Functions
- [x] `mulle_vm_get_version` - Get the full version as a single integer
- [x] `mulle_vm_get_version_major` - Get the major version number
- [x] `mulle_vm_get_version_minor` - Get the minor version number
- [x] `mulle_vm_get_version_patch` - Get the patch version number

### Configuration Structures
- [x] `mulle_vm_config` structure
- [x] `mulle_vm_config_default` global variable
- [x] `mulle_vm_mapping` structure
- [x] `mulle_vm` structure

### Core VM Functions
- [x] `mulle_vm_init` - Initialize a VM instance
- [x] `mulle_vm_done` - Clean up a VM instance
- [x] `mulle_vm_reset` - Reset a VM instance
- [x] `mulle_vm_get_pagesize` - Get the page size of a VM

### Walking Functions
- [x] `mulle_vm_walk_callback_t` callback type
- [x] `mulle_vm_walk` - Walk through VM memory range
- [x] `mulle_vm_walk_reverse` - Walk through VM memory range in reverse

### I/O Functions
- [x] `mulle_vm_read` - Read data from VM
- [x] `mulle_vm_read_copy` - Read data from VM with copying
- [x] `mulle_vm_write` - Write data to VM

### Memory Management Functions
- [x] `mulle_vm_insert` - Insert data into VM
- [x] `mulle_vm_append` - Append data to VM
- [x] `mulle_vm_delete` - Delete data from VM
- [x] `mulle_vm_copy` - Copy data between VMs
- [x] `mulle_vm_cut` - Cut data from one VM to another
- [x] `mulle_vm_paste` - Paste data from one VM to another

### Statistics Functions
- [x] `mulle_vm_statistics` structure
- [x] `mulle_vm_get_statistics` - Get VM statistics

### Enumeration Functions
- [x] `mulle_vm_mappingenumerator` structure
- [x] `mulle_vm_mapping_enumerate` - Create enumerator for VM mappings
- [x] `mulle_vm_mappingenumerator_next` - Get next mapping from enumerator
- [x] `mulle_vm_mappingenumerator_done` - Clean up enumerator
- [x] `mulle_vm_mapping_for` macro - For loop for VM mappings
- [x] `mulle_vm_mappingreverseenumerator` structure
- [x] `mulle_vm_mapping_reverseenumerate` - Create reverse enumerator for VM mappings
- [x] `mulle_vm_mappingreverseenumerator_next` - Get next mapping from reverse enumerator
- [x] `mulle_vm_mappingreverseenumerator_done` - Clean up reverse enumerator
- [x] `mulle_vm_mapping_for_reverse` macro - Reverse for loop for VM mappings

### Page Functions (from mulle-vm-page.h)
- [x] `mulle_vm_page` structure
- [x] `mulle_vm_page_init` - Initialize a page
- [x] `mulle_vm_page_address` - Get address of page data
- [x] `mulle_vm_page_get_length` - Get length of page data
- [x] `mulle_vm_page_get_data` - Get data from page
- [x] `mulle_vm_page_get_available` - Get available space in page
- [x] `mulle_vm_page_get_available_front` - Get available space at front of page
- [x] `mulle_vm_page_get_available_back` - Get available space at back of page
- [x] `mulle_vm_page_alloc` - Allocate a page
- [x] `mulle_vm_page_free` - Free a page
- [x] `mulle_vm_page_insert_bytes` - Insert bytes into page
- [x] `mulle_vm_page_join` - Join two pages

### Standard Library Functions (from mulle-vm-stdlib.h)
- [x] `mulle_vm_memchr` - Find character in VM memory
- [x] `mulle_vm_memrchr` - Find character in VM memory (reverse)
- [x] `mulle_vm_memmatch` - Find matching characters in VM memory
- [x] `mulle_vm_memrmatch` - Find matching characters in VM memory (reverse)
- [x] `mulle_vm_memspn` - Find span of matching characters in VM memory
- [x] `mulle_vm_memrspn` - Find span of matching characters in VM memory (reverse)

## Detailed Documentation for mulle_vm_get_version

### Function
```c
MULLE__VM_GLOBAL
uint32_t mulle_vm_get_version(void);
```

### Description
Returns the full version of the mulle-vm library as a single 32-bit integer. The version is encoded in the format `(major << 20) | (minor << 8) | patch`.

### Return Value
The full version as a 32-bit integer where:
- Bits 31-20: Major version
- Bits 19-8: Minor version
- Bits 7-0: Patch version

### Example
```c
#include "mulle-vm.h"

uint32_t version = mulle_vm_get_version();
unsigned int major = version >> 20;
unsigned int minor = (version >> 8) & 0xFFF;
unsigned int patch = version & 0xFF;

printf("mulle-vm version: %u.%u.%u\n", major, minor, patch);
```

### See Also
- `mulle_vm_get_version_major()`
- `mulle_vm_get_version_minor()`
- `mulle_vm_get_version_patch()`

## Detailed Documentation for mulle_vm_get_version_major

### Function
```c
static inline unsigned int mulle_vm_get_version_major(void);
```

### Description
Returns the major version number of the mulle-vm library. This is a static inline function that extracts the major version from the `MULLE__VM_VERSION` constant.

### Return Value
The major version number as an unsigned integer.

### Example
```c
#include "mulle-vm.h"

unsigned int major = mulle_vm_get_version_major();
printf("Major version: %u\n", major);
```

### See Also
- `mulle_vm_get_version()`
- `mulle_vm_get_version_minor()`

## Detailed Documentation for Walking Functions

### Callback Type: mulle_vm_walk_callback_t

```c
typedef int mulle_vm_walk_callback_t(struct mulle_data data,
                                  struct mulle_range vm_range,
                                  void *userinfo);
```

#### Description
Callback function type for walking through VM memory ranges. This callback is invoked for each contiguous memory range encountered during a walk operation. The callback receives the actual data content and the corresponding virtual memory range.

#### Parameters
- **data**: The actual data content as a mulle_data structure
- **vm_range**: The virtual memory range corresponding to this data
- **userinfo**: User-provided context pointer passed to the walk function

#### Return Value
Return 0 to continue walking, any other value stops the walk. A negative value indicates an error condition.

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

static int walk_callback(struct mulle_data data,
                     struct mulle_range range,
                     void *context)
{
 printf("Range [%zu, %zu): %.*s\n",
        range.location, range.location + range.length,
        (int)data.length, (char *)data.bytes);
 return 0;  // continue walking
}
```

### Function: mulle_vm_walk

```c
int mulle_vm_walk(struct mulle_vm *vm,
               struct mulle_range range,
               mulle_vm_walk_callback_t *callback,
               void *userinfo);
```

#### Description
Walk through VM memory range in forward direction. This function iterates through the specified memory range in the VM, invoking the provided callback for each contiguous data segment found. The walk proceeds from lower to higher memory addresses.

#### Parameters
- **vm**: Pointer to the VM structure to walk through
- **range**: The memory range to walk through
- **callback**: Function to call for each data segment encountered
- **userinfo**: User-provided context pointer passed to the callback

#### Return Value
Returns the last return value from the callback, or an error code if the walk fails before any callback is invoked.

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

static int print_callback(struct mulle_data data,
                      struct mulle_range range,
                      void *unused)
{
 printf("Data at [%zu, %zu): %.*s\n",
        range.location, range.location + range.length,
        (int)data.length, (char *)data.bytes);
 return 0;
}

int main(void)
{
 struct mulle_vm vm;
 mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
 
 mulle_vm_append(&vm, "Hello", 5);
 mulle_vm_append(&vm, " World", 6);
 
 struct mulle_vm_statistics stats = mulle_vm_get_statistics(&vm);
 mulle_vm_walk(&vm, mulle_range_make(0, stats.vm_size), print_callback, NULL);
 
 mulle_vm_done(&vm);
 return 0;
}
```

### Function: mulle_vm_walk_reverse

```c
int mulle_vm_walk_reverse(struct mulle_vm *vm,
                       struct mulle_range range,
                       mulle_vm_walk_callback_t *callback,
                       void *userinfo);
```

#### Description
Walk through VM memory range in reverse direction. This function iterates through the specified memory range in the VM, invoking the provided callback for each contiguous data segment found. The walk proceeds from higher to lower memory addresses (reverse order).

#### Parameters
- **vm**: Pointer to the VM structure to walk through
- **range**: The memory range to walk through
- **callback**: Function to call for each data segment encountered
- **userinfo**: User-provided context pointer passed to the callback

#### Return Value
Returns the last return value from the callback, or an error code if the walk fails before any callback is invoked.

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

static int reverse_print_callback(struct mulle_data data,
                              struct mulle_range range,
                              void *unused)
{
 printf("Reverse: [%zu, %zu): %.*s\n",
        range.location, range.location + range.length,
        (int)data.length, (char *)data.bytes);
 return 0;
}

int main(void)
{
 struct mulle_vm vm;
 mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
 
 mulle_vm_append(&vm, "First", 5);
 mulle_vm_append(&vm, "Second", 6);
 mulle_vm_append(&vm, "Third", 5);
 
 struct mulle_vm_statistics stats = mulle_vm_get_statistics(&vm);
 mulle_vm_walk_reverse(&vm, mulle_range_make(0, stats.vm_size),
                      reverse_print_callback, NULL);
 
 mulle_vm_done(&vm);
 return 0;
}
```
- `mulle_vm_get_version_patch()`

## Detailed Documentation for mulle_vm_get_version_minor

### Function
```c
static inline unsigned int mulle_vm_get_version_minor(void);
```

### Description
Returns the minor version number of the mulle-vm library. This is a static inline function that extracts the minor version from the `MULLE__VM_VERSION` constant.

### Return Value
The minor version number as an unsigned integer.

### Example
```c
#include "mulle-vm.h"

unsigned int minor = mulle_vm_get_version_minor();
printf("Minor version: %u\n", minor);
```

### See Also
- `mulle_vm_get_version()`
- `mulle_vm_get_version_major()`
- `mulle_vm_get_version_patch()`

## Detailed Documentation for mulle_vm_get_version_patch

### Function
```c
static inline unsigned int mulle_vm_get_version_patch(void);
```

### Description
Returns the patch version number of the mulle-vm library. This is a static inline function that extracts the patch version from the `MULLE__VM_VERSION` constant.

### Return Value
The patch version number as an unsigned integer.

### Example
```c
#include "mulle-vm.h"

unsigned int patch = mulle_vm_get_version_patch();
printf("Patch version: %u\n", patch);
```

### See Also
- `mulle_vm_get_version()`
- `mulle_vm_get_version_major()`
## Detailed Documentation for Configuration Structures

### Structure: mulle_vm_config

```c
struct mulle_vm_config
{
   void     *(*alloc_pages)( size_t size);
   int      (*free_pages)( void *p, size_t size);
   void     (*shutdown)( struct mulle_vm *vm);
   size_t   pagesize;
   size_t   skew;
   size_t   cachesize;
};
```

#### Description
Configuration structure for initializing a mulle-vm instance. This structure allows customization of memory allocation behavior, page management, and other VM parameters.

#### Members
- **alloc_pages**: Memory allocation function for pages. If NULL, uses system malloc.
- **free_pages**: Memory deallocation function for pages. If NULL, uses system free.
- **shutdown**: Optional shutdown callback called during mulle_vm_done().
- **pagesize**: Size of each page in bytes. Default is MULLE__VM_PAGESIZE_DEFAULT (0x4000).
- **skew**: Skew parameter for address calculation. Default is MULLE__VM_SKEW_DEFAULT (25).
- **cachesize**: Cache size parameter (currently unused).

#### Example
```c
#include "mulle-vm.h"

struct mulle_vm_config config = {
   .alloc_pages = NULL,  // Use system malloc
   .free_pages = NULL,   // Use system free
   .shutdown = NULL,     // No custom shutdown
   .pagesize = 4096,     // 4KB pages
   .skew = 25,           // Default skew
   .cachesize = 0        // Unused
};
```

### Global Variable: mulle_vm_config_default

```c
struct mulle_vm_config mulle_vm_config_default;
```

#### Description
Default configuration for mulle-vm instances. Contains sensible defaults for most use cases:
- pagesize: MULLE__VM_PAGESIZE_DEFAULT (0x4000)
- skew: MULLE__VM_SKEW_DEFAULT (25)
- alloc_pages/free_pages: NULL (uses system malloc/free)
- shutdown: NULL (no custom shutdown)
- cachesize: 0

#### Example
```c
#include "mulle-vm.h"

struct mulle_vm vm;
mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
```

### Structure: mulle_vm_mapping

```c
struct mulle_vm_mapping
{
   struct mulle_vm_page   *page;
   uintptr_t              address;
};
```

#### Description
Mapping structure that associates a virtual memory address with a page. This structure represents the mapping between a virtual address space and the actual page that contains the data. Mappings are local to a specific VM instance, but the underlying pages can be shared between multiple VMs.

#### Members
- **page**: Pointer to the actual page containing the data
- **address**: Virtual address in the VM's address space

### Structure: mulle_vm

```c
struct mulle_vm
{
   struct mulle_structarray    mappings;           // struct mulle_vm_mapping
   struct mulle_pointerarray   freed_ospages;       // freed_ospages of free os pages (of pagesize each)
   struct mulle_vm_config      config;
};
```

#### Description
Main virtual memory structure representing a VM instance. Contains the mappings between virtual addresses and physical pages, a pool of freed operating system pages for reuse, and the configuration parameters.

#### Members
- **mappings**: Array of mulle_vm_mapping structures mapping virtual addresses to pages
- **freed_ospages**: Array of pointers to freed operating system pages for reuse
- **config**: Configuration parameters for this VM instance
## Detailed Documentation for Core VM Functions

### Function: mulle_vm_init

```c
void mulle_vm_init(struct mulle_vm *vm,
                   struct mulle_vm_config *config,
                   struct mulle_allocator *allocator);
```

#### Description
Initialize a VM instance with the given configuration. This function initializes a mulle_vm structure with the provided configuration. If config is NULL, the default configuration (mulle_vm_config_default) is used. The VM will be ready for use after initialization.

#### Parameters
- **vm**: Pointer to the VM structure to initialize
- **config**: Configuration parameters, or NULL for default configuration
- **allocator**: Memory allocator to use, or NULL for system allocator

#### Example
```c
#include "mulle-vm.h"

struct mulle_vm vm;
mulle_vm_init(&vm, &mulle_vm_config_default, NULL);

// VM is now ready for use
```

### Function: mulle_vm_done

```c
void mulle_vm_done(struct mulle_vm *vm);
```

#### Description
Clean up a VM instance and release all resources. This function releases all resources associated with a VM instance, including all pages, mappings, and the optional shutdown callback is called if provided in the configuration.

#### Parameters
- **vm**: Pointer to the VM structure to clean up

#### Example
```c
#include "mulle-vm.h"

struct mulle_vm vm;
mulle_vm_init(&vm, &mulle_vm_config_default, NULL);

// Use the VM...

mulle_vm_done(&vm);  // Clean up all resources
```

### Function: mulle_vm_reset

```c
void mulle_vm_reset(struct mulle_vm *vm);
```

#### Description
Reset a VM instance to its initial state. This function frees all pages and mappings, effectively clearing all data from the VM, but keeps the freed_ospages pool alive for reuse. The VM remains initialized and ready for new data.

#### Parameters
- **vm**: Pointer to the VM structure to reset

#### Example
```c
#include "mulle-vm.h"

struct mulle_vm vm;
mulle_vm_init(&vm, &mulle_vm_config_default, NULL);

// Add some data...
mulle_vm_append(&vm, "Hello", 5);

// Reset to clear all data
mulle_vm_reset(&vm);

// VM is now empty but still initialized
```

### Function: mulle_vm_get_pagesize

```c
static inline size_t mulle_vm_get_pagesize(struct mulle_vm *vm);
```

#### Description
Get the page size of a VM instance. Returns the configured page size for the given VM. If vm is NULL, returns the default page size (MULLE__VM_PAGESIZE_DEFAULT).

#### Parameters
- **vm**: Pointer to the VM structure, or NULL for default

#### Return Value
The page size in bytes

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

struct mulle_vm vm;
mulle_vm_init(&vm, &mulle_vm_config_default, NULL);

size_t pagesize = mulle_vm_get_pagesize(&vm);
printf("Page size: %zu bytes\n", pagesize);

mulle_vm_done(&vm);
```
## Detailed Documentation for I/O Functions

### Function: mulle_vm_read

```c
static inline const void *mulle_vm_read(struct mulle_vm *vm, 
                                       struct mulle_range range, 
                                       void *buffer);
```

#### Description
Read data from a virtual memory range. This function reads data from the specified range in the VM. If the requested range fits entirely within a single page, it returns a direct pointer to the data without copying. If the range spans multiple pages and a buffer is provided, it copies the data into the buffer. If the range spans multiple pages and no buffer is provided, it returns NULL.

#### Parameters
- **vm**: Pointer to the VM structure to read from
- **range**: The memory range to read from (location and length)
- **buffer**: Optional buffer to copy data into if range spans multiple pages. Can be NULL.

#### Return Value
- **Direct pointer**: If the range fits within a single page, returns a const pointer to the data
- **Buffer pointer**: If the range spans multiple pages and buffer is provided, returns the buffer pointer with data copied
- **NULL**: If the range is invalid, out of bounds, spans multiple pages without buffer, or if vm/range.length is 0

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char buffer[1024];
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "Hello, World!", 13);
    
    // Read from single page (direct pointer)
    struct mulle_range range = mulle_range_make(0, 5);
    const void *data = mulle_vm_read(&vm, range, NULL);
    if (data) {
        printf("Direct read: %.*s\n", 5, (char*)data);
    }
    
    // Read across pages (using buffer)
    range = mulle_range_make(0, 13);
    data = mulle_vm_read(&vm, range, buffer);
    if (data) {
        printf("Buffered read: %.*s\n", 13, (char*)data);
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- The returned pointer from single-page reads is valid only as long as the VM structure remains unchanged
- For multi-page reads, the buffer must be large enough to hold the requested data
- The function handles copy-on-write semantics internally when pages are shared
- `mulle_vm_get_version_minor()`
### Function: mulle_vm_read_copy

```c
static inline void *mulle_vm_read_copy(struct mulle_vm *vm,
                                      struct mulle_range range,
                                      void *buffer);
```

#### Description
Read data from a virtual memory range with guaranteed copying. This function always copies the requested data into the provided buffer, regardless of whether the range fits within a single page or spans multiple pages. This is useful when you need to modify the read data without affecting the original VM contents.

#### Parameters
- **vm**: Pointer to the VM structure to read from
- **range**: The memory range to read from (location and length)
- **buffer**: Buffer to copy data into. Must be large enough to hold the requested data.

#### Return Value
- **Buffer pointer**: Always returns the provided buffer pointer with data copied
- **NULL**: If vm, buffer, or range.length is 0, or if the range is invalid/out of bounds

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char buffer[1024];
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "Hello, World!", 13);
    
    // Always copy into buffer
    struct mulle_range range = mulle_range_make(0, 13);
    void *data = mulle_vm_read_copy(&vm, range, buffer);
    if (data) {
        printf("Copied data: %.*s\n", 13, (char*)data);
        
        // Safe to modify buffer without affecting VM
        ((char*)data)[0] = 'h';
        printf("Modified buffer: %.*s\n", 13, (char*)data);
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- Always copies data, even for single-page reads
- The buffer must be large enough to hold the requested data
- Safe for concurrent access scenarios where you need to ensure data isolation
- More predictable behavior than mulle_vm_read for buffer-based processing
### Function: mulle_vm_write

```c
static inline void *mulle_vm_write(struct mulle_vm *vm,
                                  struct mulle_range range,
                                  void *buffer);
```

#### Description
Write data to a virtual memory range. This function writes the provided data into the specified range in the VM. The function handles both single-page and multi-page writes, automatically managing page boundaries and copy-on-write semantics.

#### Parameters
- **vm**: Pointer to the VM structure to write to
- **range**: The memory range to write to (location and length)
- **buffer**: Buffer containing the data to write. Must not be NULL.

#### Return Value
- **Buffer pointer**: Returns the provided buffer pointer on successful write
- **NULL**: If vm, buffer, or range.length is 0, or if the range is invalid/out of bounds

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char write_data[] = "Hello, World!";
    char read_buffer[1024];
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Write data at specific location
    struct mulle_range range = mulle_range_make(0, 13);
    void *result = mulle_vm_write(&vm, range, write_data);
    if (result) {
        printf("Write successful\n");
        
        // Verify the write
        const void *read_data = mulle_vm_read(&vm, range, read_buffer);
        if (read_data) {
            printf("Read back: %.*s\n", 13, (char*)read_data);
        }
    }
    
    // Overwrite part of the data
    char new_data[] = "Hi";
    range = mulle_range_make(0, 2);
    result = mulle_vm_write(&vm, range, new_data);
    if (result) {
        printf("Overwrite successful\n");
        
        // Read the modified data
        range = mulle_range_make(0, 13);
        const void *read_data = mulle_vm_read(&vm, range, read_buffer);
        if (read_data) {
            printf("Modified data: %.*s\n", 13, (char*)read_data);
        }
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- Handles both single-page and multi-page writes transparently
- Manages copy-on-write semantics for shared pages
- Automatically handles page boundaries and memory allocation
- The range must be within the current VM bounds - use mulle_vm_insert to extend the VM
## Detailed Documentation for Memory Management Functions

### Function: mulle_vm_insert

```c
MULLE__VM_GLOBAL
int mulle_vm_insert(struct mulle_vm *vm,
                   struct mulle_range range,
                   void *bytes);
```

#### Description
Insert data into the VM at a specific location. This function inserts the provided data bytes into the VM at the specified range location. The function handles page management, splitting existing pages if necessary, and adjusting virtual addresses of subsequent pages. This operation extends the VM size by the length of the inserted data.

#### Parameters
- **vm**: Pointer to the VM structure to insert into
- **range**: The location where data should be inserted (location specifies the byte offset, length specifies the number of bytes to insert)
- **bytes**: Pointer to the data bytes to insert. Must not be NULL.

#### Return Value
- **0**: Success - data was inserted successfully
- **EINVAL**: Invalid parameters (vm is NULL, range.length is 0, bytes is NULL, or range.location is invalid)
- **ENOMEM**: Memory allocation failed

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char data1[] = "Hello";
    char data2[] = "Beautiful ";
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add initial data
    mulle_vm_append(&vm, data1, 5);
    
    // Insert data in the middle
    struct mulle_range insert_range = mulle_range_make(5, 10);
    int result = mulle_vm_insert(&vm, insert_range, data2);
    if (result == 0) {
        printf("Insert successful\n");
        
        // Read the result
        char buffer[64];
        struct mulle_range read_range = mulle_range_make(0, 15);
        const void *data = mulle_vm_read(&vm, read_range, buffer);
        if (data) {
            printf("Result: %.*s\n", 15, (char*)data);
            // Output: "HelloBeautiful "
        }
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- The range.location must be within the current VM bounds (0 <= location <= current_length)
- The operation increases the VM size by range.length bytes
- Existing data at and after the insertion point is shifted forward
- Handles page splitting and merging automatically
- May trigger copy-on-write for shared pages
### Function: mulle_vm_append

```c
MULLE__VM_GLOBAL
int mulle_vm_append(struct mulle_vm *vm,
                   void *bytes,
                   size_t length);
```

#### Description
Append data to the end of the VM. This is a convenience function that appends the provided data to the end of the current VM contents. It calculates the appropriate range location automatically and delegates to mulle_vm_insert.

#### Parameters
- **vm**: Pointer to the VM structure to append to
- **bytes**: Pointer to the data bytes to append. Must not be NULL.
- **length**: Number of bytes to append

#### Return Value
- **0**: Success - data was appended successfully
- **EINVAL**: Invalid parameters (vm is NULL, bytes is NULL, or length is 0)
- **ENOMEM**: Memory allocation failed

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char part1[] = "Hello";
    char part2[] = ", ";
    char part3[] = "World!";
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Append data in multiple calls
    int result = mulle_vm_append(&vm, part1, 5);
    if (result == 0) printf("First append successful\n");
    
    result = mulle_vm_append(&vm, part2, 2);
    if (result == 0) printf("Second append successful\n");
    
    result = mulle_vm_append(&vm, part3, 6);
    if (result == 0) printf("Third append successful\n");
    
    // Read the complete data
    char buffer[64];
    struct mulle_vm_statistics stats = mulle_vm_get_statistics(&vm);
    struct mulle_range range = mulle_range_make(0, stats.vm_size);
    const void *data = mulle_vm_read(&vm, range, buffer);
    if (data) {
        printf("Complete data: %.*s\n", (int)stats.vm_size, (char*)data);
        // Output: "Hello, World!"
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- This is a convenience wrapper around mulle_vm_insert
- Automatically calculates the insertion location as the current VM length
- More efficient than manually calculating ranges for sequential appends
- Commonly used for building VM content incrementally
### Function: mulle_vm_delete

```c
MULLE__VM_GLOBAL
void mulle_vm_delete(struct mulle_vm *vm,
                    struct mulle_range range);
```

#### Description
Delete data from the VM. This function removes the specified range of data from the VM, potentially removing entire pages or modifying existing pages. The function handles page cleanup, merging adjacent pages when possible, and adjusting virtual addresses of subsequent pages. This operation decreases the VM size by the length of the deleted range.

#### Parameters
- **vm**: Pointer to the VM structure to delete from
- **range**: The memory range to delete (location and length)

#### Return Value
None (void function)

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm vm;
    char data[] = "Hello, Beautiful World!";
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add initial data
    mulle_vm_append(&vm, data, strlen(data));
    
    // Delete "Beautiful " from the middle
    struct mulle_range delete_range = mulle_range_make(7, 10);
    mulle_vm_delete(&vm, delete_range);
    
    // Read the result
    char buffer[64];
    struct mulle_vm_statistics stats = mulle_vm_get_statistics(&vm);
    struct mulle_range read_range = mulle_range_make(0, stats.vm_size);
    const void *result = mulle_vm_read(&vm, read_range, buffer);
    if (result) {
        printf("After deletion: %.*s\n", (int)stats.vm_size, (char*)result);
        // Output: "Hello, World!"
    }
    
    // Delete everything
    mulle_vm_delete(&vm, mulle_range_make_all());
    stats = mulle_vm_get_statistics(&vm);
    printf("VM size after delete all: %zu\n", stats.vm_size);
    // Output: 0
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- The range must be within the current VM bounds
- The operation decreases the VM size by range.length bytes
- Handles page splitting and merging automatically
- May trigger copy-on-write for shared pages
- Empty ranges (length 0) are silently ignored
- Invalid ranges are silently ignored
### Function: mulle_vm_copy

```c
MULLE__VM_GLOBAL
int mulle_vm_copy(struct mulle_vm *dst,
                 struct mulle_vm *src,
                 struct mulle_range range);
```

#### Description
Copy data from one VM to another. This function copies the specified range of data from the source VM to the destination VM. The destination VM is reset (cleared) before the copy operation. Pages are shared between VMs when possible for memory efficiency.

#### Parameters
- **dst**: Pointer to the destination VM structure. Must be different from src.
- **src**: Pointer to the source VM structure. Must be different from dst.
- **range**: The memory range to copy from the source VM

#### Return Value
- **0**: Success - data was copied successfully
- **-1**: Error - dst and src are the same VM, or range is invalid

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm src, dst;
    char data[] = "Hello, World! This is a test.";
    
    mulle_vm_init(&src, &mulle_vm_config_default, NULL);
    mulle_vm_init(&dst, &mulle_vm_config_default, NULL);
    
    // Add data to source
    mulle_vm_append(&src, data, strlen(data));
    
    // Copy first part to destination
    struct mulle_range copy_range = mulle_range_make(0, 13);
    int result = mulle_vm_copy(&dst, &src, copy_range);
    if (result == 0) {
        printf("Copy successful\n");
        
        // Verify the copy
        char buffer[64];
        struct mulle_vm_statistics stats = mulle_vm_get_statistics(&dst);
        struct mulle_range read_range = mulle_range_make(0, stats.vm_size);
        const void *copied_data = mulle_vm_read(&dst, read_range, buffer);
        if (copied_data) {
            printf("Copied data: %.*s\n", (int)stats.vm_size, (char*)copied_data);
            // Output: "Hello, World!"
        }
    }
    
    // Copy another range
    copy_range = mulle_range_make(13, strlen(data) - 13);
    result = mulle_vm_copy(&dst, &src, copy_range);
    if (result == 0) {
        char buffer[64];
        struct mulle_vm_statistics stats = mulle_vm_get_statistics(&dst);
        struct mulle_range read_range = mulle_range_make(0, stats.vm_size);
        const void *copied_data = mulle_vm_read(&dst, read_range, buffer);
        if (copied_data) {
            printf("Second copy: %.*s\n", (int)stats.vm_size, (char*)copied_data);
            // Output: " This is a test."
        }
    }
    
    mulle_vm_done(&src);
    mulle_vm_done(&dst);
    return 0;
}
```

#### Notes
- The destination VM is always reset (cleared) before copying
- Pages are shared between VMs when possible for memory efficiency
- The source VM remains unchanged
- dst and src must be different VM instances
- Handles partial page copies by creating new pages as needed
### Function: mulle_vm_cut

```c
int mulle_vm_cut(struct mulle_vm *dst,
                struct mulle_vm *src,
                struct mulle_range range);
```

#### Description
Cut data from one VM to another. This function combines mulle_vm_copy and mulle_vm_delete operations in sequence. It copies the specified range from the source VM to the destination VM, then deletes that range from the source VM. This is equivalent to a "cut and paste" operation.

#### Parameters
- **dst**: Pointer to the destination VM structure. Must be different from src.
- **src**: Pointer to the source VM structure. Must be different from dst.
- **range**: The memory range to cut from the source VM

#### Return Value
- **0**: Success - data was cut successfully
- **-1**: Error - dst and src are the same VM, or range is invalid
- Other error codes from underlying mulle_vm_copy or mulle_vm_delete operations

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm src, dst;
    char data[] = "Hello, World! This is a test.";
    
    mulle_vm_init(&src, &mulle_vm_config_default, NULL);
    mulle_vm_init(&dst, &mulle_vm_config_default, NULL);
    
    // Add data to source
    mulle_vm_append(&src, data, strlen(data));
    
    // Cut "World!" from source to destination
    struct mulle_range cut_range = mulle_range_make(7, 6);
    int result = mulle_vm_cut(&dst, &src, cut_range);
    if (result == 0) {
        printf("Cut operation successful\n");
        
        // Verify source after cut
        char buffer[64];
        struct mulle_vm_statistics stats = mulle_vm_get_statistics(&src);
        struct mulle_range read_range = mulle_range_make(0, stats.vm_size);
        const void *src_data = mulle_vm_read(&src, read_range, buffer);
        if (src_data) {
            printf("Source after cut: %.*s\n", (int)stats.vm_size, (char*)src_data);
            // Output: "Hello,  This is a test."
        }
        
        // Verify destination after cut
        stats = mulle_vm_get_statistics(&dst);
        read_range = mulle_range_make(0, stats.vm_size);
        const void *dst_data = mulle_vm_read(&dst, read_range, buffer);
        if (dst_data) {
            printf("Destination after cut: %.*s\n", (int)stats.vm_size, (char*)dst_data);
            // Output: "World!"
        }
    }
    
    mulle_vm_done(&src);
    mulle_vm_done(&dst);
    return 0;
}
```

#### Notes
- This is a convenience function that combines copy and delete operations
- The destination VM is reset (cleared) before the copy operation
- The source VM is modified by removing the specified range
- dst and src must be different VM instances
- More efficient than separate copy and delete operations
### Function: mulle_vm_paste

```c
int mulle_vm_paste(struct mulle_vm *dst,
                  struct mulle_range range,
                  struct mulle_vm *src);
```

#### Description
Paste data from one VM into another at a specific location. This function deletes the specified range in the destination VM, then inserts all data from the source VM at that location. The source VM remains unchanged. This is equivalent to a "paste" operation that replaces content.

#### Parameters
- **dst**: Pointer to the destination VM structure. Must be different from src.
- **range**: The memory range in the destination VM to replace with source data
- **src**: Pointer to the source VM structure. Must be different from dst.

#### Return Value
- **0**: Success - data was pasted successfully
- **-1**: Error - dst and src are the same VM

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm dst, src;
    char dst_data[] = "Hello, OLD World!";
    char src_data[] = "Beautiful";
    
    mulle_vm_init(&dst, &mulle_vm_config_default, NULL);
    mulle_vm_init(&src, &mulle_vm_config_default, NULL);
    
    // Add data to VMs
    mulle_vm_append(&dst, dst_data, strlen(dst_data));
    mulle_vm_append(&src, src_data, strlen(src_data));
    
    // Replace "OLD" with "Beautiful" from src
    struct mulle_range replace_range = mulle_range_make(7, 3);
    int result = mulle_vm_paste(&dst, replace_range, &src);
    if (result == 0) {
        printf("Paste operation successful\n");
        
        // Verify the result
        char buffer[64];
        struct mulle_vm_statistics stats = mulle_vm_get_statistics(&dst);
        struct mulle_range read_range = mulle_range_make(0, stats.vm_size);
        const void *data = mulle_vm_read(&dst, read_range, buffer);
        if (data) {
            printf("Result: %.*s\n", (int)stats.vm_size, (char*)data);
            // Output: "Hello, Beautiful World!"
        }
    }
    
    // Source VM remains unchanged
    char src_buffer[64];
    struct mulle_vm_statistics src_stats = mulle_vm_get_statistics(&src);
    struct mulle_range src_range = mulle_range_make(0, src_stats.vm_size);
    const void *src_data_check = mulle_vm_read(&src, src_range, src_buffer);
    if (src_data_check) {
        printf("Source unchanged: %.*s\n", (int)src_stats.vm_size, (char*)src_data_check);
        // Output: "Beautiful"
    }
    
    mulle_vm_done(&dst);
    mulle_vm_done(&src);
    return 0;
}
```

#### Notes
- The destination VM is modified by replacing the specified range
- The source VM remains completely unchanged
- dst and src must be different VM instances
- If the source VM is empty, the operation simply deletes the range in the destination
- Handles page splitting and merging automatically
- More efficient than separate delete and insert operations
## Detailed Documentation for Statistics Functions

### Structure: mulle_vm_statistics

```c
struct mulle_vm_statistics
{
    size_t   vm_size;
    size_t   page_size;
};
```

#### Description
Statistics structure that provides information about the current state of a VM instance. This structure contains metrics about the virtual memory usage and page allocation.

#### Members
- **vm_size**: Total size of used virtual memory in bytes. This represents the sum of all used ranges across all pages.
- **page_size**: Total size of allocated page memory in bytes. This includes both used and unused portions of all pages.

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    struct mulle_vm_statistics stats;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Get initial statistics
    stats = mulle_vm_get_statistics(&vm);
    printf("Initial - VM size: %zu, Page size: %zu\n", 
           stats.vm_size, stats.page_size);
    
    // Add some data
    mulle_vm_append(&vm, "Hello, World!", 13);
    
    // Get statistics after adding data
    stats = mulle_vm_get_statistics(&vm);
    printf("After append - VM size: %zu, Page size: %zu\n", 
           stats.vm_size, stats.page_size);
    
    // Add more data to potentially trigger new page allocation
    char large_data[10000];
    memset(large_data, 'A', sizeof(large_data));
    mulle_vm_append(&vm, large_data, sizeof(large_data));
    
    stats = mulle_vm_get_statistics(&vm);
    printf("After large append - VM size: %zu, Page size: %zu\n", 
           stats.vm_size, stats.page_size);
    
    mulle_vm_done(&vm);
    return 0;
}
```

### Function: mulle_vm_get_statistics

```c
MULLE__VM_GLOBAL
struct mulle_vm_statistics mulle_vm_get_statistics(struct mulle_vm *vm);
```

#### Description
Get statistics about the current state of a VM instance. This function returns a structure containing information about the virtual memory usage and page allocation for the specified VM.

#### Parameters
- **vm**: Pointer to the VM structure to get statistics for. Can be NULL.

#### Return Value
- **Statistics structure**: A mulle_vm_statistics structure containing:
  - vm_size: Total used virtual memory in bytes
  - page_size: Total allocated page memory in bytes
- **Zeroed structure**: If vm is NULL, returns a structure with both fields set to 0

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Monitor memory usage
    struct mulle_vm_statistics stats;
    
    // Empty VM
    stats = mulle_vm_get_statistics(&vm);
    printf("Empty VM: %zu bytes used, %zu bytes allocated\n", 
           stats.vm_size, stats.page_size);
    
    // Add small data
    mulle_vm_append(&vm, "test", 4);
    stats = mulle_vm_get_statistics(&vm);
    printf("Small data: %zu bytes used, %zu bytes allocated\n", 
           stats.vm_size, stats.page_size);
    
    // Calculate efficiency
    if (stats.page_size > 0) {
        double efficiency = (double)stats.vm_size / stats.page_size * 100.0;
        printf("Memory efficiency: %.2f%%\n", efficiency);
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- vm_size represents the actual data stored in the VM
- page_size includes both used and unused portions of allocated pages
- The ratio vm_size/page_size indicates memory utilization efficiency
- Useful for monitoring memory usage and optimizing page sizes
- Safe to call with NULL vm parameter
## Detailed Documentation for Enumeration Functions

### Structure: mulle_vm_mappingenumerator

```c
struct mulle_vm_mappingenumerator
{
    MULLE_STRUCTARRAYENUMERATOR_BASE;
};
```

#### Description
Enumerator structure for iterating through VM mappings in forward order. This structure is used to enumerate all mappings in a VM instance, providing access to each mapping's page and virtual address information.

### Function: mulle_vm_mapping_enumerate

```c
static inline struct mulle_vm_mappingenumerator
   mulle_vm_mapping_enumerate(struct mulle_vm *vm);
```

#### Description
Create an enumerator for iterating through VM mappings in forward order. This function initializes an enumerator that can be used to traverse all mappings in the VM from first to last.

#### Parameters
- **vm**: Pointer to the VM structure to enumerate

#### Return Value
- **Enumerator**: A mulle_vm_mappingenumerator structure ready for iteration

### Function: mulle_vm_mappingenumerator_next

```c
static inline int
   mulle_vm_mappingenumerator_next(struct mulle_vm_mappingenumerator *rover,
                                   struct mulle_vm_mapping **info);
```

#### Description
Get the next mapping from the enumerator. This function advances the enumerator and returns the next mapping in the sequence.

#### Parameters
- **rover**: Pointer to the enumerator structure
- **info**: Pointer to store the next mapping information

#### Return Value
- **1**: Success - mapping was retrieved and stored in *info
- **0**: End of enumeration - no more mappings available

### Function: mulle_vm_mappingenumerator_done

```c
static inline void
   mulle_vm_mappingenumerator_done(struct mulle_vm_mappingenumerator *rover);
```

#### Description
Clean up the enumerator. This function performs any necessary cleanup for the enumerator. Currently a no-op but provided for future compatibility.

#### Parameters
- **rover**: Pointer to the enumerator structure

### Macro: mulle_vm_mapping_for

```c
#define mulle_vm_mapping_for(vm_ptr, info) \
    for(struct mulle_vm_mappingenumerator _rover__##info = mulle_vm_mapping_enumerate(vm_ptr), \
            *_rover__##info##__i = (void *)0; \
        !_rover__##info##__i; \
        _rover__##info##__i = ((void *)1)) \
      for(struct mulle_vm_mapping *info; \
           mulle_vm_mappingenumerator_next(&_rover__##info, &info); )
```

#### Description
Convenience macro for iterating through VM mappings in forward order. This macro provides a clean for-loop syntax for enumerating all mappings in a VM.

#### Parameters
- **vm_ptr**: Pointer to the VM structure to iterate
- **info**: Variable name for the current mapping (will be declared automatically)

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "Hello", 5);
    mulle_vm_append(&vm, " ", 1);
    mulle_vm_append(&vm, "World", 5);
    
    // Enumerate mappings using the macro
    printf("Forward enumeration:\n");
    mulle_vm_mapping_for(&vm, mapping)
    {
        printf("Address: %zu, Length: %zu\n", 
               mapping->address, mapping->page->used.length);
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

### Structure: mulle_vm_mappingreverseenumerator

```c
struct mulle_vm_mappingreverseenumerator
{
    MULLE_STRUCTARRAYREVERSEENUMERATOR_BASE;
};
```

#### Description
Enumerator structure for iterating through VM mappings in reverse order. This structure is used to enumerate all mappings in a VM instance from last to first.

### Function: mulle_vm_mapping_reverseenumerate

```c
static inline struct mulle_vm_mappingreverseenumerator
   mulle_vm_mapping_reverseenumerate(struct mulle_vm *vm);
```

#### Description
Create a reverse enumerator for iterating through VM mappings. This function initializes an enumerator that can be used to traverse all mappings in the VM from last to first.

#### Parameters
- **vm**: Pointer to the VM structure to enumerate

#### Return Value
- **Enumerator**: A mulle_vm_mappingreverseenumerator structure ready for reverse iteration

### Function: mulle_vm_mappingreverseenumerator_next

```c
static inline int
   mulle_vm_mappingreverseenumerator_next(struct mulle_vm_mappingreverseenumerator *rover,
                                          struct mulle_vm_mapping **info);
```

#### Description
Get the next mapping from the reverse enumerator. This function advances the enumerator and returns the next mapping in reverse sequence.

#### Parameters
- **rover**: Pointer to the reverse enumerator structure
- **info**: Pointer to store the next mapping information

#### Return Value
- **1**: Success - mapping was retrieved and stored in *info
- **0**: End of enumeration - no more mappings available

### Function: mulle_vm_mappingreverseenumerator_done

```c
static inline void
   mulle_vm_mappingreverseenumerator_done(struct mulle_vm_mappingreverseenumerator *rover);
```

#### Description
Clean up the reverse enumerator. This function performs any necessary cleanup for the reverse enumerator. Currently a no-op but provided for future compatibility.

#### Parameters
- **rover**: Pointer to the reverse enumerator structure

### Macro: mulle_vm_mapping_for_reverse

```c
#define mulle_vm_mapping_for_reverse(vm_ptr, info) \
    for(struct mulle_vm_mappingreverseenumerator _rover__##info = mulle_vm_mapping_reverseenumerate(vm_ptr), \
            *_rover__##info##__i = (void *)0; \
        !_rover__##info##__i; \
        _rover__##info##__i = ((void *)1)) \
      for(struct mulle_vm_mapping *info; \
           mulle_vm_mappingreverseenumerator_next(&_rover__##info, &info); )
```

#### Description
Convenience macro for iterating through VM mappings in reverse order. This macro provides a clean for-loop syntax for enumerating all mappings in a VM from last to first.

#### Parameters
- **vm_ptr**: Pointer to the VM structure to iterate
- **info**: Variable name for the current mapping (will be declared automatically)

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "First", 5);
    mulle_vm_append(&vm, "Second", 6);
    mulle_vm_append(&vm, "Third", 5);
    
    // Enumerate mappings in reverse order
    printf("Reverse enumeration:\n");
    mulle_vm_mapping_for_reverse(&vm, mapping)
    {
        printf("Address: %zu, Length: %zu\n", 
               mapping->address, mapping->page->used.length);
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- All enumeration functions are safe to use even with empty VMs
- The enumerator structures are lightweight and stack-allocated
- Reverse enumeration is useful for operations that need to process mappings from end to beginning
- The macros provide type-safe iteration without manual memory management
## Detailed Documentation for Page Functions (from mulle-vm-page.h)

### Structure: mulle_vm_page

```c
struct mulle_vm_page
{
    mulle_vm_ospage         *bytes;         // mapped page start
    struct mulle_range      used;           // used range [location, location+length)
    size_t                  size;           // total mapped size
    mulle_atomic_pointer_t  retain_count;
};
```

#### Description
Represents a single page of virtual memory within the VM system. This structure contains the actual memory buffer, the used portion of that buffer, and metadata for memory management including reference counting for copy-on-write semantics.

#### Members
- **bytes**: Pointer to the actual mapped memory page (operating system page)
- **used**: Range indicating which portion of the page contains valid data
- **size**: Total size of the mapped page in bytes
- **retain_count**: Atomic reference counter for copy-on-write sharing

### Function: mulle_vm_page_init

```c
static inline void mulle_vm_page_init(struct mulle_vm_page *page,
                                     mulle_vm_ospage *bytes,
                                     struct mulle_range used,
                                     size_t size);
```

#### Description
Initialize a page structure with the given parameters. This function sets up the page structure with the provided memory buffer, used range, and total size.

#### Parameters
- **page**: Pointer to the page structure to initialize
- **bytes**: Pointer to the mapped memory page
- **used**: Range indicating the used portion of the page
- **size**: Total size of the mapped page

### Function: mulle_vm_page_address

```c
static inline void *mulle_vm_page_address(struct mulle_vm_page *page,
                                         uintptr_t relative);
```

#### Description
Get the address of data within a page at a specific offset. This function returns a pointer to the actual memory location corresponding to the given offset within the page's used range.

#### Parameters
- **page**: Pointer to the page structure
- **relative**: Offset from the start of the used range

#### Return Value
- **Pointer**: Address within the page at the specified offset
- **NULL**: If page is NULL

### Function: mulle_vm_page_get_length

```c
static inline size_t mulle_vm_page_get_length(struct mulle_vm_page *page);
```

#### Description
Get the length of the used portion of a page. This function returns the size of the valid data within the page.

#### Parameters
- **page**: Pointer to the page structure

#### Return Value
- **Length**: Size of the used portion in bytes
- **0**: If page is NULL

### Function: mulle_vm_page_get_data

```c
static inline struct mulle_data mulle_vm_page_get_data(struct mulle_vm_page *page);
```

#### Description
Get the complete data from a page as a mulle_data structure. This function returns a structure containing both the pointer to the data and its length.

#### Parameters
- **page**: Pointer to the page structure

#### Return Value
- **Data structure**: mulle_data containing pointer and length
- **Invalid data**: If page is NULL

### Function: mulle_vm_page_get_available

```c
static inline uintptr_t mulle_vm_page_get_available(struct mulle_vm_page *page);
```

#### Description
Get the total available space in a page. This function returns the amount of unused space in the page, which can be used for inserting additional data.

#### Parameters
- **page**: Pointer to the page structure

#### Return Value
- **Available space**: Total unused bytes in the page
- **0**: If page is NULL

### Function: mulle_vm_page_get_available_front

```c
static inline uintptr_t mulle_vm_page_get_available_front(struct mulle_vm_page *page);
```

#### Description
Get the available space at the front of a page. This function returns the amount of space available before the used range, which can be used for prepending data.

#### Parameters
- **page**: Pointer to the page structure

#### Return Value
- **Available front space**: Bytes available before the used range
- **0**: If page is NULL

### Function: mulle_vm_page_get_available_back

```c
static inline uintptr_t mulle_vm_page_get_available_back(struct mulle_vm_page *page);
```

#### Description
Get the available space at the back of a page. This function returns the amount of space available after the used range, which can be used for appending data.

#### Parameters
- **page**: Pointer to the page structure

#### Return Value
- **Available back space**: Bytes available after the used range
- **0**: If page is NULL

### Function: mulle_vm_page_alloc

```c
MULLE__VM_GLOBAL
struct mulle_vm_page *mulle_vm_page_alloc(void);
```

#### Description
Allocate a new page structure. This function allocates memory for a new mulle_vm_page structure.

#### Return Value
- **Pointer**: Newly allocated page structure
- **NULL**: If allocation fails

### Function: mulle_vm_page_free

```c
MULLE__VM_GLOBAL
void mulle_vm_page_free(struct mulle_vm_page *page);
```

#### Description
Free a page structure. This function releases the memory used by a page structure. Note that this does not free the underlying memory page - that is handled separately.

#### Parameters
- **page**: Pointer to the page structure to free

### Function: mulle_vm_page_insert_bytes

```c
static inline int mulle_vm_page_insert_bytes(struct mulle_vm_page *page,
                                            uintptr_t address,
                                            struct mulle_range range,
                                            void *bytes);
```

#### Description
Insert bytes into a page at a specific location. This function inserts the provided data into the page at the specified range, handling memory movement as needed.

#### Parameters
- **page**: Pointer to the page structure
- **address**: Virtual address for the insertion
- **range**: Range specifying where to insert and how much data
- **bytes**: Pointer to the data to insert

#### Return Value
- **0**: Success
- **-1**: Error (invalid parameters or page is NULL)

### Function: mulle_vm_page_join

```c
MULLE__VM_GLOBAL
int _mulle_vm_page_join(struct mulle_vm_page *a, struct mulle_vm_page *b);
```

#### Description
Attempt to join two adjacent pages. This function tries to merge two pages if they are contiguous and can be combined efficiently.

#### Parameters
- **a**: First page to join
- **b**: Second page to join

#### Return Value
- **-1**: Cannot join - pages should remain separate
- **0**: No change needed
- **1**: Pages were joined - second page can be freed

#### Example
```c
#include "mulle-vm-page.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct mulle_vm_page *page;
    char buffer[4096];
    
    // Allocate and initialize a page
    page = mulle_vm_page_alloc();
    if (page) {
        // Initialize with some data
        mulle_vm_page_init(page, buffer, mulle_range_make(100, 50), 4096);
        
        printf("Page initialized:\n");
        printf("  Total size: %zu\n", page->size);
        printf("  Used length: %zu\n", mulle_vm_page_get_length(page));
        printf("  Available space: %zu\n", mulle_vm_page_get_available(page));
        printf("  Available front: %zu\n", mulle_vm_page_get_available_front(page));
        printf("  Available back: %zu\n", mulle_vm_page_get_available_back(page));
        
        mulle_vm_page_free(page);
    }
    
    return 0;
}
```

#### Notes
- Page functions operate on individual pages and are typically used internally
- The retain_count field is used for copy-on-write page sharing
- All functions handle NULL page parameters gracefully where appropriate
- Page joining can improve memory efficiency by reducing fragmentation
## Detailed Documentation for Standard Library Functions (from mulle-vm-stdlib.h)

### Function: mulle_vm_memchr

```c
uintptr_t mulle_vm_memchr(struct mulle_vm *vm, struct mulle_range range, int c);
```

#### Description
Search for the first occurrence of a character in VM memory. This function scans the specified range of VM memory for the first occurrence of the given character.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **c**: The character to search for (as int, but treated as unsigned char)

#### Return Value
- **Address**: Virtual address of the first occurrence of the character
- **(uintptr_t)-1**: Character not found in the specified range

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "Hello, World!", 13);
    
    // Search for 'W'
    struct mulle_range range = mulle_range_make(0, 13);
    uintptr_t pos = mulle_vm_memchr(&vm, range, 'W');
    if (pos != (uintptr_t)-1) {
        printf("Found 'W' at position: %zu\n", pos);
        // Output: Found 'W' at position: 7
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

### Function: mulle_vm_memrchr

```c
uintptr_t mulle_vm_memrchr(struct mulle_vm *vm, struct mulle_range range, int c);
```

#### Description
Search for the last occurrence of a character in VM memory. This function scans the specified range of VM memory for the last occurrence of the given character.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **c**: The character to search for (as int, but treated as unsigned char)

#### Return Value
- **Address**: Virtual address of the last occurrence of the character
- **(uintptr_t)-1**: Character not found in the specified range

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>

int main(void)
{
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "Hello, World! Hello again!", 26);
    
    // Search for last 'o'
    struct mulle_range range = mulle_range_make(0, 26);
    uintptr_t pos = mulle_vm_memrchr(&vm, range, 'o');
    if (pos != (uintptr_t)-1) {
        printf("Last 'o' found at position: %zu\n", pos);
        // Output: Last 'o' found at position: 24
    }
    
    mulle_vm_done(&vm);
    return 0;
}
```

### Function: mulle_vm_memmatch

```c
uintptr_t mulle_vm_memmatch(struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
```

#### Description
Find the first occurrence of any matching character in VM memory. This function scans the specified range for the first character that matches any character in the provided match table.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **match**: 256-element array where non-zero values indicate matching characters

#### Return Value
- **Address**: Virtual address of the first matching character
- **(uintptr_t)-1**: No matching character found in the specified range

### Function: mulle_vm_memrmatch

```c
uintptr_t mulle_vm_memrmatch(struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
```

#### Description
Find the last occurrence of any matching character in VM memory. This function scans the specified range for the last character that matches any character in the provided match table.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **match**: 256-element array where non-zero values indicate matching characters

#### Return Value
- **Address**: Virtual address of the last matching character
- **(uintptr_t)-1**: No matching character found in the specified range

### Function: mulle_vm_memspn

```c
size_t mulle_vm_memspn(struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
```

#### Description
Find the length of the initial span of matching characters in VM memory. This function returns the length of the initial segment of the range that consists entirely of characters from the match table.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **match**: 256-element array where non-zero values indicate matching characters

#### Return Value
- **Length**: Number of consecutive matching characters at the start of the range
- **0**: No matching characters at the start

### Function: mulle_vm_memrspn

```c
size_t mulle_vm_memrspn(struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
```

#### Description
Find the length of the final span of matching characters in VM memory. This function returns the length of the final segment of the range that consists entirely of characters from the match table.

#### Parameters
- **vm**: Pointer to the VM structure to search in
- **range**: The memory range to search within
- **match**: 256-element array where non-zero values indicate matching characters

#### Return Value
- **Length**: Number of consecutive matching characters at the end of the range
- **0**: No matching characters at the end

#### Example
```c
#include "mulle-vm.h"
#include <stdio.h>
#include <ctype.h>

int main(void)
{
    struct mulle_vm vm;
    uint8_t match[256] = {0};
    
    mulle_vm_init(&vm, &mulle_vm_config_default, NULL);
    
    // Add some data
    mulle_vm_append(&vm, "   Hello, World!   ", 19);
    
    // Set up match table for whitespace
    match[' '] = 1;
    match['\t'] = 1;
    match['\n'] = 1;
    
    // Find leading whitespace
    struct mulle_range range = mulle_range_make(0, 19);
    size_t leading_ws = mulle_vm_memspn(&vm, range, match);
    printf("Leading whitespace: %zu characters\n", leading_ws);
    // Output: Leading whitespace: 3 characters
    
    // Find trailing whitespace
    size_t trailing_ws = mulle_vm_memrspn(&vm, range, match);
    printf("Trailing whitespace: %zu characters\n", trailing_ws);
    // Output: Trailing whitespace: 3 characters
    
    mulle_vm_done(&vm);
    return 0;
}
```

#### Notes
- All standard library functions operate on virtual memory ranges
- The match table for memmatch/memrmatch/memspn/memrspn uses 256 bytes where each index corresponds to a character value
- These functions provide C standard library equivalents for VM memory
- All functions handle invalid ranges gracefully
- Performance is optimized for large memory ranges