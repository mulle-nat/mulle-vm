### 0.7.57



* Fix parameter types of ``_mulle_vm_tree_insert_page_after_node`` and ``_mulle_vm_tree_insert_page_before_node`` in the API docs from `struct `mulle_rbtree` *` to `struct `mulle_vm_tree` *`.
* Update the documented ``MULLE__VM_VERSION`` to 0.7.57.



* writes into a copy-on-write shared page are now stored back into the node, so inserts are no longer silently lost and the copied page no longer leaks
* mid-page inserts re-read the page after a split, fixing inserts that failed with ERANGE when the page had been copied on write
* replace casts between distinct struct pointer types with proper nested member access (`&tree->rbtree.base`), fixing state read-back being stale or zeroed by GCC at -O2/-O3
* **BREAKING** ``_mulle_vm_tree_insert_page_before_node`` and ``_mulle_vm_tree_insert_page_after_node`` now take `struct `mulle_vm_tree` *` instead of `struct `mulle_rbtree` *`

### 0.7.56

Various small improvements
