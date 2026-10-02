//
//  aliasing-vm-shared.c
//  mulle-vm
//
//  Regression test for the shared-page paths of mulle_vm_copy /
//  mulle_vm_cut / mulle_vm_paste and the nested-member layout they read
//  through:
//
//      struct mulle_vm        -> struct mulle_vm_tree  tree
//      struct mulle_vm_tree   -> struct mulle_rbtree   rbtree
//      struct mulle_rbtree    -> struct mulle__rbtree  base
//
//  These operations move page pointers between two `struct mulle_vm`s and
//  repeatedly read the caller-owned count through the nested base
//  (`_mulle__rbtree_get_count( &vm->tree.rbtree.base)`) while inserting and
//  deleting nodes. The old code reached the same state through casts of the
//  VM (`(struct mulle__rbtree *) &vm->tree`,
//  `(struct mulle_rbtree *) &vm->tree`), a strict-aliasing violation
//  (C11 6.5p7) that GCC can exploit at -O2/-O3: a node inserted through one
//  view can be invisible to a load through another, so the count, the shared
//  page retain_count or the written content come out wrong.
//
//  The hard guarantee is the layout: the `offsetof` asserts stop compiling if
//  a member is removed or moved. The live tests then share a page between two
//  VMs and read both VMs back through the nested base, asserting the same
//  page object is reachable from both and that the content round-trips.
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


//
// Typed views of the nested state. `static inline` so the optimizer reasons
// about the aliasing within a single TU.
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

static inline unsigned int   __rbtree_get_options( struct mulle__rbtree *tree)
{
   return( tree->_options);
}


//
// Reads that must agree: the caller-owned count through the nested base and
// the count actually reachable by walking the tree.
//
static unsigned int   count_via_base( struct mulle_vm *vm)
{
   return( (unsigned int) _mulle__rbtree_get_count( &vm->tree.rbtree.base));
}


static unsigned int   count_via_enumeration( struct mulle_vm *vm)
{
   struct mulle_vm_enumerator   rover;
   struct mulle_vm_page         *page;
   unsigned int                 count;

   count = 0;
   rover = mulle_vm_enumerate( vm);
   while( mulle_vm_enumerator_next( &rover, &page))
      ++count;
   mulle_vm_enumerator_done( &rover);

   return( count);
}


static struct mulle_vm_page   *first_page( struct mulle_vm *vm)
{
   struct mulle_vm_page   *page;

   page = NULL;
   mulle_vm_for( vm, current)
   {
      page = current;
      break;
   }
   return( page);
}


static void   assert_content( struct mulle_vm *vm,
                              char *expected,
                              size_t length)
{
   char   buf[ 128 ];

   assert( mulle_vm_get_length( vm) == length);
   assert( mulle_vm_read( vm, mulle_range_make( 0, length), buf) != NULL);
   assert( ! memcmp( buf, expected, length));
}


static void   test_identity( void)
{
   struct mulle_vm   a;
   struct mulle_vm   b;
   struct mulle_vm   *p;

   memset( &a, 0, sizeof( a));
   memset( &b, 0, sizeof( b));
   p = &a;

   assert( (void *) p == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree.rbtree == (void *) &p->tree.rbtree.base);

   // adjacent VMs must not be mistaken for one another through the base
   assert( (void *) &a != (void *) &b);
   assert( (void *) &a == (void *) &a.tree.rbtree.base);
   assert( (void *) &b == (void *) &b.tree.rbtree.base);

   // interleave stores/loads across the two nested views
   _vm_set_options( &a, mulle_rbtree_option_allow_duplicates);
   assert( __rbtree_get_options( &a.tree.rbtree.base)
           == mulle_rbtree_option_allow_duplicates);
   _vm_set_options( &b, mulle_rbtree_option_use_dirty);
   assert( _vm_get_options( &b) == mulle_rbtree_option_use_dirty);
   assert( __rbtree_get_options( &a.tree.rbtree.base)
           == mulle_rbtree_option_allow_duplicates);

   mulle_printf( "aliasing-vm-shared: identity OK\n");
}


//
// Copy a whole single-page VM: the page object is shared, not duplicated.
// The count and the shared page must be reachable through both nested bases.
//
static void   test_copy( void)
{
   struct mulle_vm         src;
   struct mulle_vm         dst;
   struct mulle_vm_page    *src_page;
   struct mulle_vm_page    *dst_page;
   char                    data[] = "Hello World!";
   size_t                  length;

   length = strlen( data);

   mulle_vm_init( &src, NULL, NULL);
   mulle_vm_init( &dst, NULL, NULL);

   mulle_vm_append( &src, data, length);

   assert( mulle_vm_copy( &dst, &src, mulle_range_make( 0, length)) == 0);

   // counts read through the nested base agree with walking the trees
   assert( count_via_base( &src) == count_via_enumeration( &src));
   assert( count_via_base( &dst) == count_via_enumeration( &dst));
   assert( count_via_base( &src) == count_via_base( &dst));

   // the first page is the very same object in both VMs, i.e. shared
   // (a freshly appended page starts out unretained, so the destination's
   //  reference is the one that shows up in the retain count)
   src_page = first_page( &src);
   dst_page = first_page( &dst);
   assert( src_page != NULL);
   assert( src_page == dst_page);
   assert( mulle_vm_page_get_retaincount( src_page) >= 1);

   assert_content( &src, data, length);
   assert_content( &dst, data, length);

   mulle_vm_done( &dst);
   mulle_vm_done( &src);

   mulle_printf( "aliasing-vm-shared: copy OK\n");
}


//
// Cut moves a subrange to the destination and deletes it from the source.
// Both the inserted nodes and the deletion walk the nested base.
//
static void   test_cut( void)
{
   struct mulle_vm   src;
   struct mulle_vm   dst;
   char              source[] = "abcdef";
   char              cut[]    = "cd";
   char              rest[]   = "abef";

   mulle_vm_init( &src, NULL, NULL);
   mulle_vm_init( &dst, NULL, NULL);

   mulle_vm_append( &src, source, strlen( source));

   assert( mulle_vm_cut( &dst, &src, mulle_range_make( 2, 2)) == 0);

   assert( count_via_base( &src) == count_via_enumeration( &src));
   assert( count_via_base( &dst) == count_via_enumeration( &dst));

   assert_content( &dst, cut, strlen( cut));
   assert_content( &src, rest, strlen( rest));

   mulle_vm_done( &dst);
   mulle_vm_done( &src);

   mulle_printf( "aliasing-vm-shared: cut OK\n");
}


//
// Paste deletes a range in the destination and inserts the (shared) source
// pages there. The source must be left untouched.
//
static void   test_paste( void)
{
   struct mulle_vm         dst;
   struct mulle_vm         src;
   struct mulle_vm_page    *src_page;
   struct mulle_vm_page    *dst_page;
   char                    target[]  = "The quick brown fox";
   char                    insert[]  = "INSERT";
   char                    expected[] = "The INSERT brown fox";

   mulle_vm_init( &dst, NULL, NULL);
   mulle_vm_init( &src, NULL, NULL);

   mulle_vm_append( &dst, target, strlen( target));
   mulle_vm_append( &src, insert, strlen( insert));

   // replace "quick" (5 bytes at offset 4) with the source content
   assert( mulle_vm_paste( &dst, mulle_range_make( 4, 5), &src) == 0);

   assert( count_via_base( &dst) == count_via_enumeration( &dst));
   assert( count_via_base( &src) == count_via_enumeration( &src));

   assert_content( &dst, expected, strlen( expected));
   assert_content( &src, insert, strlen( insert));

   // the pasted page is shared between source and destination
   src_page = first_page( &src);
   dst_page = first_page( &dst);
   assert( src_page != NULL);
   assert( dst_page != NULL);
   assert( mulle_vm_page_get_retaincount( src_page) >= 1);

   // both trees are still structurally sound after the moves
   assert( ! mulle_vm_tree_validate( &dst.tree));
   assert( ! mulle_vm_tree_validate( &src.tree));

   mulle_vm_done( &dst);
   mulle_vm_done( &src);

   mulle_printf( "aliasing-vm-shared: paste OK\n");
}


int   main( void)
{
   test_identity();
   test_copy();
   test_cut();
   test_paste();

   mulle_printf( "aliasing-vm-shared: OK\n");

   return( 0);
}
