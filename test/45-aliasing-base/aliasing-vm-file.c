//
//  aliasing-vm-file.c
//  mulle-vm
//
//  Regression test for the file-backed VM path
//  (mulle_vm_init_with_contents_of_file / mulle_vm_writev_contents) and the
//  nested-member layout it reads through:
//
//      struct mulle_vm        -> struct mulle_vm_tree  tree
//      struct mulle_vm_tree   -> struct mulle_rbtree   rbtree
//      struct mulle_rbtree    -> struct mulle__rbtree  base
//
//  The file path is the one place where the old code punned the VM directly:
//
//      _mulle__rbtree_get_count( (struct mulle__rbtree *) &vm->tree)
//      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm)
//
//  i.e. it accessed a `struct mulle_vm` through a pointer to an unrelated
//  type. That is a strict-aliasing violation (C11 6.5p7) which GCC can
//  exploit at -O2/-O3, so the page count and the writev iovec array can be
//  computed against stale or zeroed tree state. The code now spells
//  `&vm->tree.rbtree.base`.
//
//  The hard guarantee is the layout: the `offsetof` asserts stop compiling if
//  a member is removed or moved. The live test then loads a real file, reads
//  the count/length through the nested base, enumerates and writes the VM out
//  again through the iovec path, which is exactly what the old cast fed.
//
#ifdef NDEBUG
# undef NDEBUG
#endif
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "include.h"
#include <mulle-vm/mulle-vm-file.h>


_Static_assert( offsetof( struct mulle_rbtree, base) == 0,
                "mulle_rbtree.base must be the first member");
_Static_assert( offsetof( struct mulle_vm_tree, rbtree) == 0,
                "mulle_vm_tree.rbtree must be the first member");
_Static_assert( offsetof( struct mulle_vm, tree) == 0,
                "mulle_vm.tree must be the first member");


#define FILE_PATH     "test_data.txt"
#define FILE_CONTENT  "VfL Bochum 1848\n"
#define FILE_LENGTH   16


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


static void   test_identity( void)
{
   struct mulle_vm   vm;
   struct mulle_vm   *p;

   memset( &vm, 0, sizeof( vm));
   p = &vm;

   assert( (void *) p == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree == (void *) &p->tree.rbtree.base);
   assert( (void *) &p->tree.rbtree == (void *) &p->tree.rbtree.base);

   mulle_printf( "aliasing-vm-file: identity OK\n");
}


//
// The tree options set up by mulle_vm_init_with_contents_of_file must be
// observable through both the VM view and the innermost base view.
//
static void   test_options_roundtrip( struct mulle_vm *vm)
{
   unsigned int   options;

   options = _vm_get_options( vm);
   assert( options & mulle_rbtree_option_use_dirty);

   _vm_set_options( vm, mulle_rbtree_option_allow_duplicates);
   assert( __rbtree_get_options( &vm->tree.rbtree.base)
           == mulle_rbtree_option_allow_duplicates);

   __rbtree_get_options( &vm->tree.rbtree.base);   // keep the view live
   _vm_set_options( vm, options);
   assert( _vm_get_options( vm) == options);

   mulle_printf( "aliasing-vm-file: options OK\n");
}


//
// Live file-backed VM. The loads happen before any tree mutation, so this is
// the pure read path the file loader and writev feed.
//
static void   test_live_file( void)
{
   struct mulle_vm              vm;
   struct mulle_vm_enumerator   rover;
   struct mulle_vm_page         *page;
   struct mulle_vm_statistics   stats;
   const void                   *data;
   char                         buf[ 64 ];
   char                         out[ 64 ];
   ssize_t                      n_read;
   int                          fds[ 2 ];
   int                          count;
   int                          rval;

   rval = mulle_vm_init_with_contents_of_file( &vm, FILE_PATH, NULL);
   assert( rval == 0);

   // the nested views all denote the same live object
   assert( (void *) &vm == (void *) &vm.tree.rbtree.base);

   test_options_roundtrip( &vm);

   // the page count, read through the nested base, is 1 for a whole-file map
   assert( _mulle__rbtree_get_count( &vm.tree.rbtree.base) == 1);
   assert( ! mulle_rbtree_is_empty( &vm.tree.rbtree));

   stats = mulle_vm_get_statistics( &vm);
   assert( stats.vm_size == FILE_LENGTH);
   assert( mulle_vm_get_length( &vm) == FILE_LENGTH);

   assert( ! mulle_vm_tree_validate( &vm.tree));

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

   // read the mapped content back through the VM
   data = mulle_vm_read( &vm, mulle_range_make( 0, FILE_LENGTH), buf);
   assert( data != NULL);
   assert( ! memcmp( data, FILE_CONTENT, FILE_LENGTH));

   // the iovec path recomputes the count through the nested base and must
   // write the exact mapped bytes
   assert( pipe( fds) == 0);

   rval = mulle_vm_writev_contents( &vm, fds[ 1 ]);
   assert( rval == FILE_LENGTH);
   close( fds[ 1 ]);

   memset( out, 0, sizeof( out));
   n_read = read( fds[ 0 ], out, sizeof( out));
   close( fds[ 0 ]);

   assert( n_read == FILE_LENGTH);
   assert( ! memcmp( out, FILE_CONTENT, FILE_LENGTH));

   mulle_vm_done( &vm);

   mulle_printf( "aliasing-vm-file: live file OK\n");
}


int   main( void)
{
   test_identity();
   test_live_file();

   mulle_printf( "aliasing-vm-file: OK\n");

   return( 0);
}
