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
   .alloc_pages = mulle_mmap_alloc_pages,
   .free_pages  = mulle_mmap_free_pages,
   .skew        = MULLE__VM_SKEW_DEFAULT
};

// we don't really care to leak check this, so use &mulle_allocator_stlib
MULLE_C_CONSTRUCTOR( load)
static void   load( void)
{
   mulle_vm_config_default.pagesize = mulle_mmap_get_system_pagesize();
}


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
   mulle_pointerarray_init( &vm->freed_ospages,
                            0,
                            allocator);
   assert( (! config->pagesize || config->pagesize == mulle_pow2round( config->pagesize)) && "mulle-vm: VM pagesize corrupted");
   assert( config->skew <= 100 && "mulle-vm: VM skew must be >= 0 <= 100");

   vm->config = *config;
   // AI documented it like this, LOL
   vm->config.alloc_pages = vm->config.alloc_pages ? vm->config.alloc_pages : mulle_mmap_alloc_pages;
   vm->config.free_pages  = vm->config.free_pages  ? vm->config.free_pages  : mulle_mmap_free_pages;
}


static void   _mulle_vm_reset_freed_ospages( struct mulle_vm *vm);
static void   _mulle_vm_release_all_pages( struct mulle_vm *vm);


void   mulle_vm_done( struct mulle_vm *vm)
{
   if( ! vm)
      return;

   _mulle_vm_reset_freed_ospages( vm);
   _mulle_pointerarray_done( &vm->freed_ospages);
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
   mulle_vm_for( vm, info)
   {
      assert( info->page != NULL && "mulle-vm: mapping with no page");
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

   assert( (page->size % mulle_vm_get_pagesize(vm)) == 0 && "mulle-vm: VM page is not a multiple of pagesize");

   assert(((uintptr_t) page->bytes % mulle_vm_get_pagesize( vm)) == 0 && "mulle-vm: bytes of VM page are not size aligned");
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
 **
 **
 */
static void *   _mulle_vm_alloc_ospages( struct mulle_vm *vm, size_t size)
{
   size_t   pagesize;
   void     *mem;

   assert( size == round_up_to_pagesize( size, mulle_vm_get_pagesize( vm)));

   pagesize = mulle_vm_get_pagesize( vm);
   mem      = (size == pagesize)
              ? mulle_pointerarray_pop( &vm->freed_ospages)
              : NULL;

   if( ! mem)
   {
      mem = (vm->config.alloc_pages)( size); // hmm ...
      if( ! mem)
         (_mulle_rbtree_get_allocator( (struct mulle_rbtree *) &vm->tree)->fail)( NULL, NULL, size);
   }

#ifdef DEBUG
   mulle_memset_uint32( mem, 0x214E3A55, size);
#endif
   return( mem);
}


// the mulle_vm_page MUST have been removed from pages already!
// (if you are freeing all the pages from mulle_vm_page)
static void   _mulle_vm_free_ospages( struct mulle_vm *vm,
                                      void *mem,
                                      size_t size)
{
   size_t   count;
   size_t   pagesize;
   size_t   avail;
   int      rval;

   pagesize = mulle_vm_get_pagesize( vm);
   assert( size == round_down_to_pagesize( size, pagesize));

   count = mulle_pointerarray_get_count( &vm->freed_ospages);
   assert( vm->config.cachesize >= count);
   avail = vm->config.cachesize - count;

   //
   // freed_ospages individual pages for reuse
   //
   while( count < avail)
   {
      mulle_pointerarray_add( &vm->freed_ospages, mem);
      size -= pagesize;
      if( ! size)
         return;

      mem = &((char *) mem)[ pagesize];
      ++count;
   }

   // free rest wholesale
   rval = (vm->config.free_pages)( mem, size);
   if( rval)
   {
      // what to do ?
      fprintf( stderr, "mulle-vm: page %p wrongly freed by mishaved thread ? (%d)\n", mem, rval);
#ifdef DEBUG
      abort();
#endif
   }
}


static void   _mulle_vm_free_unused_ospages( struct mulle_vm *vm,
                                             struct mulle_vm_page *page)
{
#ifdef MULLE_VM_SHRINK_OSPAGE_USAGE
   size_t   free_size;
   char     *free_page;

   // do not do shared pages
   if( _mulle_atomic_pointer_read( &page->retain_count))
      return;
   //
   // the page can't be empty though, if it is, it's an error. you should
   // have called mulle_vm_remove_page_at_index
   //
   assert( page->used.length);

   // try to recover pages from the current set of pages
   //
   // figure out if the free space is in
   // front and or back
   free_size = mulle_vm_page_get_available_front( page);
   if( free_size >= mulle_vm_get_pagesize( vm))
   {
      assert( page->size == round_down_to_pagesize( page->size, mulle_vm_get_pagesize( vm)));

      free_size            = round_down_to_pagesize( free_size, mulle_vm_get_pagesize( vm));
      free_page            = page->bytes;

      page->bytes          = &((char *) page->bytes)[ free_size];
      page->used.location -= free_size;

      _mulle_vm_free_ospages( vm, free_page, free_size);
      page->size          -= free_size;
   }

   free_size = mulle_vm_page_get_available_back( page);
   if( free_size >= mulle_vm_get_pagesize( vm))
   {
      assert( page->size == round_down_to_pagesize( page->size, mulle_vm_get_pagesize( vm)));

      free_size            = round_down_to_pagesize( free_size, mulle_vm_get_pagesize( vm));
      free_page            = &((char *) page->bytes)[ page->size - free_size];

      _mulle_vm_free_ospages( vm, free_page, free_size);

      page->size          -= free_size;
   }
#endif
}


static void   _mulle_vm_reset_freed_ospages( struct mulle_vm *vm)
{
   void     *page;
   size_t   pagesize;
   int      rval;

   pagesize = mulle_vm_get_pagesize( vm);
   mulle_pointerarray_for( &vm->freed_ospages, page)
   {
      rval = (vm->config.free_pages)( page, pagesize);
      if( rval)
      {
      // what to do ?
         fprintf( stderr, "mulle-vm: page %p wrongly freed by mishaved thread ? (%d)\n",
                           page, rval);
#ifdef DEBUG
         abort();
#endif
      }
   }
   mulle_pointerarray_reset( &vm->freed_ospages);
}



/**
 **
 **
 **
 */
#ifndef MULLE_TEST
static
#endif
struct mulle_vm_page *
   _mulle_vm_new_page_with_bytes( struct mulle_vm *vm,
                                  void *bytes,
                                  size_t length)
{
   size_t                 size;
   char                   *mem;
   size_t                 free_size;
   struct mulle_range     range;
   struct mulle_vm_page   *page;

   assert( vm->config.skew <= 100);

   size = round_up_to_pagesize( length, mulle_vm_get_pagesize( vm));
   mem  = _mulle_vm_alloc_ospages( vm, size);

   free_size      = size - length;
   range.location = free_size * vm->config.skew / 100;
   range.length   = length;

   // copy block to middle (or offset from there) so we have space in front
   // and back to add.
   memcpy( &mem[ range.location], bytes, length);
#ifdef DEBUG
   mulle_memset_uint32( &mem[ 0], 0xFEEDFACE, range.location);
   mulle_memset_uint32( &mem[ mulle_range_get_max( range)], 0xFEEDFACE, size - mulle_range_get_max( range));
#endif

   page = mulle_vm_page_alloc();
   mulle_vm_page_init( page, mem, range, size);
   return( page);
}


static inline struct mulle_vm_page *
   _mulle_vm_retain_page( struct mulle_vm_page *page)
{
   _mulle_atomic_pointer_increment( &page->retain_count);
   return( page);
}


// the page MUST have been removed from pages already!
static inline void   _mulle_vm_release_page( struct mulle_vm *vm,
                                             struct mulle_vm_page *page)
{
   if( ! _mulle_atomic_pointer_decrement( &page->retain_count))
   {
      _mulle_vm_free_ospages( vm, page->bytes, page->size); // hmm ...
      page->bytes = (void *) (uintptr_t) 0xDEADFACEDEADFACELL;
      mulle_vm_page_free( page);
   }
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

   _mulle_vm_release_page( vm, page);
   return( copy);
}


/**
 **
 **
 **
 */

#ifndef MULLE_TEST
static
#endif
MULLE_C_NONNULL_FIRST
void   _mulle_vm_add_page( struct mulle_vm *vm, struct mulle_vm_page *page)
{
   mulle_vm_tree_add( &vm->tree, page);
}


MULLE_C_NONNULL_FIRST_SECOND
void   _mulle_vm_append_ospages( struct mulle_vm *vm,
                                 void *pages,
                                 size_t length)
{
   size_t                 size;
   struct mulle_vm_page   *page;

   size = round_up_to_pagesize( length, mulle_vm_get_pagesize( vm));
   page = mulle_vm_page_alloc();
   mulle_vm_page_init( page, pages, mulle_range_make( 0, length), size);
   _mulle_vm_add_page( vm, page);
}


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
   uintptr_t              offset;
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

struct mulle_vm_page *
   _mulle_vm_split( struct mulle_vm *vm,
                    struct mulle_vm_tree_search_result result,
                    uintptr_t at)
{
#if 0 // TODO: implement split functionality
   struct mulle_vm_page   *page;
   struct mulle_vm_page   *tail;
   size_t                 offset;
   size_t                 head_len, tail_len;
   void                   *tail_bytes;

   assert( vm);
   assert( page);
   assert( mulle_range_contains_location( mulle_vm_node_get_range( info), at));

   // If 'at' is at the start of the page's used range, no splitting needed
   if( at == 0)
       return( NULL);

   page     = mulle_vm_tree_search_result_get_page( &result);

   // Compute lengths of head and tail parts
   head_len = at;
   tail_len = page->used.length - head_len;

   // Prepare the "tail" bytes pointer inside the old page
   tail_bytes = (uint8_t *) page->bytes + page->used.location + offset;

   // Create new page for the tail part with the tail bytes copied
   tail = _mulle_vm_new_page_with_bytes( vm, tail_bytes, tail_len);

   // Copy-on-write if needed before modifying original page
   // Shrink original page to only hold the head part
   page              = _mulle_vm_will_write_page( vm, page);
   page->used.length = head_len;

   mulle_vm_tree_mark_result_as_dirty( result);
   // caller needs to mark us a dirty

   // Insert new tail page right after the original page in the VM
   mulle_vm_tree_insert_page_after_result( vm, tail, result);

   // Return pointer to the newly inserted tail page
   return( tail);
#endif
   // Simple stub - not implemented yet
   return( NULL);
}


#if 0 // TODO: fix missing semicolon and implement properly
static void   mulle_vm_tree_did_write( struct mulle_vm *vm, struct mulle_rbnode *node)
{
   _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, node);
   _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
}
#endif

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

#if 0
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
      return( 0);

   assert( bytes);

   result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      length = mulle_vm_get_length( vm);
      if( length != range.location) // clean append ?
      {
         errno = ERANGE;
         return( -1);
      }

      // append at end (ez)
      new_page = _mulle_vm_new_page_with_bytes( vm, bytes, range.length);
      mulle_vm_insert_page( vm, new_page, range.location);
      return( 0);
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
         return( 0);
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
   new_page = _mulle_vm_new_page_with_bytes( vm, bytes, range.length);
   mulle_vm_insert_page( vm, new_page, range.location);

   return( 0);
}
#endif


// Simple implementation of mulle_vm_append that creates a new page
int   mulle_vm_append( struct mulle_vm *vm,
                       void *bytes,
                       size_t length)
{
   struct mulle_vm_page   *page;
   
   if( ! vm || ! bytes || length == 0)
      return( EINVAL);
   
   // Use the VM's page allocation system which handles proper alignment
   page = _mulle_vm_new_page_with_bytes( vm, bytes, length);
   if( ! page)
      return( ENOMEM);
   
   _mulle_vm_add_page( vm, page);
   
   return( 0);
}


// Simple implementation of mulle_vm_insert focused on address 0 insertion (prepending)
int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
{
   struct mulle_vm_page                 *new_page;
   struct mulle_vm_page                 *first_page;
   struct mulle_vm_tree_search_result   result;
   size_t                               vm_length;
   size_t                               insert_length;
   
   if( ! vm || ! bytes)
      return( EINVAL);
   
   // Get length of data to insert from range parameter
   insert_length = range.length;
   
   if( insert_length == 0)
      return( 0);
   
   vm_length = mulle_vm_get_length( vm);
   
   // Handle insertion at end (append case)
   if( range.location == vm_length)
   {
      return( mulle_vm_append( vm, bytes, insert_length));
   }
   
   // Handle insertion at arbitrary positions
   // Search for the position to determine which page contains it
   result = _mulle_vm_tree_search( &vm->tree, range.location);
   if( ! mulle_vm_tree_search_result_is_valid( result))
   {
      // Should not happen since we already checked range.location <= vm_length
      errno = EINVAL;
      return( -1);
   }
   
   // Special case: insertion at address 0 (prepending to start)
   if( range.location == 0)
   {
      first_page = result.node->page;
      
      // Check if first page has space at the front for prepending
      size_t available_front = mulle_vm_page_get_available_front( first_page);
      if( available_front >= insert_length)
      {
         // We can prepend to the existing first page
         first_page = _mulle_vm_will_write_page( vm, first_page);
         result.node->page = first_page; // Update tree node after potential COW
         
         // Move the used range forward to make space at the front
         first_page->used.location -= insert_length;
         first_page->used.length   += insert_length;
         
         // Copy the new data to the front
         memcpy( (uint8_t *) first_page->bytes + first_page->used.location,
                 bytes,
                 insert_length);
         
         // Mark the specific node as dirty and then recalculate tree totals
         struct mulle_rbnode *rbnode = mulle_vm_node_get_rbnode( result.node);
         _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, rbnode);
         _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
         
         return( 0);
      }
      else
      {
         // Create a new page and insert it before the first node
         new_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
         if( ! new_page)
            return( ENOMEM);
         
         // Insert the new page before the first node
         int rval = mulle_vm_tree_insert_page_before_node( &vm->tree, new_page, result.node);
         if( rval)
         {
            _mulle_vm_release_page( vm, new_page);
            return( rval);
         }
         
         return( 0);
      }
   }
   
   // Check if we're inserting at the beginning of a page (offset == 0)
   // This means we're at a page boundary - try to optimize by using adjacent page space
   if( result.offset == 0)
   {
      fprintf( stderr, "DEBUG: Inserting at page boundary, offset=0\n");
      // Try to find the previous page to see if we can use adjacent page optimization
      struct mulle_vm_tree_search_result prev_result = {0};
      struct mulle_vm_page *prev_page = NULL;
      struct mulle_vm_page *curr_page = result.node->page;
      
      if( range.location > 0)
      {
         // Get the previous page (the page containing location-1)
         prev_result = _mulle_vm_tree_search( &vm->tree, range.location - 1);
         if( mulle_vm_tree_search_result_is_valid( prev_result))
         {
            prev_page = prev_result.node->page;
            fprintf( stderr, "DEBUG: Found previous page\n");
         }
         else
         {
            fprintf( stderr, "DEBUG: Previous page search failed\n");
         }
      }
      
      // Check if we can optimize by using available space in adjacent pages
      if( prev_page && curr_page)
      {
         // For the previous page, we can only append at the back
         size_t prev_available_back = mulle_vm_page_get_available_back( prev_page);
         
         // For the current page, we can use total available space because
         // _mulle_vm_page_insert_bytes can move data to make room at the front
         size_t curr_total_available = mulle_vm_page_get_available_front( curr_page) + 
                                       mulle_vm_page_get_available_back( curr_page);
         
         size_t total_available = prev_available_back + curr_total_available;
         
         fprintf( stderr, "DEBUG: prev_available_back=%zu, curr_total_available=%zu, total=%zu, insert_length=%zu\n", 
                 prev_available_back, curr_total_available, total_available, insert_length);
         
         // If we have enough space in adjacent pages, use it instead of creating new page
         if( total_available >= insert_length)
         {
            fprintf( stderr, "DEBUG: Using page optimization\n");
            size_t bytes_to_prev = (insert_length <= prev_available_back) ? insert_length : prev_available_back;
            size_t bytes_to_curr = insert_length - bytes_to_prev;
            
            // Insert part of data at the back of previous page
            if( bytes_to_prev > 0)
            {
               prev_page = _mulle_vm_will_write_page( vm, prev_page);
               prev_result.node->page = prev_page; // Update tree node after potential COW
               
               int rval = _mulle_vm_page_insert_bytes( prev_page, prev_page->used.length, bytes, bytes_to_prev);
               if( rval != 0)
               {
                  fprintf( stderr, "DEBUG: Failed to insert into prev_page, rval=%d\n", rval);
                  // Fallback to creating new page if insertion fails
                  goto create_new_page;
               }
               fprintf( stderr, "DEBUG: Successfully inserted %zu bytes into prev_page\n", bytes_to_prev);
            }
            
            // Insert remaining data at the front of current page
            if( bytes_to_curr > 0)
            {
               curr_page = _mulle_vm_will_write_page( vm, curr_page);
               result.node->page = curr_page; // Update tree node after potential COW
               
               int rval = _mulle_vm_page_insert_bytes( curr_page, 0, (uint8_t*)bytes + bytes_to_prev, bytes_to_curr);
               if( rval != 0)
               {
                  fprintf( stderr, "DEBUG: Failed to insert into curr_page, rval=%d\n", rval);
                  // This is problematic - we already modified prev_page
                  // For now, continue anyway as the data is partially inserted
               }
               else
               {
                  fprintf( stderr, "DEBUG: Successfully inserted %zu bytes into curr_page\n", bytes_to_curr);
               }
            }
            
            // Mark both nodes as dirty and recalculate tree totals
            if( prev_page)
            {
               struct mulle_rbnode *prev_rbnode = mulle_vm_node_get_rbnode( prev_result.node);
               _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, prev_rbnode);
            }
            if( curr_page)
            {
               struct mulle_rbnode *curr_rbnode = mulle_vm_node_get_rbnode( result.node);
               _mulle__rbtree_mark_node_as_dirty( (struct mulle__rbtree *) &vm->tree, curr_rbnode);
            }
            _mulle_rbtree_walk_dirty( (struct mulle_rbtree *) &vm->tree);
            
            fprintf( stderr, "DEBUG: Page optimization completed successfully\n");
            return( 0);
         }
         else
         {
            fprintf( stderr, "DEBUG: Not enough space for optimization\n");
         }
      }
      else
      {
         fprintf( stderr, "DEBUG: Missing pages: prev_page=%p, curr_page=%p\n", prev_page, curr_page);
      }
      
create_new_page:
      // Create a new page and insert it before the current node
      new_page = _mulle_vm_new_page_with_bytes( vm, bytes, insert_length);
      if( ! new_page)
         return( ENOMEM);
      
      // Insert the new page before the current node
      int rval = mulle_vm_tree_insert_page_before_node( &vm->tree, new_page, result.node);
      if( rval)
      {
         _mulle_vm_release_page( vm, new_page);
         return( rval);
      }
      
      return( 0);
   }
   
   // If we reach here, we're inserting in the middle of a page (offset > 0)
   // This requires splitting the page at the insertion point
   // For now, return ENOSYS since page splitting is not implemented yet
   errno = ENOSYS;
   return( -1);
}


#if 0
void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range)
{
   struct mulle_vm_node                 *start;
   struct mulle_vm_node                 *end;
   struct mulle_vm_node                 *prev;
   struct mulle_vm_page                 *page;
   int                                  end_deleted;
   intptr_t                             adjust_idx;
   intptr_t                             end_index;
   intptr_t                             idx;
   intptr_t                             start_index;
   size_t                               length;
   uintptr_t                            new_address;
   uintptr_t                            offset;
   uintptr_t                            remainder;
   void                                 *delete;
   void                                 *keep;
   void                                 *sentinel;
   struct mulle_vm_tree_search_result   start;
   struct mulle_vm_tree_search_result   end;

   if( ! vm)
      return;

   mulle_vm_assert( vm);

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! range.length)
      return;

   start = _mulle_vm_tree_search( &vm->tree, range.location);
   end   = _mulle_vm_tree_search( &vm->tree, mulle_range_get_last_location( range));

   assert( mulle_vm_tree_search_result_is_valid( start));
   assert( mulle_vm_tree_search_result_is_valid( end));
   // though not absolutely necessary, deletion from back to front is
   // nicer

   end_deleted = 0;

   //
   // special case if we only delete within one page
   //
   if( mulle_vm_tree_search_result_is_equal( start, end))
   {
      // delete everything ?
      page    = mulle_vm_tree_search_result_get_page( start);
      offset  = mulle_vm_tree_search_result_get_offset( start);
      if( offset == 0 && page->used.length == range.length)
      {
         mulle_vm_tree_remove( &vm->tree, range.location);
         mulle_vm_assert( vm);
         return;
      }

      page = _mulle_vm_will_write_page( vm, page);
      // remove stuff from front only ?
      if( ! offset)
      {
         page->used.location += range.length;
      }
      else
      {
         // remove stuff from back only ?
         if( offset == mulle_range_max( page->used))
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

            delete     = &((char *) page->bytes)[ page->used.location + offset];
            keep       = &((char *) delete)[ range.length];
            sentinel   = &((char *) page->bytes)[ page->used.location + page->used.length];

            memmove( delete, keep, sentinel - keep);
         }
      }
      page->used.length -= range.length;

      _mulle_vm_free_unused_ospages( vm, page);
      mulle_vm_assert_page( vm, page);
      mulle_vm_assert( vm);
      return;
   }

   //
   // multipage handling, we delete from the back
   //
   // We know that last page is a hit and that we have at least one more
   // page. So we only have deletion of all or some byte from the front (sic)
   // of this page
   //

   adjust_idx = start_index + 1;

   remainder = mulle_range_get_max( range) - end->address;
   assert( remainder);

   // check if we can ditch the whole page
   end_deleted = end->page->used.length == remainder;
   if( end_deleted)
   {
      _mulle_vm_remove_index( vm, end_index);
   }
   else
   {
      // remove head bytes from page
      end = _mulle_vm_will_write_page( vm, end);
      end->page->used.location += remainder;
      end->page->used.length   -= remainder;
      _mulle_vm_free_unused_ospages( vm, end->page);
      mulle_vm_assert_page( vm, end->page);
   }

   for( idx = end_index - 1; idx > start_index; --idx)
   {
      _mulle_vm_remove_index( vm, idx);
   }

   start     = _mulle_vm_get_mapping( vm, start_index);
   remainder = range.location - start->address;

   //
   //  0-6                       0-2    <-- start_index <-- end_index
   //  6-14                      2-10   ***
   // 14-22 -- delete {2,4} --> 10-18
   // 22-23                     18-10
   //
   //
   //  0-6                       0-2    <-- start_index
   //  6-14                      2-6    <-- end_index
   // 14-22 -- delete {2,8} -->  6-14   ***
   // 22-23                     14-15
   //
   // problem is we have between start and end a number of pages, we might
   // have gotten rid of as well
   if( start->address == range.location && start->page->used.length <= range.length)
   {
      _mulle_vm_remove_index( vm, start_index);
      adjust_idx--;
   }
   else
   {
      // remove tail bytes from page, this means we have to adjust the next
      // page by remainder, but subsequent pages by - (intptr_t) range.length
      start = _mulle_vm_will_write_page( vm, start);

      start->page->used.length = remainder;
      _mulle_vm_free_unused_ospages( vm, start->page);
      mulle_vm_assert_page( vm, start->page);
   }

   if( ! end_deleted)
   {
      // After multipage deletion, recalculate the address of the remaining end page
      // It should be positioned right after any remaining start page content
      new_address = 0;
      if( adjust_idx > 0)
      {
         // There's a remaining start page, position end page after it
         prev        = _mulle_vm_get_mapping( vm, adjust_idx - 1);
         new_address = prev->address + prev->page->used.length;
      }
      _mulle_vm_change_address( vm, adjust_idx, new_address);
      adjust_idx++;
   }

   _mulle_vm_adjust_addresses( vm, adjust_idx, - (intptr_t) range.length);
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
   return( -1);
}
#endif


int  _mulle_vm_walk( struct mulle_vm *vm,
                     struct mulle_range range,
                     mulle_vm_walk_callback_t  *callback,
                     void *userinfo,
                     int direction)
{
#if 0 // TODO: implement walk functions
   // Implementation disabled for now
#endif
   return( -1);
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
   struct mulle_vm_page       *copy;
   struct mulle_vm_page       *page;
   uintptr_t                  vm_address;
   uintptr_t                  remainder;
   intptr_t                   end_index;
   intptr_t                   start_index;
   intptr_t                   idx;
   size_t                     length;

   if( dst == src)
   {
      errno = EINVAL;
      return( -1);
   }

   assert( dst->config.alloc_pages == src->config.alloc_pages);
   assert( dst->config.free_pages == src->config.free_pages);

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
      copy = _mulle_vm_retain_page( start->page);
      _mulle_vm_add_page( dst, copy);
   }
   else
   {
      // create a new page
      copy = _mulle_vm_copy_page( src, start->page, range);
      _mulle_vm_add_page( dst, copy);
   }
   vm_address += copy->used.length;

   // special case if we only copy one page, we are done otherwise...
   if( ! mulle_vm_tree_search_result_is_equal( start_result, end_result))
   {
      struct mulle_vm_node *current_node = start;
      struct mulle_vm_node *next_node;
      
      // Walk through nodes between start and end
      mulle_vm_tree_for( &src->tree, info)
      {
         if( info == start)
            continue; // skip start node, already handled
         if( info == end)
            break;    // don't include end node yet
            
         copy = _mulle_vm_retain_page( info->page);
         _mulle_vm_add_page( dst, copy);
         vm_address += copy->used.length;
      }

      remainder = mulle_range_get_max( range) - vm_address;
      assert( remainder);

      // special treatment for last page
      if( end->page->used.length == remainder)
      {
         copy = _mulle_vm_retain_page( end->page);
         _mulle_vm_add_page( dst, copy);
      }
      else
      {
         // create a new page
         copy = _mulle_vm_copy_page( src, end->page, range);
         _mulle_vm_add_page( dst, copy);
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
   struct mulle_vm_page        *tail_page;
   struct mulle_vm_node    *prev;
   intptr_t                    idx;
   intptr_t                    start_index;
   size_t                      length;

   if( dst == src)
   {
      errno = EINVAL;
      return( -1);
   }

   assert( dst->config.alloc_pages == src->config.alloc_pages);
   assert( dst->config.free_pages == src->config.free_pages);

   // delete range if any
   // TODO: mulle_vm_delete( dst, range);

   // if nothing to paste, we is done (so mean we must have a page in src)
   length = mulle_vm_get_length( src);
   if( length == 0)
      return( 0);

   tail_page   = NULL;
   start       = NULL;
   struct mulle_vm_tree_search_result start_result;
   
   start_result = _mulle_vm_tree_search( &dst->tree, range.location);
   if( mulle_vm_tree_search_result_is_valid( start_result))
   {
      start     = start_result.node;
      tail_page = _mulle_vm_split( dst, start_result, range.location);
      if( ! tail_page)
         start = NULL; // if no split happened, start is the insertion point
   }

   // Insert all pages from src into dst
   mulle_vm_tree_for( &src->tree, src_node)
   {
      _mulle_vm_retain_page( src_node->page);
      if( start)
      {
         mulle_vm_tree_insert_page_after_node( &dst->tree, src_node->page, start);
         struct mulle_vm_tree_search_result next_result = _mulle_vm_tree_search( &dst->tree, range.location + src_node->page->used.length);
         start = next_result.node;
      }
      else
      {
         _mulle_vm_add_page( dst, src_node->page);
      }
   }

   if( tail_page)
   {
      // Find the last inserted node to check if we can join with tail_page
      struct mulle_vm_tree_search_result last_result = _mulle_vm_tree_search( &dst->tree, range.location);
      if( mulle_vm_tree_search_result_is_valid( last_result))
      {
         struct mulle_vm_node *last_node = last_result.node;
         switch( _mulle_vm_page_join( last_node->page, tail_page))
         {
         case -1 : // can drop tail
            _mulle_vm_release_page( dst, tail_page);
            break;

         case 0 :  // no join possible, add tail separately
            _mulle_vm_add_page( dst, tail_page);
            break;

         case +1 : // can drop prev (replace with tail)
            mulle_vm_tree_remove( &dst->tree, range.location);
            _mulle_vm_add_page( dst, tail_page);
            break;
         }
      }
      else
      {
         _mulle_vm_add_page( dst, tail_page);
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
   struct mulle_vm_node         *info;
   struct mulle_vm_statistics   stats = { 0 };

   if( vm)
   {
      mulle_vm_for( vm, info)
      {
         mulle_vm_assert_page( vm, info->page);

         stats.vm_size   += info->page->used.length;
         stats.page_size += info->page->size;
      }
   }
   return( stats);
}
