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
.global jr_arm_call
.type jr_arm_call, %function

jr_arm_call:
    // Save general-purpose registers
    stp x29, x30, [sp, #-16]!
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!

    // Save NEON registers
    stp d8, d9, [sp, #-16]!
    stp d10, d11, [sp, #-16]!
    stp d12, d13, [sp, #-16]!
    stp d14, d15, [sp, #-16]!

    // Validate stack alignment
    mov x9, sp                 // Copy SP to a general-purpose register
    and x9, x9, #0xF           // Check alignment
    cbnz x9, .L_alignment_error

    // Switch to new stack and call function
    mov x9, sp
    mov sp, x0
    blr x1

    // Restore original stack
    mov sp, x9

    // Restore NEON registers
    ldp d14, d15, [sp], #16
    ldp d12, d13, [sp], #16
    ldp d10, d11, [sp], #16
    ldp d8, d9, [sp], #16

    // Restore general-purpose registers
    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ldp x29, x30, [sp], #16

    ret


.global jr_setjmp
.type jr_setjmp, %function

jr_setjmp:
    stp x19, x20, [x0, #0]      // Save x19 and x20
    stp x21, x22, [x0, #16]     // Save x21 and x22
    stp x23, x24, [x0, #32]     // Save x23 and x24
    stp x25, x26, [x0, #48]     // Save x25 and x26
    stp x27, x28, [x0, #64]     // Save x27 and x28
    stp x29, x30, [x0, #80]     // Save frame pointer (x29) and link register (x30)
    mov x9, sp                  // Save stack pointer
    str x9, [x0, #96]
    mrs x9, nzcv                // Read CPSR
    str x9, [x0, #104]          // Save CPSR in the context
    mov w0, #0                  // Return 0
    ret
.global jr_longjmp
.type jr_longjmp, %function

jr_longjmp:
    ldp x19, x20, [x0, #0]      // Restore x19 and x20
    ldp x21, x22, [x0, #16]     // Restore x21 and x22
    ldp x23, x24, [x0, #32]     // Restore x23 and x24
    ldp x25, x26, [x0, #48]     // Restore x25 and x26
    ldp x27, x28, [x0, #64]     // Restore x27 and x28
    ldp x29, x30, [x0, #80]     // Restore frame pointer and link register
    ldr x9, [x0, #96]           // Restore stack pointer
    mov sp, x9

    // Validate alignment
    mov x9, sp                 // Copy SP to a general-purpose register
    and x9, x9, #0xF           // Check alignment
    cbnz x9, .L_alignment_error // Branch if not aligned
    mov w0, w1                  // Set return value
    ret

    

.L_alignment_error:
    // Print an error or handle misalignment
    mov w0, #1                  // Error return value
    ret

