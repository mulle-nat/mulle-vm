//
//  mulle-vm.h
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
#ifndef mulle__vm_h__
#define mulle__vm_h__

#include "include.h"

#include "mulle-vm-page.h"
#include "mulle-vm-tree.h"
#include <stdint.h>


/*
 *  (c) 2025 nat <|ORGANIZATION|>
 *
 *  version:  major, minor, patch
 */
#define MULLE__VM_VERSION  ((0UL << 20) | (7 << 8) | 57)


static inline unsigned int   mulle_vm_get_version_major( void)
{
   return( MULLE__VM_VERSION >> 20);
}


static inline unsigned int   mulle_vm_get_version_minor( void)
{
   return( (MULLE__VM_VERSION >> 8) & 0xFFF);
}


static inline unsigned int   mulle_vm_get_version_patch( void)
{
   return( MULLE__VM_VERSION & 0xFF);
}


MULLE__VM_GLOBAL
uint32_t   mulle_vm_get_version( void);



#ifndef MULLE__VM_PAGESIZE_DEFAULT
# define MULLE__VM_PAGESIZE_DEFAULT  0x4000
#endif
#ifndef MULLE__VM_SKEW_DEFAULT
# define MULLE__VM_SKEW_DEFAULT  25
#endif


struct mulle_vm;


/**
 * Configuration structure for initializing a mulle-vm instance.
 *
 * This structure allows customization of memory allocation behavior,
 * page management, and other VM parameters.
 * The allocator/free functionality must deal with memory on a page level.
 *  So you must be able to allocate three pages
 * and then free the one in the middle. This precludes malloc/free.
 */
struct mulle_vm_config
{
   /** Optional shutdown callback called during mulle_vm_done(). */
   void     (*shutdown)( struct mulle_vm *vm);
   
   /** Skew parameter for address calculation. Default is MULLE__VM_SKEW_DEFAULT (25). */
   size_t   skew;
};


/**
 * Default configuration for mulle-vm instances.
 *
 * Contains sensible defaults for most use cases:
 * - skew: MULLE__VM_SKEW_DEFAULT (25)
 * - shutdown: NULL (no custom shutdown)
 */
MULLE__VM_GLOBAL
struct mulle_vm_config     mulle_vm_config_default;



/**
 * Main virtual memory structure representing a VM instance.
 *
 * This structure contains all the state needed to manage a virtual memory
 * space, including mappings between virtual addresses and physical pages,
 * a pool of freed operating system pages for reuse, and configuration parameters.
 */
struct mulle_vm
{
   /** Tree of mulle_vm_node structures defining memory ranges */
   struct mulle_vm_tree        tree;
   
   /** Configuration parameters for this VM instance */
   struct mulle_vm_config      config;
};


/**
 * Initialize a VM instance with the given configuration.
 *
 * This function initializes a mulle_vm structure with the provided configuration.
 * If config is NULL, the default configuration (mulle_vm_config_default) is used.
 * The VM will be ready for use after initialization.
 *
 * @param vm Pointer to the VM structure to initialize
 * @param config Configuration parameters, or NULL for default configuration
 * @param allocator Memory allocator to use, or NULL for system allocator
 */
MULLE__VM_GLOBAL
void   mulle_vm_init( struct mulle_vm *vm,
                      struct mulle_vm_config *config,
                      struct mulle_allocator *allocator);


/**
 * Clean up a VM instance and release all resources.
 *
 * This function releases all resources associated with a VM instance,
 * including all pages, mappings, and the optional shutdown callback
 * is called if provided in the configuration.
 *
 * @param vm Pointer to the VM structure to clean up
 */
MULLE__VM_GLOBAL
void   mulle_vm_done( struct mulle_vm *vm);


/**
 * Reset a VM instance to its initial state.
 *
 * This function frees all pages and mappings, effectively clearing all
 * data from the VM, but keeps the freed_ospages pool alive for reuse.
 * The VM remains initialized and ready for new data.
 *
 * @param vm Pointer to the VM structure to reset
 */
MULLE__VM_GLOBAL
void   mulle_vm_reset( struct mulle_vm *vm);



/*
 *
 */

/**
 * Callback function type for walking through VM memory ranges.
 *
 * This callback is invoked for each contiguous memory range encountered
 * during a walk operation. The callback receives the actual data content
 * and the corresponding virtual memory range.
 *
 * @param data The actual data content as a mulle_data structure
 * @param vm_range The virtual memory range corresponding to this data
 * @param userinfo User-provided context pointer passed to the walk function
 * @return Return 0 to continue walking, any other value stops the walk.
 *         A negative value indicates an error condition.
 */
typedef int   mulle_vm_walk_callback_t( struct mulle_data data,
                                        struct mulle_range vm_range,
                                        void *userinfo);

/**
 * Walk through VM memory range in forward direction.
 *
 * This function iterates through the specified memory range in the VM,
 * invoking the provided callback for each contiguous data segment found.
 * The walk proceeds from lower to higher memory addresses.
 *
 * @param vm Pointer to the VM structure to walk through
 * @param range The memory range to walk through
 * @param callback Function to call for each data segment encountered
 * @param userinfo User-provided context pointer passed to the callback
 * @return Returns the last return value from the callback, or an error code
 *         if the walk fails before any callback is invoked
 */
MULLE__VM_GLOBAL
int  _mulle_vm_walk( struct mulle_vm *vm,
                     struct mulle_range range,
                     mulle_vm_walk_callback_t  *callback,
                     void *userinfo,
                     int direction);

MULLE_C_NONNULL_THIRD
static inline
int  mulle_vm_walk( struct mulle_vm *vm,
                    struct mulle_range range,
                    mulle_vm_walk_callback_t  *callback,
                    void *userinfo)
{
   if( ! vm)
      return( 0);
   return( _mulle_vm_walk( vm, range, callback, userinfo, +1));
}


/**
 * Walk through VM memory range in reverse direction.
 *
 * This function iterates through the specified memory range in the VM,
 * invoking the provided callback for each contiguous data segment found.
 * The walk proceeds from higher to lower memory addresses (reverse order).
 *
 * @param vm Pointer to the VM structure to walk through
 * @param range The memory range to walk through
 * @param callback Function to call for each data segment encountered
 * @param userinfo User-provided context pointer passed to the callback
 * @return Returns the last return value from the callback, or an error code
 *         if the walk fails before any callback is invoked
 */
MULLE_C_NONNULL_THIRD
static inline
int  mulle_vm_walk_reverse( struct mulle_vm *vm,
                            struct mulle_range range,
                            mulle_vm_walk_callback_t  *callback,
                            void *userinfo)
{
   if( ! vm)
      return( 0);
   return( _mulle_vm_walk( vm, range, callback, userinfo, -1));
}


/*
 *                      General API
 */
static inline uintptr_t   mulle_vm_get_length( struct mulle_vm *vm)
{
   if( ! vm)
      return( 0);

   return( mulle_vm_tree_get_total_length( &vm->tree));
}


static inline struct mulle_vm_page  *
   _mulle_vm_new_page_with_bytes( struct mulle_vm *vm, void *bytes, size_t length)
{
   return( mulle_vm_page_new_with_bytes( bytes, length, vm->config.skew));
}

//
// Main: optimized read
// Returns: pointer to VM memory if contained in one page, else to buffer
// (if not NULL), otherwise NULL.
// Out-of-bounds yields NULL.
//
MULLE__VM_GLOBAL
void *   _mulle_vm_io( struct mulle_vm      *vm,
                       struct mulle_range   range,
                       void                 *buffer,
                       int                  write_flag);

static inline const void *
   mulle_vm_read( struct mulle_vm *vm, struct mulle_range range, void *buffer)
{

   if( ! vm || ! range.length)
      return( NULL);

   return( _mulle_vm_io( vm, range, buffer, 0));
}


// use this if you want to modify the read bytes, returned bytes will always
// be copied into buffer
static inline void *
   mulle_vm_read_copy( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *buffer)
{
   void   *result;

   if( ! vm || ! buffer || ! range.length)
      return( NULL);

   result = _mulle_vm_io( vm, range, buffer, 0);
   if( result != buffer)
      memcpy( buffer, result, range.length);
   return( buffer);
}


static inline void *
   mulle_vm_write( struct mulle_vm *vm, struct mulle_range range, void *buffer)
{
   if( ! vm || ! buffer || ! range.length)
      return( NULL);

   return( _mulle_vm_io( vm, range, buffer, 1));
}


MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_THIRD
int   _mulle_vm_insert( struct mulle_vm *vm,
                        struct mulle_range range,
                        void *bytes);

static inline
int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes)
{
   if( ! vm || ! bytes)
      return( 0);

   return( _mulle_vm_insert( vm, range, bytes));
}


MULLE__VM_GLOBAL
MULLE_C_NONNULL_FIRST_SECOND
void  _mulle_vm_append( struct mulle_vm *vm,
                       void *bytes,
                       size_t length);

static inline void   mulle_vm_append( struct mulle_vm *vm, void *bytes, size_t length)
{
   if( ! vm || ! bytes)
      return;
   _mulle_vm_append( vm, bytes, length);
}


// this deletes the range from the vm, some pages will be evicted
MULLE__VM_GLOBAL
void   _mulle_vm_delete( struct mulle_vm *vm,
                         struct mulle_range range);

static inline
void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range)
{
   if( vm)
      _mulle_vm_delete( vm, range);
}

//
// dst and src must be different
// copy range pages to dst (shared). These pages will be appended,
// in general you will want to reset dst before calling mulle_vm_copy
//
MULLE__VM_GLOBAL
int  mulle_vm_copy( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);

// copy range pages to dst (shared)
// delete range in source afterwards
// this calling mulle_vm_copy and mulle_vm_delete in succession
// dst and src must be different
MULLE__VM_GLOBAL
int   mulle_vm_cut( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);

// delete range in destination, then insert everything from src
// there (sharing pages)
// dst and src must be different
MULLE__VM_GLOBAL
int   mulle_vm_paste( struct mulle_vm *dst,
                      struct mulle_range range,
                      struct mulle_vm *src);

/*
 *                         statistics
 */
struct mulle_vm_statistics
{
   size_t   vm_size;
   size_t   page_size;
};


MULLE__VM_GLOBAL
struct mulle_vm_statistics   mulle_vm_get_statistics( struct mulle_vm *vm);





/**
 ** Enumerates pages. If you need nodes use the mulle_vm_tree.
 **/
#define mulle_vm_enumerator     mulle_vm_treeenumerator


static inline struct mulle_vm_enumerator
   mulle_vm_enumerate( struct mulle_vm *vm)
{
   return( mulle_vm_tree_enumerate( vm ? &vm->tree : NULL));
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_enumerator_next( struct mulle_vm_enumerator *rover,
                             struct mulle_vm_page **page)
{
   struct mulle_vm_node   *node;

   if( ! mulle_vm_treeenumerator_next( rover, &node))
   {
      *page = NULL;
      return( 0);
   }

   *page = node->page;
   return( 1);
}


static inline void
   mulle_vm_enumerator_done( struct mulle_vm_enumerator *rover)
{
   mulle_vm_treeenumerator_done( rover);
}


#define mulle_vm_for( tree_ptr, page)                                                \
   for( struct mulle_vm_enumerator _rover__ ## page = mulle_vm_enumerate( tree_ptr), \
           *_rover__ ## page ## __i = (void *) 0;                                    \
        ! _rover__ ## page ## __i;                                                   \
        _rover__ ## page ## __i = ((void *) 1))                                      \
      for( struct mulle_vm_page  *page;                                              \
           mulle_vm_enumerator_next( &_rover__ ## page, &page); )


// reverse
#define mulle_vm_reverseenumerator     mulle_vm_treereverseenumerator


static inline struct mulle_vm_reverseenumerator
   mulle_vm_reverseenumerate( struct mulle_vm *vm)
{
   return( mulle_vm_tree_reverseenumerate( vm ? &vm->tree : NULL));
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_reverseenumerator_next( struct mulle_vm_reverseenumerator *rover,
                                    struct mulle_vm_page **page)
{
   struct mulle_vm_node   *node;

   if( ! mulle_vm_treereverseenumerator_next( rover, &node))
   {
      *page = NULL;
      return( 0);
   }

   *page = node->page;
   return( 1);
}


static inline void
   mulle_vm_reverseenumerator_done( struct mulle_vm_reverseenumerator *rover)
{
   mulle_vm_treereverseenumerator_done( rover);
}


#define mulle_vm_for_reverse( tree_ptr, page)                                                      \
   for( struct mulle_vm_reverseenumerator _rover__ ## page = mulle_vm_reverseenumerate( tree_ptr), \
           *_rover__ ## page ## __i = (void *) 0;                                                  \
        ! _rover__ ## page ## __i;                                                                 \
        _rover__ ## page ## __i = ((void *) 1))                                                    \
      for( struct mulle_vm_page *page;                                                             \
           mulle_vm_reverseenumerator_next( &_rover__ ## page, &page); )

/*
 * The following files are auto-generated.
 * The _mulle-vm-provide header is generated by
 * mulle-match-to-c during a reflect.
 */
#include "_mulle-vm-provide.h"

/*
 * The versioncheck header can be generated with
 * mulle-project-dependency-versions, but it is optional.
 */
#ifdef __has_include
# if __has_include( "_mulle-vm-versioncheck.h")
#  include "_mulle-vm-versioncheck.h"
# endif
#endif

#endif
