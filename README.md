# jr_coro

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview

jr_coro is a lightweight coroutine library for C, designed to test an idea i had. Inspired by stackable coroutines using longjmp and setjmp. But with their own allocated stack.

The idea is to malloc an area, move the stackpointer there then setup up jump buffers to allow for easy switching. The register saving and restoring is done inside `aarch64.S` to avoid problems with `-O3` optimizations.

I plan to use this in a Lisp interpreter that I'm currently writing.

## Features

- Lightweight and minimalistic coroutine implementation
- Only implemented for ARM (AARCH64) architecture

## Getting Started

At the moment this has only been built and tested on an WSL2 running on AARCH64.

For other architecture new assembly files implementing longjmp and setjmp needs to be implemented. The jmp_buf struct needs to be updated accordingly.

### Prerequisites

- ARM toolchain (e.g., GCC for ARM)
- Make

### Building the Project

To build the project, run the following command:

```sh
make all
```

This will compile the source files and create the `main.out` executable.

### Running the Example

After building the project, you can run the example program:

```sh
./main.out
```

### Running Tests

The tests are very basic at the moment, just run the main.out or:

```sh
make test
```

Which should yield this output:

```
make test
cc -O3 -c aarch64.S jr_coro.c
ar rcs jr_coro_aarch64.a aarch64.o jr_coro.o
cc -O3 main.c jr_coro_aarch64.a -o main.out
./main.out
[main] Creating coroutine...
[main] Resuming coroutine...
[main] Yielded value (coro 1):0 0
[main] Yielded value (coro 2):0 0
[main] Yielded value (coro 1):0 1
[main] Yielded value (coro 2):0 1
[main] Yielded value (coro 1):0 2
[main] Yielded value (coro 2):0 2
[main] Yielded value (coro 1):0 3
[main] Yielded value (coro 2):0 3
[main] Yielded value (coro 1):0 4
[main] Yielded value (coro 2):0 4
[main] Yielded value (coro 1):1 4
[main] Yielded value (coro 2):0 5
[main] Yielded value (coro 2):0 6
[main] Yielded value (coro 2):0 7
[main] Yielded value (coro 2):0 8
[main] Yielded value (coro 2):0 9
[main] Yielded value (coro 2):1 9
[main] DONE
```

## API Reference

### Creating a Coroutine

```c
jr_coro_t jr_coro(int (*fn)(size_t), size_t args, size_t stack_size);
```

Creates a new coroutine with the specified function, arguments, and stack size.

### Resuming a Coroutine

```c
coro_status_t resume_coro(jr_coro_t *coro);
```

Resumes the specified coroutine and returns its status.

### Yielding a Value

```c
void yield(size_t value);
```

Yields a value from the coroutine.
The yielded value can be found on the `yielded_value` field in the coro struct.

### Freeing a Coroutine

```c
void jr_coro_free(jr_coro_t *coro);
```

Frees the memory allocated for the coroutine.

## Example

Here is a simple example demonstrating how to use the jr_coro library:

```c
#include "jr_coro.h"
#include <stdio.h>

int test_fn(size_t args) {
    for (int i = 0; i < args; i++) {
        yield(i);
    }
    return 0;
}

int main() {
    printf("[main] Creating coroutine...\n");
    jr_coro_t coro = jr_coro(test_fn, 5, 1024 * 1024); // 1MB stack

    printf("[main] Resuming coroutine...\n");
    while (resume_coro(&coro) != DONE) {
        printf("[main] Yielded value: %lu\n", coro.yielded_value);
    }

    jr_coro_free(&coro);
    printf("[main] DONE\n");
    return 0;
}
```

## License

This project is licensed under the MIT License - see the LICENSE
 file for details.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.
