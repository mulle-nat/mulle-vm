//
//  mulle-vm-tree.c
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
#include "mulle-vm-tree.h"

#include "include-private.h"
#include "mulle-vm-page.h"


//
//  Simple stub implementation for mulle-vm-tree
//  Just enough to compile and provide basic functionality
//

#include "include.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define NO_TRACE_FUZZ
//


//
// No forward declarations needed - functions are in mulle-vm-page.h
//

//
// Custom value callback functions for mulle_vm_node management
// The mulle_vm_node structure itself is part of the node extra data,
// so we only need to manage the page within it
//
static void   *mulle_vm_node_retain( const struct mulle_container_valuecallback *callback,
                                     void *p,
                                     struct mulle_allocator *allocator)
{
    struct mulle_vm_node *info = (struct mulle_vm_node *) p;

    mulle_vm_page_retain( info->page);
    return( info);
}


static void   mulle_vm_node_release( const struct mulle_container_valuecallback *callback,
                                     void *p,
                                     struct mulle_allocator *allocator)
{
    struct mulle_vm_node *info = (struct mulle_vm_node *) p;

    mulle_vm_page_release( info->page);
}



static char *   mulle_vm_node_describe( const struct mulle_container_valuecallback *callback,
                                        void *p,
                                        struct mulle_allocator **p_allocator)
{
   struct mulle_vm_node *info = (struct mulle_vm_node *) p;
   char   *s;
   struct mulle_allocator *allocator = *p_allocator;

   mulle_allocator_asprintf( allocator, &s,
                                 "{ page=%p, length=%zu, total=%zu}",
                                 info->page, info->page->used.length, info->total);

   return( s);
}




// Custom value callback structure
static struct mulle_container_valuecallback   mulle_vm_node_callback =
{
    .retain   = mulle_vm_node_retain,
    .release  = mulle_vm_node_release,
    .describe = mulle_vm_node_describe,
    .userinfo = NULL
};


//
// Dirty callback function for maintaining subtree totals
//
static void   mulle_vm_tree_dirty_callback( void *value, void *left, void *right)
{
    struct mulle_vm_node *n = (struct mulle_vm_node *) value;
    struct mulle_vm_node *l = (struct mulle_vm_node *) left;
    struct mulle_vm_node *r = (struct mulle_vm_node *) right;

    // total = left_subtree_total + current_length + right_subtree_total
    n->total = (l ? l->total : 0) +
               n->page->used.length +
               (r ? r->total : 0);
}

//
// Position-based comparison function for length nodes
// For now, just use pointer comparison to get a working tree
//
static int   mulle_vm_node_compare( void *a, void *b)
{
    struct mulle_vm_node *node_a = (struct mulle_vm_node *) a;
    struct mulle_vm_node *node_b = (struct mulle_vm_node *) b;

    // Simple pointer comparison for now - this makes all nodes unique
    if( node_a->page < node_b->page) return -1;
    if( node_a->page > node_b->page) return 1;
    return 0;
}


size_t  mulle_vm_tree_search_node_total( struct mulle_vm_tree *tree,
                                         struct mulle_vm_node *node)
{
   struct mulle__rbtree   *rb__tree = (struct mulle__rbtree *) tree;
   struct mulle_vm_node   *left;
   struct mulle_vm_node   *right;
   struct mulle_rbnode    *rbnode;
   struct mulle_rbnode    *left_rbnode;
   struct mulle_rbnode    *right_rbnode;
   struct mulle_rbnode    *nil_node;
   size_t                 total;

   if( ! tree || ! node)
      return( 0);

   nil_node = _mulle__rbtree_get_nil_node( rb__tree);

   rbnode = _mulle_vm_node_get_rbnode( node);
   assert( rbnode != nil_node);

   total = node->page->used.length;

   left_rbnode = rbnode->_left;
   if( left_rbnode != nil_node)
   {
      left   = _mulle_vm_rbnode_get_vm_node( left_rbnode);
      total += mulle_vm_tree_search_node_total( tree, left);
   }

   right_rbnode = rbnode->_right;
   if( right_rbnode != nil_node)
   {
      right  = _mulle_vm_rbnode_get_vm_node( right_rbnode);
      total += mulle_vm_tree_search_node_total( tree, right);
   }

   return( total);
}

//
// Initialize length tree with proper configuration
//
void  mulle_vm_tree_init( struct mulle_vm_tree *tree,
                          struct mulle_allocator *allocator)
{
   if( ! tree)
      return;

   struct mulle_rbtree_config   config =
   {
      .comparison = mulle_vm_node_compare,
      .dirty      = mulle_vm_tree_dirty_callback,
      .node_extra = sizeof( struct mulle_vm_node),
      .callback   = &mulle_vm_node_callback,
      .options    = mulle_rbtree_option_use_extra | mulle_rbtree_option_use_dirty
   };

   mulle_rbtree_init_with_config( (struct mulle_rbtree *) tree, &config, allocator);

   if( _mulle__rbtree_get_extra_size( (struct mulle__rbtree *) tree) < sizeof(struct mulle_vm_node))
     abort();
}


// Calculate total length of all text in tree
//
size_t   mulle_vm_tree_get_total_length( struct mulle_vm_tree *tree)
{
   struct mulle_vm_node   *node;
   struct mulle_rbnode    *root;
   struct mulle_rbnode    *nil;

   if( ! tree)
      return( 0);

   nil  = _mulle__rbtree_get_nil_node( (struct mulle__rbtree *) tree);
   root = _mulle__rbtree_get_root_node( (struct mulle__rbtree *) tree);

   if( root == nil)
      return( 0);

   node = _mulle__rbtree_get_node_value( (struct mulle__rbtree *) tree, root);
   return( node->total);
}


char  *mulle_vm_tree_validate_total_length( struct mulle_vm_tree *tree)
{
   struct mulle_vm_node   *node;
   size_t                 total;

   total = 0;
   mulle_rbtree_for( (struct mulle_rbtree *) tree, node)
   {
      if( mulle_vm_tree_search_node_total( tree, node) != node->total)
         return( "node total is wrong");

      total += node->page->used.length;
   }

   if( total != mulle_vm_tree_get_total_length( tree))
      return( "tree total is wrong");
   return( NULL);
}


char  *mulle_vm_tree_validate( struct mulle_vm_tree *tree)
{
   char   *err;

   assert( ! _mulle__rbtree_is_dirty( (struct mulle__rbtree *) tree));

   err = mulle__rbtree_validate( (struct mulle__rbtree *) tree);
   if( err)
      return( err);

   return( mulle_vm_tree_validate_total_length( (struct mulle_vm_tree *) tree));
}


static int   always_right_comparison( void *a, void *b)
{
   return( +1);
}


struct mulle_vm_node *
   mulle_vm_tree_new_node( struct mulle_vm_tree *tree,
                           struct mulle_vm_page *page)
{
   struct mulle_vm_node     extra = { 0 };
   struct mulle_rbnode      *rbnode;
   struct mulle_vm_node     *node;
   struct mulle_allocator   *allocator;
   struct mulle_rbtree      *rb_tree  = (struct mulle_rbtree *) tree;
   struct mulle__rbtree     *rb__tree = (struct mulle__rbtree *) rb_tree;
   void                     *value;

   extra.page  = page;
   extra.total = 0;  // Will be calculated by dirty callback

   // need payload for comparison before insertion, don't change it
   // afterwards
   allocator   = _mulle_rbtree_get_allocator( rb_tree);
   value       = mulle_vm_node_callback.retain( &mulle_vm_node_callback,
                                                &extra,
                                                allocator);
   rbnode      = _mulle__rbtree_new_node( rb__tree, value);
   node        = _mulle_rbnode_get_extra( rbnode);

   return( node);
}

//
// Append a node to the end of the VM space
//
struct mulle_vm_node *    _mulle_vm_tree_add_page( struct mulle_vm_tree *tree, struct mulle_vm_page *page)
{
   struct mulle_vm_node     *node;
   struct mulle_rbnode      *rbnode;
   struct mulle__rbtree     *rb__tree = (struct mulle__rbtree *) tree;

   node   = mulle_vm_tree_new_node( tree, page);
   rbnode = _mulle_vm_node_get_rbnode( node);
   // this can't fail with an return code (only raise)
   _mulle__rbtree_insert_node( rb__tree,
                               rbnode,
                               always_right_comparison);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) tree);
#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( (struct mulle_rbtree *) tree));
#endif
   return( node);
}



struct mulle_vm_node *   _mulle_vm_tree_insert_page_before_node( struct mulle_rbtree *tree,
                                                                 struct mulle_vm_page *page,
                                                                 struct mulle_vm_node *node)
{
   struct mulle_vm_node   *new_node;
   struct mulle_rbnode    *rbnode;
   struct mulle_rbnode    *new_rbnode;
   struct mulle__rbtree   *rb__tree = (struct mulle__rbtree *) tree;

   rbnode     = _mulle_vm_node_get_rbnode( node);

   new_node   = mulle_vm_tree_new_node( (struct mulle_vm_tree *) tree, page);
   new_rbnode = _mulle_vm_node_get_rbnode( new_node);
   // this can't fail with an return code (only raise)
   _mulle__rbtree_insert_node_before_node( rb__tree,
                                           new_rbnode,
                                           rbnode);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) tree);
#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( (struct mulle_rbtree *) tree));
#endif
   return( new_node);
}



struct mulle_vm_node *   _mulle_vm_tree_insert_page_after_node( struct mulle_rbtree *tree,
                                                                struct mulle_vm_page *page,
                                                                struct mulle_vm_node *node)
{
   struct mulle_vm_node   *new_node;
   struct mulle_rbnode    *rbnode;
   struct mulle_rbnode    *new_rbnode;
   struct mulle__rbtree   *rb__tree = (struct mulle__rbtree *) tree;

   rbnode     = _mulle_vm_node_get_rbnode( node);

   new_node   = mulle_vm_tree_new_node( (struct mulle_vm_tree *) tree, page);
   new_rbnode = _mulle_vm_node_get_rbnode( new_node);
   // this can't fail with an return code (only raise)
   _mulle__rbtree_insert_node_after_node( rb__tree,
                                          new_rbnode,
                                          rbnode);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) tree);
#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( (struct mulle_rbtree *) tree));
#endif
   return( new_node);
}


int   _mulle_vm_tree_remove_node( struct mulle_vm_tree *tree,
                                  struct mulle_vm_node *node)
{
   struct mulle_rbnode   *rbnode;
   struct mulle_rbtree   *rb_tree = (struct mulle_rbtree *) tree;

   rbnode = _mulle_vm_node_get_rbnode( node);
   _mulle_rbtree_remove_node( rb_tree, rbnode);
   return( 0);
}


int   _mulle_vm_tree_remove( struct mulle_vm_tree *tree, uintptr_t position)
{
   struct mulle_vm_tree_search_result  result;

   result = _mulle_vm_tree_search( tree, position);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      errno = ENOENT;
      return( -1);
   }

   return( _mulle_vm_tree_remove_node( tree, result.node));
}


int   _mulle_vm_tree_remove_page( struct mulle_vm_tree *tree,
                                  struct mulle_vm_page *page)
{
   struct mulle_vm_node   *found;

   found = NULL;
   mulle_vm_tree_for( tree, node)
   {
      if( page == node->page)
      {
         found = node;
         break;
      }
   }

   if( found)
      return( _mulle_vm_tree_remove_node( tree, found));

   errno = ENOENT;
   return( -1);
}


struct mulle_vm_tree_search_result
   _mulle_vm_tree_search_trace( struct mulle_vm_tree *tree,
                                uintptr_t position)
{
   struct mulle__rbtree                 *rb__tree = (struct mulle__rbtree *) tree;
   struct mulle_rbnode                  *nil;
   struct mulle_rbnode                  *rbnode;
   struct mulle_rbnode                  *root;
   struct mulle_vm_node                 *node;
   struct mulle_rbnode                  *left;
   struct mulle_vm_node                 *right_node;
   struct mulle_vm_node                 *left_node;
   struct mulle_vm_tree_search_result   result = { 0 };
   uintptr_t                            search_position;
   size_t                               left_total;
   size_t                               length;

   nil  = _mulle__rbtree_get_nil_node( rb__tree);
   root = _mulle__rbtree_get_root_node( rb__tree);

   mulle_fprintf( stderr, "Start search for %td\n", position);
   search_position = position;

   for( rbnode = root; rbnode != nil;)
   {
       node       = _mulle__rbtree_get_node_value( rb__tree, rbnode);

       // Get left subtree total
       length     = node->page->used.length;
       left_total = 0;
       mulle_fprintf( stderr, "Visiting node with page %p with length %ld and total %td\n",
                              node->page,
                              node->page->used.length,
                              node->total);
       left       = rbnode->_left;
       if( left != nil)
       {
           left_node = _mulle__rbtree_get_node_value( rb__tree, left);
           mulle_fprintf( stderr, "Have left node with page %p with length %ld and total %td\n",
                                  left_node->page,
                                  left_node->page->used.length,
                                  left_node->total);
           left_total = left_node->total;
       }

       if( rbnode->_right != nil)
       {
           right_node = _mulle__rbtree_get_node_value( rb__tree, rbnode->_right);
           mulle_fprintf( stderr, "Have right node with page %p with length %ld and total %td\n",
                                  right_node->page,
                                  right_node->page->used.length,
                                  right_node->total);
       }

       // Case 1: Go left (position is in left subtree)
       if( search_position < left_total)
       {
           mulle_fprintf( stderr, "%td < %td going left\n", search_position, left_total);
           rbnode = left;
           continue;
       }

       // Case 2: Found it (position is in current rbnode)
       if( search_position < left_total + length)
       {
          mulle_fprintf( stderr, "Found it at %td < %td\n", search_position, left_total + length);
          result.node   = node;
          result.offset = search_position - left_total;
          break;
       }

       mulle_fprintf( stderr, "Going right %td >= %td (now searching for %td)\n",
                                    search_position, left_total + length,
                                    search_position - (left_total + length));

       // Case 3: Go right (position is in right subtree)
       search_position -= (left_total + length);

       rbnode           = rbnode->_right;
   }

   if( rbnode == nil)
      mulle_fprintf( stderr, "Not found\n");
   return( result);
}


//
// Find node at a specific position using manual tree traversal
// Returns the node containing the position and sets offset to the position within that node
// Position is 0-based (first character is position 0)
//

struct mulle_vm_tree_search_result
   _mulle_vm_tree_search( struct mulle_vm_tree *tree,
                          uintptr_t position)
{
   struct mulle__rbtree                 *rb__tree = (struct mulle__rbtree *) tree;
   struct mulle_rbnode                  *nil;
   struct mulle_rbnode                  *rbnode;
   struct mulle_rbnode                  *root;
   struct mulle_vm_node                 *node;
   struct mulle_rbnode                  *left;
   struct mulle_vm_node                 *left_node;
   struct mulle_vm_tree_search_result   result = { 0 };
   uintptr_t                            search_position;
   size_t                               left_total;
   size_t                               length;

   nil  = _mulle__rbtree_get_nil_node( rb__tree);
   root = _mulle__rbtree_get_root_node( rb__tree);

   search_position = position;

   for( rbnode = root; rbnode != nil;)
   {
       node       = _mulle__rbtree_get_node_value( rb__tree, rbnode);

       // Get left subtree total
       length     = node->page->used.length;
       left_total = 0;

       left       = rbnode->_left;
       if( left != nil)
       {
           left_node  = _mulle__rbtree_get_node_value( rb__tree, left);
           left_total = left_node->total;
       }

       // Case 1: Go left (position is in left subtree)
       if( search_position < left_total)
       {
           rbnode = left;
           continue;
       }

       // Case 2: Found it (position is in current rbnode)
       if( search_position < left_total + length)
       {
          result.node   = node;
          result.offset = search_position - left_total;
          break;
       }

       // Case 3: Go right (position is in right subtree)
       search_position -= (left_total + length);

       rbnode           = rbnode->_right;
   }

   return( result);
}


struct mulle_vm_page  *mulle_vm_tree_search( struct mulle_vm_tree *tree,
                                             uintptr_t position,
                                             uintptr_t *offset)
{
   struct mulle_vm_tree_search_result   result;

   if( ! tree)
       return( NULL);

   result = _mulle_vm_tree_search( tree, position);
   if( ! mulle_vm_tree_search_result_is_valid( result))
      return( NULL);

   *offset = mulle_vm_tree_search_result_get_offset( result);
   return( mulle_vm_tree_search_result_get_page( result));
}



//
// Print tree structure for debugging
//
void   mulle_vm_tree_print( struct mulle_vm_tree *tree)
{
   struct mulle_vm_node            *rbnode;
   char                            *s;
   struct mulle_allocator          *allocator;

   mulle_rbtree_for( (struct mulle_rbtree *) tree, rbnode)
   {
      allocator = &mulle_allocator_stdlib;  // keep in loop
      s         = mulle_vm_node_callback.describe( &mulle_vm_node_callback, rbnode, &allocator);
      printf( "%s\n", s);
      mulle_allocator_free( allocator, s);
   }
}



#if 0
// not "API"
static struct mulle_rbnode *   _mulle_vm_tree_attach( struct mulle_rbtree *tree,
                                                      struct mulle_vm_page *page,
                                                      struct mulle_rbnode *parent,
                                                      int direction)
{
   struct mulle_vm_node   extra;
   void                   *value;
   struct mulle_rbnode    *node;
   struct mulle__rbtree   *rb__tree = (struct mulle__rbtree *) tree;

   extra.page   = page;
   extra.total  = 0;  // Will be calculated by dirty callback

   // need payload for comparison before insertion, don't change it
   // afterwards
   value = mulle_vm_node_callback.retain( &mulle_vm_node_callback,
                                          &extra,
                                          _mulle_rbtree_get_allocator( tree));
   node  = _mulle__rbtree_new_node( rb__tree, value);

   // Use the library's attach function
   if( direction < 0)
      _mulle__rbtree_insert_node_before_node( rb__tree, node, parent);
   else
      _mulle__rbtree_insert_node_after_node( rb__tree, node, parent);

   _mulle__rbtree_mark_node_as_dirty( rb__tree, node);

   return( node);
}


int   _mulle_vm_tree_insert( struct mulle_rbtree *tree,
                             struct mulle_vm_page *page,
                             uintptr_t position)
{
   struct mulle__rbtree        *rb__tree = (struct mulle__rbtree *) tree;
   struct mulle_allocator      *allocator;
   struct mulle_vm_node        *info;
   struct mulle_rbnode         *node;
   struct mulle_rbnode         *nil;
   char                        *tail_text;
   char                        *err;
   struct mulle_vm_tree_search_result   result;
   size_t                      total;

   nil       = _mulle__rbtree_get_nil_node(rb__tree);
   allocator = _mulle_rbtree_get_allocator(tree);

   // Find the node containing the position
   result = _mulle_vm_tree_search( tree, position);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      // check if we append
      total = mulle_vm_tree_get_total_length( tree);
      if( position != total)
      {
         errno = ERANGE;
         return( -1);
      }

      return( mulle_vm_tree_add_page( tree, page));
   }

   if( result.offset == 0)
   {
      // insert ahead ez
      (void) _mulle_vm_tree_attach( tree, text, result.node, -1);

      _mulle_rbtree_walk_dirty( tree);
#ifdef DEBUG_HARDCORE
      assert( ! mulle_vm_tree_validate( tree));
#endif
      return( 0);
   }


   // we
   // a) add new text behind the node
   // b) add tail text behind new text node
   // c) shrink original to head and mark it as dirty
   // --> tree should be balanced here
   // Store original values

   /*
    * a)
    */
   node = _mulle_vm_tree_attach( tree, page, result.node, +1);
   assert( node);

   // walk after first attachment, to get proper numbers
   _mulle_rbtree_walk_dirty( tree);

#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( tree));
#endif

   // we are already done: the "just append" case
   if( result.offset >= result.info->length)
      return( 0);  // Success, no error

   /*
    * b)
    */
   // gotta split the mulle_vm_page in two, so we actually need a
   // new page here
   // TODO: complicated
   tail_text = &result.info->text[ result.offset];
   node      = _mulle_vm_tree_attach( tree, tail_text, node, +1);
   if( ! node)
      return( "failed to hack original node");
   // Validate after second attachment
   _mulle_rbtree_walk_dirty(tree);

#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( tree));
#endif
   /*
    * c)
    */
   result.info->page->used.length = result.offset;
   // free pages we don't need now
   _mulle__rbtree_mark_node_as_dirty( rb__tree, result.node);

   _mulle_rbtree_walk_dirty( tree);
#ifdef DEBUG_HARDCORE
   assert( ! mulle_vm_tree_validate( tree));
#endif
   return( 0);
}
#endif



