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


/**
 **
 **
 **
 */
static inline struct mulle_range
   mulle_vm_mapping_get_range( struct mulle_vm_mapping *info)
{
   if( ! info)
      return( mulle_range_make_invalid());

   return( mulle_range_make( info->address, info->page->used.length));
}


static inline struct mulle_data
   mulle_vm_mapping_get_data_in_range( struct mulle_vm_mapping *info,
                                       struct mulle_range range)
{
   if( ! info)
      return( mulle_data_make_invalid());

   range.location -= info->address;
   range           = mulle_range_validate_against_length( range, info->page->used.length);
   return( mulle_data_make( mulle_vm_page_address( info->page, range.location),
                            range.length));
}



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

   mulle_structarray_init( &vm->mappings,
                           sizeof( struct mulle_vm_mapping),
                           alignof( struct mulle_vm_mapping),
                           0,
                           allocator);
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

   _mulle_vm_release_all_pages( vm);
   _mulle_structarray_done( &vm->mappings);

   if( vm->config.shutdown)
      (*vm->config.shutdown)( vm);

}


void   mulle_vm_reset( struct mulle_vm *vm)
{
   if( ! vm)
      return;

   _mulle_vm_release_all_pages( vm);
   _mulle_structarray_reset( &vm->mappings);
}


static inline void   mulle_vm_assert( struct mulle_vm *vm)
{
#if DEBUG_VERBOSE || ! defined( NDEBUG)
   uintptr_t              vm_address;
   struct mulle_vm_page   *page;
   unsigned               i;
#endif

#ifndef NDEBUG
   uintptr_t    expect;
#endif
   if( ! vm)
      return;

#ifndef NDEBUG
   expect = 0;
#endif
#if DEBUG_VERBOSE || ! defined( NDEBUG)
   i = 0;
   mulle_vm_mapping_for( vm, info)
   {
      page       = info->page;
      assert( page != NULL && "mulle-vm: mapping with no page");
      vm_address = info->address;
      ++i;

      // output vm range, size of pages, and hole in front and back
#ifdef DEBUG_VERBOSE
      mulle_fprintf( stderr, "%3u: %2td-%-2td \"%.*s\"\n",
                               i,
                               vm_address,
                               vm_address + page->used.length,
                               (int) page->used.length,
                               (char *) mulle_vm_page_address( page, 0));
#endif
#ifndef NDEBUG
      assert( info->address == expect && "mulle-vm: miscalculated vm_address of page");
      expect += info->page->used.length;
#endif
   }
#ifdef DEBUG_VERBOSE
   fprintf( stderr, "\n");
#endif
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
         (vm->mappings.allocator->fail)( NULL, NULL, size);
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
   _mulle_vm_retain_page( struct mulle_vm *vm,
                          struct mulle_vm_page *page)
{
   assert( vm);

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


void   _mulle_vm_release_all_pages( struct mulle_vm *vm)
{
   static struct mulle_vm_mapping  *mapping;

   mulle_structarray_for( &vm->mappings, mapping)
   {
      _mulle_vm_release_page( vm, mapping->page);
   }
}


static inline struct mulle_vm_mapping  *
   _mulle_vm_get_mapping( struct mulle_vm *vm, intptr_t index)
{
   struct mulle_vm_mapping   *info;

   info = _mulle_structarray_get( &vm->mappings, index);
   mulle_vm_assert_page( vm, info->page);
   return( info);
}


// ---
// Binary search helper for finding prefix sums on the fly
//
// Returns index of page containing logical_offset,
// sets *in_page_offset, *page_bytes_available.
// Returns -1 if not found.

static intptr_t
   _mulle_vm_search_mapping_index_exact( struct mulle_vm *vm,
                                          uintptr_t target_val)
{
   intptr_t                   low;
   intptr_t                   mid;
   intptr_t                   high;
   intptr_t                   n;
   uintptr_t                  mid_val;
   struct mulle_vm_mapping   *info;

   n = (intptr_t) _mulle_structarray_get_count( &vm->mappings);
   if (n == 0)
       return( -1);

   low  = 0;
   high = n - 1;

   while( low <= high)
   {
      mid     = low + (high - low) / 2;
      info    = _mulle_vm_get_mapping( vm, mid);
      mid_val = (uintptr_t) info->address;

      if (mid_val == target_val)
         return( mid);      // exact match found

      if (mid_val < target_val)
         low = mid + 1;
      else
         high = mid - 1;
   }

    // Exact value not found
    return( -1);
}



static intptr_t   _mulle_vm_search_mapping_index( struct mulle_vm *vm,
                                                   uintptr_t target_val)
{
   intptr_t                   low;
   intptr_t                   mid;
   intptr_t                   high;
   intptr_t                   n;
   uintptr_t                  upper_val;
   uintptr_t                  mid_val;
   struct mulle_vm_mapping   *info;

   n = (intptr_t) _mulle_structarray_get_count( &vm->mappings);
   if( n == 0)
      return( -1);

   upper_val = mulle_vm_get_length( vm);
   if( target_val >= upper_val)
      return( -1);

   low  = 0;
   high = n;

   while( low < high)
   {
      mid     = low + (high - low) / 2;
      info    = _mulle_vm_get_mapping( vm, mid);
      mid_val = (uintptr_t) info->address;

      if( mid_val <= target_val)
         low = mid + 1;  // mid is valid, search right side for better match
      else
         high = mid;     // too big, search left side
   }

   return( (low == 0) ?  -1 : low - 1);
}


/**
 **
 ** COW
 **
 */

//
// this will possibly change the page in info and in vm->mappings, if you
// cached the pointer in info->page, you are in deep shit now
//
void   _mulle_vm_will_write_mapping( struct mulle_vm *vm,
                                     struct mulle_vm_mapping *info,
                                     intptr_t index)
{
   struct mulle_vm_page   *private_page;
   struct mulle_vm_page   *shared_page;
   void                   *bytes;
   size_t                 length;

   // if not shared we don't need to copy on write
   shared_page = info->page;
   if( ! _mulle_atomic_pointer_read( &shared_page->retain_count))
      return;

   //
   // copy on write
   //
   bytes        = mulle_vm_page_address( shared_page, 0);
   length       = shared_page->used.length;
   private_page = _mulle_vm_new_page_with_bytes( vm, bytes, length);

   _mulle_vm_release_page( vm, shared_page);
   info->page   = private_page;
}


/**
 **
 **
 **
 */
static void   _mulle_vm_change_address( struct mulle_vm *vm,
                                        intptr_t index,
                                        uintptr_t adr)
{
   struct mulle_vm_mapping  *info;

   info = mulle_structarray_get_lenient( &vm->mappings, index);
   if( info)
      info->address = (intptr_t) adr;
}



static void   _mulle_vm_adjust_addresses( struct mulle_vm *vm,
                                          intptr_t index,
                                          intptr_t diff)
{
   struct mulle_vm_mapping  *info;

   for(;;)
   {
      info = mulle_structarray_get_lenient( &vm->mappings, index);
      if( ! info)
         break;
      info->address += diff;
      ++index;
   }
}



/**
 **
 **
 **
 */
// even more lowlevel
MULLE_C_NONNULL_FIRST_SECOND
static void   __mulle_vm_add_page_with_address( struct mulle_vm *vm,
                                                struct mulle_vm_page *page,
                                                uintptr_t vm_address)
{
   struct mulle_vm_mapping  info;

   assert( vm);

   mulle_vm_assert_page( vm, page);

   info.page    = page;
   info.address = vm_address;
   mulle_structarray_add( &vm->mappings, &info);

   mulle_vm_assert( vm);
}


#ifndef MULLE_TEST
static
#endif
MULLE_C_NONNULL_FIRST
void   _mulle_vm_add_page( struct mulle_vm *vm, struct mulle_vm_page *page)
{
   uintptr_t   vm_address;

   vm_address = mulle_vm_get_length( vm);
   __mulle_vm_add_page_with_address( vm, page, vm_address);
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
void   _mulle_vm_remove_mapping_at_index( struct mulle_vm *vm,
                                          intptr_t index)
{
   struct mulle_vm_mapping   *mapping;

   mapping = _mulle_vm_get_mapping( vm, index);
   _mulle_vm_release_page( vm, mapping->page);
   mulle_structarray_remove_at_index( &vm->mappings, index);
}


int   mulle_vm_remove_mapping_at_location( struct mulle_vm *vm,
                                           uintptr_t location)
{
   intptr_t                  index;
   struct mulle_vm_page      *page;
   struct mulle_vm_mapping   *info;
   intptr_t                  diff;

   index = _mulle_vm_search_mapping_index_exact( vm, location);
   if( index < 0)
      return( ENOENT);

   info = _mulle_vm_get_mapping( vm, index);
   page = info->page;
   diff = -(intptr_t) page->used.length;

   _mulle_vm_remove_mapping_at_index( vm, index);
   _mulle_vm_adjust_addresses( vm, index, diff);

   mulle_vm_assert( vm);
}


// Private, no fixup
static void   _mulle_vm_insert_page( struct mulle_vm *vm,
                                     size_t index,
                                     struct mulle_vm_page *page)
{
   struct mulle_vm_mapping   info;
   struct mulle_vm_mapping   *prev;
   size_t                    count;

   mulle_vm_assert_page( vm, page);

   count = _mulle_structarray_get_count( &vm->mappings);
   if( index >= count)
   {
      if( index != count)
         abort();

      _mulle_vm_add_page( vm, page);
      return;
   }

   info.address = 0;
   info.page    = page;

   if( index)
   {
      prev         = _mulle_vm_get_mapping( vm, index - 1);
      info.address = prev->address + prev->page->used.length;
   }

   mulle_structarray_insert( &vm->mappings, index, &info);
}


// Public, with automatic fixup
void   mulle_vm_insert_page( struct mulle_vm *vm,
                             size_t index,
                             struct mulle_vm_page *page)
{
   if( ! vm)
      return;

   assert( page);

   _mulle_vm_insert_page( vm, index, page);
   _mulle_vm_adjust_addresses( vm, index + 1, page->used.length);

   mulle_vm_assert( vm);
}


struct mulle_vm_page *
   _mulle_vm_copy_page( struct mulle_vm *vm,
                        struct mulle_vm_page *page,
                        uintptr_t vm_address,
                        struct mulle_range range)
{
   struct mulle_vm_page   *copy;
   struct mulle_range     page_range;
   struct mulle_range     intersection;
   uintptr_t              offset;
   void                   *bytes;

   // figure out what we need
   page_range   = mulle_range_make( vm_address, page->used.length);
   intersection = mulle_range_intersection( range, page_range);

   assert( intersection.length);
   assert( intersection.location >= vm_address);

   offset = intersection.location - vm_address;
   bytes  = mulle_vm_page_address( page, offset);
   copy   = _mulle_vm_new_page_with_bytes( vm, bytes, intersection.length);
   return( copy);
}



// Splits the page at 'at' (logical vm address).
// If 'at' == info->address (start of page), returns NULL (no split).
// Otherwise, shrinks info->page to head, inserts new tail page after it,
// and returns pointer to new tail page.
//
// Precondition: vm, info, and info->page are valid; at is within the used range.

struct mulle_vm_page *
   _mulle_vm_split_mapping_at( struct mulle_vm *vm,
                               struct mulle_vm_mapping *info,
                               intptr_t index,
                               uintptr_t at)
{
    struct mulle_vm_page   *page;
    struct mulle_vm_page   *tail;
    size_t                 offset;
    size_t                 head_len, tail_len;
    void                   *tail_bytes;

    assert( vm);
    assert( info);
    assert( info->page);
    assert( mulle_range_contains_location( mulle_vm_mapping_get_range( info), at));

    // If 'at' is at the start of the page's used range, no splitting needed
    if( at == info->address)
        return( NULL);

    // Compute offset into the page's used bytes where split should occur
    offset = at - info->address;

    // Compute lengths of head and tail parts
    head_len = offset;
    tail_len = info->page->used.length - head_len;
    page     = info->page;
    if( index == -1)
      index  = _mulle_vm_search_mapping_index( vm, at);

    // Prepare the "tail" bytes pointer inside the old page
    tail_bytes = (uint8_t *) page->bytes + page->used.location + offset;

    // Create new page for the tail part with the tail bytes copied
    tail = _mulle_vm_new_page_with_bytes( vm, tail_bytes, tail_len);

    // Copy-on-write if needed before modifying original page
    // Shrink original page to only hold the head part
    _mulle_vm_will_write_mapping( vm, info, index);
    info->page->used.length = head_len;

    // Insert new tail page right after the original page in the VM
    _mulle_vm_insert_page( vm, index + 1, tail);

    // Return pointer to the newly inserted tail page
    return( tail);
}



/**
 **
 **
 **
 */
int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
{
   struct mulle_range         head_range;
   struct mulle_vm_page       *new_page;
   struct mulle_vm_page       *tail_page;
   struct mulle_vm_mapping    *info;
   size_t                     available;
   size_t                     count;
   intptr_t                   idx;
   int                        append;
   int                        rval;

   if( ! vm)
      return( EINVAL);

   // hmm, OK...
   if( range.length == 0)
      return( 0);

   assert( bytes);

   idx    = -1;
   append = 0;

   if( mulle_vm_get_length( vm) == range.location) // clean append ?
   {
      count  = mulle_structarray_get_count( &vm->mappings);
      if( count)
      {
         idx  = count - 1;
         info = _mulle_vm_get_mapping( vm, idx);
      }
      append = 1;
   }
   else
   {
      idx = _mulle_vm_search_mapping_index( vm, range.location);
      if( idx == -1) // nope, the range.location is wrong
         return( EINVAL);

      info = _mulle_vm_get_mapping( vm, idx);
      if( range.location == info->address)
      {
         // clean prepend
         idx = idx - 1;
         if( idx >= 0)
         {
            info   = _mulle_vm_get_mapping( vm, idx);
            append = 1;
         }
         else
         {
            info = NULL;
            idx  = -1;
         }
      }
   }

   //
   // if we have a page, we try to put as many bytes into it as fit
   //
   if( idx != -1)
   {
      // can page hold insertion bytes
      available = mulle_vm_page_get_available( info->page);
      if( available >= range.length)
      {
         // can't fail
         _mulle_vm_will_write_mapping( vm, info, idx);
         rval = _mulle_vm_page_insert_bytes( info->page, info->address, range, bytes);
         assert( ! rval);
         if( info->address > range.location)
            info->address = range.location;

         _mulle_vm_adjust_addresses( vm, idx + 1, range.length);
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
         _mulle_vm_will_write_mapping( vm, info, idx);
         rval       = _mulle_vm_page_insert_bytes( info->page, info->address, head_range, bytes);
         assert( ! rval);

         // reduce range we want to add at index
         range = mulle_range_make( range.location + available,
                                   range.length - available);
         bytes = &((char *) bytes)[ available];
      }
      else
      {
         tail_page = _mulle_vm_split_mapping_at( vm, info, idx, range.location);
         mulle_vm_assert_page( vm, tail_page);
      }
   }

   //
   // so we insert at page after the current index
   // the address will be automatically recomputed
   //
   new_page = _mulle_vm_new_page_with_bytes( vm, bytes, range.length);
   mulle_vm_insert_page( vm, idx + 1, new_page);

   return( 0);
}


int   mulle_vm_append( struct mulle_vm *vm,
                       void *bytes,
                       size_t length)
{
   struct mulle_range   range;

   range = mulle_range_make( mulle_vm_get_length( vm), length);
   return( mulle_vm_insert( vm, range, bytes));
}


void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range)
{
   struct mulle_vm_mapping    *start;
   struct mulle_vm_mapping    *end;
   struct mulle_vm_mapping    *prev;
   int                        end_deleted;
   intptr_t                   adjust_idx;
   intptr_t                   end_index;
   intptr_t                   idx;
   intptr_t                   start_index;
   size_t                     length;
   uintptr_t                  new_address;
   uintptr_t                  offset;
   uintptr_t                  remainder;
   void                       *delete;
   void                       *keep;
   void                       *sentinel;

   if( ! vm)
      return;

   mulle_vm_assert( vm);

   length = mulle_vm_get_length( vm);
   range  = mulle_range_validate_against_length( range, length);
   if( ! range.length)
      return;

   start_index = _mulle_vm_search_mapping_index( vm, range.location);
   end_index   = _mulle_vm_search_mapping_index( vm, mulle_range_get_last_location( range));

   assert( start_index != -1);
   assert( end_index != -1);
   // though not absolutely necessary, deletion from back to front is
   // nicer

   end         = _mulle_vm_get_mapping( vm, end_index);
   end_deleted = 0;
   //
   // special case if we only delete within one page
   //
   if( start_index == end_index)
   {
      // delete everything ?
      if( end->address == range.location && end->page->used.length == range.length)
      {
         _mulle_vm_remove_mapping_at_index( vm, end_index);
         // move following pages down
         _mulle_vm_adjust_addresses( vm, end_index, - (intptr_t) range.length);
         mulle_vm_assert( vm);
         return;
      }

      _mulle_vm_will_write_mapping( vm, end, end_index);

      // remove stuff from front only ?
      if( end->address == range.location)
      {
         end->page->used.location += range.length;
      }
      else
      {
         // remove stuff from back only ?
         if( end->address + end->page->used.length == range.location + range.length)
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

            offset     = range.location - end->address;
            delete     = &((char *) end->page->bytes)[ end->page->used.location + offset];
            keep       = &((char *) delete)[ range.length];
            sentinel   = &((char *) end->page->bytes)[ end->page->used.location + end->page->used.length];

            memmove( delete, keep, sentinel - keep);
         }
      }
      end->page->used.length -= range.length;

      _mulle_vm_free_unused_ospages( vm, end->page);
      mulle_vm_assert_page( vm, end->page);
      _mulle_vm_adjust_addresses( vm, end_index + 1, - (intptr_t) range.length);
      mulle_vm_assert( vm);
      return;
   }

   //
   // multipage handling
   //
   // We know that last page is a hit and that we have at lease one more
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
      _mulle_vm_remove_mapping_at_index( vm, end_index);
   }
   else
   {
      // remove head bytes from page
      _mulle_vm_will_write_mapping( vm, end, end_index);
      end->page->used.location += remainder;
      end->page->used.length   -= remainder;
      _mulle_vm_free_unused_ospages( vm, end->page);
      mulle_vm_assert_page( vm, end->page);
   }

   for( idx = end_index - 1; idx > start_index; --idx)
   {
      _mulle_vm_remove_mapping_at_index( vm, idx);
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
      _mulle_vm_remove_mapping_at_index( vm, start_index);
      adjust_idx--;
   }
   else
   {
      // remove tail bytes from page, this means we have to adjust the next
      // page by remainder, but subsequent pages by - (intptr_t) range.length
      _mulle_vm_will_write_mapping( vm, start, start_index);

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
void *   _mulle_vm_io( struct mulle_vm      *vm,
                       struct mulle_range   range,
                       void                 *buffer,
                       int                  write_flag)  // 1 w: 0: r
{
   size_t                    buf_pos;
   size_t                    i;
   uintptr_t                 logical_pos;
   size_t                    offset;
   size_t                    page_bytes_available;
   size_t                    remaining;
   size_t                    to_copy;
   size_t                    usable_len;
   size_t                    usable_start;
   size_t                    n;
   intptr_t                  info_index;
   struct mulle_vm_mapping   *info;

   if( range.length == 0)
   {
      errno = EINVAL;
      return( NULL);
   }

   n = mulle_structarray_get_count( &vm->mappings);
   if( n == 0)
   {
      errno = EINVAL;
      return( NULL);
   }

   info_index = _mulle_vm_search_mapping_index( vm, range.location);
   if( info_index == -1)
   {
      errno = EINVAL;
      return( NULL);
   }

   info   = _mulle_vm_get_mapping( vm, info_index);
   offset = (size_t)((uintptr_t) range.location - info->address);

   // Check if the read fits entirely inside the found page
   if( ! write_flag && (offset + range.length <= info->page->used.length))
   {
      return( (void *) ((uint8_t *) info->page->bytes + info->page->used.location + offset));
   }

   // The buffer must be provided (multispan page read, or write)
   if( ! buffer)
   {
      errno = ENOSPC;
      return( NULL);
   }

   remaining   = range.length;
   logical_pos = (uintptr_t) range.location;
   buf_pos     = 0;

   // Copy data page by page until either done or error
   for( i = (size_t) info_index; i < n && remaining > 0; i++)
   {
      info         = mulle_structarray_get( &vm->mappings, i);
      usable_start = info->page->used.location;
      usable_len   = info->page->used.length;

      if( usable_len == 0)
         continue;

      if( logical_pos < info->address)
      {
         // Requested read position is before this page's logical start, hole or invalid range
         break;
      }

      offset = (size_t)(logical_pos - info->address);
      if( offset >= usable_len)
      {
         // Logical position beyond used range of this page, either hole or invalid
         break;
      }

      page_bytes_available = usable_len - offset;
      to_copy = (remaining < page_bytes_available) ? remaining : page_bytes_available;

      if( write_flag)
         memcpy( (uint8_t *) info->page->bytes + usable_start + offset,
                 (uint8_t *) buffer + buf_pos,
                 to_copy);
      else
         memcpy( (uint8_t *) buffer + buf_pos,
                 (uint8_t *) info->page->bytes + usable_start + offset,
                 to_copy);

      buf_pos     += to_copy;
      remaining   -= to_copy;
      logical_pos += to_copy;
   }

   if( remaining != 0)
   {
      // The requested i/o length couldn't be fulfilled fully
      errno = EINVAL;
      return( NULL);
   }

   return( buffer);
}


int  _mulle_vm_walk( struct mulle_vm *vm,
                     struct mulle_range range,
                     mulle_vm_walk_callback_t  *callback,
                     void *userinfo,
                     int direction)
{
   intptr_t                   offset;
   struct mulle_vm_mapping   *startinfo;
   struct mulle_vm_mapping   *endinfo;
   struct mulle_vm_mapping   *info;
   int                        rval;
   struct mulle_data          data;
   struct mulle_range         page_range;
   struct mulle_range         intersection;
   uintptr_t                  start_location;
   uintptr_t                  end_location;
   intptr_t                   startinfo_index;
   intptr_t                   endinfo_index;
   intptr_t                   info_index;

   if( ! callback)
   {
      errno = EINVAL;
      return( -1);
   }

   range = mulle_range_validate_against_length( range, mulle_vm_get_length( vm));
   if( ! mulle_range_is_valid( range))
   {
      errno = ERANGE;
      return( -1);
   }

   if( range.length == 0)
      return( 0);

   assert( direction == 1 || direction == -1);

   if( direction == 1)
   {
      start_location = range.location;
      end_location   = mulle_range_get_last_location( range);
   }
   else
   {
      end_location   = range.location;
      start_location = mulle_range_get_last_location( range);
   }

   startinfo_index = _mulle_vm_search_mapping_index( vm, start_location);
   assert( startinfo_index >= 0);
   startinfo = _mulle_vm_get_mapping( vm, startinfo_index);

   // Check if the read fits entirely inside the found page
   offset = (intptr_t) range.location - startinfo->address;

   if( offset >= 0 && offset + range.length <= startinfo->page->used.length)
   {
      endinfo_index = startinfo_index;
      endinfo       = startinfo;
   }
   else
   {
      endinfo_index = _mulle_vm_search_mapping_index( vm, end_location);
      assert( endinfo_index >= 0);
      endinfo       = _mulle_vm_get_mapping( vm, endinfo_index);

      assert( direction == 1
              ? endinfo_index > startinfo_index
              : endinfo_index < startinfo_index);
   }

   page_range      = mulle_vm_mapping_get_range( startinfo);
   intersection    = mulle_range_intersection( page_range, range);
   data            = mulle_vm_mapping_get_data_in_range( startinfo, intersection);
   rval            = (*callback)( data, intersection, userinfo);

   // preset errno to something, that callback then can change
   errno = ENOENT;
   if( rval || endinfo_index == startinfo_index)
      return( rval);

   // Copy data page by page until either done or error
   for( info_index = startinfo_index + direction; info_index != endinfo_index; info_index += direction)
   {
      info         = mulle_structarray_get( &vm->mappings, info_index);
      page_range   = mulle_vm_mapping_get_range( info);
      data         = mulle_vm_mapping_get_data_in_range( info, page_range);

      rval = (*callback)( data, page_range, userinfo);
      if( rval)
         return( rval);
   }

   assert( endinfo != startinfo);  // just to think less here :)

   page_range      = mulle_vm_mapping_get_range( endinfo);
   intersection    = mulle_range_intersection( page_range, range);
   data            = mulle_vm_mapping_get_data_in_range( endinfo, intersection);

   rval            = (*callback)( data, intersection, userinfo);
   return( rval);
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
   struct mulle_vm_mapping    *start;
   struct mulle_vm_mapping    *end;
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
   mulle_vm_delete( dst, mulle_range_make_all());

   length = mulle_vm_get_length( src);
   range  = mulle_range_validate_against_length( range, length);
   if( ! mulle_range_is_valid( range))
   {
      errno = ERANGE;
      return( -1);
   }

   if( range.length == 0)
      return( 0);

   start_index = _mulle_vm_search_mapping_index( src, range.location);
   assert( start_index != -1);
   start       = _mulle_structarray_get( &src->mappings, start_index);

   end_index = _mulle_vm_search_mapping_index( src, mulle_range_get_last_location( range));
   assert( end_index != -1);
   end       = _mulle_structarray_get( &src->mappings, end_index);

   // can we use page wholesale ?
   vm_address = 0;
   if( start->address == range.location && start->page->used.length <= range.length)
   {
      copy = _mulle_vm_retain_page( dst, start->page);
      __mulle_vm_add_page_with_address( dst, copy, vm_address);
   }
   else
   {
      // create a new page
      copy = _mulle_vm_copy_page( src, start->page, start->address, range);
      __mulle_vm_add_page_with_address( dst, copy, vm_address);
   }
   vm_address += copy->used.length;

   // special case if we only copy one page, we are done otherwise...
   if( start_index != end_index)
   {
      for( idx = start_index + 1; idx < end_index; idx++)
      {
         page = _mulle_structarray_get( &src->mappings, idx);
         copy = _mulle_vm_retain_page( dst, page);
         __mulle_vm_add_page_with_address( dst, copy, vm_address);
         vm_address += copy->used.length;
      }

      remainder = mulle_range_get_max( range) - vm_address;
      assert( remainder);

      // special treatment for last page
      if( end->page->used.length == remainder)
      {
         copy = _mulle_vm_retain_page( dst, end->page);
         __mulle_vm_add_page_with_address( dst, copy, vm_address);
      }
      else
      {
         // create a new page
         copy = _mulle_vm_copy_page( src, end->page, vm_address, range);
         __mulle_vm_add_page_with_address( dst, copy, vm_address);
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
   struct mulle_vm_mapping    *start;
   struct mulle_vm_page        *tail_page;
   struct mulle_vm_mapping    *prev;
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
   mulle_vm_delete( dst, range);

   // if nothing to paste, we is done (so mean we must have a page in src)
   length = mulle_vm_get_length( src);
   if( length == 0)
      return( 0);

   tail_page   = NULL;
   start       = NULL;
   start_index = _mulle_vm_search_mapping_index( dst, range.location);
   if( start_index != -1)
   {
      start     = _mulle_structarray_get( &dst->mappings, start_index);
      tail_page = _mulle_vm_split_mapping_at( dst, start, start_index, range.location);
      if( ! tail_page)
         start_index = -1;
   }
   else
   {
      start_index = _mulle_structarray_get_count( &dst->mappings) - 1;
   }

   idx = start_index; // -1 is OK
   mulle_vm_mapping_for( src, src_page)
   {
      idx++;
      _mulle_vm_retain_page( dst, src_page->page);
      _mulle_vm_insert_page( dst, idx, src_page->page);
   }

   if( tail_page)
   {
      prev = _mulle_structarray_get( &dst->mappings, idx);
      switch( _mulle_vm_page_join( prev->page, tail_page))
      {
      case -1 : // can drop tail
         _mulle_vm_remove_mapping_at_index( dst, idx + 1);
         break;

      case 0 :  // no do
         break;

      case +1 : // can drop prev
         _mulle_vm_remove_mapping_at_index( dst, idx);
         break;
      }
   }

   // Fix address calculation for the tail_page first, then adjust subsequent pages
   if( tail_page)
   {
      // The tail_page should be positioned right after the last inserted page
      struct mulle_vm_mapping    *last_inserted;
      uintptr_t                  tail_address;

      last_inserted = _mulle_structarray_get( &dst->mappings, idx);
      tail_address  = last_inserted->address + last_inserted->page->used.length;
      _mulle_vm_change_address( dst, idx + 1, tail_address);

      // Now adjust all pages after the tail page
      _mulle_vm_adjust_addresses( dst, idx + 2, + (intptr_t) length);
   }
   else
   {
      // No tail page, just adjust pages after the last inserted page
      _mulle_vm_adjust_addresses( dst, idx + 1, + (intptr_t) length);
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
   struct mulle_vm_mapping     *info;
   struct mulle_vm_statistics   stats = { 0 };

   if( vm)
   {
      mulle_structarray_for( &vm->mappings, info)
      {
         mulle_vm_assert_page( vm, info->page);

         stats.vm_size   += info->page->used.length;
         stats.page_size += info->page->size;
      }
   }
   return( stats);
}
