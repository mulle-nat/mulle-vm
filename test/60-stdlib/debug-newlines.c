#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>


static void   show_vm( struct mulle_vm *vm)
{
   struct mulle_vm_statistics   info;
   size_t                 total_pages;

   info        = mulle_vm_get_statistics( vm);
   total_pages = info.page_size / mulle_vm_get_pagesize();
   mulle_printf( "%lu pages, %lu bytes\n", total_pages, info.vm_size);
}


static void   dump_vm_content_debug( struct mulle_vm *vm)
{
   char                         buf[1024];
   const void                   *s;
   struct mulle_vm_statistics   info;
   size_t                       i;

   info = mulle_vm_get_statistics( vm);
   s = mulle_vm_read( vm, mulle_range_make( 0, info.vm_size), buf);
   if( s)
   {
      mulle_printf( "VM content (%zu bytes): \"", info.vm_size);
      fwrite( s, 1, info.vm_size, stdout);
      mulle_printf( "\"\n");
      
      mulle_printf( "Hex dump: ");
      for( i = 0; i < info.vm_size; i++)
      {
         mulle_printf( "%02x ", ((unsigned char*)s)[i]);
      }
      mulle_printf( "\n");
      
      mulle_printf( "Positions: ");
      for( i = 0; i < info.vm_size; i++)
      {
         char c = ((char*)s)[i];
         if( c == '\n')
            mulle_printf( "\\n@%zu ", i);
         else
            mulle_printf( "%c@%zu ", c, i);
      }
      mulle_printf( "\n");
   }
}


static void   test_newline_search_sequential( void)
{
   struct mulle_vm   vm;
   uintptr_t         result;
   struct mulle_range search_range;
   uintptr_t         line_start;
   size_t            total_length;
   int               line_num;

   mulle_printf( "== Test Sequential Newline Search (reproducing mulle-editor issue) ==\n");
   mulle_vm_init( &vm, NULL, NULL);
   
   // Load the exact same data as our test file
   mulle_vm_append( &vm, "Hello World\n", 12);
   mulle_vm_append( &vm, "Second Line\n", 12);
   
   show_vm( &vm);
   dump_vm_content_debug( &vm);
   
   total_length = mulle_vm_get_length( &vm);
   mulle_printf( "Total length: %zu\n", total_length);
   
   // Now do the exact same logic as mulle_editor_build_tree
   line_start = 0;
   line_num = 1;
   
   mulle_printf( "\n=== Sequential Search ===\n");
   while( line_start < total_length)
   {
      // Search for next newline starting from current position
      search_range = mulle_range_make( line_start, total_length - line_start);
      result = mulle_vm_memchr( &vm, search_range, '\n');
      
      mulle_printf( "Line %d: line_start=%zu, search_range=[%zu,%zu], newline_pos=%zu\n", 
             line_num, line_start, search_range.location, search_range.length, result);
      
      if( result == mulle_not_found_e)
      {
         mulle_printf( "  -> Last line without newline, range=[%zu,%zu]\n", 
                line_start, total_length - line_start);
         break;
      }
      else
      {
         mulle_printf( "  -> Line with newline, range=[%zu,%zu] (includes newline)\n", 
                line_start, result - line_start + 1);
         
         // Move to next line
         line_start = result + 1;
         line_num++;
      }
   }
   
   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


static void   test_newline_from_file( void)
{
   struct mulle_vm   temp_vm;
   struct mulle_vm   vm;
   uintptr_t         result;
   struct mulle_range search_range;
   uintptr_t         line_start;
   size_t            total_length;
   int               line_num;
   int               load_result;

   mulle_printf( "== Test Newline Search from File (like mulle-editor does) ==\n");
   
   // Create test file first
   FILE *f = fopen( "test-newlines.txt", "w");
   if( f)
   {
      mulle_fprintf( f, "Hello World\nSecond Line\n");
      fclose( f);
   }
   
   mulle_vm_init( &vm, NULL, NULL);
   
   // Load file exactly like mulle-editor does
   load_result = mulle_vm_init_with_contents_of_file( &temp_vm, "test-newlines.txt", NULL);
   if( load_result != 0)
   {
      mulle_printf( "Failed to load file: %d\n", load_result);
      mulle_vm_done( &vm);
      return;
   }
   
   mulle_vm_paste( &vm, mulle_range_make_all(), &temp_vm);
   mulle_vm_done( &temp_vm);
   
   show_vm( &vm);
   dump_vm_content_debug( &vm);
   
   total_length = mulle_vm_get_length( &vm);
   mulle_printf( "Total length from file: %zu\n", total_length);
   
   // Now do the exact same logic as mulle_editor_build_tree
   line_start = 0;
   line_num = 1;
   
   mulle_printf( "\n=== Sequential Search from File ===\n");
   while( line_start < total_length)
   {
      // Search for next newline starting from current position
      search_range = mulle_range_make( line_start, total_length - line_start);
      result = mulle_vm_memchr( &vm, search_range, '\n');
      
      mulle_printf( "Line %d: line_start=%zu, search_range=[%zu,%zu], newline_pos=%zu\n", 
             line_num, line_start, search_range.location, search_range.length, result);
      
      if( result == mulle_not_found_e)
      {
         mulle_printf( "  -> Last line without newline, range=[%zu,%zu]\n", 
                line_start, total_length - line_start);
         break;
      }
      else
      {
         mulle_printf( "  -> Line with newline, range=[%zu,%zu] (includes newline)\n", 
                line_start, result - line_start + 1);
         
         // Move to next line
         line_start = result + 1;
         line_num++;
      }
   }
   
   // Clean up
   remove( "test-newlines.txt");
   
   mulle_printf( "\n");
   mulle_vm_done( &vm);
}


int   main( void)
{
   mulle_printf( "Testing newline search with mulle_vm_memchr\n");
   mulle_printf( "==========================================\n\n");

   test_newline_search_sequential();
   test_newline_from_file();

   mulle_printf( "Debug tests completed.\n");
   return( 0);
}
