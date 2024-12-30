/*
MIT License

Copyright (c) 2024 Johan Risch

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include "jr_coro.h"

// Implemented in assembly.
extern int __jr_setjmp(void *buf);
extern void __jr_longjmp(void *buf, int value);
extern void __jr_asm_call(void *new_sp, void (*func)(void));

jr_jmp_buf __jr_main_context;
jr_coro_t *__jr_current_coro = NULL;


void __jr_safe_asm_call(void *new_sp, void (*func)(void)) {
    assert((uintptr_t)new_sp % 16 == 0 
           && "New stack pointer not 16-byte aligned!");

    if (__jr_setjmp(&__jr_main_context) == 0) {
        __jr_asm_call(new_sp, func);
    }

    assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
           && "Stack not 16-byte aligned!");
}

void __jr_coro_entry() {
    if (__jr_current_coro->initialized == 0) {
        __jr_current_coro->initialized = 1;
        
        assert(__jr_current_coro->fn != NULL && "Function pointer is NULL");

        __jr_current_coro->fn(__jr_current_coro->args);
        __jr_current_coro->finished = 1;
    }

    __jr_longjmp(&__jr_main_context, 1); // Return to main context after completion
}


void __jr_switch_to_coro(jr_coro_t *coro) {
    __jr_current_coro = coro;

    if (coro->initialized) {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
        
        if(__jr_setjmp(&__jr_main_context) == 0){
            __jr_longjmp(&(coro->context), 1);
        }

        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    } else {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");

        __jr_safe_asm_call(coro->stack_pointer, __jr_coro_entry);

        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    }
}

void jr_coro_yield(size_t value)   {
    assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
           && "Stack not 16-byte aligned!");
    size_t local_value = value;
    jr_coro_t *cc = (jr_coro_t *)__jr_current_coro;
    if (__jr_current_coro == NULL) {
        return;
    }
    cc->yielded_value = local_value;
    if (__jr_setjmp(&(cc->context)) == 0) {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");

        __jr_longjmp(&__jr_main_context, 1);

        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    } else {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    }
}


coro_status_t jr_resume_coro(jr_coro_t *coro) {
    volatile jr_coro_t *volatile_coro = coro;
    __jr_switch_to_coro((jr_coro_t *)volatile_coro);

    return volatile_coro->finished ? DONE : WAITING;
}


jr_coro_t jr_coro(int (*fn)(size_t), size_t args, size_t stack_size) {
    jr_coro_t coro;
    coro.fn = fn;
    coro.args = args;
    coro.initialized = 0;

    void *base_pointer = malloc(stack_size + 16);
    assert(base_pointer != NULL && "Failed to allocate memory for coroutine stack");

    uintptr_t aligned_top = ((uintptr_t)base_pointer + stack_size) & ~((uintptr_t)0xF);

    coro.stack_pointer = (void *)aligned_top;
    coro.base_pointer = base_pointer;

    assert((uintptr_t)coro.stack_pointer % 16 == 0 
           && "Stack pointer not 16-byte aligned!");
    return coro;
}

void jr_coro_free(jr_coro_t *coro) {
    assert(coro != NULL && "Coro is NULL");
    if (coro->base_pointer != NULL) {
        free(coro->base_pointer);
        coro->base_pointer = NULL;
        coro->stack_pointer = NULL;
    }
}