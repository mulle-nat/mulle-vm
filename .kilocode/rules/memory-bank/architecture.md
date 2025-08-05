# mulle-vm Architecture Decisions

## Page Memory Management (Critical - Do Not Violate)

**ALL pages in mulle-vm ALWAYS have valid backing memory allocated.**
- **All pages**: Have a non-zero `used_range` indicating what portion of the page contains data
- **NEVER**: No mulle_vm_mapping struct ever has a NULL `address` pointer
- **DYNAMIC ALLOCATION**: Pages can be allocated on-demand in write operations, if no more space is available

This means:
1. `mulle_vm_init` may or may not pre-allocate some pages with valid memory but zero range
2. `mulle_vm_read` and `mulle_vm_write` should never check for `page_info->address == NULL`
3. Memory optimization comes from tracking usage via `used_range`, not by having NULL pointers

**Previous Mistakes Made:**
- Added NULL pointer checks for `page_info->address`, its always not NULL
- Added mulle_range_is_valid checks for `page_info->used_range`, they are always valid
- Misunderstood that unused pages could have NULL backing memory
  