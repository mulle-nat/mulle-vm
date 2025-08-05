// Static memory pool - 32 pages of pagesize bytes each
#ifndef DEBUG_N_PAGES
# define DEBUG_N_PAGES    32
#endif
#ifndef DEBUG_PAGE_SIZE
# define DEBUG_PAGE_SIZE  8
#endif

#ifndef DEBUG_N_PAGES
# define DEBUG_N_PAGES    32
#endif
#ifndef DEBUG_PAGE_SIZE
# define DEBUG_PAGE_SIZE  8
#endif

// stupid: for tests we need two vms so each with different memory


static char   debug_memory_pool[DEBUG_N_PAGES][DEBUG_PAGE_SIZE];
static int    debug_page_used[DEBUG_N_PAGES] = {0};  // Track which pages are in use

// Allocator function
static void   *debug_alloc_pages(size_t size)
{
    // Round up to nearest page size
    size_t pages_needed = (size + DEBUG_PAGE_SIZE - 1) / DEBUG_PAGE_SIZE;

    // Check if we have enough free pages
    if (pages_needed > DEBUG_N_PAGES) {
        abort();  // Out of memory
    }

    // Find contiguous free pages
    for (int i = 0; i <= DEBUG_N_PAGES - pages_needed; i++) {
        int found = 1;
        for (int j = 0; j < pages_needed; j++) {
            if (debug_page_used[i + j]) {
                found = 0;
                break;
            }
        }

        if (found) {
            // Mark pages as used
            for (int j = 0; j < pages_needed; j++) {
                debug_page_used[i + j] = 1;
            }

            // Return pointer to first page
            return &debug_memory_pool[i][0];
        }
    }

    // No contiguous space found
    abort();  // Out of memory
}

// Free function
static int   debug_free_pages( void *p, size_t size)
{
    // Round up to nearest page size
    size_t pages_needed = (size + DEBUG_PAGE_SIZE - 1) / DEBUG_PAGE_SIZE;

    // Find the starting page
    int page_index = (char *)p - &debug_memory_pool[0][0];
    if (page_index % DEBUG_PAGE_SIZE != 0) {
        return -1;  // Invalid pointer
    }

    page_index /= DEBUG_PAGE_SIZE;

    // Check bounds
    if (page_index >= DEBUG_N_PAGES || page_index + pages_needed > DEBUG_N_PAGES) {
        return -1;  // Invalid pointer or size
    }

    // Mark pages as free
    for (int i = 0; i < pages_needed; i++) {
        if (!debug_page_used[page_index + i]) {
            return -1;  // Page not allocated
        }
        debug_page_used[page_index + i] = 0;
    }

    return 0;  // Success
}


static void   debug_check_vm( struct mulle_vm *vm)
{
   for( int i = 0; i < DEBUG_N_PAGES; i++)
   {
      if( debug_page_used[ i] != 0)
      {
         printf( "** page %d still in use\n", i);
      }
   }
}


struct mulle_vm_config   mulle_vm_debug_config =
{
   .alloc_pages = debug_alloc_pages,
   .free_pages  = debug_free_pages,
   .shutdown    = debug_check_vm,
   .pagesize    = DEBUG_PAGE_SIZE,
   .skew        = MULLE__VM_SKEW_DEFAULT
};



struct mulle_vm_config   mulle_vm_debug_nocheck_config =
{
   .alloc_pages = debug_alloc_pages,
   .free_pages  = debug_free_pages,
   .shutdown    = 0,
   .pagesize    = DEBUG_PAGE_SIZE,
   .skew        = MULLE__VM_SKEW_DEFAULT
};
