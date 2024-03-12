#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

#define SOCK_MTP 3

struct SOCK_INFO {
    int sock_id = 0;
    int errno = 0;
    int port = 0;
    char *ip = NULL;
};

typedef struct swnd {
        int seq_number[5];
        int window_size;
} swnd;

typedef struct rwnd {
        int seq_number[5];
        int window_size;
} rwnd;

struct Socket {
    int free = 0;
    int pid;
    int sock_id;
    char *destip;
    int destport;
    char *sendbuf[10];
    char *recvbuf[5];  
    swnd sender_window;
    rwnd receiver_window;
};

int errno;

//A shared memory chunk SM containing the information about 25 MTP sockets
struct SOCK_INFO SM[25];

int m_socket(int domain, int type, int protocol);
int m_bind(int sock_id, char *srcip, int srcport, char *destip, int destport);
int m_sendto(int sock_id, char *buf, size_t len, char *destip, int destport);
int m_recvfrom(int sock_id, char *buf, size_t len, char *srcip, int srcport);
int m_close(int sock_id);

