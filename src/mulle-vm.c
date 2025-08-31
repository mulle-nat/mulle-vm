#include "include-private.h"

#include "mulle-vm.h"

#include <assert.h> // for assert
#include <stddef.h>
#include <string.h> // for memcpy, memmove

#include "mulle-vm-page.h"


#ifdef DEBUG
# define DEBUG_VERBOSE                 1
#endif
#define MULLE_VM_SHRINK_OSPAGE_USAGE   1


uint32_t   mulle_vm_get_version( void)
{
   return( MULLE__VM_VERSION);
}


struct mulle_vm_config     mulle_vm_config_default =
{
   .skew = MULLE__VM_SKEW_DEFAULT
};


/**
 **
 **
 **
 */
void   mulle_vm_init( struct mulle_vm *vm,
                      struct mulle_vm_config *config,
                      struct mulle_allocator *allocator)
{
   if( ! vm)
      return;

   if( ! config)
      config = &mulle_vm_config_default;

   memset( vm, 0, sizeof( *vm));

   mulle_vm_tree_init( &vm->tree, allocator);
   assert( config->skew <= 100 && "mulle-vm: VM skew must be >= 0 <= 100");

   vm->config = *config;
}


void   mulle_vm_done( struct mulle_vm *vm)
{
   if( ! vm)
      return;

   vm->config.cachesize = 0;

   mulle_vm_tree_done( &vm->tree);

   if( vm->config.shutdown)
      (*vm->config.shutdown)( vm);

}


void   mulle_vm_reset( struct mulle_vm *vm)
{
   if( ! vm)
      return;

   mulle_vm_tree_reset( &vm->tree);
}


static inline void   mulle_vm_assert( struct mulle_vm *vm)
{
#ifndef NDEBUG
   struct mulle_vm_page  *page;
   struct mulle_rbnode   *rbnode;
   char                  *complaint;

   complaint = mulle__rbtree_validate( (struct mulle__rbtree *) &vm->tree.rbtree);
   if( complaint)
   {
      mulle_fprintf( stderr, "tree corrupted: %s\n", complaint);
      abort();
   }

   complaint = mulle_vm_tree_validate( &vm->tree);
   if( complaint)
   {
      mulle_fprintf( stderr, "node corrupted: %s\n", complaint);
      abort();
   }

   mulle_vm_tree_for( &vm->tree, node)
   {
      page   = _mulle_vm_node_get_page( node);
      rbnode = _mulle_vm_node_get_rbnode( node);

      assert( page != NULL && "mulle-vm: mapping with no page");
      assert( page->used.length && "mulle-vm: used out of bounds");
      assert( (mulle_range_get_max( page->used) <= page->size) && "mulle-vm: used out of bounds");
      assert( ! _mulle_rbnode_is_dirty( rbnode));
   }
#endif
}


static inline void   mulle_vm_assert_page( struct mulle_vm *vm, struct mulle_vm_page *page)
{
   if( ! page || ! vm)
      return;

   // corruption check
   assert( (intptr_t) _mulle_atomic_pointer_read( &page->retain_count) >= 0 && "mulle-vm: VM page is over released");

   assert( page->bytes != NULL && "mulle-vm: VM page has no backing memory");

   assert( page->used.location + page->used.length <= page->size && "mulle-vm: VM page is corrupted");

   assert( (page->size % mulle_vm_get_pagesize()) == 0 && "mulle-vm: VM page is not a multiple of pagesize");

   assert(((uintptr_t) page->bytes % mulle_vm_get_pagesize()) == 0 && "mulle-vm: bytes of VM page are not size aligned");
   // pages must not have holes
}



// Helper to round size up to pagesize multiple
static inline size_t   round_up_to_pagesize( size_t size, size_t pagesize)
{
   return( ((size + pagesize - 1) / pagesize) * pagesize);
}


static inline size_t   round_down_to_pagesize( size_t size, size_t pagesize)
{
   return( (size / pagesize) * pagesize);
}


/**
 **
 ** COW
 **
 */

//
// this will possibly change the page in info and, if you
// cached the pointer to info->page and reuse it after _mulle_vm_will_write_page,
// you are in deep shit...
//
struct mulle_vm_page *   _mulle_vm_will_write_page( struct mulle_vm *vm,
                                                   struct mulle_vm_page *page)
{
   struct mulle_vm_page   *copy;
   void                   *bytes;
   size_t                 length;

   // if not shared we don't need to copy on write
   if( ! _mulle_atomic_pointer_read( &page->retain_count))
      return( page);

   //
   // copy on write
   //
   bytes  = mulle_vm_page_address( page, 0);
   length = page->used.length;
   copy   = _mulle_vm_new_page_with_bytes( vm, bytes, length);

   mulle_vm_page_release( page);
   return( copy);
}


/**
 **
 **
 **
 */
MULLE_C_NONNULL_FIRST_SECOND
void   _mulle_vm_append_ospages( struct mulle_vm *vm,
                                 void *pages,
                                 size_t length)
{
   size_t                 size;
   struct mulle_vm_page   *page;

   size = round_up_to_pagesize( length, mulle_vm_get_pagesize());
   page = mulle_vm_page_alloc();
   mulle_vm_page_init( page, pages, mulle_range_make( 0, length), size);
   mulle_vm_tree_add_page( &vm->tree, page);
}



struct mulle_vm_page *
   _mulle_vm_copy_page( struct mulle_vm *vm,
                        struct mulle_vm_page *page,
                        struct mulle_range copy_range)
{
   struct mulle_vm_page   *copy;
   void                   *bytes;

   // figure out what we need
   assert( mulle_range_get_max( copy_range) <= page->used.length);

   bytes  = mulle_vm_page_address( page, copy_range.location);
   copy   = _mulle_vm_new_page_with_bytes( vm, bytes, copy_range.length);

   return( copy);
}



// Splits the page at 'at'  offset into used size
//
// If 'at' == 0 returns NULL (no split).
// Otherwise, shrinks info->page to head, inserts new tail page after it,
// and returns pointer to new tail page.
//
// Precondition: vm, info, and info->page are valid; at is within the used range.

struct mulle_vm_node  *
   _mulle_vm_split( struct mulle_vm *vm,
                    struct mulle_vm_tree_search_result result,
                    uintptr_t at)
{
   struct mulle_rbnode    *rbnode;
   struct mulle_vm_page   *page;
   struct mulle_vm_page   *tail;
   struct mulle_vm_node   *node;
   size_t                 head_len;
   size_t                 tail_len;
   void                   *tail_bytes;

   assert( vm);
   assert( mulle_vm_tree_search_result_is_valid( result));

   // If 'at' is at the start of the page's used range, no splitting needed
   if( at == 0)
   {
      return( NULL);
   }

   page = mulle_vm_tree_search_result_get_page( result);
   assert( page);
   assert( at <= page->used.length);

   // Compute lengths of head and tail parts
   head_len = at;
   tail_len = page->used.length - head_len;

   if( tail_len == 0)
   {
      return( NULL); // Nothing to split off
   }

   // Prepare the "tail" bytes pointer inside the old page
   tail_bytes = (uint8_t *) page->bytes + page->used.location + at;

   // Create new page for the tail part with the tail bytes copied
   tail = _mulle_vm_new_page_with_bytes( vm, tail_bytes, tail_len);

   // Copy-on-write if needed before modifying original page
   page = _mulle_vm_will_write_page( vm, page);
   result.node->page = page; // Update tree node after potential COW

   // Shrink original page to only hold the head part
   assert( page->used.location + head_len <= page->size);
   page->used.length = head_len;

   // Mark the node as dirty and recalculate tree totals
   rbnode = _mulle_vm_node_get_rbnode( result.node);
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);

   // Insert new tail page right after the original page in the VM
   node = mulle_vm_tree_insert_page_after_node( &vm->tree, tail, result.node);

   // now its retained by the tree so release it here
   mulle_vm_page_release( tail);

   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

   // Return pointer to the newly inserted tail page
   return( node);
}


//
// this function will not walk dirty!!
// the node you pass in may become invalid!!
//
void   _mulle_vm_try_coalesce( struct mulle_vm *vm,
                               struct mulle_vm_node *node)
{
   int                    rval;
   size_t                 node_front_available;
   size_t                 node_back_available;
   size_t                 left_back_available;
   size_t                 right_front_available;
   size_t                 left_available;
   size_t                 right_available;
   size_t                 node_available;
   size_t                 node_front_occupied;
   size_t                 node_back_occupied;
   size_t                 pagesize;
   struct mulle_vm_node   *left;
   struct mulle_vm_node   *right;
   struct mulle_vm_page   *lpage;
   struct mulle_vm_page   *rpage;
   struct mulle_vm_page   *page;
   struct mulle_range     copy_range;
   int                    coalesce_front;
   int                    coalesce_back;

   //if( ! node)
      return;

   //
   // check if the front and back page are filling half a page.if they do
   // we just don't bother with coalescing
   //
   page                  = mulle_vm_node_get_page( node);
   node_front_available  = mulle_vm_page_get_available_front( page);
   node_back_available   = mulle_vm_page_get_available_back( page);
   pagesize              = mulle_vm_get_pagesize();
   if( node_front_available < pagesize / 2 && node_back_available < pagesize / 2)
      return;

   left            = mulle_vm_tree_previous_node( &vm->tree, node);
   right           = mulle_vm_tree_next_node( &vm->tree, node);

   lpage           = mulle_vm_node_get_page( left);
   rpage           = mulle_vm_node_get_page( right);

   node_available  = mulle_vm_page_get_available( page);
   left_available  = mulle_vm_page_get_available( lpage);
   right_available = mulle_vm_page_get_available( rpage);

   // Anothe easy cop out...
   // Can we get (theoretically) a page back at all ?
   // Else we don't bother...
   if( left_available + node_available < pagesize
       && right_available + node_available < pagesize)
      return;

   node_front_occupied = mulle_vm_page_get_used_length_first_ospage( page, pagesize);
   node_back_occupied  = mulle_vm_page_get_used_length_last_ospage( page, pagesize);

   left_back_available = mulle_vm_page_get_available_back( lpage);
//   left_back_occupied    = pagesize - (left_back_available > pagesize ? pagesize : left_back_available);

   //
   // Can we offload from front page to previous node ?
   // a) yes if back space is large enough
   // b) yes if its a singlepage node and there is enough available space total
   //
   coalesce_front = left_back_available >= node_front_occupied
                    || (left_available >= node_front_occupied && lpage->size == pagesize);

   if( coalesce_front)
   {
      lpage      = _mulle_vm_will_write_page( vm, lpage);
      left->page = lpage;

      assert( node_front_occupied <= page->used.length);
      copy_range = mulle_range_make( page->used.location, node_front_occupied);
      rval       = _mulle_vm_page_insert_bytes( lpage,
                                                lpage->used.length,
                                                &((char *) page->bytes)[ copy_range.location],
                                                copy_range.length);
      assert( ! rval);
      // and mark as dirty
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree,
                                         _mulle_vm_node_get_rbnode( left));

      page                 = _mulle_vm_will_write_page( vm, page);
      node->page           = page;

      assert( page->used.length >= copy_range.length);
      page->used.location += copy_range.length;
      page->used.length   -= copy_range.length;
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree,
                                         _mulle_vm_node_get_rbnode( node));

      if( ! page->used.length)
      {
         // complete remove node and return
         mulle_vm_tree_remove_node( &vm->tree, node);
         return;
      }

      node_back_available = mulle_vm_page_get_available_back( page);
      node_back_occupied  = mulle_vm_page_get_used_length_last_ospage( page, pagesize);
   }

   right_front_available = mulle_vm_page_get_available_front( rpage);
//   right_front_occupied  = pagesize - (right_front_available > pagesize ? pagesize : right_front_available);

   coalesce_back = right_front_available >= node_back_occupied
                   || (right_available >= node_back_occupied && rpage->size == pagesize);
   if( coalesce_back)
   {
      rpage       = _mulle_vm_will_write_page( vm, rpage);
      right->page = rpage;

      assert( node_back_occupied <= page->used.length);
      copy_range = mulle_range_make( mulle_range_get_max( page->used) - node_back_occupied,
                                     node_back_occupied);
      rval       = _mulle_vm_page_insert_bytes( rpage,
                                                0,
                                                &((char *) page->bytes)[ copy_range.location],
                                                copy_range.length);
      assert( ! rval);

      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree,
                                         _mulle_vm_node_get_rbnode( right));

      page                 = _mulle_vm_will_write_page( vm, page);
      node->page           = page;

      assert( page->used.length >= copy_range.length);
      page->used.location += copy_range.length;
      page->used.length   -= copy_range.length;
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree,
                                         _mulle_vm_node_get_rbnode( node));

      if( ! page->used.length)
      {
         mulle_vm_tree_remove_node( &vm->tree, node);
         return;
      }
   }

   mulle_vm_page_free_unused_ospages( page);
}


static void   _mulle_vm_insert_into_large_enough_node( struct mulle_vm *vm,
                                                       struct mulle_vm_node *node,
                                                       uintptr_t offset,
                                                       void *bytes,
                                                       size_t length)
{
   struct mulle_rbnode   *rbnode;
   struct mulle_vm_page  *page;
#ifndef NDEBUG
   int                   rval;
#endif

   if( ! length)
      return;

   page = _mulle_vm_node_get_page( node);
   page = _mulle_vm_will_write_page( vm, page);
   rval = _mulle_vm_page_insert_bytes( page, offset, bytes, length);
   assert( rval == 0);
   rbnode = _mulle_vm_node_get_rbnode( node);
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
}



int   _mulle_vm_insert( struct mulle_vm *vm,
                        struct mulle_range range,
                        void *bytes)
{
   size_t                               offset;
   size_t                               total;
   size_t                               tail_available;
   size_t                               available;
   size_t                               front_available;
   size_t                               front_length;
   size_t                               insert_length;
   struct mulle_vm_node                 *tail_node;
   struct mulle_vm_node                 *insert_node;
   struct mulle_vm_node                 *node;
   struct mulle_vm_page                 *tail_page;
   struct mulle_vm_page                 *insert_page;
   struct mulle_vm_page                 *page;
   struct mulle_vm_tree_search_result   result;

   assert( vm);
   assert( bytes);

   insert_length = range.length;
   if( ! insert_length)
      return( 0);

   /* empty VM: create first page */
   /* split page if needed */
   total = mulle_vm_get_length( vm);

   if( range.location >= total)
   {
      if( range.location != total)
      {
         errno = ERANGE;
         return( -1);
      }

      if( ! total)
      {
         insert_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
         mulle_vm_tree_add_page( &vm->tree, insert_page);
         mulle_vm_page_release( insert_page);
         // nothing to coalesce for sure
         return( 0);
      }

      result = _mulle_vm_tree_search( &vm->tree, total - 1);
      assert( mulle_vm_tree_search_result_is_valid( result));
      page   = mulle_vm_tree_search_result_get_page( result);
      offset = page->used.length;
   }
   else
   {
      result = _mulle_vm_tree_search( &vm->tree, range.location);
      /* compute offset inside page */
      page   = mulle_vm_tree_search_result_get_page( result);
      offset = mulle_vm_tree_search_result_get_offset( result);
   }

   node      = mulle_vm_tree_search_result_get_node( result);
   tail_node = NULL;
   tail_page = NULL;

   //
   // we do a split of the current page, if we insert in the middle
   //
   if( offset && offset < page->used.length)
   {
      tail_node = _mulle_vm_split( vm, result, offset);
   }
   else
   {
      /* otherwise try cheap prepend/append to current page */
      /* if there is enough room this will be it */
      available = mulle_vm_page_get_available( page);
      if( available >= insert_length)
      {
         // append
         _mulle_vm_insert_into_large_enough_node( vm,
                                                  node,
                                                  offset,
                                                  bytes,
                                                  insert_length);
         return( 0);
      }

      // so we do NOT have enough room in, node
      // in the prepend case (offset == 0) we get the previous node
      // as the first node
      if( ! offset)
      {
         tail_node = node;
         tail_page = page;

         // this node must exist!! logically it will
         node = mulle_vm_tree_previous_node( &vm->tree, node);
         if( ! node)
         {
            // so here we have the special case that we prepend at zero.
            // deal with this here exhaustively, because the special
            // case gums up the nice code below. We happen to know from
            // above that we cant do a "cheap" insert into available
            //
            insert_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
            node = mulle_vm_tree_insert_page_before_node( &vm->tree, insert_page, tail_node);
            mulle_vm_page_release( insert_page);
            _mulle_vm_try_coalesce( vm, node);   // why not ...
            _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
            return( 0);
         }
         page = _mulle_vm_node_get_page( node);
      }
      else
      {
         tail_node = mulle_vm_tree_next_node( &vm->tree, node);
         if( tail_node)
            tail_page = _mulle_vm_node_get_page( tail_node);
      }
   }

   assert( node);
   assert( page);

   //
   // FROM HERE ON WE KNOW ITS A CLEAN APPEND TO "page"
   //
   // lets see if we can write everything into the combined pages
   // maybe again, as page will have changed due to split
   //
   front_available = mulle_vm_page_get_available( page);
   tail_available  = mulle_vm_page_get_available( tail_page);

   if( front_available + tail_available >= insert_length)
   {
      // append
      front_length = front_available > insert_length
                     ? insert_length
                     : front_available;
      _mulle_vm_insert_into_large_enough_node( vm,
                                               node,
                                               page->used.length,
                                               bytes,
                                               front_length);
      _mulle_vm_insert_into_large_enough_node( vm,
                                               tail_node,
                                               0,
                                               &((char *) bytes)[ front_length],
                                               insert_length - front_length);
      return( 0);
   }

   //
   // so then we drop a page into the middle and assume that this bodes well
   // for the next inserts. Anyway we could try to pare insert_length down
   // to the next pagesize and spill over to front and back but
   //
   insert_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
   insert_node = mulle_vm_tree_insert_page_after_node( &vm->tree, insert_page, node);
   mulle_vm_page_release( insert_page);

   _mulle_vm_try_coalesce( vm, insert_node);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

   return( 0);
}


void   _mulle_vm_append( struct mulle_vm *vm, void *bytes, size_t length)
{
   struct mulle_range   range;

   range = mulle_range_make( mulle_vm_get_length( vm), length);
   _mulle_vm_insert( vm, range, bytes);
}


void   mulle_vm_node_printf( FILE *fp, void *p)
{
   struct mulle_vm_node *node = (struct mulle_vm_node *) p;

   mulle_fprintf( fp, "{ page=%p, length=%zu, total=%zu}",
                      node->page, node->page->used.length, node->total);

}


#ifdef DEBUG
long   mulle_vm_delete_break = 100241800;
long   mulle_vm_delete_attempt;
#endif

void  mulle_vm_tree_write_dot_file( char *filename,
                                    struct mulle_vm_tree *tree,
                                    void (*print_value_fn)( FILE *fp, void *))
{
   FILE   *fp;

   fp = fopen( filename, "w");
   mulle__rbtree_node_dot_fprintf( fp,
                                   (struct mulle__rbtree *) &tree->rbtree,
                                   print_value_fn);
   fclose( fp);
}


static inline void   mulle_vm_mark_node_as_dirty( struct mulle_vm *vm,
                                                  struct mulle_vm_node *node)
{
   struct mulle_rbnode   *rbnode;

   if( node)
   {
      rbnode = _mulle_vm_node_get_rbnode( node);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
   }
}


void   _mulle_vm_delete( struct mulle_vm *vm,
                         struct mulle_range range)
{
   size_t                               length;
   size_t                               start_offset;
   size_t                               end_offset;
   struct mulle_vm_node                 *node;
   struct mulle_vm_node                 *middle_node;
   struct mulle_vm_node                 *prev_node;
   struct mulle_vm_page                 *page;
   struct mulle_vm_tree_search_result   start_result;
   struct mulle_vm_tree_search_result   end_result;
   uint8_t                              *delete_ptr;
   uint8_t                              *keep;
   uint8_t                              *sentinel;

#ifdef DEBUG
   ++mulle_vm_delete_attempt;
   assert( mulle_vm_delete_attempt <= mulle_vm_delete_break);
#endif

   assert( vm);
   mulle_vm_assert( vm);

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! mulle_range_is_valid( range))
      return;

   if( ! range.length)
      return;

   start_result = _mulle_vm_tree_search( &vm->tree, range.location);
   end_result   = _mulle_vm_tree_search( &vm->tree,
                                         mulle_range_get_last_location( range));

#ifndef NDEBUG
   {
      extern struct mulle_vm_tree_search_result
         _mulle_vm_tree_search_trace( struct mulle_vm_tree *tree,
                                      uintptr_t position);

      if( ! mulle_vm_tree_search_result_is_valid( start_result))
      {
         mulle__rbtree_node_dot_fprintf( stderr,
                                         (struct mulle__rbtree *) &vm->tree.rbtree,
                                         mulle_vm_node_printf);
         _mulle_vm_tree_search_trace( &vm->tree, range.location);
         abort();
      }

      if( ! mulle_vm_tree_search_result_is_valid( end_result))
      {
         mulle__rbtree_node_dot_fprintf( stderr,
                                         (struct mulle__rbtree *) &vm->tree.rbtree,
                                         mulle_vm_node_printf);
         _mulle_vm_tree_search_trace( &vm->tree, mulle_range_get_last_location( range));
         abort();
      }
   }
#endif

#ifdef DEBUG
   if( mulle_vm_delete_attempt == mulle_vm_delete_break)
      mulle_vm_tree_write_dot_file( "a.dot", &vm->tree, mulle_vm_node_printf);
#endif

   /* SINGLE-PAGE DELETE */
   if( start_result.node == end_result.node)
   {
      page         = mulle_vm_tree_search_result_get_page( start_result);
      start_offset = mulle_vm_tree_search_result_get_offset( start_result);

      page                    = _mulle_vm_will_write_page( vm, page);
      start_result.node->page = page;

      /* handle front, back, or middle deletion */
      if( start_offset == 0)
      {
         /* delete from front */
         page->used.location += range.length;
         assert( page->used.length >= range.length);
         page->used.length   -= range.length;
      }
      else
         if( start_offset + range.length == page->used.length)
         {
            /* delete from back */
            assert( page->used.length >= range.length);
            page->used.length -= range.length;
         }
         else
         {
            /* delete middle: shift trailing bytes down */
            delete_ptr = &((uint8_t *) page->bytes)[ page->used.location + start_offset];
            keep       = &((uint8_t *) delete_ptr)[ range.length];
            sentinel   = &((uint8_t *) page->bytes)[ page->used.location + page->used.length];

            memmove( delete_ptr, keep, (char *) sentinel - (char *) keep);
            assert( page->used.length >= range.length);
            page->used.length -= range.length;
         }

      if( ! page->used.length)
      {
         mulle_vm_tree_remove_node( &vm->tree, start_result.node);
      }
      else
      {
         mulle_vm_mark_node_as_dirty( vm, start_result.node);

         /* coalesce if small */
         _mulle_vm_try_coalesce( vm, start_result.node);
         _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
      }

      mulle_vm_assert( vm);
      return;
   }

   /* MULTI-PAGE DELETE */

   /* trim last page to tail part or remove */
   node        = end_result.node;
   // remember predecessor node
   middle_node = mulle_vm_tree_previous_node( &vm->tree, node);
   end_offset  = mulle_vm_tree_search_result_get_offset( end_result);

   if( end_offset + 1 == node->page->used.length)
   {
      mulle_vm_tree_remove_node( &vm->tree, end_result.node);
      end_result.node = NULL;
   }
   else
   {
      node->page = _mulle_vm_will_write_page( vm, node->page);
      page       = node->page;

      assert( page->used.location + end_offset + 1 <= page->size);

      page->used.location += end_offset + 1;
      page->used.length   -= end_offset + 1;
      mulle_vm_mark_node_as_dirty( vm, node);

      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
   }
   mulle_vm_assert( vm);

#ifdef DEBUG
   if( mulle_vm_delete_attempt == mulle_vm_delete_break)
      mulle_vm_tree_write_dot_file( "b.dot", &vm->tree, mulle_vm_node_printf);
#endif

   /* remove full pages in between */
   for( node = middle_node; node && node != start_result.node; node = prev_node)
   {
      prev_node = mulle_vm_tree_previous_node( &vm->tree, node);
      if( mulle_vm_delete_attempt == mulle_vm_delete_break)
         _mulle__rbtree_enable_marking( (struct mulle__rbtree *) &vm->tree.rbtree);

      // mulle_vm_tree_remove_node( &vm->tree,node);
      {
         struct mulle_rbnode   *rbnode;

         rbnode = _mulle_vm_node_get_rbnode( node);
         {
            struct mulle_allocator   *allocator;
            void                     *value;
            struct mulle_rbtree      *rb_tree  = (struct mulle_rbtree *) &vm->tree;
            struct mulle__rbtree     *rb__tree = (struct mulle__rbtree *) &vm->tree;

            value     = _mulle__rbtree_get_node_value( rb__tree, rbnode);
            allocator = _mulle_rbtree_get_allocator( rb_tree);
            (*rb_tree->callback.release)( &rb_tree->callback, value, allocator);
//            _mulle__rbtree_mark_node_as_dirty( rb__tree, rbnode); // this will marker parents
            _mulle__rbtree_remove_node( rb__tree, rbnode);

#ifdef DEBUG
             if( mulle_vm_delete_attempt == mulle_vm_delete_break)
                 mulle_vm_tree_write_dot_file( "c.dot", &vm->tree, mulle_vm_node_printf);
#endif
            _mulle_rbtree_walk_dirty( rb_tree);
         }
      }

      mulle_vm_assert( vm);
   }

   // trim down first node (or remove)
   start_offset = mulle_vm_tree_search_result_get_offset( start_result);

   if( ! start_offset)
   {
      mulle_vm_tree_remove_node( &vm->tree, start_result.node);
      start_result.node = NULL;
   }
   else
   {
      node              = start_result.node;
      node->page        = _mulle_vm_will_write_page( vm, node->page);
      page              = node->page;

      assert( page->used.location + start_offset <= page->size);

      page->used.length = start_offset;
      mulle_vm_mark_node_as_dirty( vm, node);

      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
   }
   mulle_vm_assert( vm);

   /* coalesce head and tail survivors if small */
   /* if we removed both, we could want to coalesce the predecessor
    * or successor, is it worth it ? */
   if( end_result.node)
      _mulle_vm_try_coalesce( vm, end_result.node);

   if( start_result.node)
      _mulle_vm_try_coalesce( vm, start_result.node);


   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

#ifdef DEBUG
   if( mulle_vm_delete_attempt == mulle_vm_delete_break)
      mulle_vm_tree_write_dot_file( "c.dot", &vm->tree, mulle_vm_node_printf);

#endif

   mulle_vm_assert( vm);
   return;
}


/*

Function: mulle_vm_io

Description:
Reads (or writes) a range of bytes from a virtual memory abstraction
(struct mulle_vm) composed of multiple mapped pages.
The logical memory is a concatenation of the used ranges from each mapped page.
***Beware: Partial reads and writes will happen until the I/O function determines that
the range can not be satisfied.***

  - If the requested read range fits entirely within a single mapped page,
    this function returns a direct pointer
    to the memory inside that page without copying.
  - If the read spans multiple pages:
     - If `buffer` is non-NULL, the function copies the requested data into
       `buffer` and returns `buffer`.
     - If `buffer` is NULL, the function returns NULL and sets errno to ENOSPC
       indicating no buffer space provided.
  - If the read range is invalid or out-of-bounds, returns NULL and sets
    errno to EINVAL.

Parameters:
vm - pointer to the struct mulle_vm representing the virtual memory with mapped pages.
range - a struct mulle_range specifying the logical offset (location) and number of bytes (length)
to read from the virtual memory.
buffer - pointer to a writable buffer to copy data across multiple pages if needed, or NULL if
only direct pointer reads are desired.

Returns:
- On success:
- Pointer directly into a mapped page if the read fits in one page.
- Pointer to buffer if the read spans multiple pages and buffer is provided.
- On failure:
- NULL, with errno set to:
- EINVAL for invalid or out-of-bounds range.
- ENOSPC if buffer is NULL but required (multi-page read).

Notes:
- The function performs a binary search to locate the start page efficiently.
- No partial reads: either the entire requested range is returned or NULL.
- The mapped pages must be sorted by their logical vm_address.
*/
// Implementation of _mulle_vm_io for tree structure, simplified for single page operations for now
void   *_mulle_vm_io( struct mulle_vm *vm,
                      struct mulle_range range,
                      void *buffer,
                      int write_flag)  // 1 w: 0: r
{
   struct mulle_vm_page                 *page;
   struct mulle_vm_tree_search_result   result;
   uintptr_t                            offset;
   size_t                               length;
   size_t                               bytes_copied;
   size_t                               bytes_remaining;
   uintptr_t                            current_pos;
   uint8_t                              *dest_buffer;
   struct mulle_vm_tree_search_result   current_result;
   struct mulle_vm_page                 *current_page;
   uintptr_t                            current_offset;
   size_t                               available_in_page;
   size_t                               to_copy;

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! mulle_range_is_valid( range))
   {
      errno = ERANGE;
      return( NULL);
   }

   if( range.length == 0)
      return( buffer);

   // Find the page and get the offset within that page
   result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      errno = EINVAL;
      return( NULL);
   }
   
   page   = result.node->page;
   offset = result.offset;

   // Check if the requested range fits entirely within this page
   if( offset + range.length > page->used.length)
   {
      // Multi-page operation - buffer is required for spanning pages
      if( ! buffer)
      {
         errno = ENOSPC;
         return( NULL);
      }
      
      // For now, only support multi-page reads, not writes
      if( write_flag)
      {
         errno = ENOSYS;
         return( NULL);
      }
      
      // Implement simple multi-page read by copying data page by page
      bytes_copied    = 0;
      bytes_remaining = range.length;
      current_pos     = range.location;
      dest_buffer     = (uint8_t *) buffer;
      
      while( bytes_remaining > 0)
      {
         // Find the page for current position
         current_result = _mulle_vm_tree_search( &vm->tree, current_pos);
         if( ! mulle_vm_tree_search_result_is_valid( current_result))
         {
            errno = EINVAL;
            return( NULL);
         }
         
         current_page   = current_result.node->page;
         current_offset = current_result.offset;
         
         // Calculate how much we can read from this page
         available_in_page = current_page->used.length - current_offset;
         to_copy           = (bytes_remaining < available_in_page) ? bytes_remaining : available_in_page;
         
         // Copy data from this page
         memcpy( dest_buffer + bytes_copied,
                 (uint8_t *) current_page->bytes + current_page->used.location + current_offset,
                 to_copy);
         
         bytes_copied    += to_copy;
         bytes_remaining -= to_copy;
         current_pos     += to_copy;
      }
      
      return( buffer);
   }

   // For reads, we can return a direct pointer if no buffer copying is needed
   if( ! write_flag && ! buffer)
   {
      return( (void *) ((uint8_t *) page->bytes + page->used.location + offset));
   }

   // The buffer must be provided for writes or when explicitly requested
   if( ! buffer)
   {
      errno = ENOSPC;
      return( NULL);
   }

   if( write_flag)
   {
      // Make sure we can write to this page (COW if shared)
      page = _mulle_vm_will_write_page( vm, page);
      
      // Update the tree node to point to the new page (if COW happened)
      result.node->page = page;
      
      memcpy( (uint8_t *) page->bytes + page->used.location + offset,
              buffer,
              range.length);
   }
   else
   {
      memcpy( buffer,
              (uint8_t *) page->bytes + page->used.location + offset,
              range.length);
   }

   return( buffer);
}


int  _mulle_vm_walk( struct mulle_vm *vm,
                     struct mulle_range range,
                     mulle_vm_walk_callback_t  *callback,
                     void *userinfo,
                     int direction)
{
   struct mulle_vm_tree_search_result   start_result;
   struct mulle_vm_tree_search_result   end_result;
   struct mulle_data                    data;
   struct mulle_range                   vm_range;
   struct mulle_range                   intersection;
   uintptr_t                            current_vm_address;
   int                                  rval;
   size_t                               vm_length;

   if( ! vm || ! callback)
   {
      errno = EINVAL;
      return( -1);
   }

   vm_length = mulle_vm_get_length( vm);
   range     = mulle_range_validate_against_length( range, vm_length);
   if( ! mulle_range_is_valid( range))
   {
      errno = ERANGE;
      return( -1);
   }

   if( ! range.length)
      return( 0);

   // Find the starting page
   start_result = _mulle_vm_tree_search( &vm->tree, range.location);
   end_result   = _mulle_vm_tree_search( &vm->tree, mulle_range_get_last_location( range));

   // MEMO this can't really happen since we validate above so we just
   // assert
   assert( mulle_vm_tree_search_result_is_valid( start_result));
   assert( mulle_vm_tree_search_result_is_valid( end_result));

   if( direction > 0)
   {
      // Forward walk
      current_vm_address = 0;
      mulle_vm_for( vm, current_page)
      {
         // Calculate the VM range for this node
         vm_range = mulle_range_make( current_vm_address, current_page->used.length);
         if( ! mulle_range_is_valid( vm_range))
            continue;

         // Find intersection with requested range
         intersection = mulle_range_intersection( vm_range, range);
         if( intersection.length > 0)
         {
            // Calculate offset within the page
            size_t page_offset = intersection.location - current_vm_address;

            // Create data structure for callback
            data = mulle_data_make(
               (uint8_t *) current_page->bytes + current_page->used.location + page_offset,
               intersection.length);

            // Call the callback
            rval = (*callback)( data, intersection, userinfo);
            if( rval != 0)
               return( rval);
         }

         current_vm_address += current_page->used.length;

         // Stop if we've passed the end of the requested range
         if( current_vm_address >= mulle_range_get_max( range))
            break;
      }
   }
   else
   {
      // Reverse walk
      current_vm_address = vm_length;
      mulle_vm_for_reverse( vm, current_page)
      {
         current_vm_address -= current_page->used.length;

         // Calculate the VM range for this node
         vm_range = mulle_range_make( current_vm_address, current_page->used.length);
         if( ! mulle_range_is_valid( vm_range))
            continue;

         // Find intersection with requested range
         intersection = mulle_range_intersection( vm_range, range);
         if( intersection.length > 0)
         {
            // Calculate offset within the page
            //size_t page_offset = intersection.location - current_vm_address;
            size_t page_offset = intersection.location - vm_range.location;

            // Create data structure for callback
            data = mulle_data_make(
               (uint8_t *) current_page->bytes + current_page->used.location + page_offset,
               intersection.length);

            // Call the callback
            rval = (*callback)( data, intersection, userinfo);
            if( rval != 0)
               return( rval);
         }

         // Stop if we've passed the start of the requested range
         if( current_vm_address <= range.location)
            break;
      }
   }

   return( 0);
}


/**
 **
 **
 **
 */


// copy range src pages to dst (shared)
// delete everything in dst before
// this calling mulle_vm_copy and mulle_vm_delete in succession

int   mulle_vm_copy( struct mulle_vm *dst,
                     struct mulle_vm *src,
                     struct mulle_range range)
{
   struct mulle_vm_node                 *end_node;
   struct mulle_vm_node                 *start_node;
   struct mulle_vm_page                 *copy;
   struct mulle_vm_tree_search_result   end_result;
   struct mulle_vm_tree_search_result   start_result;
   struct mulle_range                   copy_range;
   size_t                               remainder;
   size_t                               length;
   uintptr_t                            start_offset;
   uintptr_t                            end_offset;

   if( dst == src)
   {
      errno = EINVAL;
      return( -1);
   }

   // clean original, we do this always
   // TODO: mulle_vm_delete( dst, mulle_range_make_all());

   length = mulle_vm_get_length( src);
   range  = mulle_range_validate_against_length( range, length);
   if( ! mulle_range_is_valid( range))
   {
      errno = ERANGE;
      return( -1);
   }

   if( range.length == 0)
      return( 0);

   start_result = _mulle_vm_tree_search( &src->tree, range.location);
   assert( mulle_vm_tree_search_result_is_valid( start_result));
   start_node   = start_result.node;
   start_offset = mulle_vm_tree_search_result_get_offset( start_result);

   end_result = _mulle_vm_tree_search( &src->tree, mulle_range_get_last_location( range));
   assert( mulle_vm_tree_search_result_is_valid( end_result));
   end_node   = end_result.node;
   end_offset = mulle_vm_tree_search_result_get_offset( end_result);

   remainder = range.length;
   // can we use page wholesale ?
   if( ! start_offset && start_node->page->used.length <= remainder)
   {
      //copy = mulle_vm_page_retain( start_node->page);
      mulle_vm_tree_add_page( &dst->tree, start_node->page);

      remainder -= start_node->page->used.length;
   }
   else
   {
      copy_range = mulle_range_make( start_offset,
                                     start_node->page->used.length - start_offset);
      if( end_node == start_node)
         copy_range.length -= start_node->page->used.length - end_offset - 1; // -1 because end_offset is inside

      copy = _mulle_vm_copy_page( src, start_node->page, copy_range);
      mulle_vm_tree_add_page( &dst->tree, copy);
      mulle_vm_page_release( copy);

      remainder -= copy_range.length;
   }

   // special case if we only copy one page, we are done otherwise...
   if( ! remainder)
      return( 0);

   // Walk through nodes between start_node and end_node
   // could also use next here...
   mulle_vm_tree_for( &src->tree, node)
   {
      if( node == start_node)
         continue; // skip start_node node, already handled
      if( node == end_node)
         break;    // don't include end_node node yet

      //copy = mulle_vm_page_retain( node->page);
      mulle_vm_tree_add_page( &dst->tree, node->page);

      remainder -= node->page->used.length;
   }

   // special treatment for last page
   if( end_node->page->used.length == remainder)
   {
      // copy = mulle_vm_page_retain( end_node->page);
      mulle_vm_tree_add_page( &dst->tree, end_node->page);
   }
   else
   {
      assert( remainder == end_node->page->used.length - end_offset);

      copy_range = mulle_range_make( 0, remainder);

      copy = _mulle_vm_copy_page( src, end_node->page, copy_range);
      mulle_vm_tree_add_page( &dst->tree, copy);
      mulle_vm_page_release( copy);
   }

   // no need to adjust, we added to empty vm
   mulle_vm_assert( dst);
   mulle_vm_assert( src);
   return( 0);
}


// copy range pages to dst (shared)
// delete range in source afterwards
// this calling mulle_vm_copy and mulle_vm_delete in succession
int   mulle_vm_cut( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range)
{
   int   rval;

   rval = mulle_vm_copy( dst, src, range);
   if( rval)
      return( rval);

   mulle_vm_delete( src, range);
   return( 0);
}


//
// delete "selected" range in dst, then
// copy src into range.location
// src remains unchanged
//
int   mulle_vm_paste( struct mulle_vm *dst,
                      struct mulle_range range,
                      struct mulle_vm *src)
{
   struct mulle_vm_node                 *start_node;
   struct mulle_vm_node                 *node;
   size_t                               length;
   struct mulle_vm_tree_search_result   start_result;

   if( dst == src)
   {
      errno = EINVAL;
      return( -1);
   }

   // delete range if any
   mulle_vm_delete( dst, range);

   // if nothing to paste, we is done (so mean we must have a page in src)
   length = mulle_vm_get_length( src);
   if( length == 0)
      return( 0);

   start_result = _mulle_vm_tree_search( &dst->tree, range.location);
   start_node   = mulle_vm_tree_search_result_get_node( start_result);

   // append: get this out of the way
   if( ! mulle_vm_tree_search_result_is_valid( start_result))
   {
      mulle_vm_tree_for( &src->tree, src_node)
      {
         mulle_vm_tree_add_page( &dst->tree, src_node->page);
      }
      return( 0);
   }

   // prepend: get this out of the way
   if( ! range.location)
   {
      node = start_node;
      mulle_vm_tree_for_reverse( &src->tree, src_node)
      {
         node = mulle_vm_tree_insert_page_before_node( &dst->tree, src_node->page, node);
      }
      return( 0);
   }

   // ok split off current node
   _mulle_vm_split( dst, start_result, range.location);
   node      = start_node;
   mulle_vm_tree_for( &src->tree, src_node)
   {
      node = mulle_vm_tree_insert_page_after_node( &dst->tree, src_node->page, node);
   }

   mulle_vm_assert( src);
   mulle_vm_assert( dst);

   return( 0);
}


/**
 **
 **
 **
 */
// Return total logical bytes and count of pages
struct mulle_vm_statistics   mulle_vm_get_statistics( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   stats = { 0 };

   if( vm)
   {
      mulle_vm_for( vm, page)
      {
         mulle_vm_assert_page( vm, page);

         stats.vm_size   += page->used.length;
         stats.page_size += page->size;
      }
   }
   return( stats);
}
