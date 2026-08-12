// Native MPTCP smoke test. Not a benchmark or secure protocol.
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef IPPROTO_MPTCP
#define IPPROTO_MPTCP 262
#endif

static void die(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

static int make_socket(void) {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_MPTCP);
    if (fd < 0) die("socket(IPPROTO_MPTCP)");
    return fd;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "server: %s server BIND_IP PORT\nclient: %s client PEER_IP PORT [BYTES]\n", argv[0], argv[0]);
        return 2;
    }
    char *end = NULL;
    long parsed_port = strtol(argv[3], &end, 10);
    if (!end || *end != '\0' || parsed_port < 1 || parsed_port > 65535) {
        fprintf(stderr, "invalid port\n"); return 2;
    }
    const int port = (int)parsed_port;
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons((uint16_t)port) };
    if (inet_pton(AF_INET, argv[2], &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid IPv4 address\n"); return 2;
    }
    if (strcmp(argv[1], "server") == 0) {
        int s = make_socket(), one = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
        if (listen(s, 4) < 0) die("listen");
        int c = accept(s, NULL, NULL); if (c < 0) die("accept");
        char buf[1 << 16]; uint64_t total = 0; ssize_t n;
        for (;;) {
            n = read(c, buf, sizeof(buf));
            if (n > 0) { total += (uint64_t)n; continue; }
            if (n == 0) break;
            if (errno == EINTR) continue;
            die("read");
        }
        printf("received=%" PRIu64 " bytes\n", total);
        close(c); close(s); return 0;
    }
    if (strcmp(argv[1], "client") == 0) {
        uint64_t goal = argc > 4 ? strtoull(argv[4], NULL, 10) : (1ULL << 30);
        int s = make_socket(); if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("connect");
        char buf[1 << 16]; memset(buf, 0xa5, sizeof(buf)); uint64_t sent = 0;
        while (sent < goal) {
            size_t want = goal - sent < sizeof(buf) ? (size_t)(goal - sent) : sizeof(buf);
            ssize_t n = write(s, buf, want);
            if (n > 0) { sent += (uint64_t)n; continue; }
            if (n < 0 && errno == EINTR) continue;
            if (n == 0) { errno = EIO; }
            die("write");
        }
        shutdown(s, SHUT_WR); printf("sent=%" PRIu64 " bytes\n", sent); close(s); return 0;
    }
    fprintf(stderr, "mode must be server or client\n"); return 2;
}
