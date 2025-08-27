//
//  basic-tree.c
//  mulle-vm
//
//  Test implementation of mulle-vm-tree functionality
//  Step 1: Just allocate and delete a tree
//

#include "include.h"

int   main( void)
{
   struct mulle_vm_tree   tree;
   
   mulle_printf( "mulle-vm-tree Basic Tests\n");
   mulle_printf( "========================\n");
   
   // Test 1: Simple tree creation and destruction
   mulle_printf( "Test 1: Tree init/done\n");
   mulle_vm_tree_init( &tree, NULL);
   mulle_printf( "Tree initialized successfully\n");
   
   mulle_vm_tree_done( &tree);
   mulle_printf( "Tree destroyed successfully\n");
   
   mulle_printf( "\nAll tests completed!\n");
   
   return( 0);
}
