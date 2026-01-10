# Node Coalescing Tests

This directory contains comprehensive tests for the `_mulle_vm_try_coalesce` function, which handles merging adjacent pages in the virtual memory system.

## Test Files

### `coalescing-test.c`
Basic coalescing scenarios:
- **Predecessor coalescing**: Merging with previous page
- **Successor coalescing**: Merging with next page  
- **Empty page removal**: Removing completely empty pages

### `coalescing-final-test.c`
Comprehensive edge case tests:
- **Predecessor coalescing**: Basic predecessor merge scenarios
- **Successor coalescing**: Basic successor merge scenarios
- **Boundary conditions**: Edge cases with small data amounts
- **Empty page removal**: Testing complete page removal through coalescing

### `coalescing-conditions-test.c`
Targeted tests for specific coalescing conditions:
- **Predecessor coalescing**: Tests both sides of OR condition:
  - `left_back_available >= node_front_occupied`
  - `left_available >= node_front_occupied && lpage->size == pagesize`
- **Successor coalescing**: Tests both sides of OR condition:
  - `right_front_available >= node_back_occupied`
  - `right_available >= node_back_occupied && rpage->size == pagesize`
- **Edge cases**: Tests where coalescing should NOT occur

### `coalescing-simultaneous-test.c`
Tests for simultaneous predecessor and successor coalescing:
- **Single call coalescing**: When deleting a middle page triggers both predecessor and successor merging
- **Multi-page scenarios**: Complex cases with 3+ pages
- **Bridge deletion**: Single character between pages that triggers coalescing
- **Cascading effects**: Multiple coalescing operations from single deletion

## Running Tests

```bash
# Run individual tests
mulle-sde test run test/50-vm/coalescing-test.c
mulle-sde test run test/50-vm/coalescing-final-test.c
mulle-sde test run test/50-vm/coalescing-conditions-test.c
mulle-sde test run test/50-vm/coalescing-simultaneous-test.c

# Run all coalescing tests
mulle-sde test run test/50-vm/coalescing-test.c test/50-vm/coalescing-final-test.c test/50-vm/coalescing-conditions-test.c test/50-vm/coalescing-simultaneous-test.c
```

## Test Scenarios Covered

### 1. Predecessor Coalescing
When a node has available space at the back and can merge with the previous node.

### 2. Successor Coalescing  
When a node has available space at the front and can merge with the next node.

### 3. Empty Page Removal
When deletion of all data in a page triggers complete removal of the empty page.

### 4. Boundary Conditions
- Small data amounts (single characters)
- Half-page threshold conditions
- Multiple consecutive coalescing operations

## Expected Behavior

The tests verify that:
1. **Page count decreases** when coalescing occurs
2. **Data integrity is maintained** after coalescing
3. **Empty pages are properly removed** when all data is deleted
4. **Coalescing only occurs when beneficial** (threshold conditions are met)

## API Used

These tests use the standard mulle-vm API:
- `mulle_vm_init()` / `mulle_vm_done()`
- `mulle_vm_append()`
- `mulle_vm_delete()`
- `mulle_vm_get_statistics()`
- `mulle_vm_read()`
- `mulle_vm_get_length()`

## Output Format

Tests display:
- Page count before and after operations
- Total bytes in VM
- Actual data content for verification