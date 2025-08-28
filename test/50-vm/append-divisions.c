//
//  append-divisions.c
//  mulle-vm
//
//  Test appending 26-character string "abcdefghijklmnopqrstuvwxyz" 
//  in various division combinations
//

#include "include.h"
#include <mulle-vm/mulle-vm-debug.inc>

static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
static const size_t alphabet_len = 26;

int   test_append( struct mulle_vm *vm, const char *data, size_t len)
{
   char         buffer[ 64];
   const void   *result; 
   struct mulle_range range;
   size_t       total_len;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) data, len);

   total_len = mulle_vm_get_length( vm);
   if( total_len != len)
      return( 1);

   range  = mulle_range_make( 0, len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, data, len) != 0);
}

int   test_append_2_parts( struct mulle_vm *vm, size_t part1_len)
{
   char         buffer[ 64];
   const void   *result;
   struct mulle_range range;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) alphabet, part1_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len), alphabet_len - part1_len);

   range  = mulle_range_make( 0, alphabet_len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, alphabet, alphabet_len) != 0);
}

int   test_append_3_parts( struct mulle_vm *vm, size_t part1_len, size_t part2_len)
{
   char         buffer[ 64];
   const void   *result;
   struct mulle_range range;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) alphabet, part1_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len), part2_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len), alphabet_len - part1_len - part2_len);

   range  = mulle_range_make( 0, alphabet_len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, alphabet, alphabet_len) != 0);
}

int   test_append_4_parts( struct mulle_vm *vm, size_t part1_len, size_t part2_len, size_t part3_len)
{
   char         buffer[ 64];
   const void   *result;
   struct mulle_range range;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) alphabet, part1_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len), part2_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len), part3_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len), alphabet_len - part1_len - part2_len - part3_len);

   range  = mulle_range_make( 0, alphabet_len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, alphabet, alphabet_len) != 0);
}

int   test_append_5_parts( struct mulle_vm *vm, size_t part1_len, size_t part2_len, size_t part3_len, size_t part4_len)
{
   char         buffer[ 64];
   const void   *result;
   struct mulle_range range;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) alphabet, part1_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len), part2_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len), part3_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len), part4_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len + part4_len), alphabet_len - part1_len - part2_len - part3_len - part4_len);

   range  = mulle_range_make( 0, alphabet_len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, alphabet, alphabet_len) != 0);
}

int   test_append_6_parts( struct mulle_vm *vm, size_t part1_len, size_t part2_len, size_t part3_len, size_t part4_len, size_t part5_len)
{
   char         buffer[ 64];
   const void   *result;
   struct mulle_range range;

   mulle_vm_reset( vm);
   mulle_vm_append( vm, (void *) alphabet, part1_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len), part2_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len), part3_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len), part4_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len + part4_len), part5_len);
   mulle_vm_append( vm, (void *) (alphabet + part1_len + part2_len + part3_len + part4_len + part5_len), alphabet_len - part1_len - part2_len - part3_len - part4_len - part5_len);

   range  = mulle_range_make( 0, alphabet_len);
   result = mulle_vm_read( vm, range, buffer);
   if( ! result)
      return( 1);

   return( memcmp( result, alphabet, alphabet_len) != 0);
}

int   main( void)
{
   struct mulle_vm vm;
   size_t          i;
   size_t          j;

   mulle_vm_init( &vm, &mulle_vm_debug_config, NULL);

   /* Test 1 division: whole string */
   if( test_append( &vm, alphabet, alphabet_len))
      return( 1);

   /* Test 2 divisions: (1,25), (2,24), ..., (25,1) */
   for( i = 1; i < alphabet_len; i++)
   {
      if( test_append_2_parts( &vm, i))
         return( 1);
   }

   /* Test 3 divisions: (1,1,24), (1,2,23), ..., (24,1,1) */
   for( i = 1; i < alphabet_len - 1; i++)
   {
      for( j = 1; j < alphabet_len - i; j++)
      {
         if( test_append_3_parts( &vm, i, j))
            return( 1);
      }
   }

   /* Test 4 divisions: minimal combinations */
   for( i = 1; i <= 3; i++)
   {
      for( j = 1; j <= 3 && i + j < 24; j++)
      {
         for( size_t k = 1; k <= 3 && i + j + k < 25; k++)
         {
            size_t remainder = alphabet_len - i - j - k;
            if( remainder >= 1 && remainder <= 3)
            {
               if( test_append_4_parts( &vm, i, j, k))
                  return( 1);
            }
         }
      }
   }

   /* Test 5 divisions: minimal combinations */
   for( i = 1; i <= 2; i++)
   {
      for( j = 1; j <= 2 && i + j < 23; j++)
      {
         for( size_t k = 1; k <= 2 && i + j + k < 24; k++)
         {
            for( size_t l = 1; l <= 2 && i + j + k + l < 25; l++)
            {
               size_t remainder = alphabet_len - i - j - k - l;
               if( remainder >= 1 && remainder <= 2)
               {
                  if( test_append_5_parts( &vm, i, j, k, l))
                     return( 1);
               }
            }
         }
      }
   }

   /* Test 6 divisions: minimal combinations */
   for( i = 1; i <= 2; i++)
   {
      for( j = 1; j <= 2 && i + j < 22; j++)
      {
         for( size_t k = 1; k <= 2 && i + j + k < 23; k++)
         {
            for( size_t l = 1; l <= 2 && i + j + k + l < 24; l++)
            {
               for( size_t m = 1; m <= 2 && i + j + k + l + m < 25; m++)
               {
                  size_t remainder = alphabet_len - i - j - k - l - m;
                  if( remainder == 1)
                  {
                     if( test_append_6_parts( &vm, i, j, k, l, m))
                        return( 1);
                  }
               }
            }
         }
      }
   }

   mulle_vm_done( &vm);
   return( 0);
}