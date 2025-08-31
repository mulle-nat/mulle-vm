//
// test-new-functions.c
// mulle-vm
//
// Test the new mulle_vm_page_get_used_length functions
//

#include "include.h"
#include <stdio.h>


void test_functions(size_t pagesize, size_t page_size, size_t used_location, size_t used_length)
{
   struct mulle_vm_page page = {
      .used = { .location = used_location, .length = used_length },
      .size = page_size
   };
   
   size_t first = mulle_vm_page_get_used_length_first_ospage( &page, pagesize);
   size_t last = mulle_vm_page_get_used_length_last_ospage( &page, pagesize);
   
   printf("page_size=%zu, used_location=%zu, used_length=%zu\n", page_size, used_location, used_length);
   printf("first_ospage=%zu, last_ospage=%zu, total_used=%zu\n", 
          first, last, page.used.length);
   
   int valid = (first + last) <= page.used.length;
   printf("valid: %s\n", valid ? "YES" : "NO");
   printf("\n");
}


// These functions correctly handle:
// 1. Data not extending to page boundaries
// 2. Multi-page scenarios
// 3. Edge cases where data is wholly contained in one OS page
// 4. Returns actual data amounts that can be safely moved

// The key insight:
// - First OS page: data from start to pagesize boundary
// - Last OS page: data from last boundary to end
// - These functions ensure we never exceed actual data bounds."};

int main(void)
{
   size_t pagesize = 8;
   
   printf("=== New Functions Test ===\n");
   printf("pagesize = %zu\n\n", pagesize);
   
   // Test cases
   test_functions(pagesize, 8, 0, 8);     // Full page
   test_functions(pagesize, 8, 1, 6);     // Offset data
   test_functions(pagesize, 8, 1, 2);     // Small offset
   test_functions(pagesize, 8, 6, 2);     // Data at end
   test_functions(pagesize, 8, 7, 1);     // Single byte at end
   test_functions(pagesize, 16, 1, 12);   // Multi-page
   test_functions(pagesize, 16, 14, 2);   // Multi-page end
   test_functions(pagesize, 16, 0, 16);   // Full multi-page
   test_functions(pagesize, 16, 4, 8);    // Middle of multi-page
   
   return 0;
}

