#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>

#define SOCK_MTP 3

struct SOCK_INFO {
    int sock_id;
    int err_no;
    int port ;
    char *ip ;
};

typedef struct swnd {
        int seq_number[5];
        int window_size;
        time_t send_time;
} swnd;

typedef struct rwnd {
        int seq_number[5];
        int window_size;
        time_t recv_time;
} rwnd;

struct Socket {
    int free ;
    int pid;
    int sock_id;
    char *destip;
    int destport;
    char *sendbuf[10];
    char *recvbuf[5];  
    swnd sender_window;
    rwnd receiver_window;
};

//A shared memory chunk SM containing the information about 25 MTP sockets

void wait_sem(int sem_id, int sem_num) {
    struct sembuf sops;
    sops.sem_num = sem_num;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
}

// Function to signal a semaphore
void signal_sem(int sem_id, int sem_num) {
    struct sembuf sops;
    sops.sem_num = sem_num;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
}

int Sem1, Sem2;



int m_socket(int domain, int type, int protocol);
int m_bind(int sock_id, char *srcip, int srcport, char *destip, int destport);
int m_sendto(int sock_id, char *buf, size_t len, char *destip, int destport);
int m_recvfrom(int sock_id, char *buf, size_t len, char *srcip, int srcport);
int m_close(int sock_id);

