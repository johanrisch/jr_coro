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
#ifndef JR_CORO_H
#define JR_CORO_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file jr_coro.h
 * @brief Header file for the coroutine library.
 *
 * This file contains the definitions and function declarations for the coroutine library.
 */

#if defined(__aarch64__)
#define JR_JMP_BUF_SIZE 18
#elif defined(__x86_64__)
#define JR_JMP_BUF_SIZE 8
#else
#error "Unsupported architecture"
#endif

/**
 * @struct jr_jmp_buf
 * @brief Structure to hold the state of a coroutine.
 *
 * This structure is used to save and restore the state of a coroutine.
 */
typedef struct {
    uint64_t regs[JR_JMP_BUF_SIZE]; /**< Array to hold the register values */
} jr_jmp_buf;

/**
 * @enum coro_status_t
 * @brief Enumeration for coroutine status.
 *
 * This enumeration defines the possible statuses of a coroutine.
 */
typedef enum {
    WAITING, /**< Coroutine is waiting */
    DONE     /**< Coroutine is done */
} coro_status_t;

/**
 * @struct jr_coro_t
 * @brief Structure to represent a coroutine.
 *
 * This structure holds the information needed to manage a coroutine.
 */
typedef struct {
    void *stack_pointer;   /**< Aligned stack pointer */
    void *base_pointer;    /**< Original malloc pointer */
    int (*fn)(size_t args); /**< Function pointer for the coroutine function */
    size_t yielded_value;  /**< Value yielded by the coroutine */
    uint8_t initialized;   /**< Flag indicating if the coroutine is initialized */
    uint8_t finished;      /**< Flag indicating if the coroutine is finished */
    size_t args;           /**< Arguments for the coroutine function */
    jr_jmp_buf context;    /**< Jump buffer to save the coroutine context */
} jr_coro_t;

/**
 * @brief Set the jump buffer.
 *
 * This function sets the jump buffer for the coroutine.
 *
 * @param buf Pointer to the jump buffer.
 * @return 0 if returning directly, non-zero if returning from a long jump.
 */
int __jr_setjmp(void *buf);

/**
 * @brief Perform a long jump.
 *
 * This function performs a long jump to the specified jump buffer.
 *
 * @param buf Pointer to the jump buffer.
 * @param value Value to return from the set jump.
 */
void __jr_longjmp(void *buf, int value);

/**
 * @brief Call a function with a new stack pointer.
 *
 * This function switches to a new stack pointer and calls the specified function.
 *
 * @param new_sp Pointer to the new stack.
 * @param func Function to call.
 */
void asm_call(void *new_sp, void (*func)(void));

/**
 * @brief Safely call a function with a new stack pointer.
 *
 * This function safely switches to a new stack pointer and calls the specified function.
 *
 * @param new_sp Pointer to the new stack.
 * @param func Function to call.
 */
void safe_asm_call(void *new_sp, void (*func)(void));

/**
 * @brief Entry point for the coroutine.
 *
 * This function is the entry point for the coroutine.
 */
void __jr_coro_entry();

/**
 * @brief Switch to a coroutine.
 *
 * This function switches to the specified coroutine.
 *
 * @param coro Pointer to the coroutine.
 */
void __jr_switch_to_coro(jr_coro_t *coro);

/**
 * @brief Yield a value from the coroutine.
 *
 * This function yields a value from the coroutine.
 *
 * @param value Value to yield.
 */
void jr_coro_yield(size_t value);

/**
 * @brief Resume a coroutine.
 *
 * This function resumes the specified coroutine.
 *
 * @param coro Pointer to the coroutine.
 * @return Status of the coroutine after resuming.
 */
coro_status_t jr_resume_coro(jr_coro_t *coro);

/**
 * @brief Create a new coroutine.
 *
 * This function creates a new coroutine with the specified function, arguments, and stack size.
 *
 * @param fn Function pointer for the coroutine function.
 * @param args Arguments for the coroutine function.
 * @param stack_size Size of the stack for the coroutine.
 * @return The created coroutine.
 */
jr_coro_t jr_coro(int (*fn)(size_t), size_t args, size_t stack_size);

/**
 * @brief Free a coroutine.
 *
 * This function frees the memory allocated for the coroutine.
 *
 * @param coro Pointer to the coroutine.
 */
void jr_coro_free(jr_coro_t *coro);

#endif // JR_CORO_H