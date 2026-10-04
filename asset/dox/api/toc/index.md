# mulle-vm Library Documentation for AI
<!-- Keywords: virtual, memory, pages, tree, insert, copy, paste -->

## 1. Introduction & Purpose

`mulle-vm` is a user-space virtual memory (VM) system implemented as a C
library. It presents an extensible elastic byte store addressed by a `uintptr_t`
offset rather than by real memory addresses. It is designed for editors and
applications that need to insert and delete chunks of data in large documents,
because insertion and deletion run at O(log n)-ish tree cost instead of O(n)
`memmove` cost.

- The VM is page-based: data is stored in OS pages kept in a red-black tree
  keyed by their logical position. A virtual address space is a concatenation
  of the "used" ranges of all mapped pages.
- Reads and writes (the common editor case) are slower than with a plain
  buffer because data may span pages, but they are still fast enough for
  editing workloads. It is *detrimental* in pure read-only scenarios.
- Pages can be **shared** between `struct mulle_vm` instances via reference
  counting with **copy-on-write** (COW). This makes `copy` / `cut` / `paste`
  of large ranges almost free (no data copying) and is safe across threads as
  long as each VM instance is only touched by one thread at a time.
- A single `struct mulle_vm` is **not thread-safe**. Use external locking for
  concurrent access to one instance; different instances may be edited from
  different threads, even when they share pages.

The library is part of the `mulle-core` family. Its direct dependencies are
`mulle-core` (types like `struct mulle_range`, `struct mulle_data`,
`struct mulle_allocator`, the `struct mulle_rbtree`, atomics, the
`MULLE_C_*` macros) and `mulle-iovec`.

## 2. Key Concepts & Design Philosophy

- **Range-addressed storage.** All public operations take a
  `struct mulle_range { uintptr_t location; size_t length; }` that addresses
  the *logical* VM (offsets relative to position 0 of the VM), not physical
  memory. Helper constructors `mulle_range_make`, `mulle_range_make_all`,
  `mulle_range_get_max`, `mulle_range_intersection`, etc. come from
  `mulle-core` (`mulle-range.h`).
- **Pages with slack ("skew").** Each `struct mulle_vm_page` maps a block of
  OS pages. The used range is placed inside the mapped block with "breathing
  room" in front and behind (controlled by `skew`, default 25%). Insertions
  into a page that fits exploit this slack so no new OS page is needed,
  which is what makes many edits cheap.
- **Red-black tree of nodes.** The `struct mulle_vm_tree` maps page → position
  and maintains subtree totals so lookup is O(log n). Split, coalescing and
  dirty-walk logic keep pages large and contiguous after edits.
- **Reference counting + COW.** `mulle_vm_page_retain`/`release` manage
  `retain_count`. Writes to a shared page (retain_count != 0) trigger a
  copy-on-write: a fresh private page is created from the shared bytes.
  `mulle_vm_copy`, `mulle_vm_cut` and `mulle_vm_paste` mostly *share* pages
  instead of copying bytes, which is the key design win.
- **Global page allocation.** OS page allocation uses a single, globally
  configured `struct mulle_ospage_allocator` guarded by a mutex. This is what
  makes cross-VM page sharing (and cross-thread paste) possible. The default
  allocator is `mulle_mmap_alloc_pages`/`mulle_mmap_free_pages` from
  `mulle-core`; it must support freeing pages out of the middle of a mapping
  (malloc/free cannot, since they can't do that).
- **Freed page cache.** Freed single OS pages are cached for reuse
  (`mulle_vm_set_ospage_cachesize`, default 16 pages) to avoid repeated
  mmap/munmap.
- **Versioning.** `MULLE__VM_VERSION` (currently 0.7.57) with inline accessor
  functions.

## 3. Core API & Data Structures

All signatures below are copied verbatim from the public headers in `src/`.

The envelope header is `mulle-vm.h`. It already includes
`mulle-vm-page.h` and `mulle-vm-tree.h`. `mulle-vm-stdlib.h`,
`mulle-vm-file.h` and the standalone `mulle-mem*.h` helpers must be included
separately when used.

```c
#include <mulle-vm/mulle-vm.h>          // main API (page + tree included)
#include <mulle-vm/mulle-vm-stdlib.h>   // byte-search helpers
#include <mulle-vm/mulle-vm-file.h>     // file load/write
```

### 3.1. `mulle-vm.h` (main API)

#### Version and defaults

```c
#define MULLE__VM_VERSION  ((0UL << 20) | (7 << 8) | 57)
```

```c
static inline unsigned int   mulle_vm_get_version_major( void)
static inline unsigned int   mulle_vm_get_version_minor( void)
static inline unsigned int   mulle_vm_get_version_patch( void)
MULLE__VM_GLOBAL
uint32_t   mulle_vm_get_version( void);
```

Default page size macro: `MULLE__VM_PAGESIZE_DEFAULT` is `0x4000` (16 KB;
only used if no OS page size is configured). Default skew:
`MULLE__VM_SKEW_DEFAULT` is `25`.

#### `struct mulle_vm_config`
- **Purpose:** Configuration for a VM. Allocation/free functions must deal
  with memory on a page level (you must be able to allocate three pages and
  then free the middle one — this precludes malloc/free).
- **Key Fields:**
  - `shutdown`: optional callback invoked during `mulle_vm_done()`.
  - `skew`: percentage of a page's slack placed before the used range
    (0..100, default 25).

```c
struct mulle_vm_config
{
   void     (*shutdown)( struct mulle_vm *vm);
   size_t   skew;
};

MULLE__VM_GLOBAL
struct mulle_vm_config     mulle_vm_config_default;
```

#### `struct mulle_vm`
- **Purpose:** The VM instance. Contains the page tree plus its config.
- **Key Fields:** `tree` (the `mulle_vm_tree` of pages), `config`.

```c
struct mulle_vm
{
   struct mulle_vm_tree        tree;
   struct mulle_vm_config      config;
};
```

#### Lifecycle functions

```c
MULLE__VM_GLOBAL
void   mulle_vm_init( struct mulle_vm *vm,
                      struct mulle_vm_config *config,
                      struct mulle_allocator *allocator);
```
Initialize `vm`. If `config` is NULL the default config is used; if
`allocator` is NULL the system allocator is used (for the tree nodes).
NULL `vm` is a no-op.

```c
MULLE__VM_GLOBAL
void   mulle_vm_done( struct mulle_vm *vm);
```
Release all resources (pages, mappings); calls `config.shutdown` if set.

```c
MULLE__VM_GLOBAL
void   mulle_vm_reset( struct mulle_vm *vm);
```
Free all pages/mappings and reset to initial state, but keep the freed-OS-page
pool alive. VM stays initialized.

#### Walk callback and walk functions

```c
typedef int   mulle_vm_walk_callback_t( struct mulle_data data,
                                        struct mulle_range vm_range,
                                        void *userinfo);
```
`data` is the actual content (borrowed pointer, valid during the call),
`vm_range` the corresponding logical VM range, `userinfo` the context passed
into the walk. Return 0 to continue; any other value stops the walk; a
negative value signals an error.

```c
MULLE__VM_GLOBAL
int  _mulle_vm_walk( struct mulle_vm *vm,
                     struct mulle_range range,
                     mulle_vm_walk_callback_t  *callback,
                     void *userinfo,
                     int direction);
```

```c
MULLE_C_NONNULL_THIRD
static inline
int  mulle_vm_walk( struct mulle_vm *vm,
                    struct mulle_range range,
                    mulle_vm_walk_callback_t  *callback,
                    void *userinfo);
```
Walk `range` forward (low→high address). Returns zero on completed success,
the first non-zero callback return value if a callback stops it, or -1 with
`errno` set (EINVAL for NULL `vm`/`callback`, ERANGE for an out-of-bounds
range). Walk starts at the tree-search result node, so it is O(log n + m)
instead of O(n).

```c
MULLE_C_NONNULL_THIRD
static inline
int  mulle_vm_walk_reverse( struct mulle_vm *vm,
                            struct mulle_range range,
                            mulle_vm_walk_callback_t  *callback,
                            void *userinfo);
```
Reverse (high→low address) variant.

#### Size and statistics

```c
static inline uintptr_t   mulle_vm_get_length( struct mulle_vm *vm)
```
Total logical length (sum of used lengths of all pages); 0 if `vm` is NULL.

```c
struct mulle_vm_statistics
{
   size_t   vm_size;
   size_t   page_size;
};

MULLE__VM_GLOBAL
struct mulle_vm_statistics   mulle_vm_get_statistics( struct mulle_vm *vm);
```
`vm_size` = total used bytes, `page_size` = sum of all mapped page sizes
(more than `vm_size` when pages have slack).

#### Low-level page-creation helper

```c
static inline struct mulle_vm_page  *
   _mulle_vm_new_page_with_bytes( struct mulle_vm *vm, void *bytes, size_t length)
```
Wraps `mulle_vm_page_new_with_bytes( bytes, length, vm->config.skew)`.
This creates a new page in VM-local *virtual* positions; the page still has
to be added to the tree. Note this helper is an internal symbol (exported for
tests); prefer `mulle_vm_insert`/`mulle_vm_append`.

#### I/O: read / write

```c
MULLE__VM_GLOBAL
void *   _mulle_vm_io( struct mulle_vm      *vm,
                       struct mulle_range   range,
                       void                 *buffer,
                       int                  write_flag);
```

```c
static inline const void *
   mulle_vm_read( struct mulle_vm *vm, struct mulle_range range, void *buffer)
```
Read `range` from the VM. If `vm` is NULL or `range.length` is 0 returns
NULL. If the range fits in one page, returns a **borrowed direct pointer** to
the VM memory (callers may read it, but must not write through it unless they
use `mulle_vm_read_copy`). If it spans pages, `buffer` must be non-NULL and it
memcpy's the data into `buffer` and returns `buffer`. Out-of-bounds or NULL
buffer on multi-page reads yields NULL with `errno` (ERANGE/ENOSPC/EINVAL).

```c
static inline void *
   mulle_vm_read_copy( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *buffer)
```
Like `mulle_vm_read`, but always copies into `buffer` and always returns
`buffer` (safe to modify). Returns NULL if `vm`, `buffer` or `range.length`
is NULL/0.

```c
static inline void *
   mulle_vm_write( struct mulle_vm *vm, struct mulle_range range, void *buffer)
```
Overwrite `range` with `buffer` contents. Performs COW on shared pages.
Supports multi-page writes (applied page-by-page with COW). Returns `buffer`
on success, NULL if `vm`/`buffer`/`range.length` is NULL/0. If it doesn't fit
the existing allocation the write is still applied where it fits.

#### Structural edits: insert / append / delete

```c
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_THIRD
int   _mulle_vm_insert( struct mulle_vm *vm,
                        struct mulle_range range,
                        void *bytes);
```
Insert `bytes` (length = `range.length`) at `range.location`, shifting
existing data up. Returns 0 on success, -1 with `errno` (ERANGE if
`location > total length`; other errors possible). Inserting at
`location == total length` appends; inserting into the middle may split
pages.

```c
static inline
int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
```
Thin wrapper: returns 0 if `vm` or `bytes` is NULL.

```c
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_SECOND
void  _mulle_vm_append( struct mulle_vm *vm,
                       void *bytes,
                       size_t length);
```
Append `length` bytes *at the end* of the VM (position = current total
length).

```c
static inline void   mulle_vm_append( struct mulle_vm *vm, void *bytes, size_t length)
```

```c
MULLE__VM_GLOBAL
void   _mulle_vm_delete( struct mulle_vm *vm,
                         struct mulle_range range);
```
Delete `range`, evicting pages as needed (may coalesce/shrink pages).
Out-of-bounds parts of the range are ignored.

```c
static inline
void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range)
```

#### Cross-VM transfer: copy / cut / paste

All three refuse `dst == src` and return -1 with `errno = EINVAL`.

```c
MULLE__VM_GLOBAL
int  mulle_vm_copy( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);
```
Append `range` from `src` onto `dst`, **sharing pages** (ref-counted).
Generally you want to `reset` `dst` first. Returns 0 on success, -1 on error.

```c
MULLE__VM_GLOBAL
int   mulle_vm_cut( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);
```
`mulle_vm_copy` followed by `mulle_vm_delete` on the source.

```c
MULLE__VM_GLOBAL
int   mulle_vm_paste( struct mulle_vm *dst,
                      struct mulle_range range,
                      struct mulle_vm *src);
```
Delete `range` in `dst`, then insert all of `src` at `range.location`
(sharing pages; `src` unchanged). This is the pasteboard pattern.

#### Enumerating pages

`mulle_vm_enumerator` is an alias for `mulle_vm_treeenumerator`. To iterate
the pages of a VM:

```c
#define mulle_vm_enumerator     mulle_vm_treeenumerator
static inline struct mulle_vm_enumerator
   mulle_vm_enumerate( struct mulle_vm *vm)
static inline int
   mulle_vm_enumerator_next( struct mulle_vm_enumerator *rover,
                             struct mulle_vm_page **page)
static inline void
   mulle_vm_enumerator_done( struct mulle_vm_enumerator *rover)

#define mulle_vm_for( tree_ptr, page)  ...
```

Reverse variants exist too (`mulle_vm_reverseenumerator`,
`mulle_vm_reverseenumerate`, `mulle_vm_reverseenumerator_next`,
`mulle_vm_reverseenumerator_done`, `mulle_vm_for_reverse`).

### 3.2. `mulle-vm-page.h` (pages and OS page allocation)

#### `struct mulle_ospage_allocator`
- **Purpose:** Global configuration of how OS pages are obtained/released.
  Set once before any VM is used (via `mulle_vm_set_ospage_allocator`).
- **Key Fields:** `pagesize` (must be a power of two), `alloc(size_t)`,
  `free(void *, size_t)`, `fail(...)` (no-return, called on allocation
  failure).

```c
struct mulle_ospage_allocator
{
   size_t   pagesize;
   void     *(*alloc)( size_t);
   int      (*free)( void *, size_t);
   void     (*fail)( struct mulle_allocator *, void *, size_t)  MULLE_C_NO_RETURN;
};

MULLE__VM_GLOBAL
void   mulle_vm_set_ospage_allocator( struct mulle_ospage_allocator *allocator);

MULLE__VM_GLOBAL
void   mulle_vm_set_ospage_cachesize( size_t n_pages);
```
`mulle_vm_set_ospage_cachesize` sets the number of freed single pages kept
for reuse (default 16).

```c
MULLE__VM_GLOBAL
mulle_vm_ospage *   _mulle_vm_alloc_ospages( size_t size);
MULLE__VM_GLOBAL
void                _mulle_vm_free_ospages( mulle_vm_ospage *mem, size_t size);
MULLE__VM_GLOBAL
size_t   mulle_vm_get_pagesize( void);
MULLE__VM_GLOBAL
void   mulle_vm_reset_freed_ospages( void);
```
`mulle_vm_get_pagesize` returns the configured OS page size in bytes.

#### `struct mulle_vm_page`
- **Purpose:** One mapped chunk of OS pages holding part of the VM. The
  logical VM is the concatenation of each page's `used` range.
- **Key Fields:** `bytes` (start of mapped OS memory), `used`
  (`mulle_range` of used bytes *within* the mapped block — note this is an
  offset into `bytes`/relative address, not the VM-logical position),
  `size` (total mapped size, multiple of pagesize), `retain_count`
  (`mulle_atomic_pointer_t`; non-zero ⇒ page is shared).
- **Sharing/lifecycle:** A page is created unique (retain_count 0). It
  becomes shared when `mulle_vm_tree_add_page` retains it on behalf of a
  second tree. Only non-shared pages may be modified in-place; shared pages
  must be COW'd first.

```c
struct mulle_vm_page
{
   mulle_vm_ospage         *bytes;         // mapped page start
   struct mulle_range      used;           // used range [location, location+length)
   size_t                  size;           // total mapped size
   mulle_atomic_pointer_t  retain_count;
};
```

#### Page construction / destruction

```c
static inline void   mulle_vm_page_init( struct mulle_vm_page *page,
                                         mulle_vm_ospage *bytes,
                                         struct mulle_range used,
                                         size_t size)

MULLE__VM_GLOBAL
struct mulle_vm_page   *mulle_vm_page_alloc( void);
MULLE__VM_GLOBAL
void   mulle_vm_page_free( struct mulle_vm_page *page);

MULLE__VM_GLOBAL
struct mulle_vm_page *  mulle_vm_page_new_with_bytes( void *bytes,
                                                      size_t length,
                                                      unsigned int skew);
```
`mulle_vm_page_new_with_bytes` maps `length` rounded up to a pagesize
multiple, positions the used range with `skew`% slack in front, copies the
bytes there and returns a new page with retain_count 0.

#### Retain / release

```c
MULLE__VM_GLOBAL
struct mulle_vm_page *mulle_vm_page_retain( struct mulle_vm_page *page);
MULLE__VM_GLOBAL
void   mulle_vm_page_release( struct mulle_vm_page *page);
static inline intptr_t
   mulle_vm_page_get_retaincount( struct mulle_vm_page *page)
```
`release` frees the page when the count hits zero. The header comments note
`release`/`get_retaincount` are really only for tests.

#### Page accessors (all static inline)

```c
static inline void *
   mulle_vm_page_address( struct mulle_vm_page *page, uintptr_t relative)
static inline size_t
   mulle_vm_page_get_length( struct mulle_vm_page *page)
static inline struct mulle_data
   mulle_vm_page_get_data( struct mulle_vm_page *page)
static inline size_t
   mulle_vm_page_get_available( struct mulle_vm_page *page)
static inline size_t
   mulle_vm_page_get_available_front( struct mulle_vm_page *page)
static inline size_t
   mulle_vm_page_get_available_back( struct mulle_vm_page *page)
static inline int
   mulle_vm_page_is_shared( struct mulle_vm_page *page)
static inline size_t
   mulle_vm_page_get_used_length_first_ospage( struct mulle_vm_page *page,
                                               size_t pagesize)
static inline size_t
   mulle_vm_page_get_used_length_last_ospage( struct mulle_vm_page *page,
                                               size_t pagesize)
```
`mulle_vm_page_address( page, relative)` maps an offset into the used range
to a real pointer (aborts on NULL page or out-of-bounds offset).
`mulle_vm_page_get_data` returns `mulle_data` for the whole used range.
Note: `_mulle_vm_page_offset` / `_mulle_vm_page_address` /
`_mulle_vm_page_get_length` are the unchecked/fatal variants.

#### Page edit and maintenance

```c
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_THIRD
int  _mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t offset,
                                  void *bytes,
                                  size_t length);
static inline
MULLE_C_NONNULL_THIRD
int  mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t offset,
                                  void *bytes,
                                  size_t length)
```
Insert into a **non-shared** page at `offset` relative to the used range
(0 = prepend, `used.length` = append, other = split). Returns 0 on success;
-1 with `errno` EACCES (page shared), ENOSPC (won't fit), ERANGE (offset not
adjacent/inside).

```c
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_SECOND
int   _mulle_vm_page_join( struct mulle_vm_page *a, struct mulle_vm_page *b);
```
Try to merge two logically consecutive pages. -1 if `b` fits appended at the
back of `a`; +1 if `a` fits prepended at the front of `b`; 0 otherwise.

```c
MULLE__VM_GLOBAL
void   mulle_vm_page_free_unused_ospages( struct mulle_vm_page *page);
```
Trim unused leading/trailing OS pages from an oversized mapping.

### 3.3. `mulle-vm-tree.h` (the page index)

The `mulle_vm_tree` is a `struct mulle_rbtree` (from `mulle-core`) whose
per-node payload is a `struct mulle_vm_node`. Nodes are ordered by virtual
position and each keeps a `total` of the cumulative length of that subtree,
which is what gives O(log n) positional search.

```c
struct mulle_vm_tree
{
   struct mulle_rbtree   rbtree;
};

struct mulle_vm_node
{
   struct mulle_vm_page   *page;
   size_t                 total;
};

struct mulle_vm_tree_search_result
{
   struct mulle_vm_node   *node;
   uintptr_t              offset;
};
```

#### Lifecycle

```c
MULLE__VM_GLOBAL
void  mulle_vm_tree_init( struct mulle_vm_tree *tree,
                          struct mulle_allocator *allocator);
static inline void  mulle_vm_tree_done( struct mulle_vm_tree *tree)
static inline struct mulle_allocator *
   mulle_vm_tree_get_allocator( struct mulle_vm_tree *tree)
static inline void  mulle_vm_tree_reset( struct mulle_vm_tree *tree)
```
`mulle_vm_tree_done`/`reset` are no-ops on NULL. `tree->rbtree` owned the
pages via retain/destroy callbacks; adding a page retains it, removing
releases it.

#### Insert / remove pages

```c
MULLE__VM_GLOBAL
struct mulle_vm_node *   _mulle_vm_tree_add_page( struct mulle_vm_tree *tree,
                                                  struct mulle_vm_page *page);
static inline
struct mulle_vm_node *   mulle_vm_tree_add_page( struct mulle_vm_tree *tree,
                                                struct mulle_vm_page *page);

MULLE__VM_GLOBAL
struct mulle_vm_node *   _mulle_vm_tree_insert_page_after_node( struct mulle_vm_tree *tree,
                                                                struct mulle_vm_page *page,
                                                                struct mulle_vm_node *node);
static inline struct mulle_vm_node *
   mulle_vm_tree_insert_page_after_node( struct mulle_vm_tree *tree,
                                          struct mulle_vm_page *page,
                                          struct mulle_vm_node *node);
MULLE__VM_GLOBAL
struct mulle_vm_node *
   _mulle_vm_tree_insert_page_before_node( struct mulle_vm_tree *tree,
                                            struct mulle_vm_page *page,
                                            struct mulle_vm_node *node);
static inline struct mulle_vm_node *
   mulle_vm_tree_insert_page_before_node( struct mulle_vm_tree *tree,
                                           struct mulle_vm_page *page,
                                           struct mulle_vm_node *node);

MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove_page( struct mulle_vm_tree *tree, struct mulle_vm_page *page);
static inline int
   mulle_vm_tree_remove_page( struct mulle_vm_tree *tree, struct mulle_vm_page *page);
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove_node( struct mulle_vm_tree *tree, struct mulle_vm_node *node);
static inline int
   mulle_vm_tree_remove_node( struct mulle_vm_tree *tree, struct mulle_vm_node *node);
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove( struct mulle_vm_tree *tree, uintptr_t position);
static inline int
   mulle_vm_tree_remove( struct mulle_vm_tree *tree, uintptr_t position)
```
These are mostly used internally by the VM operations (insert/delete/coalesce)
and by tests. `_remove_page` is described in the header as "exhaustive and
bad!!" — prefer the node-based removal.

#### Search

```c
MULLE__VM_GLOBAL
struct mulle_vm_tree_search_result
   _mulle_vm_tree_search( struct mulle_vm_tree *tree,
                          uintptr_t position);

MULLE__VM_GLOBAL
struct mulle_vm_page  *mulle_vm_tree_search( struct mulle_vm_tree *tree,
                                             uintptr_t position,
                                             uintptr_t *offset);
```
O(log n): returns the node whose page contains `position` and the `offset`
*within that page's used range*. The result is invalid if `position` is
beyond the total length.

#### Search-result accessors (static inline)

```c
static inline int
   mulle_vm_tree_search_result_is_valid( struct mulle_vm_tree_search_result result)
static inline struct mulle_rbnode *
   mulle_vm_tree_search_result_get_rbnode( struct mulle_vm_tree_search_result result)
static inline struct mulle_vm_node *
   mulle_vm_tree_search_result_get_node( struct mulle_vm_tree_search_result result)
static inline struct mulle_vm_page *
   mulle_vm_tree_search_result_get_page( struct mulle_vm_tree_search_result result)
static inline uintptr_t
   mulle_vm_tree_search_result_get_offset( struct mulle_vm_tree_search_result result)
static inline int
   mulle_vm_tree_search_result_is_equal( struct mulle_vm_tree_search_result result,
                                         struct mulle_vm_tree_search_result other)
```

#### Neighbor iteration (static inline)

```c
static inline struct mulle_vm_node *
   mulle_vm_tree_previous_node( struct mulle_vm_tree *tree,
                                struct mulle_vm_node *node)
static inline struct mulle_vm_node *
   mulle_vm_tree_next_node( struct mulle_vm_tree *tree,
                            struct mulle_vm_node *node)
```

#### Node/page navigation

```c
static inline struct mulle_rbnode *
   _mulle_vm_node_get_rbnode( struct mulle_vm_node *node)
static inline struct mulle_vm_node *
   _mulle_vm_rbnode_get_vm_node( struct mulle_rbnode *rbnode)
static inline struct mulle_vm_page *
   _mulle_vm_node_get_page( struct mulle_vm_node *node)
static inline struct mulle_vm_page *
   mulle_vm_node_get_page( struct mulle_vm_node *node)
```
Get page for a node; get/convert between the rbtree node and its embedded
vm_node payload.

#### Validation and totals

```c
MULLE__VM_GLOBAL
char  *mulle_vm_node_validate( struct mulle_vm_node *node);
MULLE__VM_GLOBAL
char  *mulle_vm_tree_validate( struct mulle_vm_tree *tree);
MULLE__VM_GLOBAL
size_t   mulle_vm_tree_get_total_length( struct mulle_vm_tree *tree);
MULLE__VM_GLOBAL
void   mulle_vm_tree_print( struct mulle_vm_tree *tree);
```
`*_validate` return an error string (or NULL when valid). `tree_print` prints
a debug representation.

#### Enumerating tree nodes

```c
#define mulle_vm_treeenumerator     mulle_rbtreeenumerator
static inline struct mulle_vm_treeenumerator
   mulle_vm_tree_enumerate( struct mulle_vm_tree *tree)
static inline int
   mulle_vm_treeenumerator_next( struct mulle_vm_treeenumerator *rover,
                                 struct mulle_vm_node **node)
static inline void
   mulle_vm_treeenumerator_done( struct mulle_vm_treeenumerator *rover)
#define mulle_vm_tree_for( tree_ptr, node)  ...
#define mulle_vm_treereverseenumerator     mulle_rbtreereverseenumerator
static inline struct mulle_vm_treereverseenumerator
   mulle_vm_tree_reverseenumerate( struct mulle_vm_tree *tree)
static inline int
   mulle_vm_treereverseenumerator_next( struct mulle_vm_treereverseenumerator *rover,
                                        struct mulle_vm_node **node)
static inline void
   mulle_vm_treereverseenumerator_done( struct mulle_vm_treereverseenumerator *rover)
#define mulle_vm_tree_for_reverse( tree_ptr, node)  ...
```
The `mulle_vm_for`/`mulle_vm_tree_for` macros iterate a whole enumeration in a
simple two-loop form; `break`/`continue` work as expected.

### 3.4. `mulle-vm-stdlib.h` (byte search over VM ranges)

Search functions operate across page boundaries by walking the VM. Positions
returned are **absolute logical VM offsets**.

```c
MULLE__VM_GLOBAL
uintptr_t   mulle_vm_memchr( struct mulle_vm *vm, struct mulle_range range, int c);
MULLE__VM_GLOBAL
uintptr_t   mulle_vm_memrchr( struct mulle_vm *vm, struct mulle_range range, int c);

MULLE__VM_GLOBAL
uintptr_t   mulle_vm_memmatch( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
MULLE__VM_GLOBAL
uintptr_t   mulle_vm_memrmatch( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);

MULLE__VM_GLOBAL
size_t   mulle_vm_memspn( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
MULLE__VM_GLOBAL
size_t   mulle_vm_memrspn( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
```

- `mulle_vm_memchr` / `mulle_vm_memrchr`: first/last occurrence of byte `c`.
- `mulle_vm_memmatch` / `mulle_vm_memrmatch`: first/last byte that is marked
  in the 256-byte `match` table (non-zero entry means "matches").
- `mulle_vm_memspn` / `mulle_vm_memrspn`: length of the leading/trailing run
  of bytes that are marked in the `match` table.
- All return `mulle_not_found_e` (an `errno`-style sentinel from
  `mulle-core`) when nothing matches — check against it, not against 0.
- NULL `vm` returns `mulle_not_found_e`.

### 3.5. `mulle-vm-file.h` (file I/O)

```c
MULLE__VM_GLOBAL
int   mulle_vm_init_with_contents_of_file( struct mulle_vm *vm,
                                           char *path,
                                           struct mulle_allocator *allocator);
MULLE__VM_GLOBAL
int   mulle_vm_writev_contents( struct mulle_vm *vm, int fd);
```
`mulle_vm_init_with_contents_of_file` initializes `vm` and fills it with the
contents of file `path` (0 on success). `mulle_vm_writev_contents` writes the
VM contents to file descriptor `fd` (gathering iovec write). Both return 0 on
success and non-zero on failure.

### 3.6. Standalone memory helpers (low-level, used internally)

These `*.h` helpers are public headers but are NOT pulled in by `mulle-vm.h`.
They are used by `mulle-vm-stdlib.c` and exposed for reuse.

- `mulle-memchr.h` (header-only): `static inline void *mulle_memchr(const void *s, int c, size_t n)` and `static inline void *mulle_memrchr(const void *s, int c, size_t n)` — optimized byte scans over a plain buffer (uses `load_u64_unaligned` word scans). Not SQL-standard `memchr`: there is no null-termination semantics, it scans exactly `n` bytes.
- `mulle-memmatch.h` (header-only): `static inline void *mulle_memmatch(const void *buffer, size_t length, const uint8_t match[256])` and `static inline void *mulle_memrmatch(...)` — first/last byte matching a 256-byte set table.
- `mulle-memspn.h` (header-only): `static inline size_t mulle_memspn(const void *buffer, size_t length, const uint8_t match[256])` and `static inline size_t mulle_memrspn(...)` — leading/trailing run length of matching bytes.
- `mulle-memmem.h` (has `mulle-memmem.c`): streaming KMP-style substring search with explicit state, so it can be fed buffer chunks:
  ```c
  struct mulle_memmem_state { const void *needle; size_t needle_len; size_t *lps; size_t buffer[ MULLE_MEMMEM_BUFFER_SIZE]; size_t match_index; size_t total_bytes; };
  ```
  ```c
  MULLE__VM_GLOBAL
  void   mulle_memmem_init( struct mulle_memmem_state *state,
                            const void *needle,
                            size_t needle_len);
  MULLE__VM_GLOBAL
  void   mulle_memmem_done( struct mulle_memmem_state *state);
  MULLE__VM_GLOBAL
  size_t   mulle_memmem( const void *data,
                         size_t data_len,
                         struct mulle_memmem_state *state);
  ```
  `mulle_memmem` returns the absolute offset of the match start (the beginning
  of the needle) or `mulle_not_found_e`. Mirror-image `mulle_memrmem_state`
  and `mulle_memrmem_init/done/memrmem` return the absolute offset of the end
  of the needle. `MULLE_MEMMEM_BUFFER_SIZE` (default 24) is the inline buffer
  size; larger needles allocate `lps` via the system allocator.

## 4. Performance Characteristics

- **`_mulle_vm_tree_search` / `mulle_vm_tree_search`:** O(log n) via the
  rbtree with subtree `total` lengths.
- **`mulle_vm_insert` / `_mulle_vm_insert`:** typically O(log n) tree work
  plus a page-local memmove *within one page* (no global memmove), which is
  the whole point of the design. Falling back to allocating a new page. The
  `skew` slack means many adjacent edits stay in-page.
- **`mulle_vm_delete`:** O(log n) search plus page-local trimming; whole pages
  are dropped from the tree; residual pages are coalesced when small.
- **`mulle_vm_read`:** O(log n) to find the page, then either O(1) direct
  pointer return (single page) or O(m) copying over the pages in the range.
- **`mulle_vm_write`:** O(log n) search + O(m) memcpy over affected pages,
  with possible COW (allocation + copy) per shared page. Multi-page writes are
  supported and applied page-by-page.
- **`mulle_vm_walk` / `mulle_vm_walk_reverse`:** O(log n + m) — starts at the
  tree-search result, not at the beginning of the address space.
- **`mulle_vm_copy` / `mulle_vm_cut` / `mulle_vm_paste`:** mostly O(m) *tree
  node* work sharing pages via refcount increment; no byte copies except
  partial page edges. This is the standout performance feature (fast
  copy/paste of huge ranges).
- **`mulle_vm_memchr` & friends:** O(total bytes scanned) across pages via
  `mulle_vm_walk`.
- **Memory:** each page maps a pagesize-multiple chunk; idle slack is wasted
  until edits use it or `mulle_vm_page_free_unused_ospages` trims it (called
  during coalescing). Freed single OS pages are cached (default 16) for reuse.
- **Thread-safety:** global OS-page allocation is guarded by a global mutex;
  `retain_count` is atomic (COW is safe across threads). But a single
  `struct mulle_vm` is **not** thread-safe for edits — wrap access in your own
  lock. Sharing the *same* page across VMs used by different threads relies on
  the refcount + global allocator lock only.

## 5. AI Usage Recommendations & Patterns

**Best practices**
- Always pair `mulle_vm_init` with `mulle_vm_done`. Use `mulle_vm_reset` to
  cheaply clear a VM while keeping its freed-page cache warm.
- Use `mulle_vm_append` / `mulle_vm_insert` for all data insertion into a VM —
  do not hand-build pages and add them to the tree yourself (that path is for
  internal logic and tests).
- When reading a range you intend to mutate, use `mulle_vm_read_copy` (or
  `mulle_vm_write` into a copied region). `mulle_vm_read` may return a
  *borrowed direct pointer* to VM memory — writing through it is a bug and
  will corrupt the VM. Copy-on-write means the pointer is also invalidated by
  later edits, so do not keep it across insert/delete operations.
- For copy/paste between VMs (`dst` must be a *different* VM from `src`),
  `reset` `dst` first and then call `mulle_vm_copy` / `mulle_vm_paste`;
  pages are shared, not copied.
- Check for `mulle_not_found_e` when using the `mulle_vm_mem*` search
  functions; a "not found" result is a large positive sentinel, **not** -1 and
  not 0.
- Check the integer returns of `mulle_vm_copy`/`cut`/`paste` (0 = success,
  -1 with `errno`), and expect `mulle_vm_insert` to fail with `ERANGE` if you
  try to insert past the end of the VM.

**Common pitfalls**
- Passing `dst == src` to `copy`/`cut`/`paste` returns EINVAL.
- Modifying a shared page directly: only non-shared pages (retain_count == 0)
  may be edited in place; the tree/VM layer does COW itself, but if you poke
  page bytes behind its back on a shared page you corrupt other VMs.
- Forgetting that `page->used` is a range *within* the mapped block, not a
  VM-logical address.
- `mulle_vm_read` with a NULL `buffer` on a multi-page range returns NULL with
  `errno = ENOSPC` (only single-page direct reads may have NULL buffer).
- A single VM is not thread-safe; keep each VM on one thread at a time even
  when pages are shared across VMs.
- Do not call `mulle_vm_page_release`/`get_retaincount` in production code;
  they exist mainly for tests.

**Idiomatic usage**
- Iterate pages with `mulle_vm_for( &vm, page) { ... }` and `mulle_vm_for_reverse`.
- Walk content segments with `mulle_vm_walk`/`mulle_vm_walk_reverse` plus a
  `mulle_vm_walk_callback_t`.
- The `mulle-vm-debug.inc` project header (not installed) provides a static
  debug OS-page allocator and `mulle_vm_debug_config` for tests; production
  code uses the default mmap-based allocator.

## 6. Integration Examples

### Example 1: Creating, populating and reading a VM

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

int   main( void)
{
   struct mulle_vm      vm;
   struct mulle_range   range;
   const void           *data;
   size_t               length;
   char                 buffer[ 256];

   mulle_vm_init( &vm, NULL, NULL);                 // default config

   mulle_vm_append( &vm, "Hello, VM World!", 16);   // create first page
   length = mulle_vm_get_length( &vm);
   printf( "VM length: %zu\n", length);

   // read a single-page span -> borrowed direct pointer
   data = mulle_vm_read( &vm, mulle_range_make( 0, 5), buffer);
   printf( "First 5 bytes: \"%.*s\"\n", 5, (char *) data);

   // read the whole VM into our own buffer
   data = mulle_vm_read_copy( &vm, mulle_range_make( 0, length), buffer);
   printf( "Whole content: \"%.*s\"\n", (int) length, (char *) data);

   // overwrite a range
   mulle_vm_write( &vm, mulle_range_make( 0, 5), "Mulle");

   mulle_vm_done( &vm);
   return( 0);
}
```

### Example 2: Editor-style insert and delete

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

static void   show( struct mulle_vm *vm)
{
   char        *s;
   size_t      length;
   const void  *data;

   length = mulle_vm_get_length( vm);
   s      = malloc( length + 1);
   data   = mulle_vm_read_copy( vm, mulle_range_make( 0, length), s);
   s[ length] = '\0';
   printf( "%s\n", s);
   free( s);
}

int   main( void)
{
   struct mulle_vm   vm;
   char              word[] = "quick";

   mulle_vm_init( &vm, NULL, NULL);

   mulle_vm_append( &vm, "The brown fox", 13);
   mulle_vm_insert( &vm, mulle_range_make( 4, strlen( word)), word);  // "The quick brown fox"
   show( &vm);

   mulle_vm_delete( &vm, mulle_range_make( 4, 5));                    // drop "quick"
   show( &vm);

   mulle_vm_done( &vm);
   return( 0);
}
```

### Example 3: Copy / paste with shared pages (COW)

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

int   main( void)
{
   struct mulle_vm   vm;
   struct mulle_vm   copy;
   char              buffer[ 64];
   size_t            length;

   mulle_vm_init( &vm, NULL, NULL);
   mulle_vm_insert( &vm, mulle_range_make( 0, strlen( "VfL Bochum 1848")), "VfL Bochum 1848");

   mulle_vm_init( &copy, NULL, NULL);
   mulle_vm_copy( &copy, &vm, mulle_range_make( 4, 6));   // share "Bochum" pages

   mulle_vm_paste( &vm, mulle_range_make( 0, 3), &copy);  // replace "VfL"
   mulle_vm_paste( &vm, mulle_range_make( 14, 4), &copy); // replace "1848"

   length = mulle_vm_get_length( &vm);
   mulle_vm_read_copy( &vm, mulle_range_make( 0, length), buffer);
   printf( "Result: \"%.*s\" (expect \"Bochum Bochum\")\n",
           (int) length, buffer);

   mulle_vm_done( &copy);
   mulle_vm_done( &vm);
   return( 0);
}
```

### Example 4: Walking a range with a callback

```c
#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>

static int   print_segment( struct mulle_data data,
                            struct mulle_range vm_range,
                            void *userinfo)
{
   (void) userinfo;
   printf( "segment @{%lu, %lu}: \"%.*s\"\n",
           (unsigned long) vm_range.location,
           (unsigned long) vm_range.length,
           (int) data.length,
           (char *) data.bytes);
   return( 0);   // 0 continues, non-zero stops
}

int   main( void)
{
   struct mulle_vm   vm;

   mulle_vm_init( &vm, NULL, NULL);
   mulle_vm_append( &vm, "abcd", 4);
   mulle_vm_append( &vm, "efgh", 4);
   mulle_vm_append( &vm, "ijkl", 4);

   mulle_vm_walk( &vm, mulle_range_make( 0, mulle_vm_get_length( &vm)),
                  print_segment, NULL);
   mulle_vm_walk_reverse( &vm, mulle_range_make( 0, mulle_vm_get_length( &vm)),
                          print_segment, NULL);

   mulle_vm_done( &vm);
   return( 0);
}
```

### Example 5: Byte search over the whole VM

```c
#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-stdlib.h>
#include <stdio.h>

int   main( void)
{
   struct mulle_vm   vm;
   uintptr_t         offset;

   mulle_vm_init( &vm, NULL, NULL);
   mulle_vm_append( &vm, "the quick brown fox", 19);

   offset = mulle_vm_memchr( &vm, mulle_range_make_all(), 'o');
   if( offset != mulle_not_found_e)
      printf( "first 'o' at offset %lu\n", (unsigned long) offset);

   offset = mulle_vm_memrchr( &vm, mulle_range_make_all(), 'o');
   if( offset != mulle_not_found_e)
      printf( "last 'o' at offset %lu\n", (unsigned long) offset);

   mulle_vm_done( &vm);
   return( 0);
}
```

## 7. Dependencies

From `.mulle/etc/sourcetree/config`:

- `mulle-core` — provides `struct mulle_range`, `struct mulle_data`,
  `struct mulle_allocator`, `struct mulle_rbtree`/`mulle_rbnode`, atomics,
  the `MULLE_C_*` macros, `mulle_mmap_alloc_pages`/`mulle_mmap_free_pages`,
  `mulle_printf`/`mulle_fprintf`, `mulle_not_found_e`, `mulle_pointerarray`,
  `mulle_storage`.
- `mulle-iovec` — iovec compatibility layer (used by `mulle_vm_writev_contents`).