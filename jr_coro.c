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

extern int jr_setjmp(void *buf);
extern void jr_longjmp(void *buf, int value);

typedef struct {
    uint64_t x19_x20[2];
    uint64_t x21_x22[2];
    uint64_t x23_x24[2];
    uint64_t x25_x26[2];
    uint64_t x27_x28[2];
    uint64_t x29_x30[2];
    uint64_t x8;   
    uint64_t sp;   
    uint64_t CPSR; 
} jr_jmp_buf;


#define DEBUG_LEVEL 0
#if DEBUG_LEVEL > 0
#define DEBUG_PRINT(level, fmt, ...) \
    do { if (level <= DEBUG_LEVEL) fprintf(stderr, fmt, ##__VA_ARGS__); } \
    while (0)
#else
#define DEBUG_PRINT(level, fmt, ...) do {} while (0)
#endif


typedef enum {
    WAITING,
    DONE
} coro_status_t;

extern void jr_arm_call(void *new_sp, void (*func)(void));
jr_jmp_buf jr_main_context;

void safe_jr_arm_call(void *new_sp, void (*func)(void)) {
    DEBUG_PRINT(2, "[safe_jr_arm_call] Current stack pointer: %p\n", 
                __builtin_frame_address(0));
    DEBUG_PRINT(2, "[safe_jr_arm_call] New stack pointer: %p\n", new_sp);
    assert((uintptr_t)new_sp % 16 == 0 
           && "New stack pointer not 16-byte aligned!");

    if (jr_setjmp(&jr_main_context) == 0) {
        jr_arm_call(new_sp, func);
    } else {
        DEBUG_PRINT(2, "[main] Back on original stack after longjmp\n");
    }

    assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
           && "Stack not 16-byte aligned!");
    DEBUG_PRINT(2, "[safe_jr_arm_call] Restored stack pointer: %p\n", 
                __builtin_frame_address(0));
}

typedef struct {
    void *stack_pointer;   // Aligned stack pointer
    void *base_pointer;    // Original malloc pointer
    int (*fn)(size_t args);
    size_t yielded_value;
    uint8_t initialized;
    uint8_t finished;
    size_t args;
    jr_jmp_buf context;
} jr_coro_t;

volatile jr_coro_t *jr_current_coro = NULL;
int yielded_value = 0;

void coro_entry() {
    DEBUG_PRINT(2, "[coro_entry] Entering coro_entry\n");

    if (jr_current_coro->initialized == 0) {
        jr_current_coro->initialized = 1;
        
        DEBUG_PRINT(2, "[coro_entry] Initializing coroutine\n");
        assert(jr_current_coro->fn != NULL && "Function pointer is NULL");

        jr_current_coro->fn(jr_current_coro->args);
        jr_current_coro->finished = 1;
        
        DEBUG_PRINT(2, "[coro_entry] Coroutine finished\n");
    }

    DEBUG_PRINT(2, "[coro_entry] Coroutine finished, returning to main context\n");

    jr_longjmp(&jr_main_context, 1); // Return to main context after completion
}


void switch_to_coro(jr_coro_t *coro) {
    DEBUG_PRINT(2, "[switch_to_coro] Switching to coro: %p\n", coro->stack_pointer);
    jr_current_coro = coro;

    if (coro->initialized) {
        DEBUG_PRINT(2, "[switch_to_coro] Coro initialized, jumping to: %p\n", 
                    coro->stack_pointer);
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
        
        if(jr_setjmp(&jr_main_context) == 0){
            jr_longjmp(&(coro->context), 1);
        }

        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    } else {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");

        safe_jr_arm_call(coro->stack_pointer, coro_entry);

        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    }

    DEBUG_PRINT(2, "[switch_to_coro] Returned from coro\n");
}

void yield(size_t value)   {
    assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
           && "Stack not 16-byte aligned!");
    size_t local_value = value;
    jr_coro_t *cc = (jr_coro_t *)jr_current_coro;
    DEBUG_PRINT(2, "[yield] Yielding value: %lu\n", local_value);
    if (jr_current_coro == NULL) {
        DEBUG_PRINT(2, "[yield] No jr_current_coro, returning\n");
        return;
    }
    cc->yielded_value = local_value;
    if (jr_setjmp(&(cc->context)) == 0) {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
        DEBUG_PRINT(1, "[yield] Saving coroutine context, jumping to main context %lu\n", 
                    local_value);
        jr_longjmp(&jr_main_context, 1);
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
    } else {
        assert(((uintptr_t)__builtin_frame_address(0) % 16) == 0 
               && "Stack not 16-byte aligned!");
        DEBUG_PRINT(2, "[yield] Back on coroutine stack\n");
    }
}


coro_status_t resume_coro(jr_coro_t *coro) {
    volatile jr_coro_t *volatile_coro = coro;
    DEBUG_PRINT(2, "[resume_coro] Resuming coro\n");
    switch_to_coro((jr_coro_t *)volatile_coro);
    DEBUG_PRINT(1, "[resume_coro] Resumed coro, yielded value: %lu\n", 
                volatile_coro->yielded_value);
    return volatile_coro->finished ? DONE : WAITING;
}


jr_coro_t jr_coro(int (*fn)(size_t), size_t args, size_t stack_size) {
    DEBUG_PRINT(2, "[jr_coro] Creating new coroutine with stack size: %zu\n", 
                stack_size);
    jr_coro_t coro;
    coro.fn = fn;
    coro.args = args;
    coro.initialized = 0;

    void *base_pointer = malloc(stack_size + 16);
    if (base_pointer == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    uintptr_t aligned_top = ((uintptr_t)base_pointer + stack_size) & ~((uintptr_t)0xF);

    coro.stack_pointer = (void *)aligned_top;
    coro.base_pointer = base_pointer;

    assert((uintptr_t)coro.stack_pointer % 16 == 0 
           && "Stack pointer not 16-byte aligned!");
    DEBUG_PRINT(2, "[jr_coro] Coroutine stack pointer: %p\n", coro.stack_pointer);
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