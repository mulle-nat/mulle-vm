#include <mulle-vm/mulle-vm.h>
#include <mulle-allocator/mulle-allocator.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


MULLE__VM_GLOBAL
int  _mulle_vm_page_insert_bytes( struct mulle_vm_page *page,
                                  uintptr_t vm_address,
                                  struct mulle_range range,
                                  void *bytes);


static void   show_page( struct mulle_vm_page *page,
                         uintptr_t vm_address,
                         const char *title)
{
   printf( "%s: used={%lu,%lu}, size=%lu\n",
          title, page->used.location, page->used.length, page->size);
   
   if( page->used.length > 0)
   {
      printf( "  Content: \"");
      for( size_t i = 0; i < page->used.length; i++)
      {
         char c = ((char*) page->bytes)[page->used.location + i];
         if( c >= 32 && c <= 126)
            printf( "%c", c);
         else
            printf( "\\x%02x", (unsigned char)c);
      }
      printf( "\"\n");
   }
}


static void   test_clean_append( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "DEF";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Clean Append ===\n");
   
   // Setup page with "ABC" at vm_address 10, used range starts at offset 5
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[5], "ABC");
   mulle_vm_page_init( &page, buf, mulle_range_make( 5, 3), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Append "DEF" at vm_address 13 (right after "ABC")
   range = mulle_range_make( 13, 3);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After append");
   printf( "\n");
}


static void   test_clean_prepend( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "XYZ";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Clean Prepend ===\n");
   
   // Setup page with "ABC" at vm_address 10, used range starts at offset 20
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[20], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 20, 3), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Prepend "XYZ" at vm_address 7 (right before "ABC")
   range = mulle_range_make( 7, 3);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After prepend");
   printf( "\n");
}


static void   test_intersection_insert( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "XY";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Intersection Insert ===\n");
   
   // Setup page with "ABCD" at vm_address 10, used range starts at offset 10
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABCD");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 4), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Insert "XY" overlapping at vm_address 12 (intersects with "CD")
   range = mulle_range_make( 12, 2);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After intersection");
   printf( "\n");
}


static void   test_non_adjacent_invalid( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "XYZ";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Non-Adjacent (Invalid) ===\n");
   
   // Setup page with "ABC" at vm_address 10
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[5], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 5, 3), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Try to insert "XYZ" at vm_address 20 (gap between 13 and 20)
   range  = mulle_range_make( 20, 3);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d (EINVAL=%d)\n", result, EINVAL);
   show_page( &page, vm_address, "After failed insert");
   printf( "\n");
}


static void   test_insufficient_space( void)
{
   char                    buf[16];  // Small buffer
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "TOO_LONG_TO_FIT";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Insufficient Space ===\n");
   
   // Setup page with "ABC" in small buffer
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[2], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 2, 3), 16);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Try to append very long string
   range  = mulle_range_make( 13, strlen( insert_data));
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d (ENOSPC=%d)\n", result, ENOSPC);
   show_page( &page, vm_address, "After failed insert");
   printf( "\n");
}


static void   test_append_with_compaction( void)
{
   char                    buf[32];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "DEF";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Append with Compaction ===\n");
   
   // Setup page with "ABC" near the end to force compaction on append
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[25], "ABC");  // Near end of 32-byte buffer
   mulle_vm_page_init( &page, buf , mulle_range_make( 25, 3), 32);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Append "DEF" - should trigger move to front
   range = mulle_range_make( 13, 3);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After append with compaction");
   printf( "\n");
}


static void   test_prepend_with_compaction( void)
{
   char                    buf[32];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "XYZ";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Prepend with Compaction ===\n");
   
   // Setup page with "ABC" at front to force compaction on prepend
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[0], "ABC");  // At front of buffer
   mulle_vm_page_init( &page, buf , mulle_range_make( 0, 3), 32);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial");
   
   // Prepend "XYZ" - should trigger move to back
   range = mulle_range_make( 7, 3);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After prepend with compaction");
   printf( "\n");
}


static void   test_edge_cases( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[10] = "";  // Make buffer larger to avoid overflow
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Edge Cases ===\n");
   
   // Test 1: Zero-length insert (should work)
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 3), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial (zero-length test)");
   range = mulle_range_make( 13, 0);
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);

   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Zero-length insert result: %d\n", result);
   show_page( &page, vm_address, "After zero-length insert");
   
   // Test 2: Insert at exact start of used range
   printf( "\n--- Insert at exact start ---\n");
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 3), 64);
   vm_address = 10;
   
   show_page( &page, vm_address, "Initial (start test)");
   range = mulle_range_make( 10, 2);
   strcpy( insert_data, "XY");
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Insert at start result: %d\n", result);
   show_page( &page, vm_address, "After start insert");
   
   printf( "\n");
}


static void   test_empty_page_operations( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   struct mulle_range      range;
   char                    insert_data[] = "FIRST";
   int                     result;
   uintptr_t               vm_address;

   printf( "=== Test: Empty Page Operations ===\n");
   
   // Setup empty page
   memset( buf, 0, sizeof( buf));
   mulle_vm_page_init( &page, buf , mulle_range_make( 0, 0), 64);
   vm_address = 100;
   
   show_page( &page, vm_address, "Initial empty page");
   
   // Try to insert at vm_address 100 (start of empty page)
   range = mulle_range_make( 100, strlen( insert_data));
   result = _mulle_vm_page_insert_bytes( &page, vm_address, range, insert_data);
   
   printf( "Insert data: \"%.*s\" range: location={%lu, %lu}\n", (int) range.length, insert_data, range.location, range.length);
   printf( "Result: %d\n", result);
   show_page( &page, vm_address, "After insert to empty page");
   printf( "\n");
}


int   main( void)
{
   printf( "Testing _mulle_vm_page_insert_bytes function\n");
   printf( "============================================\n\n");

   test_clean_append();
   test_clean_prepend(); 
   test_intersection_insert();
   test_non_adjacent_invalid();
   test_insufficient_space();
   test_append_with_compaction();
   test_prepend_with_compaction();
   test_edge_cases();
   test_empty_page_operations();

   printf( "All tests completed.\n");
   return( 0);
}