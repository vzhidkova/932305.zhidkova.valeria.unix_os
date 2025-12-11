#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

volatile sig_atomic_t was_sighup = 0;

void handle_sighup(int signo) {
    (void)signo;
    was_sighup = 1;
}

int make_listen(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons((uint16_t)port);
    if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        close(s);
        return -1;
    }
    if (listen(s, 4) < 0) {
        close(s);
        return -1;
    }
    return s;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port\n");
        return EXIT_FAILURE;
    }

    sigset_t block_mask, orig_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &block_mask, &orig_mask) < 0) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sighup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    int listen_fd = make_listen(port);
    if (listen_fd < 0) {
        perror("make_listen");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "Listening on port %d\n", port);

    int client_fd = -1;

    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        int maxfd = listen_fd;
        if (client_fd >= 0) {
            FD_SET(client_fd, &rfds);
            if (client_fd > maxfd) maxfd = client_fd;
        }

        int ready = pselect(maxfd + 1, &rfds, NULL, NULL, NULL, &orig_mask);
        if (ready < 0) {
            if (errno == EINTR) {
                if (was_sighup) {
                    fprintf(stderr, "Received SIGHUP (via EINTR)\n");
                    was_sighup = 0;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (was_sighup) {
            fprintf(stderr, "Received SIGHUP\n");
            was_sighup = 0;
        }

        if (FD_ISSET(listen_fd, &rfds)) {
            struct sockaddr_in peer;
            socklen_t plen = sizeof(peer);
            int c = accept(listen_fd, (struct sockaddr*)&peer, &plen);
            if (c < 0) {
                perror("accept");
            } else {
                char addr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &peer.sin_addr, addr, sizeof(addr));
                fprintf(stderr, "New connection from %s:%d -> fd=%d\n",
                        addr, ntohs(peer.sin_port), c);
                if (client_fd < 0) {
                    client_fd = c;
                    fprintf(stderr, "Connection accepted (fd=%d)\n", client_fd);
                } else {
                    fprintf(stderr, "Closing extra connection fd=%d\n", c);
                    close(c);
                }
            }
        }

        if (client_fd >= 0 && FD_ISSET(client_fd, &rfds)) {
            char buf[4096];
            ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
            if (n > 0) {
                fprintf(stderr, "Received %zd bytes from client (fd=%d)\n", n, client_fd);
            } else if (n == 0) {
                fprintf(stderr, "Client (fd=%d) closed connection\n", client_fd);
                close(client_fd);
                client_fd = -1;
            } else {
                perror("recv");
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    if (client_fd >= 0) close(client_fd);
    close(listen_fd);
    return EXIT_SUCCESS;
}