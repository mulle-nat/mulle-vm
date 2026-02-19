#ifndef mulle_memmem_h__
#define mulle_memmem_h__

#include "include.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
// Maximum size of the internal buffer (avoids malloc)
#ifndef MULLE_MEMMEM_BUFFER_SIZE
# define MULLE_MEMMEM_BUFFER_SIZE  24
#endif


// Internal state
struct mulle_memmem_state
{
   const void    *needle;
   size_t        needle_len;
   size_t        *lps;
   size_t        buffer[ MULLE_MEMMEM_BUFFER_SIZE]; // Internal buffer

   size_t        match_index;   // how many matched so far (KMP state)
   size_t        total_bytes;
};


// Public API
MULLE__VM_GLOBAL
void   mulle_memmem_init( struct mulle_memmem_state *state,
                          const void *needle,
                          size_t needle_len);

MULLE__VM_GLOBAL
void   mulle_memmem_done( struct mulle_memmem_state *state);

// Returns:
//    mulle_not_found_e
//    else: absolute offset where match was found
// this is at the beginning of the needle
MULLE__VM_GLOBAL
size_t   mulle_memmem( const void *data,
                       size_t data_len,
                       struct mulle_memmem_state *state);

/*
 *
 */
// State struct
struct mulle_memrmem_state
{
   const void    *needle;
   size_t        needle_len;
   size_t        *lps;
   size_t        buffer[ MULLE_MEMMEM_BUFFER_SIZE]; // Internal buffer

   size_t        match_index;   // how many matched so far (KMP state)
   size_t        total_bytes;
};


MULLE__VM_GLOBAL
void   mulle_memrmem_init( struct mulle_memrmem_state *state,
                         const void *needle,
                         size_t needle_len);

MULLE__VM_GLOBAL
void   mulle_memrmem_done( struct mulle_memrmem_state *state);




// Returns:
//    mulle_not_found_e
//    else: absolute offset where match was found
//    this is at the end of the needle
MULLE__VM_GLOBAL
size_t   mulle_memrmem( const void *data,
                        size_t data_len,
                        struct mulle_memrmem_state *state);

#ifdef __cplusplus
}
#endif

#endif
