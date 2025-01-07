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
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "jr_coro.h" // Include the jr_coro library

#define MAX_EVENTS 10
#define PORT 8080

typedef struct {
    int sockfd;
    jr_coro_t coro;
} socket_coro_t;

int socket_coroutine(size_t sockfd) {
    char buffer[1024];
    int n;

    while (1) {
        n = read(sockfd, buffer, sizeof(buffer));
        if (n <= 0) {
            if (n == 0) {
                printf("Client disconnected\n");
            } else {
                perror("Read error");
            }
            close(sockfd);
            break;
        }
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
        write(sockfd, buffer, n); // Echo back to client
        jr_coro_yield(0); // Yield control back to the main loop
    }
    return 0;
}

int main() {
    int server_fd, new_socket, epoll_fd;
    struct sockaddr_in address;
    struct epoll_event ev, events[MAX_EVENTS];
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("Epoll create failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("Epoll ctl failed");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("Epoll wait failed");
            close(server_fd);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (new_socket == -1) {
                    perror("Accept failed");
                    continue;
                }

                ev.events = EPOLLIN | EPOLLET;
                socket_coro_t *socket_coro = malloc(sizeof(socket_coro_t));
                socket_coro->sockfd = new_socket;
                socket_coro->coro = jr_coro(socket_coroutine, (size_t)new_socket, 1024 * 1024); // 1MB stack
                ev.data.ptr = socket_coro; // Associate the socket_coro struct with the event
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("Epoll ctl add failed");
                    close(new_socket);
                    free(socket_coro);
                    continue;
                }

                jr_resume_coro(&socket_coro->coro);
            } else {
                socket_coro_t *socket_coro = (socket_coro_t *)events[i].data.ptr;
                if (socket_coro) {
                    jr_resume_coro(&socket_coro->coro);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}