#include <mulle-vm/mulle-vm.h>
#include <mulle-vm/mulle-vm-file.h>
#include <mulle-vm/mulle-vm-debug.inc>
#include <stdio.h>
#include <string.h>

int main(void)
{
   struct mulle_vm   vm1, vm2;
   const void        *data;
   struct mulle_vm_statistics stats;
   char              buf[1024];
   
   mulle_printf("=== Load file into VM1, print content ===\n");
   
   // Load file contents into VM1
   if (mulle_vm_init_with_contents_of_file(&vm1, "test_data.txt", NULL) != 0) {
      mulle_printf("ERROR: Failed to load file into VM\n");
      return 1;
   }
   
   // Print content from VM1
   stats = mulle_vm_get_statistics(&vm1);
   data = mulle_vm_read(&vm1, mulle_range_make(0, stats.vm_size), buf);
   if (data) {
      mulle_printf("VM1 content: ");
      fwrite(data, 1, stats.vm_size, stdout);
   }
   
   mulle_printf("\n=== Transfer to VM2, print content ===\n");
   
   // Initialize second VM
   mulle_vm_init(&vm2, NULL, NULL);
   
   // Transfer content from VM1 to VM2
   if (data) {
      mulle_vm_append(&vm2, (void*)data, stats.vm_size);
   }
   
   // Print content from VM2
   struct mulle_vm_statistics stats2 = mulle_vm_get_statistics(&vm2);
   data = mulle_vm_read(&vm2, mulle_range_make(0, stats2.vm_size), buf);
   if (data) {
      mulle_printf("VM2 content: ");
      fwrite(data, 1, stats2.vm_size, stdout);
   }
   
   // Clean up
   mulle_vm_done(&vm1);
   mulle_vm_done(&vm2);
   
   mulle_printf("Test completed!\n");
   return 0;
}
