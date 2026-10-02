//
//  mulle-vm-tree.h
//  mulle-vm
//
//  Copyright (c) 2025 Nat! - Mulle kybernetiK.
//  All rights reserved.
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
#ifndef mulle_vm_tree_h__
#define mulle_vm_tree_h__

#include "include.h"
#include "mulle-vm-page.h"
#include <stddef.h>


//
// So the idea with mulle_vm_tree is that you store mulle_vm_pages in it.
// The actual extra node data we manage on the mulle_rbtree such a
// mulle_vm_tree really is, also contains a total which we need to manage
// the tree itself. The actual struct mulle_rbnode that contains our
// page pointer and the total is supposed to be an implementation detail,
// that's abstracted away by this mulle_vm_tree. The mulle_vm_tree can give
// you a mulle_vm_page based on an address "location", the search result
// will also return an offset into the used area of this page for the given
// location.
//

//
// Forward declare mulle_vm_tree as rbtree
//
struct mulle_vm_tree
{
   struct mulle_rbtree   rbtree;
};

//
// Length tree node structure stored in extra data
//
struct mulle_vm_node
{
   /** Pointer to the actual page containing the data */
   /* also contains page size */
   struct mulle_vm_page   *page;

   /* the total value we use for lookup, insertion and reordering */
   size_t                 total;
};


MULLE__VM_GLOBAL
char  *mulle_vm_node_validate( struct mulle_vm_node *node);



//
// Search result structure
//
struct mulle_vm_tree_search_result
{
   struct mulle_vm_node   *node;
   uintptr_t              offset;
};


MULLE__VM_GLOBAL
void  mulle_vm_tree_init( struct mulle_vm_tree *tree,
                          struct mulle_allocator *allocator);


static inline void  mulle_vm_tree_done( struct mulle_vm_tree *tree)
{
   if( tree)
      mulle_rbtree_done( &tree->rbtree);
}


static inline struct mulle_allocator *
   mulle_vm_tree_get_allocator( struct mulle_vm_tree *tree)
{
   return( tree ? mulle_rbtree_get_allocator( &tree->rbtree) : NULL);
}


static inline void  mulle_vm_tree_reset( struct mulle_vm_tree *tree)
{
   struct mulle_allocator   *allocator;

   allocator = mulle_vm_tree_get_allocator( tree);
   mulle_vm_tree_done( tree);
   mulle_vm_tree_init( tree, allocator);
}



MULLE__VM_GLOBAL
struct mulle_vm_node *   _mulle_vm_tree_add_page( struct mulle_vm_tree *tree,
                                                  struct mulle_vm_page *page);

static inline
struct mulle_vm_node *   mulle_vm_tree_add_page( struct mulle_vm_tree *tree,
                                                struct mulle_vm_page *page)
{
   if( ! tree)
      return( NULL);

   return( _mulle_vm_tree_add_page( tree, page));
}



MULLE__VM_GLOBAL
struct mulle_vm_node *   _mulle_vm_tree_insert_page_after_node( struct mulle_vm_tree *tree,
                                                                struct mulle_vm_page *page,
                                                                struct mulle_vm_node *node);

static inline struct mulle_vm_node *
   mulle_vm_tree_insert_page_after_node( struct mulle_vm_tree *tree,
                                         struct mulle_vm_page *page,
                                         struct mulle_vm_node *node)
{
   if( ! tree)
      return( NULL);
   return( _mulle_vm_tree_insert_page_after_node( tree, page, node));
}


MULLE__VM_GLOBAL
struct mulle_vm_node *
   _mulle_vm_tree_insert_page_before_node( struct mulle_vm_tree *tree,
                                           struct mulle_vm_page *page,
                                           struct mulle_vm_node *node);

static inline struct mulle_vm_node *
   mulle_vm_tree_insert_page_before_node( struct mulle_vm_tree *tree,
                                          struct mulle_vm_page *page,
                                          struct mulle_vm_node *node)
{
   if( ! tree)
      return( NULL);
   return( _mulle_vm_tree_insert_page_before_node( tree, page, node));
}



// exhaustive and bad!!
MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove_page( struct mulle_vm_tree *tree, struct mulle_vm_page *page);

// Add mulle_vm_tree_remove
static inline int
   mulle_vm_tree_remove_page( struct mulle_vm_tree *tree, struct mulle_vm_page *page)
{
   if( ! tree)
      return( 0);

   return( _mulle_vm_tree_remove_page( tree, page));
}



MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove_node( struct mulle_vm_tree *tree, struct mulle_vm_node *node);


// Add mulle_vm_tree_remove
static inline int
   mulle_vm_tree_remove_node( struct mulle_vm_tree *tree, struct mulle_vm_node *node)
{
   if( ! tree)
      return( 0);

   return( _mulle_vm_tree_remove_node( tree, node));
}


MULLE__VM_GLOBAL
char  *mulle_vm_tree_validate( struct mulle_vm_tree *tree);


MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST
int   _mulle_vm_tree_remove( struct mulle_vm_tree *tree, uintptr_t position);

// Add mulle_vm_tree_remove
static inline int
   mulle_vm_tree_remove( struct mulle_vm_tree *tree, uintptr_t position)
{
   if( ! tree)
      return( 0);

   return( _mulle_vm_tree_remove( tree, position));
}


// Search result helper functions
static inline int
   mulle_vm_tree_search_result_is_valid( struct mulle_vm_tree_search_result result)
{
   return( result.node != NULL);
}


static inline struct mulle_rbnode *
   mulle_vm_tree_search_result_get_rbnode( struct mulle_vm_tree_search_result result)
{
   // The rbnode contains the vm_node as extra data
   // The extra data starts at &rbnode->payload  
   // So we need to get back to the rbnode from the node
   if( ! result.node)
      return( NULL);
   return( (struct mulle_rbnode *) ((char *) result.node - offsetof( struct mulle_rbnode, payload)));
}


static inline struct mulle_vm_node *
   mulle_vm_tree_search_result_get_node( struct mulle_vm_tree_search_result result)
{
   return( result.node);
}


static inline struct mulle_vm_page *
   mulle_vm_tree_search_result_get_page( struct mulle_vm_tree_search_result result)
{
   if( ! result.node)
      return( NULL);
   return( result.node->page);
}


static inline uintptr_t
   mulle_vm_tree_search_result_get_offset( struct mulle_vm_tree_search_result result)
{
   return( result.offset);
}

// Function to get rbnode from vm_node
static inline struct mulle_rbnode *
   _mulle_vm_node_get_rbnode( struct mulle_vm_node *node)
{
   // The rbnode contains the vm_node as extra data
   // The extra data starts at &rbnode->payload
   // So we need to get back to the rbnode from the node
   return( (struct mulle_rbnode *) ((char *) node - offsetof( struct mulle_rbnode, payload)));
}


static inline struct mulle_vm_node *
   _mulle_vm_rbnode_get_vm_node( struct mulle_rbnode *rbnode)
{
   return( (struct mulle_vm_node *) ((char *) rbnode + offsetof( struct mulle_rbnode, payload)));
}


// Function to get rbnode from vm_node
static inline struct mulle_vm_page *
   _mulle_vm_node_get_page( struct mulle_vm_node *node)
{
   // The rbnode contains the vm_node as extra data
   // The extra data starts at &rbnode->payload
   // So we need to get back to the rbnode from the node
   return( node->page);
}


// Function to get rbnode from vm_node
static inline struct mulle_vm_page *
   mulle_vm_node_get_page( struct mulle_vm_node *node)
{
   // The rbnode contains the vm_node as extra data
   // The extra data starts at &rbnode->payload
   // So we need to get back to the rbnode from the node
   return( node ? node->page : NULL);
}



static inline int
   mulle_vm_tree_search_result_is_equal( struct mulle_vm_tree_search_result result,
                                         struct mulle_vm_tree_search_result other)
{
   return( result.node == other.node && result.offset == other.offset);
}


static inline struct mulle_vm_node *
   mulle_vm_tree_previous_node( struct mulle_vm_tree *tree,
                                struct mulle_vm_node *node)
{
   struct mulle__rbtree   *rb__tree = &tree->rbtree.base;
   struct mulle_rbnode    *rbnode;
   struct mulle_rbnode    *prev_rbnode;
   struct mulle_vm_node   *previous;

   if( ! tree || ! node)
      return( NULL);

   rbnode      = _mulle_vm_node_get_rbnode( node);
   prev_rbnode = _mulle__rbtree_previous_node( rb__tree, rbnode);
   if( prev_rbnode == _mulle__rbtree_get_nil_node( rb__tree))
      return( NULL);

   previous = _mulle_vm_rbnode_get_vm_node( prev_rbnode);
   return( previous);
}


static inline struct mulle_vm_node *
   mulle_vm_tree_next_node( struct mulle_vm_tree *tree,
                            struct mulle_vm_node *node)
{
   struct mulle__rbtree   *rb__tree = &tree->rbtree.base;
   struct mulle_rbnode    *rbnode;
   struct mulle_rbnode    *next_rbnode;
   struct mulle_vm_node   *next;

   if( ! tree || ! node)
      return( NULL);

   rbnode      = _mulle_vm_node_get_rbnode( node);
   next_rbnode = _mulle__rbtree_next_node( rb__tree, rbnode);
   if( next_rbnode == _mulle__rbtree_get_nil_node( rb__tree))
      return( NULL);

   next = _mulle_vm_rbnode_get_vm_node( next_rbnode);
   return( next);
}



MULLE__VM_GLOBAL
struct mulle_vm_tree_search_result
   _mulle_vm_tree_search( struct mulle_vm_tree *tree,
                          uintptr_t position);

MULLE__VM_GLOBAL
struct mulle_vm_page  *mulle_vm_tree_search( struct mulle_vm_tree *tree,
                                             uintptr_t position,
                                             uintptr_t *offset);


MULLE__VM_GLOBAL
size_t   mulle_vm_tree_get_total_length( struct mulle_vm_tree *tree);


MULLE__VM_GLOBAL
void   mulle_vm_tree_print( struct mulle_vm_tree *tree);


/**
 **
 **
 **/
#define mulle_vm_treeenumerator     mulle_rbtreeenumerator


static inline struct mulle_vm_treeenumerator
   mulle_vm_tree_enumerate( struct mulle_vm_tree *tree)
{
   return( mulle_rbtree_enumerate( &tree->rbtree));
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_treeenumerator_next( struct mulle_vm_treeenumerator *rover,
                                 struct mulle_vm_node **node)
{
   return( _mulle_rbtreeenumerator_next( rover, (void **) node));
}


static inline void
   mulle_vm_treeenumerator_done( struct mulle_vm_treeenumerator *rover)
{
   _mulle_rbtreeenumerator_done( rover);
}


#define mulle_vm_tree_for( tree_ptr, node)                                                    \
   for( struct mulle_vm_treeenumerator _rover__ ## node = mulle_vm_tree_enumerate( tree_ptr), \
           *_rover__ ## node ## __i = (void *) 0;                                             \
        ! _rover__ ## node ## __i;                                                            \
        _rover__ ## node ## __i = ((void *) 1))                                               \
      for( struct mulle_vm_node *node;                                                        \
           mulle_vm_treeenumerator_next( &_rover__ ## node, &node); )


// reverse
#define mulle_vm_treereverseenumerator     mulle_rbtreereverseenumerator


static inline struct mulle_vm_treereverseenumerator
   mulle_vm_tree_reverseenumerate( struct mulle_vm_tree *tree)
{
   return( mulle_rbtree_reverseenumerate( &tree->rbtree));
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_treereverseenumerator_next( struct mulle_vm_treereverseenumerator *rover,
                                        struct mulle_vm_node **node)
{
   return( _mulle_rbtreereverseenumerator_next( rover, (void **) node));
}


static inline void
   mulle_vm_treereverseenumerator_done( struct mulle_vm_treereverseenumerator *rover)
{
   _mulle_rbtreereverseenumerator_done( rover);
}


#define mulle_vm_tree_for_reverse( tree_ptr, node)                                                          \
   for( struct mulle_vm_treereverseenumerator _rover__ ## node = mulle_vm_tree_reverseenumerate( tree_ptr), \
           *_rover__ ## node ## __i = (void *) 0;                                                           \
        ! _rover__ ## node ## __i;                                                                          \
        _rover__ ## node ## __i = ((void *) 1))                                                             \
      for( struct mulle_vm_node *node;                                                                      \
           mulle_vm_treereverseenumerator_next( &_rover__ ## node, &node); )

#endif
