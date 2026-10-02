//
//  aliasing-base.c
//  mulle-vm
//
//  Direct regression test for the nested-member layout of mulle-vm-tree:
//
//      struct mulle_vm_tree   -> struct mulle_rbtree   rbtree
//      struct mulle_rbtree    -> struct mulle__rbtree  base
//
//  `rbtree` is the first (and only) member of `struct mulle_vm_tree` and
//  `base` is the first member of `struct mulle_rbtree`, so all three
//  addresses denote the same object.
//
//  The failure this guards against: mulle-vm used to treat a
//  `struct mulle_vm_tree *` as a `struct mulle_rbtree *` (and the rbtree
//  wrapper treated a `struct mulle_rbtree *` as a `struct mulle__rbtree *`)
//  by casting between distinct types of identical layout. Accessing an object
//  through a pointer to an unrelated type is a strict-aliasing violation
//  (C11 6.5p7) that GCC exploits at -O2/-O3: a store through one view can be
//  assumed not to alias a load through another, so state read back from the
//  tree is stale or zeroed. The code now spells `&tree->rbtree.base` instead
//  of casting.
//
//  The hard guarantee is the layout: the `offsetof` asserts stop compiling if
//  a member is removed or moved, i.e. if the flat macro-repeated prefix comes
//  back. The interleaved store/load and the live enumeration then exercise the
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
_Static_assert( sizeof( struct mulle_vm_tree) >= sizeof( struct mulle_rbtree),
                "mulle_vm_tree must be at least as large as its rbtree");


//
// Three views of the same object, one per nesting level. `static inline` so
// the optimizer reasons about the aliasing within a single TU.
//
static inline unsigned int   _vm_get_options( struct mulle_vm_tree *tree)
{
   return( tree->rbtree.base._options);
}

static inline void   _vm_set_options( struct mulle_vm_tree *tree,
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
   struct mulle_vm_tree   tree;
   struct mulle_vm_tree   *p;

   memset( &tree, 0, sizeof( tree));
   p = &tree;

   assert( (void *) p == (void *) &p->rbtree);
   assert( (void *) p == (void *) &p->rbtree.base);
   assert( (void *) &p->rbtree == (void *) &p->rbtree.base);

   mulle_printf( "aliasing-base: identity OK\n");
}


//
// Store through one view, read through another (and back). The interleaved
// stores matter: the last store must be observed by the differently-typed
// load, which is what the old casts let the optimizer deny.
//
static void   test_store_load_nested( void)
{
   struct mulle_vm_tree   tree;

   memset( &tree, 0, sizeof( tree));

   // outermost store, innermost store, outermost load
   tree.rbtree.base._options = mulle_rbtree_option_allow_duplicates;
   __rbtree_set_options( &tree.rbtree.base, mulle_rbtree_option_use_dirty);
   assert( _vm_get_options( &tree) == mulle_rbtree_option_use_dirty);

   // middle store, innermost store, middle load
   _rbtree_set_options( &tree.rbtree, mulle_rbtree_option_allow_duplicates);
   __rbtree_set_options( &tree.rbtree.base, mulle_rbtree_option_use_dirty);
   assert( _rbtree_get_options( &tree.rbtree) == mulle_rbtree_option_use_dirty);

   // innermost store, outermost store, innermost load
   __rbtree_set_options( &tree.rbtree.base, mulle_rbtree_option_allow_duplicates);
   _vm_set_options( &tree, mulle_rbtree_option_use_dirty);
   assert( __rbtree_get_options( &tree.rbtree.base) == mulle_rbtree_option_use_dirty);

   // same for the node pointers
   tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x10;
   _vm_set_options( &tree, mulle_rbtree_option_allow_duplicates);
   tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x30;
   assert( __rbtree_get_root( &tree.rbtree.base) == (struct mulle_rbnode *) (uintptr_t) 0x30);

   mulle_printf( "aliasing-base: store/load OK\n");
}


//
// `mulle_rbtree_is_empty` is `static inline` and computes root/nil through
// `&tree->base`. Fabricate the pointers through the outermost view and let the
// public accessor read them.
//
static void   test_public_accessor_through_base( void)
{
   struct mulle_vm_tree   tree;

   memset( &tree, 0, sizeof( tree));

   // an empty tree has root == &_nil
   tree.rbtree.base._root = &tree.rbtree.base._nil;
   assert( mulle_rbtree_is_empty( &tree.rbtree) == 1);

   tree.rbtree.base._root = (struct mulle_rbnode *) (uintptr_t) 0x10;
   assert( mulle_rbtree_is_empty( &tree.rbtree) == 0);

   mulle_printf( "aliasing-base: accessor OK\n");
}


//
// Live tree. The build goes through the public API; the reads go through the
// nested base and through the public accessors/enumerators, which all reach
// into `&tree->rbtree.base`. A stale or hoisted load of the base state would
// show up as a wrong count, a wrong total or a missing page.
//
static void   test_live_tree( void)
{
   struct mulle_vm_tree             tree;
   struct mulle_vm_treeenumerator   rover;
   struct mulle_vm_node             *node;
   struct mulle_vm_page             *pages[ 3 ];
   void                             *bytes[ 3 ];
   char                             *data[ 3 ] = { "First", "Second", "Third" };
   size_t                           lengths[ 3 ];
   size_t                           expected_total;
   int                              i;
   int                              count;

   mulle_vm_tree_init( &tree, NULL);

   expected_total = 0;
   for( i = 0; i < 3; i++)
   {
      lengths[ i ] = strlen( data[ i ]);
      bytes[ i ]   = mulle_malloc( lengths[ i ]);
      memcpy( bytes[ i ], data[ i ], lengths[ i ]);
      pages[ i ]   = mulle_vm_page_alloc();
      mulle_vm_page_init( pages[ i ], bytes[ i ],
                          mulle_range_make( 0, lengths[ i ]),
                          lengths[ i ]);
      mulle_vm_tree_add_page( &tree, pages[ i ]);

      expected_total += lengths[ i ];
   }

   // the caller-owned count, read through the nested base, is 3
   assert( _mulle__rbtree_get_count( &tree.rbtree.base) == 3);
   assert( ! mulle_rbtree_is_empty( &tree.rbtree));

   // the tree total is maintained through the same base
   assert( mulle_vm_tree_get_total_length( &tree) == expected_total);

   // full structural validation (dirty flag, rbtree invariants, totals)
   assert( ! mulle_vm_tree_validate( &tree));

   // the enumerator stores the nested base, not a pun of the outer tree
   rover = mulle_vm_tree_enumerate( &tree);
   assert( rover._tree == &tree.rbtree.base);

   count = 0;
   while( count < 8 && mulle_vm_treeenumerator_next( &rover, &node))
   {
      assert( count < 3);
      assert( node->page == pages[ count ]);
      ++count;
   }
   assert( count == 3);

   mulle_vm_treeenumerator_done( &rover);

   // previous/next node navigation also descends through the base
   node = NULL;
   {
      struct mulle_vm_treeenumerator   walk = mulle_vm_tree_enumerate( &tree);
      assert( mulle_vm_treeenumerator_next( &walk, &node));
      mulle_vm_treeenumerator_done( &walk);
   }
   assert( mulle_vm_tree_previous_node( &tree, node) == NULL);
   assert( mulle_vm_tree_next_node( &tree, node) != NULL);
   node = mulle_vm_tree_next_node( &tree, node);
   assert( node != NULL);
   node = mulle_vm_tree_next_node( &tree, node);
   assert( node != NULL);
   assert( mulle_vm_tree_next_node( &tree, node) == NULL);

   mulle_vm_tree_done( &tree);

   for( i = 0; i < 3; i++)
      mulle_free( bytes[ i ]);

   mulle_printf( "aliasing-base: live tree OK\n");
}


int   main( void)
{
   test_identity();
   test_store_load_nested();
   test_public_accessor_through_base();
   test_live_tree();

   mulle_printf( "aliasing-base: OK\n");

   return( 0);
}
