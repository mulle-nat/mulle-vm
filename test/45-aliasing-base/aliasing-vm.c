//
//  aliasing-vm.c
//  mulle-vm
//
//  Direct regression test for the nested-member layout of the mulle-vm
//  container:
//
//      struct mulle_vm        -> struct mulle_vm_tree  tree
//      struct mulle_vm_tree   -> struct mulle_rbtree   rbtree
//      struct mulle_rbtree    -> struct mulle__rbtree  base
//
//  `tree` is the first member of `struct mulle_vm`, `rbtree` the first member
//  of `struct mulle_vm_tree` and `base` the first member of
//  `struct mulle_rbtree`, so
//
//      &vm == &vm->tree == &vm->tree.rbtree == &vm->tree.rbtree.base
//
//  all denote the same object.
//
//  The failure this guards against: mulle-vm used to pass a
//  `struct mulle_vm *` around as a `struct mulle_vm_tree *` /
//  `struct mulle_rbtree *` / `struct mulle__rbtree *` by casting between
//  distinct types of identical layout. Accessing an object through a pointer
//  to an unrelated type is a strict-aliasing violation (C11 6.5p7) that GCC
//  exploits at -O2/-O3: a store through one view can be assumed not to alias
//  a load through another, so tree state read back through the VM is stale or
//  zeroed. The code now spells `&vm->tree.rbtree.base` instead of casting.
//
//  The hard guarantee is the layout: the `offsetof` asserts stop compiling if
//  a member is removed or moved, i.e. if the flat macro-repeated prefix comes
//  back. The interleaved store/load and the live VM test then exercise the
//  access pattern the casts got wrong, at whatever optimization the build
//  uses (-O2/-O3 in the regression runs).
//
#ifdef NDEBUG
# undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "include.h"


_Static_assert( offsetof( struct mulle_rbtree, base) == 0,
                "mulle_rbtree.base must be the first member");
_Static_assert( offsetof( struct mulle_vm_tree, rbtree) == 0,
                "mulle_vm_tree.rbtree must be the first member");
_Static_assert( offsetof( struct mulle_vm, tree) == 0,
                "mulle_vm.tree must be the first member");
_Static_assert( sizeof( struct mulle_vm) >= sizeof( struct mulle_vm_tree),
                "mulle_vm must be at least as large as its tree");


//
// Four views of the same object, one per nesting level. `static inline` so
// the optimizer reasons about the aliasing within a single TU.
//
static inline unsigned int   _vm_get_options( struct mulle_vm *vm)
{
   return( vm->tree.rbtree.base._options);
}

static inline void   _vm_set_options( struct mulle_vm *vm,
                                      unsigned int options)
{
   vm->tree.rbtree.base._options = options;
}


static inline unsigned int   _vmtree_get_options( struct mulle_vm_tree *tree)
{
   return( tree->rbtree.base._options);
}

static inline void   _vmtree_set_options( struct mulle_vm_tree *tree,
                                          unsigned int options)
{
   tree->rbtree.base._options = options;
}


static inline unsigned int   _rbtree_get_options( struct mulle_rbtree *tree)
{
   return( tree->base._options);
}

static inline void   _rbtree_set_options( struct mulle_rbtree *tree,
                                          unsigned int options)
{
   tree->base._options = options;
}


static inline unsigned int   __rbtree_get_options( struct mulle__rbtree *tree)
{
   return( tree->_options);
}

static inline void   __rbtree_set_options( struct mulle__rbtree *tree,
                                           unsigned int options)
{
   tree->_options = options;
}


static inline struct mulle_rbnode   *__rbtree_get_root( struct mulle__rbtree *tree)
{
   return( tree->_root);
}


static void   test_identity( void)
{
   struct mulle_vm   vm;
   struct mulle_vm   *p;

   memset( &vm, 0, sizeof( vm));
   p = &vm;

   assert( (void *) p == (void *) &p->tree);
   assert( (void *) p == (void *) &p->tree.rbtree);
   assert( (void *) p == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree.rbtree == (void *) &p->tree.rbtree.base);

   mulle_printf( "aliasing-vm: identity OK\n");
}


//
// Store through one view, read through another (and back). The interleaved
// stores matter: the last store must be observed by the differently-typed
// load, which is what the old casts let the optimizer deny.
//
static void   test_store_load_nested( void)
{
   struct mulle_vm   vm;

   memset( &vm, 0, sizeof( vm));

   // outermost store, innermost store, outermost load
   vm.tree.rbtree.base._options = mulle_rbtree_option_allow_duplicates;
   __rbtree_set_options( &vm.tree.rbtree.base, mulle_rbtree_option_use_dirty);
   assert( _vm_get_options( &vm) == mulle_rbtree_option_use_dirty);

   // tree-level store, innermost store, tree-level load
   _vmtree_set_options( &vm.tree, mulle_rbtree_option_allow_duplicates);
   __rbtree_set_options( &vm.tree.rbtree.base, mulle_rbtree_option_use_dirty);
   assert( _vmtree_get_options( &vm.tree) == mulle_rbtree_option_use_dirty);

   // rbtree-level store, innermost store, rbtree-level load
   _rbtree_set_options( &vm.tree.rbtree, mulle_rbtree_option_allow_duplicates);
   __rbtree_set_options( &vm.tree.rbtree.base, mulle_rbtree_option_use_dirty);
   assert( _rbtree_get_options( &vm.tree.rbtree) == mulle_rbtree_option_use_dirty);

   // innermost store, outermost store, innermost load
   __rbtree_set_options( &vm.tree.rbtree.base, mulle_rbtree_option_allow_duplicates);
   _vm_set_options( &vm, mulle_rbtree_option_use_dirty);
   assert( __rbtree_get_options( &vm.tree.rbtree.base) == mulle_rbtree_option_use_dirty);

   // same for the node pointers
   vm.tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x10;
   _vm_set_options( &vm, mulle_rbtree_option_allow_duplicates);
   vm.tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x30;
   assert( __rbtree_get_root( &vm.tree.rbtree.base) == (struct mulle_rbnode *) (uintptr_t) 0x30);

   mulle_printf( "aliasing-vm: store/load OK\n");
}


//
// `mulle_rbtree_is_empty` is `static inline` and computes root/nil through
// `&tree->base`. Fabricate the pointers through the outermost view and let the
// public accessor read them.
//
static void   test_public_accessor_through_base( void)
{
   struct mulle_vm   vm;

   memset( &vm, 0, sizeof( vm));

   // an empty tree has root == &_nil
   vm.tree.rbtree.base._root = &vm.tree.rbtree.base._nil;
   assert( mulle_rbtree_is_empty( &vm.tree.rbtree) == 1);

   vm.tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x10;
   assert( mulle_rbtree_is_empty( &vm.tree.rbtree) == 0);

   mulle_printf( "aliasing-vm: accessor OK\n");
}


//
// Live VM. The build goes through the public VM API; the reads go through the
// nested tree/base and the public accessors/enumerators, which all reach into
// `&vm->tree.rbtree.base`. A stale or hoisted load of the base state would
// show up as a wrong length, count or page.
//
static void   test_live_vm( void)
{
   struct mulle_vm              vm;
   struct mulle_vm_enumerator   rover;
   struct mulle_vm_page         *page;
   struct mulle_vm_statistics   stats;
   char                         buf[ 64 ];
   char                         text[] = "Hello, VM!";
   const void                   *result;
   int                          count;

   mulle_vm_init( &vm, NULL, NULL);

   // config lives after the tree and must survive the tree aliasing
   assert( vm.config.skew == MULLE__VM_SKEW_DEFAULT);
   assert( _mulle__rbtree_get_count( &vm.tree.rbtree.base) == 0);

   mulle_vm_append( &vm, text, strlen( text));

   // the caller-owned count, read through the nested base, is 1
   assert( _mulle__rbtree_get_count( &vm.tree.rbtree.base) == 1);
   assert( ! mulle_rbtree_is_empty( &vm.tree.rbtree));

   // length is maintained through the same base
   assert( mulle_vm_get_length( &vm) == strlen( text));

   // full structural validation (dirty flag, rbtree invariants, totals)
   assert( ! mulle_vm_tree_validate( &vm.tree));

   stats = mulle_vm_get_statistics( &vm);
   assert( stats.vm_size == strlen( text));
   assert( stats.page_size > 0);

   // the enumerator stores the nested base, not a pun of the VM
   rover = mulle_vm_enumerate( &vm);
   assert( rover._tree == &vm.tree.rbtree.base);

   count = 0;
   while( count < 4 && mulle_vm_enumerator_next( &rover, &page))
   {
      assert( count < 1);
      assert( page != NULL);
      ++count;
   }
   assert( count == 1);

   mulle_vm_enumerator_done( &rover);

   // read the data back through the VM, which searches the same base
   result = mulle_vm_read( &vm, mulle_range_make( 0, strlen( text)), buf);
   assert( result != NULL);
   assert( ! memcmp( result, text, strlen( text)));

   mulle_vm_done( &vm);

   mulle_printf( "aliasing-vm: live VM OK\n");
}


int   main( void)
{
   test_identity();
   test_store_load_nested();
   test_public_accessor_through_base();
   test_live_vm();

   mulle_printf( "aliasing-vm: OK\n");

   return( 0);
}
