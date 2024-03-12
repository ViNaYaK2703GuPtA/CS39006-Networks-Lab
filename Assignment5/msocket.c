#include "msocket.h"
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int m_socket(int domain, int type, int protocol) {
    if(type != SOCK_MTP) {
        fprintf(stderr, "Invalid socket type\n");
        //errno = 1;
        exit(1);
    }
    int sock_id = socket(domain, SOCK_DGRAM, protocol);
    if (sock_id < 0) {
        perror("socket");
        exit(1);
    }

    int flag = 0;
    int index = 0;
    for(index =0; index<25;index++)
    {
        if(SM[index].free==0)
        {
            flag = 1;
            break;
        }
    }
   
    key_t semkeyA= ftok(".", 'A');
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0666);
    semctl(Sem1, 0, SETVAL, 0);
    // Signal on Sem1
    signal_sem(Sem1,0);

    // Wait on Sem2
    key_t semkeyB= ftok(".", 'B');
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0666);
    semctl(Sem2, 0, SETVAL, 0);
    wait_sem(Sem2,0);
    if(flag == 0)
    {
        errno = ENOBUFS;
        return -1;
    }
    else
    {
        SM[index].free = 1;
        SM[index].sock_id = sock_id;
        SM[index].pid = getpid();
    }

    return sock_id;
}

int m_bind(int sock_id, char *srcip, int srcport, char *destip, int destport) {
    if (bind(sockfd, addr, addrlen) < 0) {
        perror("bind");
        exit(1);
    }
}

