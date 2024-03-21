#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
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
    char ip[16];
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
    char destip[16];
    int destport;
    char sendbuf[25][10];
    char recvbuf[25][5];  
    swnd sender_window;
    rwnd receiver_window;
};







int m_socket(int domain, int type, int protocol);
int m_bind(int sock_id, char *srcip, int srcport, char *destip, int destport);
int m_sendto(int sock_id, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
int m_recvfrom(int sock_id, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int m_close(int sock_id);
void wait_sem(int sem_id, int sem_num);
void signal_sem(int sem_id, int sem_num);

