#include "jr_coro.h"
#include <stdio.h>
int test_fn(size_t args) {
    for (int i = 0; i < args; i++) {
        yield(i);
    }
    return 0;
}

int main() {
    size_t value = 0;
    printf("[main] Creating coroutine...\n");
    jr_coro_t coro1 = jr_coro(test_fn, 5, 1024 * 1024); // 1MB stack
    jr_coro_t coro2 = jr_coro(test_fn, 10, 1024 * 1024); // 1MB stack
    coro_status_t coro1_status = WAITING;
    coro_status_t coro2_status = WAITING;

    printf("[main] Resuming coroutine...\n");
    while(coro1_status != DONE || coro2_status != DONE) {
        if (coro1_status != DONE) {
            coro1_status = resume_coro(&coro1);
            printf("[main] Yielded value (coro 1):%d %lu\n",coro1_status, coro1.yielded_value);
        }
        if (coro2_status != DONE) {
            coro2_status = resume_coro(&coro2);
            printf("[main] Yielded value (coro 2):%d %lu\n",coro2.finished, coro2.yielded_value);
        }

    }
    jr_coro_free(&coro1);
    jr_coro_free(&coro2);
    

    printf("[main] DONE\n");
    return 0;
}