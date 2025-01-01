#include "jr_coro.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
int test_fn(size_t args) {
    for (int i = 0; i < args; i++) {
        jr_coro_yield(i);
    }
    return 0;
}

int test_with_stack_vars(size_t args) {
    int a = 0;
    for (int i = 0; i < args; i++) {
        a += i;
        jr_coro_yield(a);
    }
    return 0;
}

int test_with_string_on_stack(size_t args) {
    char str[11];
    for (int i = 0; i < args; i++) {
        snprintf(str, sizeof(str), "%d", i);
        jr_coro_yield(strlen(str));
    }
    return 0;
}



int main() {
    size_t value = 0;
    printf("[main] Creating coroutine...\n");
    jr_coro_t coro1 = jr_coro(test_fn, 5, 1024 * 1024); // 1MB stack
    jr_coro_t coro2 = jr_coro(test_fn, 10, 1024 * 1024); // 1MB stack
    jr_coro_t coro3 = jr_coro(test_with_stack_vars, 10, 1024 * 1024); // 1MB stack
    jr_coro_t coro4 = jr_coro(test_with_string_on_stack, 10, 1024 * 1024); // 1MB stack
    coro_status_t coro1_status = WAITING;
    coro_status_t coro2_status = WAITING;
    coro_status_t coro3_status = WAITING;
    coro_status_t coro4_status = WAITING;
    int coro1_count = 0, coro2_count = 0, coro3_count = 0, coro4_count = 0;
    printf("[main] Resuming coroutine...\n");
    while(coro1_status != DONE || coro2_status != DONE || coro3_status != DONE || coro4_status != DONE) {
        if (coro1_status != DONE) {
            coro1_status = jr_resume_coro(&coro1);
            // printf("(coro 1):  %d %lu\n",coro1_count, coro1.yielded_value);
            assert(coro1_count == 5 || (coro1.yielded_value == coro1_count && "Yielded value is wrong"));
            coro1_count++;
        }
        if (coro2_status != DONE) {
            coro2_status = jr_resume_coro(&coro2);
            // printf("(coro 2):  %d %lu\n",coro2_count, coro2.yielded_value);
            assert(coro2_count == 10 || (coro2.yielded_value == coro2_count && "Yielded value is wrong"));
            coro2_count++;
        } 
        if (coro3_status != DONE) {
            coro3_status = jr_resume_coro(&coro3);
            // printf("(coro 3):  %d %lu\n",coro3.finished, coro3.yielded_value);
            coro3_count++;
        } 
        if (coro4_status != DONE) {
            coro4_status = jr_resume_coro(&coro4);
            // printf("(coro 4):  %d %lu\n",coro4.finished, coro4.yielded_value);
            coro4_count++;
        }
    }
    assert(coro1_count == 6 && "CORO 1 count is wrong");
    printf("[CORO 1] PASS\n");
    assert(coro2_count == 11 && "CORO 2 count is wrong");
    printf("[CORO 2] PASS\n");
    assert(coro3_count == 11 && "CORO 3 count is wrong");
    assert(coro3.yielded_value == 45 && "CORO 3 Yielded value is wrong");
    printf("[CORO 3] PASS\n");
    assert(coro4_count == 11 && "CORO 4 count is wrong");
    assert(coro4.yielded_value == 1 && "CORO 4 Yielded value is wrong");
    printf("[CORO 4] PASS\n");
    jr_coro_free(&coro1);
    jr_coro_free(&coro2);
    

    printf("[main] DONE\n");
    return 0;
}