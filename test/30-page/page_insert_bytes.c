#include <mulle-vm/mulle-vm.h>
#include <mulle-allocator/mulle-allocator.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


// Function is already declared in header


static void   show_page( struct mulle_vm_page *page,
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
   char                    insert_data[] = "DEF";
   int                     result;

   printf( "=== Test: Clean Append ===\n");
   
   // Setup page with "ABC" starting at offset 5
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[5], "ABC");
   mulle_vm_page_init( &page, buf, mulle_range_make( 5, 3), 64);
   
   show_page( &page, "Initial");
   
   // Append "DEF" at end of used data (offset 3 from start of used range)
   result = _mulle_vm_page_insert_bytes( &page, 3, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 3\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After append");
   printf( "\n");
}


static void   test_clean_prepend( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[] = "XYZ";
   int                     result;

   printf( "=== Test: Clean Prepend ===\n");
   
   // Setup page with "ABC" starting at offset 20 (with front space)
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[20], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 20, 3), 64);
   
   show_page( &page, "Initial");
   
   // Prepend "XYZ" at offset 0 (front of used range)
   result = _mulle_vm_page_insert_bytes( &page, 0, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 0\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After prepend");
   printf( "\n");
}


static void   test_intersection_insert( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[] = "XY";
   int                     result;

   printf( "=== Test: Intersection Insert (offset 2) ===\n");
   
   // Setup page with "ABCD" starting at offset 10
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABCD");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 4), 64);
   
   show_page( &page, "Initial");
   
   // Insert "XY" at offset 2 (intersects with "CD")
   result = _mulle_vm_page_insert_bytes( &page, 2, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 2\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After intersection");
   printf( "\n");
}


static void   test_non_adjacent_invalid( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[] = "XYZ";
   int                     result;

   printf( "=== Test: Non-Adjacent (Invalid offset 10) ===\n");
   
   // Setup page with "ABC" starting at offset 5
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[5], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 5, 3), 64);
   
   show_page( &page, "Initial");
   
   // Try to insert "XYZ" at offset 10 (beyond end of used range)
   result = _mulle_vm_page_insert_bytes( &page, 10, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 10\n", insert_data);
   printf( "Result: %d (EINVAL=%d)\n", result, EINVAL);
   show_page( &page, "After failed insert");
   printf( "\n");
}


static void   test_insufficient_space( void)
{
   char                    buf[16];  // Small buffer
   struct mulle_vm_page    page;
   char                    insert_data[] = "TOO_LONG_TO_FIT";
   int                     result;

   printf( "=== Test: Insufficient Space ===\n");
   
   // Setup page with "ABC" in small buffer
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[2], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 2, 3), 16);
   
   show_page( &page, "Initial");
   
   // Try to append very long string at end
   result = _mulle_vm_page_insert_bytes( &page, 3, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 3\n", insert_data);
   printf( "Result: %d (ENOSPC=%d)\n", result, ENOSPC);
   show_page( &page, "After failed insert");
   printf( "\n");
}


static void   test_append_with_compaction( void)
{
   char                    buf[32];
   struct mulle_vm_page    page;
   char                    insert_data[] = "DEF";
   int                     result;

   printf( "=== Test: Append with Compaction ===\n");
   
   // Setup page with "ABC" near the end to force compaction on append
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[25], "ABC");  // Near end of 32-byte buffer
   mulle_vm_page_init( &page, buf , mulle_range_make( 25, 3), 32);
   
   show_page( &page, "Initial");
   
   // Append "DEF" at end - should trigger move to front
   result = _mulle_vm_page_insert_bytes( &page, 3, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 3\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After append with compaction");
   printf( "\n");
}


static void   test_prepend_with_compaction( void)
{
   char                    buf[32];
   struct mulle_vm_page    page;
   char                    insert_data[] = "XYZ";
   int                     result;

   printf( "=== Test: Prepend with Compaction ===\n");
   
   // Setup page with "ABC" at front to force compaction on prepend
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[0], "ABC");  // At front of buffer
   mulle_vm_page_init( &page, buf , mulle_range_make( 0, 3), 32);
   
   show_page( &page, "Initial");
   
   // Prepend "XYZ" at front - should trigger move to back
   result = _mulle_vm_page_insert_bytes( &page, 0, insert_data, strlen(insert_data));
   
   printf( "Insert data: \"%s\" at offset 0\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After prepend with compaction");
   printf( "\n");
}


static void   test_edge_cases( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[10] = "";  // Make buffer larger to avoid overflow
   int                     result;

   printf( "=== Test: Edge Cases ===\n");
   
   // Test 1: Zero-length insert (should work)
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 3), 64);
   
   show_page( &page, "Initial (zero-length test)");
   result = _mulle_vm_page_insert_bytes( &page, 3, insert_data, 0);

   printf( "Insert zero-length data at offset 3\n");
   printf( "Zero-length insert result: %d\n", result);
   show_page( &page, "After zero-length insert");
   
   // Test 2: Insert at exact start of used range
   printf( "\n--- Insert at exact start ---\n");
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[10], "ABC");
   mulle_vm_page_init( &page, buf , mulle_range_make( 10, 3), 64);
   
   show_page( &page, "Initial (start test)");
   strcpy( insert_data, "XY");
   result = _mulle_vm_page_insert_bytes( &page, 0, insert_data, 2);
   printf( "Insert data: \"%s\" at offset 0\n", insert_data);
   printf( "Insert at start result: %d\n", result);
   show_page( &page, "After start insert");
   
   printf( "\n");
}


static void   test_middle_insert( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[] = "12";
   int                     result;

   printf( "=== Test: Middle Insert ===\n");
   
   // Setup page with "ABCD" starting at location 0
   memset( buf, 0, sizeof( buf));
   strcpy( &buf[0], "ABCD");
   mulle_vm_page_init( &page, buf , mulle_range_make( 0, 4), 64);
   
   show_page( &page, "Initial");
   
   // Insert "12" at offset 2 (between "AB" and "CD")
   result = _mulle_vm_page_insert_bytes( &page, 2, insert_data, strlen( insert_data));
   
   printf( "Insert data: \"%s\" at offset 2\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After middle insert");
   printf( "\n");
}

static void   test_empty_page_operations( void)
{
   char                    buf[64];
   struct mulle_vm_page    page;
   char                    insert_data[] = "FIRST";
   int                     result;

   printf( "=== Test: Empty Page Operations ===\n");
   
   // Setup empty page
   memset( buf, 0, sizeof( buf));
   mulle_vm_page_init( &page, buf , mulle_range_make( 0, 0), 64);
   
   show_page( &page, "Initial empty page");
   
   // Try to insert at offset 0 (start of empty page)
   result = _mulle_vm_page_insert_bytes( &page, 0, insert_data, strlen( insert_data));
   
   printf( "Insert data: \"%s\" at offset 0\n", insert_data);
   printf( "Result: %d\n", result);
   show_page( &page, "After insert to empty page");
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
   test_middle_insert();
   test_empty_page_operations();

   printf( "All tests completed.\n");
   return( 0);
}