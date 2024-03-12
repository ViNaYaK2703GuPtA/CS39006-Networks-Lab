#include "msocket.h"
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int m_socket(int domain, int type, int protocol) {
    if(type != SOCK_MTP) {
        fprintf(stderr, "Invalid socket type\n");
        exit(1);
    }
    int sock_id = socket(domain, SOCK_DGRAM, protocol);
    if (sock_id < 0) {
        perror("socket");
        exit(1);
    }
    return sock_id;
}

void mbind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(sockfd, addr, addrlen) < 0) {
        perror("bind");
        exit(1);
    }
}

