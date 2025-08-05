//
//  mulle-vm-stdlib.c
//  mulle-vm
//
//  Copyright (c) <|YEAR|> Nat! - Mulle kybernetiK.
//  All rights reserved.
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
#ifndef mulle_vm_stdlib_h__
#define mulle_vm_stdlib_h__

#include "include.h"


#include <stdint.h>

struct mulle_vm;



// Search for a character in mulle_vm range
uintptr_t   mulle_vm_memchr( struct mulle_vm *vm, struct mulle_range range, int c);
uintptr_t   mulle_vm_memrchr( struct mulle_vm *vm, struct mulle_range range, int c);

// Search any of the matching characters in mulle_vm range
uintptr_t   mulle_vm_memmatch( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
uintptr_t   mulle_vm_memrmatch( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);

// Find the length of matching consecutive match characters occuring in mulle_vm range
size_t   mulle_vm_memspn( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);
size_t   mulle_vm_memrspn( struct mulle_vm *vm, struct mulle_range range, const uint8_t match[256]);

#endif
