# mulle-vm Library Documentation for AI

## 1. Introduction & Purpose

mulle-vm is a user-space virtual memory system optimized for text editors and similar applications that frequently insert and delete large amounts of data. It trades off faster insertion/deletion operations for slower read/write operations, making it ideal for editor implementations where structural edits dominate. The library manages a page-based virtual memory system with a B-tree index for efficient range operations, enabling fast bulk operations on multi-gigabyte files that would be prohibitively expensive with traditional array-based buffers.

## 2. Key Concepts & Design Philosophy

**Virtual Memory Model:**
- Data organized into fixed-size pages (configurable, typically 16KB)
- Pages linked via B-tree structure for O(log n) range lookup
- Insertion/deletion: O(log n) tree operations only, no data copying
- Reading/writing: Page-by-page traversal, slower but acceptable for editing

**Design Principles:**

- **Page-Based Abstraction:** All operations work with logical ranges; physical layout hidden.
- **Edit-Optimized:** Excels at character insertion/deletion; poor for random access.
- **Copy-on-Write Friendly:** Pages can be shared across VM instances (useful for copy/paste).
- **Allocator-Independent:** User provides page allocation/deallocation functions.
- **Not Thread-Safe:** Single-threaded use assumed; external locking required for concurrent access.

## 3. Core API & Data Structures

### 3.1 Configuration

#### Types

**`struct mulle_vm_config`**

- **Purpose:** Configuration parameters for VM initialization.
- **Fields:**
  - `shutdown`: Optional callback invoked on `mulle_vm_done()`.
  - `skew`: Address calculation skew parameter (default: 25).
  - `cachesize`: Cache size hint (currently unused).

**`struct mulle_vm_config mulle_vm_config_default`**

- **Purpose:** Pre-configured default settings.

### 3.2 Virtual Memory Instance

#### Type

**`struct mulle_vm`**

- **Purpose:** Opaque virtual memory instance.
- **Internal:**
  - `tree`: B-tree of pages.
  - `config`: Configuration settings.

#### Lifecycle Functions

**`void mulle_vm_init(struct mulle_vm *vm, struct mulle_vm_config *config, struct mulle_allocator *allocator)`**

- **Purpose:** Initialize a VM instance.
- **Parameters:**
  - `vm`: Uninitialized VM structure.
  - `config`: Configuration (NULL for defaults).
  - `allocator`: Memory allocator for internal structures.

**`void mulle_vm_done(struct mulle_vm *vm)`**

- **Purpose:** Destroy VM and free all pages.
- **Calls:** config->shutdown if provided.
- **Must Call:** To free resources.

**`void mulle_vm_reset(struct mulle_vm *vm)`**

- **Purpose:** Clear all pages without destroying VM.

#### Capacity and Information

**`uintptr_t mulle_vm_get_length(struct mulle_vm *vm)`**

- **Purpose:** Get total logical size (sum of all pages).
- **Returns:** Total bytes across all pages.

### 3.3 Data Operations

#### Read Operations

**`void *mulle_vm_read(struct mulle_vm *vm, struct mulle_range range, void *buffer)`**

- **Purpose:** Read logical range into buffer.
- **Parameters:**
  - `vm`: VM instance.
  - `range`: Logical range to read (offset, length).
  - `buffer`: Output buffer (must be >= range.length).
- **Returns:** Pointer to buffer on success, NULL on error.
- **Note:** No memory barrier; data read directly from pages.

**`void *mulle_vm_read_copy(struct mulle_vm *vm, struct mulle_range range, void *allocator)`**

- **Purpose:** Read range and allocate copy.
- **Parameters:**
  - `vm`: VM instance.
  - `range`: Logical range.
  - `allocator`: Allocator for result copy.
- **Returns:** Allocated copy of range data.

#### Write Operations

**`void *mulle_vm_write(struct mulle_vm *vm, struct mulle_range range, void *buffer)`**

- **Purpose:** Write buffer to logical range.
- **Parameters:**
  - `vm`: VM instance.
  - `range`: Logical range to write.
  - `buffer`: Source data.
- **Returns:** Pointer to written data on success, NULL on error.

### 3.4 Structural Modifications

#### Insert Operation

**`struct mulle_data mulle_vm_insert(struct mulle_vm *vm, uintptr_t offset, void *bytes, size_t length)`**

- **Purpose:** Insert bytes at logical offset.
- **Parameters:**
  - `vm`: VM instance.
  - `offset`: Logical position for insertion.
  - `bytes`: Data to insert.
  - `length`: Bytes to insert.
- **Returns:** Resulting data range.
- **Complexity:** O(log n) tree operations; no data copy.

#### Delete Operation

**`struct mulle_data mulle_vm_delete(struct mulle_vm *vm, struct mulle_range range)`**

- **Purpose:** Delete logical range.
- **Parameters:**
  - `vm`: VM instance.
  - `range`: Range to delete.
- **Returns:** Resulting data.
- **Complexity:** O(log n) tree operations.

### 3.5 Iteration

#### Types

**`typedef int mulle_vm_walk_callback_t(struct mulle_data data, void *userinfo)`**

- **Purpose:** Callback for VM walk operations.
- **Parameters:**
  - `data`: Page data range.
  - `userinfo`: User context.
- **Returns:** Non-zero to continue, 0 to stop.

#### Walk Functions

**`int mulle_vm_walk(struct mulle_vm *vm, struct mulle_range range, mulle_vm_walk_callback_t *callback, void *userinfo)`**

- **Purpose:** Walk pages in range (forward).
- **Parameters:**
  - `vm`: VM instance.
  - `range`: Logical range (NULL for all).
  - `callback`: Function called per page.
  - `userinfo`: User data passed to callback.
- **Returns:** Sum of callback returns.

**`int mulle_vm_walk_reverse(struct mulle_vm *vm, struct mulle_range range, mulle_vm_walk_callback_t *callback, void *userinfo)`**

- **Purpose:** Walk pages in reverse order.

## 4. Performance Characteristics

- **Insertion:** O(log n) + cost of creating page.
- **Deletion:** O(log n) + cost of freeing pages.
- **Read:** O(m) where m = number of pages in range.
- **Write:** O(m) where m = number of pages affected.
- **Memory:** One page per contiguous block; overhead for tree nodes.

**Optimal Use Cases:**
- Frequent insertions/deletions (editors, real-time collaborative editing).
- Multi-gigabyte files that fit in memory.

**Poor Use Cases:**
- Random access patterns.
- Read-heavy workloads (normal files in normal editors).

## 5. AI Usage Recommendations & Patterns

### Best Practices:

1. **Use for Text Editing:** VM shines in editor contexts with frequent edits.

2. **Batch Operations:** Group multiple small edits into fewer inserts/deletes.

3. **Read Efficiently:** Use walk() for sequential access, not random reads.

4. **Share Pages:** Same page can exist in multiple VMs (copy/paste pattern).

5. **Pre-allocate:** Estimate size and allocate pages upfront if possible.

### Common Pitfalls:

1. **Excessive Random Reads:** Each read traverses page tree; sequential walk is better.

2. **Not Implementing Page Allocator:** Pages must support fragmented allocation (malloc won't work).

3. **Thread-Unsafety:** No built-in locking; use external mutex for concurrent access.

4. **Large Pages:** Huge page sizes waste memory; too small causes excessive tree depth.

5. **Not Batching Edits:** Applying edits one-by-one is slow; batch into fewer larger ops.

## 6. Integration Examples

### Example 1: Basic VM Operations

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct mulle_vm vm;
    
    mulle_vm_init(&vm, NULL, NULL);
    
    // Insert text
    const char *text = "Hello, World!";
    mulle_vm_insert(&vm, 0, (void *)text, strlen(text));
    
    printf("VM size: %zu bytes\n", mulle_vm_get_length(&vm));
    
    // Read back
    char buffer[100];
    mulle_vm_read(&vm, mulle_range(0, strlen(text)), buffer);
    buffer[strlen(text)] = 0;
    printf("Content: %s\n", buffer);
    
    mulle_vm_done(&vm);
    
    return 0;
}
```

### Example 2: Text Editing Operations

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct mulle_vm vm;
    mulle_vm_init(&vm, NULL, NULL);
    
    // Simulate editor operations
    mulle_vm_insert(&vm, 0, "The quick brown", 15);
    mulle_vm_insert(&vm, 15, " fox", 4);
    mulle_vm_insert(&vm, 19, " jumps over", 11);
    
    size_t total = mulle_vm_get_length(&vm);
    char *buffer = malloc(total + 1);
    
    mulle_vm_read(&vm, mulle_range(0, total), buffer);
    buffer[total] = 0;
    printf("Text: %s\n", buffer);
    
    // Delete range
    mulle_vm_delete(&vm, mulle_range(4, 5));  // Remove "quick"
    
    total = mulle_vm_get_length(&vm);
    mulle_vm_read(&vm, mulle_range(0, total), buffer);
    buffer[total] = 0;
    printf("After delete: %s\n", buffer);
    
    free(buffer);
    mulle_vm_done(&vm);
    
    return 0;
}
```

### Example 3: Walking Pages

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>

int page_counter(struct mulle_data data, void *userinfo) {
    int *count = (int *)userinfo;
    (*count)++;
    printf("Page %d: %zu bytes\n", *count, data.length);
    return 1;
}

int main() {
    struct mulle_vm vm;
    mulle_vm_init(&vm, NULL, NULL);
    
    // Insert multiple fragments
    for (int i = 0; i < 10; i++) {
        char text[32];
        snprintf(text, sizeof(text), "Fragment %d\n", i);
        mulle_vm_insert(&vm, mulle_vm_get_length(&vm), text, strlen(text));
    }
    
    int page_count = 0;
    mulle_vm_walk(&vm, NULL, page_counter, &page_count);
    printf("Total pages: %d\n", page_count);
    
    mulle_vm_done(&vm);
    
    return 0;
}
```

### Example 4: Reverse Walking

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>

int reverse_printer(struct mulle_data data, void *userinfo) {
    printf("Page: %zu bytes at offset...\n", data.length);
    return 1;
}

int main() {
    struct mulle_vm vm;
    mulle_vm_init(&vm, NULL, NULL);
    
    mulle_vm_insert(&vm, 0, "ABCDEFGHIJ", 10);
    
    printf("Forward walk:\n");
    mulle_vm_walk(&vm, NULL, reverse_printer, NULL);
    
    printf("\nReverse walk:\n");
    mulle_vm_walk_reverse(&vm, NULL, reverse_printer, NULL);
    
    mulle_vm_done(&vm);
    
    return 0;
}
```

### Example 5: Large File Simulation

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

int main() {
    struct mulle_vm vm;
    mulle_vm_init(&vm, NULL, NULL);
    
    // Simulate large file
    const char *line = "This is a line of text.\n";
    size_t line_len = strlen(line);
    
    // Insert 1000 lines (slow with malloc; optimized with real page allocator)
    for (int i = 0; i < 1000; i++) {
        mulle_vm_insert(&vm, mulle_vm_get_length(&vm), 
                       (void *)line, line_len);
    }
    
    printf("Total size: %zu bytes\n", mulle_vm_get_length(&vm));
    
    // Delete lines 100-200
    mulle_vm_delete(&vm, mulle_range(100 * line_len, 100 * line_len));
    
    printf("After delete: %zu bytes\n", mulle_vm_get_length(&vm));
    
    mulle_vm_done(&vm);
    
    return 0;
}
```

## 7. Dependencies

Direct dependencies:
- `mulle-allocator`: Memory allocation for internal structures
- `mulle-atinit`: Initialization sequencing (for static initialization)
