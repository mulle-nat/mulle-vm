#ifndef mulle__vm_h__
#define mulle__vm_h__

#include "include.h"


#include <stdint.h>


struct mulle_vm_page;

/*
 *  (c) <|YEAR|> nat <|ORGANIZATION|>
 *
 *  version:  major, minor, patch
 */
#define MULLE__VM_VERSION  ((0UL << 20) | (7 << 8) | 56)


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
   /** Memory allocation function for pages. If NULL, uses mulle_mmap_alloc_pages. */
   void     *(*alloc_pages)( size_t size);
   
   /** Memory deallocation function for pages. If NULL, uses mulle_mmap_free_pages. */
   int      (*free_pages)( void *p, size_t size);
   
   /** Optional shutdown callback called during mulle_vm_done(). */
   void     (*shutdown)( struct mulle_vm *vm);
   
   /** Size of each page in bytes. Default is MULLE__VM_PAGESIZE_DEFAULT (0x4000). */
   size_t   pagesize;
   
   /** Skew parameter for address calculation. Default is MULLE__VM_SKEW_DEFAULT (25). */
   size_t   skew;
   
   /** Cache size parameter (currently unused). */
   size_t   cachesize;
};


/**
 * Default configuration for mulle-vm instances.
 *
 * Contains sensible defaults for most use cases:
 * - pagesize: MULLE__VM_PAGESIZE_DEFAULT (0x4000)
 * - skew: MULLE__VM_SKEW_DEFAULT (25)
 * - alloc_pages/free_pages: NULL (uses mulle_mmap_alloc_pages/free)
 * - shutdown: NULL (no custom shutdown)
 * - cachesize: 0
 */
MULLE__VM_GLOBAL
struct mulle_vm_config     mulle_vm_config_default;



/**
 * Mapping structure that associates a virtual memory address with a page.
 *
 * This structure represents the mapping between a virtual address space
 * and the actual page that contains the data. Mappings are local to a
 * specific VM instance, but the underlying pages can be shared between
 * multiple VMs.
 */
struct mulle_vm_mapping
{
   /** Pointer to the actual page containing the data */
   struct mulle_vm_page   *page;
   
   /** Virtual address in the VM's address space */
   uintptr_t              address;
};


/**
 * Main virtual memory structure representing a VM instance.
 *
 * This structure contains all the state needed to manage a virtual memory
 * space, including mappings between virtual addresses and physical pages,
 * a pool of freed operating system pages for reuse, and configuration parameters.
 */
struct mulle_vm
{
   /** Array of mulle_vm_mapping structures mapping virtual addresses to pages */
   struct mulle_structarray    mappings;
   
   /** Array of pointers to freed operating system pages for reuse */
   struct mulle_pointerarray   freed_ospages;
   
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


/**
 * Get the page size of a VM instance.
 *
 * Returns the configured page size for the given VM. If vm is NULL,
 * returns the default page size (MULLE__VM_PAGESIZE_DEFAULT).
 *
 * @param vm Pointer to the VM structure, or NULL for default
 * @return The page size in bytes
 */
static inline size_t   mulle_vm_get_pagesize( struct mulle_vm *vm)
{
   return( vm ? vm->config.pagesize : MULLE__VM_PAGESIZE_DEFAULT);
}


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
MULLE_C_NONNULL_THIRD
static inline
int  mulle_vm_walk( struct mulle_vm *vm,
                    struct mulle_range range,
                    mulle_vm_walk_callback_t  *callback,
                    void *userinfo)
{
   int  _mulle_vm_walk( struct mulle_vm *vm,
                        struct mulle_range range,
                        mulle_vm_walk_callback_t  *callback,
                        void *userinfo,
                        int direction);
   if( ! vm)
      return( ENOENT);
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
   int  _mulle_vm_walk( struct mulle_vm *vm,
                        struct mulle_range range,
                        mulle_vm_walk_callback_t  *callback,
                        void *userinfo,
                        int direction);

   if( ! vm)
      return( ENOENT);
   return( _mulle_vm_walk( vm, range, callback, userinfo, -1));
}


/*
 *                      General API
 */

//
// Main: optimized read
// Returns: pointer to VM memory if contained in one page, else to buffer
// (if not NULL), otherwise NULL.
// Out-of-bounds yields NULL.
//
static inline const void *
   mulle_vm_read( struct mulle_vm *vm, struct mulle_range range, void *buffer)
{
   MULLE__VM_GLOBAL
   void *   _mulle_vm_io( struct mulle_vm      *vm,
                          struct mulle_range   range,
                          void                 *buffer,
                          int                  write_flag);

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
   MULLE__VM_GLOBAL
   void *   _mulle_vm_io( struct mulle_vm      *vm,
                          struct mulle_range   range,
                          void                 *buffer,
                          int                  write_flag);
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
   MULLE__VM_GLOBAL
   void *   _mulle_vm_io( struct mulle_vm      *vm,
                          struct mulle_range   range,
                          void                 *buffer,
                          int                  write_flag);

   if( ! vm || ! buffer || ! range.length)
      return( NULL);

   return( _mulle_vm_io( vm, range, buffer, 1));
}


MULLE__VM_GLOBAL
int   mulle_vm_insert( struct mulle_vm *vm,
                       struct mulle_range range,
                       void *bytes);

MULLE__VM_GLOBAL
int   mulle_vm_append( struct mulle_vm *vm,
                       void *bytes,
                       size_t length);

// this deletes the range from the vm, some pages will be evicted
MULLE__VM_GLOBAL
void   mulle_vm_delete( struct mulle_vm *vm,
                        struct mulle_range range);


//
// dst and src must be different
// copy range pages to dst (shared). These pages will be appended,
// in general you will want to reset dst before calling mulle_vm_copy
//
// returns 0 on success, else some error code (usually ERANGE)
MULLE__VM_GLOBAL
int  mulle_vm_copy( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);

// copy range pages to dst (shared)
// delete range in source afterwards
// this calling mulle_vm_copy and mulle_vm_delete in succession
// dst and src must be different
int   mulle_vm_cut( struct mulle_vm *dst,
                    struct mulle_vm *src,
                    struct mulle_range range);

// delete range in destination, then insert everything from src
// there (sharing pages)
// dst and src must be different
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
 **
 **
 **/
struct mulle_vm_mappingenumerator
{
   MULLE_STRUCTARRAYENUMERATOR_BASE;
};


static inline struct mulle_vm_mappingenumerator
   mulle_vm_mapping_enumerate( struct mulle_vm *vm)
{
   struct mulle_structarrayenumerator   rover;

   rover = mulle_structarray_enumerate( &vm->mappings);
   return( *(struct mulle_vm_mappingenumerator *) &rover);
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_mappingenumerator_next( struct mulle_vm_mappingenumerator *rover,
                                     struct mulle_vm_mapping **info)
{
   return( mulle_structarrayenumerator_next( (struct mulle_structarrayenumerator *) rover,
                                             (void **) info));
}

static inline void
   mulle_vm_mappingenumerator_done( struct mulle_vm_mappingenumerator *rover)
{
}


#define mulle_vm_mapping_for( vm_ptr, info)                                                       \
   for( struct mulle_vm_mappingenumerator _rover__ ## info = mulle_vm_mapping_enumerate( vm_ptr), \
           *_rover__ ## info ## __i = (void *) 0;                                                 \
        ! _rover__ ## info ## __i;                                                                \
        _rover__ ## info ## __i = ((void *) 1))                                                   \
      for( struct mulle_vm_mapping *info;                                                         \
           mulle_vm_mappingenumerator_next( &_rover__ ## info, &info); )


// reverse
struct mulle_vm_mappingreverseenumerator
{
   MULLE_STRUCTARRAYREVERSEENUMERATOR_BASE;
};


static inline struct mulle_vm_mappingreverseenumerator
   mulle_vm_mapping_reverseenumerate( struct mulle_vm *vm)
{
   struct mulle_structarrayreverseenumerator   rover;

   rover = mulle_structarray_reverseenumerate( &vm->mappings);
   return( *(struct mulle_vm_mappingreverseenumerator *) &rover);
}


// returns 1 if ok, fills *info, else returns 0, *info->page=NULL and *info->address=0
static inline int
   mulle_vm_mappingreverseenumerator_next( struct mulle_vm_mappingreverseenumerator *rover,
                                           struct mulle_vm_mapping **info)
{
   return( mulle_structarrayreverseenumerator_next( (struct mulle_structarrayreverseenumerator *) rover,
                                                    (void **) info));
}

static inline void
   mulle_vm_mappingreverseenumerator_done( struct mulle_vm_mappingreverseenumerator *rover)
{
}


#define mulle_vm_mapping_for_reverse( vm_ptr, info)                                                              \
   for( struct mulle_vm_mappingreverseenumerator _rover__ ## info = mulle_vm_mapping_reverseenumerate( vm_ptr),  \
           *_rover__ ## info ## __i = (void *) 0;                                                                \
        ! _rover__ ## info ## __i;                                                                               \
        _rover__ ## info ## __i = ((void *) 1))                                                                  \
      for( struct mulle_vm_mapping *info;                                                                        \
           mulle_vm_mappingreverseenumerator_next( &_rover__ ## info, &info); )


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
