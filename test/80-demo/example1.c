#include <mulle-vm/mulle-vm.h>
#include <stdio.h>
#include <string.h>


static void   mulle_vm_print( struct mulle_vm *vm)
{
   const char   *s;
   size_t       length;

   length = mulle_vm_get_length( vm);
   mulle_alloca_do( buf, char, length)
   {
      s = mulle_vm_read( vm, mulle_range_make( 0, length), buf);
      mulle_printf( "\"%.*s\"\n", (int) length, s);
   }
}


int main(void)
{
   static char       text[] = "VfL Bochum 1848";
   struct mulle_vm   vm;
   struct mulle_vm   copy;

   // Initialize VMs
   mulle_vm_init( &vm, NULL, NULL);
   {
      mulle_vm_insert( &vm, mulle_range_make( 0, strlen( text)), text);
      mulle_printf( "a:1)\n");
      mulle_vm_print( &vm);

      mulle_vm_init( &copy, NULL, NULL);
      {
         mulle_vm_copy( &copy, &vm, mulle_range_make( 4, 6));  // copy Bochum
         mulle_printf( "b:1)\n");
         mulle_vm_print( &copy);

         mulle_vm_paste( &vm, mulle_range_make( 0, 3), &copy);   // replace VfL
         mulle_printf( "a:2)\n");
         mulle_vm_print( &vm);

         mulle_vm_paste( &vm, mulle_range_make( 14, 4), &copy);  // replace 1848

         mulle_printf( "a:3)\n");
         mulle_vm_print( &vm);
      }
      mulle_vm_done( &copy);

   }
   mulle_vm_done( &vm);

   return 0;
}
