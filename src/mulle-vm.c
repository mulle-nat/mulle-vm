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



#if 0
/**
 **
 **
 **
 */
static inline struct mulle_range
   mulle_vm_node_get_range( struct mulle_vm_node *info)
{
   if( ! info)
      return( mulle_range_make_invalid());

   return( mulle_range_make( info->address, info->page->used.length));
}


static inline struct mulle_data
   mulle_vm_node_get_data_in_range( struct mulle_vm_node *info,
                                       struct mulle_range range)
{
   if( ! info)
      return( mulle_data_make_invalid());

   range.location -= info->address;
   range           = mulle_range_validate_against_length( range, info->page->used.length);
   return( mulle_data_make( mulle_vm_page_address( info->page, range.location),
                            range.length));
}
#endif



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
   mulle_vm_for( vm, page)
   {
      assert( page != NULL && "mulle-vm: mapping with no page");
   }
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

#if 0
void   mulle_vm_append_bytes( struct mulle_vm *vm, void *bytes, size_t length)
{
   struct mulle_vm_page   *page;

   if( ! vm || ! bytes || length == 0)
      return;

   // Use the VM's page allocation system which handles proper alignment
   page = _mulle_vm_new_page_with_bytes( vm, bytes, length);
   mulle_vm_tree_add_page( &vm->tree, page);
}
#endif


//
// you must have called _mulle_vm_release_page( vm, end->page) beforehand
//
#ifndef MULLE_TEST
static
#endif
#if 0 // TODO: implement mulle_vm_tree_remove
int   mulle_vm_remove_page( struct mulle_vm *vm, uintptr_t location)
{
   return( mulle_vm_tree_remove( &vm->tree, location));
}
#endif


#if 0 // TODO: implement mulle_vm_tree_insert
static void  mulle_vm_insert_page( struct mulle_vm *vm,
                                   struct mulle_vm_page *page,
                                   uintptr_t location)

{
   mulle_vm_assert_page( vm, page);

   mulle_vm_tree_insert( &vm->tree, page);

   mulle_vm_assert( vm);
}
#endif



struct mulle_vm_page *
   _mulle_vm_copy_page( struct mulle_vm *vm,
                        struct mulle_vm_page *page,
                        struct mulle_range range)
{
   struct mulle_vm_page   *copy;
   struct mulle_range     page_range;
   struct mulle_range     intersection;
   void                   *bytes;

   // figure out what we need
   page_range   = page->used;
   intersection = mulle_range_intersection( range, page_range);

   assert( intersection.length);

   bytes  = mulle_vm_page_address( page, intersection.location);
   copy   = _mulle_vm_new_page_with_bytes( vm, bytes, intersection.length);

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


#if 0 // TODO: fix missing semicolon and implement properly
static void   mulle_vm_tree_did_write( struct mulle_vm *vm, struct mulle_rbnode *node)
{
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, node);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
}

/**
 **
 **
 **
 */
enum mulle_vm_insert_mode
{
   prepend,
   append,
   split,
};

int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
{
   struct mulle_range                  head_range;
   struct mulle_vm_page                *new_page;
   struct mulle_vm_page                *tail_page;
   struct mulle_vm_node                *info;
   size_t                              available;
   size_t                              count;
   size_t                              length;
   intptr_t                            idx;
   int                                 append;
   int                                 rval;
   int                                 mode;
   struct mulle_vm_tree_search_result  result;
   struct mulle_range                  remainder;

   if( ! vm)
      return( EINVAL);

   // hmm, OK...
   if( range.length == 0)
      return;

   assert( bytes);

   result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      length = mulle_vm_get_length( vm);
      if( length != range.location) // clean append ?
      {
         errno = ERANGE;
         return;
      }

      // append at end (ez)
      new_page = _mulle_vm_new_page_with_bytes( vm, vm, bytes, range.length);
      mulle_vm_insert_page( vm, new_page, range.location);
      return;
   }

   // check if we are just at the end of the page, then we a) fill up
   // current page b) copy pages worth of data into a new page
   // c) prepend rest of data into following page or create another new page
   page = mulle_vm_tree_search_result_get_page( result);
   if( range.location && ! mulle_vm_tree_search_result_get_offset( result))
   {

   }

   //
   // if we are at the very start, we dial one back and try to fill up the
   // previous page. Then we fill up pages worth of data into a new page
   // c) prepend rest of data into following page  / create another new page
   // pretty similiar
   //
   if( range.location && ! mulle_vm_tree_search_result_get_offset( result))
   {
      result         = _mulle_vm_tree_search( &vm->tree, range.location - 1);
      assert( mulle_vm_tree_search_result_is_valid( result));
      result.offset += 1;
   }


   // what remains, is we insert in the middle of an existing page. We
   // split the page into two parts. Now its basically back to appending to
   // a page

   offset    = mulle_vm_tree_search_result_get_offset( result);

   for(;;)
   {
      page      = mulle_vm_tree_search_result_get_page( result);
      available = mulle_vm_page_get_available( result.info->page);

      // we stuff as many bytes into this page and the remainder into next
      // or new
      while( available >= range.length)
      {
         result.info->page = _mulle_vm_will_write_page( vm, info->page);
         r
         rval              = _mulle_vm_page_insert_bytes( result.info->page,
                                                          offset,
                                                          bytes,
                                                          range.length);
         assert( ! rval);

         range.location += available;
         range.length   -= available;
         bytes = &((char *) bytes)[ available];
         // advertise that this node got larger
         mulle_vm_tree_did_write( vm, result.node);

         if( range.length == 0)
      }
   // otherw
      if( available >= range.length)
      {
         // can't fail
         mulle_vm_assert( vm);
         return;
      }


      //
      // if we only append, we can stuff part of the bytes into the
      // previous page and then do a clean page insert behind it
      //
      if( append)
      {
         head_range = mulle_range_make( range.location, available);
         info->page = _mulle_vm_will_write_page( vm, info->page);
         rval       = _mulle_vm_page_insert_bytes( info->page, info->address, head_range, bytes);
         assert( ! rval);

         // reduce range we want to add at index
         range = mulle_range_make( range.location + available,
                                   range.length - available);
         bytes = &((char *) bytes)[ available];
      }
      else
      {
         tail_page = _mulle_vm_split_page( vm, result, range.location);
         mulle_vm_assert_page( vm, tail_page);
      }
   }

   //
   // so we insert at page after the current index
   // the address will be automatically recomputed
   //
append:
   new_page = _mulle_vm_new_page_with_bytes( vm, vm, bytes, range.length);
   mulle_vm_insert_page( vm, new_page, range.location);

   return;
}
#endif



#if 0
static int   _mulle_vm_insert_into_page( struct mulle_vm *vm,
                                         struct mulle_vm_page *page,
                                         struct mulle_vm_node *node,
                                         uintptr_t offset,
                                         void *bytes,
                                         size_t length)
{
   struct mulle_rbnode   *rbnode;
   int                   rval;

   page = _mulle_vm_will_write_page( vm, page);
   node->page = page;

   rval = _mulle_vm_page_insert_bytes( page, offset, bytes, length);
   if( rval != 0)
      return( rval);

   rbnode = _mulle_vm_node_get_rbnode( node);
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

   return( 0);
}
#endif


#if 1 // CHATGPT
static int
   _mulle_vm_try_coalesce( struct mulle_vm *vm,
                           struct mulle_vm_node *left,
                           struct mulle_vm_node *right)
{
   int                  rval;
   size_t               left_slack;
   size_t               right_slack;
   size_t               insert_len;
   struct mulle_rbnode  *rbnode;
   struct mulle_vm_page *lpage;
   struct mulle_vm_page *rpage;

   if( ! left || ! right)
   {
      return( 0);
   }

   lpage = left->page;
   rpage = right->page;

   left_slack  = lpage->size - lpage->used.length;
   right_slack = rpage->size - rpage->used.length;

   //
   // Prefer merging into the page with more slack
   //
   if( left_slack >= rpage->used.length)
   {
      lpage      = _mulle_vm_will_write_page( vm, lpage);
      left->page = lpage;

      insert_len = rpage->used.length;
      rval = _mulle_vm_page_insert_bytes( lpage,
                                          lpage->used.length,
                                          &((char *) rpage->bytes)[ rpage->used.location],
                                          insert_len);
      assert( rval == 0);

      rbnode = _mulle_vm_node_get_rbnode( left);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
      mulle_vm_tree_remove_node( &vm->tree, right);

      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
      return( 1);
   }

   if( right_slack >= lpage->used.length)
   {
      rpage       = _mulle_vm_will_write_page( vm, rpage);
      right->page = rpage;

      insert_len = lpage->used.length;
      rval       = _mulle_vm_page_insert_bytes( rpage,
                                                0,
                                                &((char *) lpage->bytes)[ lpage->used.location],
                                                insert_len);
      assert( rval == 0);

      rbnode = _mulle_vm_node_get_rbnode( right);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);

      mulle_vm_tree_remove_node( &vm->tree, left);

      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
      return( 1);
   }

   return( 0);
}


int   _mulle_vm_insert( struct mulle_vm *vm,
                        struct mulle_range range,
                        void *bytes)
{
   int                                 rval;
   size_t                              offset;
   size_t                              total;
   size_t                              total_avail;
   size_t                              remaining;
   size_t                              use;
   size_t                              to_curr;
   size_t                              to_prev;
   size_t                              insert_length;
   struct mulle_vm_node                *prev_node;
   struct mulle_vm_node                *next_node;
   struct mulle_vm_page                *insert_page;
   struct mulle_vm_page                *page;
   struct mulle_vm_node                *node;
   struct mulle_rbnode                 *rbnode;
   struct mulle_vm_tree_search_result  result;
   uint8_t                             *cursor;

   assert( vm);
   assert( bytes);

   insert_length = range.length;
   if( ! insert_length)
      return( 0);

   /* empty VM: create first page */
   /* split page if needed */
   next_node = NULL;
   total     = mulle_vm_get_length( vm);

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
         return( 0);
      }

      result = _mulle_vm_tree_search( &vm->tree, total - 1);
      assert( mulle_vm_tree_search_result_is_valid( result));
      offset = total;
   }
   else
   {
      result = _mulle_vm_tree_search( &vm->tree, range.location);
      /* compute offset inside page */
      offset = mulle_vm_tree_search_result_get_offset( result);
   }

   node = mulle_vm_tree_search_result_get_node( result);
   page = mulle_vm_tree_search_result_get_page( result);
   if( offset && offset < page->used.length)
   {
      next_node = _mulle_vm_split( vm, result, offset);
   }

   /* try insert into current page */
   page = _mulle_vm_will_write_page( vm, page);
   rval = _mulle_vm_page_insert_bytes( page, offset, bytes, insert_length);
   if( rval == 0)
   {
      node   = mulle_vm_tree_search_result_get_node( result);
      rbnode = _mulle_vm_node_get_rbnode( node);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
      return( 0);
   }

   /* spill-over to neighbors if combined space is enough */
   prev_node   = mulle_vm_tree_previous_node( &vm->tree, node);
   if( ! next_node)
      next_node = mulle_vm_tree_next_node( &vm->tree, node);

   total_avail = 0;
   if( prev_node)
   {
      total_avail += mulle_vm_page_get_available( prev_node->page);
   }
   total_avail += mulle_vm_page_get_available( page);
   if( next_node)
   {
      total_avail += mulle_vm_page_get_available( next_node->page);
   }

   if( total_avail >= insert_length)
   {
      remaining = insert_length;
      cursor    = bytes;

      /* fill previous page first */
      if( prev_node)
      {
         use = mulle_vm_page_get_available( prev_node->page);
         to_prev = use < remaining ? use : remaining;
         if( to_prev)
         {
            rval = _mulle_vm_page_insert_bytes( prev_node->page,
                                                prev_node->page->used.length,
                                                cursor,
                                                to_prev);
            if( rval)
            {
               rbnode = _mulle_vm_node_get_rbnode( prev_node);
               _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
               _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
               return( rval);
            }
            cursor    += to_prev;
            remaining -= to_prev;
         }
      }

      /* then current page */
      if( remaining)
      {
         use     = mulle_vm_page_get_available( page);
         to_curr = use < remaining ? use : remaining;
         if( to_curr)
         {
            rval = _mulle_vm_page_insert_bytes( page, offset, cursor, to_curr);
            if( rval)
            {
               rbnode = _mulle_vm_node_get_rbnode( node);
               _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
               _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
               return( rval);
            }
            cursor    += to_curr;
            remaining -= to_curr;
         }
      }

      /* finally next page */
      if( next_node && remaining)
      {
         rval = _mulle_vm_page_insert_bytes( next_node->page, 0, cursor, remaining);
         if( rval)
         {
            rbnode = _mulle_vm_node_get_rbnode( next_node);
            _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
            _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
            return( rval);
         }
         remaining = 0;
      }

      assert( remaining == 0);
      return( 0);
   }

   /* if still cannot fit, allocate a new page */
   insert_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
   if( offset == page->used.length)
   {
      mulle_vm_tree_insert_page_after_node( &vm->tree, insert_page, node);
   }
   else
   {
      mulle_vm_tree_insert_page_before_node( &vm->tree, insert_page, node);
   }
   mulle_vm_page_release( insert_page);

   return( 0);
}


void   _mulle_vm_append( struct mulle_vm *vm, void *bytes, size_t length)
{
   struct mulle_range   range;

   range = mulle_range_make( mulle_vm_get_length( vm),
                             length);
   _mulle_vm_insert( vm, range, bytes);
}


void   _mulle_vm_delete( struct mulle_vm *vm,
                         struct mulle_range range)
{
   size_t                               length;
   size_t                               start_offset;
   size_t                               end_offset;
   size_t                               page_capacity;
   struct mulle_rbnode                  *rbnode;
   struct mulle_vm_node                 *node;
   struct mulle_vm_node                 *prev;
   struct mulle_vm_node                 *next;
   struct mulle_vm_node                 *next_node;
   struct mulle_vm_page                 *page;
   struct mulle_vm_page                 *tmp;
   struct mulle_vm_tree_search_result   start_result;
   struct mulle_vm_tree_search_result   end_result;
   uint8_t                              *delete_ptr;
   uint8_t                              *keep;
   uint8_t                              *sentinel;

   assert( vm);

   if( ! vm)
   {
      return;
   }

   mulle_vm_assert( vm);

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! range.length)
   {
      return;
   }

   start_result = _mulle_vm_tree_search( &vm->tree, range.location);
   end_result   = _mulle_vm_tree_search( &vm->tree,
                                         mulle_range_get_last_location( range));

   assert( mulle_vm_tree_search_result_is_valid( start_result));
   assert( mulle_vm_tree_search_result_is_valid( end_result));

   /* SINGLE-PAGE DELETE */
   if( start_result.node == end_result.node)
   {
      page         = mulle_vm_tree_search_result_get_page( start_result);
      start_offset = mulle_vm_tree_search_result_get_offset( start_result);

      page = _mulle_vm_will_write_page( vm, page);
      start_result.node->page = page;

      /* handle front, back, or middle deletion */
      if( start_offset == 0)
      {
         /* delete from front */
         page->used.location += range.length;
         page->used.length   -= range.length;
      }
      else if( start_offset + range.length == page->used.length)
      {
         /* delete from back */
         page->used.length -= range.length;
      }
      else
      {
         /* delete middle: shift trailing bytes down */
         delete_ptr = &((uint8_t *) page->bytes)[ page->used.location + start_offset];
         keep       = &((uint8_t *) delete_ptr)[ range.length];
         sentinel   = &((uint8_t *) page->bytes)[ page->used.location + page->used.length];

         memmove( delete_ptr, keep, (char *) sentinel - (char *) keep);
         page->used.length -= range.length;
      }

      rbnode = _mulle_vm_node_get_rbnode( start_result.node);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

      //_mulle_vm_free_unused_ospages( vm, page);

      /* coalesce if small */
      page_capacity = page->size;
      if( page->used.length <= page_capacity / 2)
      {
         node = start_result.node;
         prev = mulle_vm_tree_previous_node( &vm->tree, node);
         next = mulle_vm_tree_next_node( &vm->tree, node);

         if( prev)
         {
            _mulle_vm_try_coalesce( vm, prev, node);
         }
         else if( next)
         {
            _mulle_vm_try_coalesce( vm, node, next);
         }
      }

      mulle_vm_assert( vm);
      return;
   }

   /* MULTI-PAGE DELETE */
   node = start_result.node;
   page = node->page;
   start_offset = mulle_vm_tree_search_result_get_offset( start_result);

   page = _mulle_vm_will_write_page( vm, page);
   node->page = page;
   /* trim first page to head part */
   page->used.length = start_offset;

   rbnode = _mulle_vm_node_get_rbnode( node);
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);

   // _mulle_vm_free_unused_ospages( vm, page);

   /* trim last page to tail part */
   node = end_result.node;
   tmp  = node->page;
   end_offset = mulle_vm_tree_search_result_get_offset( end_result);

   tmp = _mulle_vm_will_write_page( vm, tmp);
   node->page = tmp;

   if( end_offset + 1 < tmp->used.length)
   {
      size_t tail_len = tmp->used.length - end_offset - 1;
      memmove( tmp->bytes, &((uint8_t *) tmp->bytes)[ tmp->used.location + end_offset + 1], tail_len);
      tmp->used.location = 0;
      tmp->used.length   = tail_len;
   }
   else
   {
      tmp->used.length = 0;
   }

   rbnode = _mulle_vm_node_get_rbnode( node);
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
   // _mulle_vm_free_unused_ospages( vm, tmp);

   /* remove full pages in between */
   node = mulle_vm_tree_next_node( &vm->tree, start_result.node);
   while( node && node != end_result.node)
   {
      next_node = mulle_vm_tree_next_node( &vm->tree, node);

      mulle_vm_tree_remove_node( &vm->tree, node);
      node = next_node;
   }

   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

   /* coalesce head and tail survivors if small */
   node = start_result.node;
   page = node->page;
   if( page && page->used.length && page->used.length <= page->size / 2)
   {
      prev = mulle_vm_tree_previous_node( &vm->tree, node);
      next = mulle_vm_tree_next_node( &vm->tree, node);

      if( prev)
      {
         _mulle_vm_try_coalesce( vm, prev, node);
      }
      else if( next)
      {
         _mulle_vm_try_coalesce( vm, node, next);
      }
   }

   node = end_result.node;
   page = node->page;
   if( page && page->used.length && page->used.length <= page->size / 2)
   {
      prev = mulle_vm_tree_previous_node( &vm->tree, node);
      next = mulle_vm_tree_next_node( &vm->tree, node);

      if( prev)
      {
         _mulle_vm_try_coalesce( vm, prev, node);
      }
      else if( next)
      {
         _mulle_vm_try_coalesce( vm, node, next);
      }
   }

   mulle_vm_assert( vm);
   return;
}


#else

int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
{
   struct mulle_vm_page                 *first_page;
   struct mulle_vm_page                 *insert_page;
   struct mulle_vm_page                 *new_page;
   struct mulle_vm_page                 *page;
   struct mulle_vm_page                 *tail_page;
   struct mulle_vm_tree_search_result   result;
   size_t                               available_front;
   size_t                               available_space;
   size_t                               insert_length;
   size_t                               vm_length;
   int                                  rval;

   if( ! vm || ! bytes)
      return( EINVAL);

   insert_length = range.length;
   if( insert_length == 0)
      return( 0);

   vm_length = mulle_vm_get_length( vm);
   if( range.location == vm_length)
   {
      mulle_vm_append( vm, bytes, insert_length);
      return( 0);
   }

   result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( result))
      return( EINVAL);

   if( range.location == 0)
   {
      first_page = result.node->page;
      available_front = mulle_vm_page_get_available_front( first_page);

      if( available_front >= insert_length)
      {
         struct mulle_rbnode *rbnode;

         first_page = _mulle_vm_will_write_page( vm, first_page);
         result.node->page = first_page;

         first_page->used.location -= insert_length;
         first_page->used.length   += insert_length;

         memcpy( (uint8_t *) first_page->bytes + first_page->used.location,
                 bytes,
                 insert_length);

         // Update tree totals
         rbnode = _mulle_vm_node_get_rbnode( result.node);
         _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
         _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

         return( 0);
      }

      new_page = _mulle_vm_new_page_with_bytes( vm, vm, bytes, insert_length);
      mulle_vm_tree_insert_page_before_node( &vm->tree, new_page, result.node);

      return( 0);
   }

   page = result.node->page;
   available_space = mulle_vm_page_get_available( page);

   if( available_space >= insert_length)
   {
      rval = _mulle_vm_insert_into_page( vm, page, result.node, result.offset, bytes, insert_length);
      return( rval);
   }

   tail_page = _mulle_vm_split( vm, result, result.offset);
   insert_page = _mulle_vm_new_page_with_bytes( vm, vm, bytes, insert_length);
   mulle_vm_tree_insert_page_after_node( &vm->tree, insert_page, result.node);

   return( 0);
}

void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range)
{
   struct mulle_vm_page                 *page;
   size_t                               length;
   uintptr_t                            offset;
   void                                 *delete_ptr;
   void                                 *keep;
   void                                 *sentinel;
   struct mulle_vm_tree_search_result   start_result;
   struct mulle_vm_tree_search_result   end_result;

   if( ! vm)
      return;

   mulle_vm_assert( vm);

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! range.length)
      return;

   start_result = _mulle_vm_tree_search( &vm->tree, range.location);
   end_result   = _mulle_vm_tree_search( &vm->tree, mulle_range_get_last_location( range));

   assert( mulle_vm_tree_search_result_is_valid( start_result));
   assert( mulle_vm_tree_search_result_is_valid( end_result));

   //
   // special case if we only delete within one page
   //
   if( start_result.node == end_result.node)
   {
      // delete everything ?
      page    = mulle_vm_tree_search_result_get_page( start_result);
      offset  = mulle_vm_tree_search_result_get_offset( start_result);
      if( offset == 0 && page->used.length == range.length)
      {
         // Complete page deletion - set page length to 0
         page = _mulle_vm_will_write_page( vm, page);
         start_result.node->page = page; // Update node's page reference in case COW happened
         page->used.length = 0;

         // Update tree totals
         struct mulle_rbnode *rbnode = _mulle_vm_node_get_rbnode( start_result.node);
         _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
         _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

         mulle_vm_assert( vm);
         return;
      }

      page = _mulle_vm_will_write_page( vm, page);
      start_result.node->page = page; // Update node's page reference in case COW happened

      // remove stuff from front only ?
      if( ! offset)
      {
         page->used.location += range.length;
         page->used.length   -= range.length;
      }
      else
      {
         // remove stuff from back only ?
         if( offset + range.length == page->used.length)
         {
            // nuthing to do here
         }
         else
         {
            // need to move bytes around here, we just move from the back to
            // the front (for now)
            //
            // vm_address: 0x1000
            // |
            // __abcDEFgh__  size: 12
            //   |        /
            //   used:{ 2, 8 }
            //
            //
            // delete:
            //   range: { 0x1003, 3 }
            //
            //      old_middle
            //      |  new_middle
            //      |  |
            // __abcDEFgh__  size: 12
            //
            // __abcDEFgh__    1
            // __abcghFgh__    2
            // __abcgh______   3

            // figure out where the relative insertion point is from page->used.location

            delete_ptr = &((char *) page->bytes)[ page->used.location + offset];
            keep       = &((char *) delete_ptr)[ range.length];
            sentinel   = &((char *) page->bytes)[ page->used.location + page->used.length];

            memmove( delete_ptr, keep, (char*)sentinel - (char*)keep);
         }
      }

      // Only decrease length if we haven't already done it above
      if( offset != 0)
      {
         page->used.length -= range.length;
      }

      // Update tree node and recalculate totals
      struct mulle_rbnode *rbnode = _mulle_vm_node_get_rbnode( start_result.node);
      _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
      _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);

      mulle_vm_assert( vm);
      return;
   }

   //
   // TODO: multipage handling for tree-based implementation
   // For now, only support single-page deletions
   //
   errno = ENOSYS; // Not implemented for multi-page deletions yet
   mulle_vm_assert( vm);
}

#endif


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
void *   _mulle_vm_io( struct mulle_vm      *vm,
                       struct mulle_range   range,
                       void                 *buffer,
                       int                  write_flag)  // 1 w: 0: r
{
   struct mulle_vm_page                 *page;
   struct mulle_vm_tree_search_result   result;
   uintptr_t                            offset;

   if( range.length == 0)
   {
      errno = EINVAL;
      return( NULL);
   }

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
      size_t bytes_copied = 0;
      size_t bytes_remaining = range.length;
      uintptr_t current_pos = range.location;
      uint8_t *dest_buffer = (uint8_t *) buffer;
      
      while( bytes_remaining > 0)
      {
         // Find the page for current position
         struct mulle_vm_tree_search_result current_result = _mulle_vm_tree_search( &vm->tree, current_pos);
         if( ! mulle_vm_tree_search_result_is_valid( current_result))
         {
            errno = EINVAL;
            return( NULL);
         }
         
         struct mulle_vm_page *current_page = current_result.node->page;
         uintptr_t current_offset = current_result.offset;
         
         // Calculate how much we can read from this page
         size_t available_in_page = current_page->used.length - current_offset;
         size_t to_copy = (bytes_remaining < available_in_page) ? bytes_remaining : available_in_page;
         
         // Copy data from this page
         memcpy( dest_buffer + bytes_copied,
                (uint8_t *) current_page->bytes + current_page->used.location + current_offset,
                to_copy);
         
         bytes_copied += to_copy;
         bytes_remaining -= to_copy;
         current_pos += to_copy;
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


#if 0
static int  _mulle_vm_walk_forward( struct mulle_vm *vm,
                                    struct mulle_range range,
                                    mulle_vm_walk_callback_t  *callback,
                                    void *userinfo)
{
   intptr_t                            offset;
   struct mulle_vm_node                *startinfo;
   struct mulle_vm_node                *endinfo;
   struct mulle_vm_node                *info;
   int                                 rval;
   struct mulle_data                   data;
   struct mulle_range                  page_range;
   struct mulle_range                  intersection;
   uintptr_t                           start_location;
   uintptr_t                           end_location;
   intptr_t                            startinfo_index;
   intptr_t                            endinfo_index;
   intptr_t                            info_index;
   struct mulle_vm_tree_search_result  start;
   struct mulle_vm_tree_search_result  end;
   uintptr_t                           vm_address;


   start_location = range.location;
   end_location   = mulle_range_get_last_location( range);

   start      = _mulle_vm_tree_search( vm, start_location);
   assert( start.node);
   startinfo  = start.info;
   vm_address = start_location;
   offset     = (intptr_t) start.offset;

   // Check if the read fits entirely inside the found page
   if( offset + range.length <= startinfo->page->used.length)
   {
      end     = start;
   }
   else
   {
      end     = _mulle_vm_tree_search( vm, end_location);
      assert( end.node);
   }
   endinfo = end.info;

   page_range   = mulle_range_make( vm_address, page->used.length);
   intersection = mulle_range_intersection( page_range, range);
   offset       = page->used.location + intersection.location - vm_address;
   data         = mulle_data_make( &((char *) page->bytes)[ offset],
                                   intersection.length);
   rval         = (*callback)( data, intersection, userinfo);

   // preset errno to something, that callback then can change
   errno = ENOENT;
   if( rval || end.node == start.node)
      return( rval);

   vm_address += intersection.length;  // move to end

   // Copy data page by page until either done or error
   node = start;
   for( ;;)
   {
      node = _mulle_vm_tree_next_node( &vm->tree, node);
      if( node == end.node)
         break;

      page_range = mulle_range_make( vm_address, page->used.length);
      data       = mulle_data_make( &((char *) page->bytes)[ page->used.location],
                                   page->used.length);
      rval       = (*callback)( data, page_range, userinfo);
      if( rval)
         return( rval);

      vm_address += page->used.length;
   }

   assert( endinfo != startinfo);  // just to think less here :)

   page_range      = mulle_range_make( vm_address, page->used.length);
   intersection    = mulle_range_intersection( page_range, range);
   data            = mulle_data_make( &((char *) page->bytes)[ page->used.location],
                                      intersection.length);

   rval            = (*callback)( data, intersection, userinfo);
   return( rval);
}
#endif


#if 0 // TODO: implement walk functions with proper tree navigation
static int  _mulle_vm_walk_backward( struct mulle_vm *vm,
                                     struct mulle_range range,
                                     mulle_vm_walk_callback_t  *callback,
                                     void *userinfo)
{
   // Implementation disabled for now
   return;
}
#endif


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
      return( EINVAL);

   vm_length = mulle_vm_get_length( vm);
   range     = mulle_range_validate_against_length( range, vm_length);
   if( ! range.length)
      return( 0);

   // Find the starting page
   start_result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( start_result))
      return( ERANGE);

   // Find the ending page
   end_result = _mulle_vm_tree_search( &vm->tree, mulle_range_get_last_location( range));
   if( ! mulle_vm_tree_search_result_is_valid( end_result))
      return( ERANGE);

   if( direction > 0)
   {
      // Forward walk
      current_vm_address = 0;
      mulle_vm_for( vm, current_page)
      {
         // Calculate the VM range for this node
         vm_range = mulle_range_make( current_vm_address, current_page->used.length);

         // Find intersection with requested range
         intersection = mulle_range_intersection( vm_range, range);
         if( intersection.length > 0)
         {
            // Calculate offset within the page
            size_t page_offset = (intersection.location >= current_vm_address) ?
                                 (intersection.location - current_vm_address) : 0;

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

         // Find intersection with requested range
         intersection = mulle_range_intersection( vm_range, range);
         if( intersection.length > 0)
         {
            // Calculate offset within the page
            size_t page_offset = (intersection.location >= current_vm_address) ?
                                 (intersection.location - current_vm_address) : 0;

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
   struct mulle_vm_node    *start;
   struct mulle_vm_node    *end;
   struct mulle_vm_page    *copy;
   uintptr_t               vm_address;
   uintptr_t               remainder;
   size_t                  length;

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

   struct mulle_vm_tree_search_result start_result;
   struct mulle_vm_tree_search_result end_result;
   
   start_result = _mulle_vm_tree_search( &src->tree, range.location);
   assert( mulle_vm_tree_search_result_is_valid( start_result));
   start        = start_result.node;

   end_result = _mulle_vm_tree_search( &src->tree, mulle_range_get_last_location( range));
   assert( mulle_vm_tree_search_result_is_valid( end_result));
   end        = end_result.node;

   // can we use page wholesale ?
   vm_address = 0;
   uintptr_t start_address = start_result.offset;
   if( start_address == range.location && start->page->used.length <= range.length)
   {
      copy = mulle_vm_page_retain( start->page);
      mulle_vm_tree_add_page( &dst->tree, copy);
   }
   else
   {
      // create a new page
      copy = _mulle_vm_copy_page( src, start->page, range);
      mulle_vm_tree_add_page( &dst->tree, copy);
   }
   vm_address += copy->used.length;

   // special case if we only copy one page, we are done otherwise...
   if( ! mulle_vm_tree_search_result_is_equal( start_result, end_result))
   {
      // Walk through nodes between start and end
      mulle_vm_tree_for( &src->tree, info)
      {
         if( info == start)
            continue; // skip start node, already handled
         if( info == end)
            break;    // don't include end node yet
            
         copy = mulle_vm_page_retain( info->page);
         mulle_vm_tree_add_page( &dst->tree, copy);
         vm_address += copy->used.length;
      }

      remainder = mulle_range_get_max( range) - vm_address;
      assert( remainder);

      // special treatment for last page
      if( end->page->used.length == remainder)
      {
         copy = mulle_vm_page_retain( end->page);
         mulle_vm_tree_add_page( &dst->tree, copy);
      }
      else
      {
         // create a new page
         copy = _mulle_vm_copy_page( src, end->page, range);
         mulle_vm_tree_add_page( &dst->tree, copy);
      }
      // vm_address += copy.used.length;
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

   // TODO: mulle_vm_delete( src, range);
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
   struct mulle_vm_node    *start;
   struct mulle_vm_node    *tail_node;
   struct mulle_vm_page    *tail_page;
   size_t                  length;

   if( dst == src)
   {
      errno = EINVAL;
      return( -1);
   }

   // delete range if any
   // TODO: mulle_vm_delete( dst, range);

   // if nothing to paste, we is done (so mean we must have a page in src)
   length = mulle_vm_get_length( src);
   if( length == 0)
      return( 0);

   tail_node   = NULL;
   start       = NULL;
   struct mulle_vm_tree_search_result start_result;
   
   start_result = _mulle_vm_tree_search( &dst->tree, range.location);
   if( mulle_vm_tree_search_result_is_valid( start_result))
   {
      start     = start_result.node;
      tail_node = _mulle_vm_split( dst, start_result, range.location);
      if( ! tail_node)
         start = NULL; // if no split happened, start is the insertion point
   }

   // Insert all pages from src into dst
   mulle_vm_tree_for( &src->tree, src_node)
   {
      //mulle_vm_page_retain( src_node->page);
      if( start)
      {
         mulle_vm_tree_insert_page_after_node( &dst->tree, src_node->page, start);
         struct mulle_vm_tree_search_result next_result = _mulle_vm_tree_search( &dst->tree, range.location + src_node->page->used.length);
         start = next_result.node;
      }
      else
      {
         mulle_vm_tree_add_page( &dst->tree, src_node->page);
      }
   }

   if( tail_node)
   {
      tail_page = _mulle_vm_node_get_page( tail_node);
      // Find the last inserted node to check if we can join with tail_page
      struct mulle_vm_tree_search_result last_result = _mulle_vm_tree_search( &dst->tree, range.location);
      if( mulle_vm_tree_search_result_is_valid( last_result))
      {
         struct mulle_vm_node *last_node = last_result.node;
         switch( _mulle_vm_page_join( last_node->page, tail_page))
         {
         case -1 : // can drop tail
            mulle_vm_page_release( tail_page);
            break;

         case 0 :  // no join possible, add tail separately
            mulle_vm_tree_add_page( &dst->tree, tail_page);
            break;

         case +1 : // can drop prev (replace with tail)
            mulle_vm_tree_remove( &dst->tree, range.location);
            mulle_vm_tree_add_page( &dst->tree, tail_page);
            break;
         }
      }
      else
      {
         mulle_vm_tree_add_page( &dst->tree, tail_page);
      }
   }

   // Note: With tree-based implementation, addresses are computed dynamically
   // based on tree traversal order and total lengths, so no manual address
   // adjustment is needed here like in the old mappings array approach.

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
