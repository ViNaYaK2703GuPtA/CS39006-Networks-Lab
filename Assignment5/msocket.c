#include "msocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_SOCKETS 25


int m_socket(int domain, int type, int protocol)
{
    if (type != SOCK_MTP)
    {
        fprintf(stderr, "Invalid socket type\n");
        // errno = 1;
        exit(1);
    }
    

    int flag = 0;
    int index = 0;

    key_t key1;
    key1 = 827;

    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    for (index = 0; index < 25; index++)
    {
        
        if (SM[index].free == 0)
        {
            flag = 1;
            break;
        }
    }

    key_t semkeyA = ftok(".", 'A');
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0666);
    semctl(Sem1, 0, SETVAL, 0);
    // Signal on Sem1
    signal_sem(Sem1, 0);

    // Wait on Sem2
    key_t semkeyB = ftok(".", 'B');
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0666);
    semctl(Sem2, 0, SETVAL, 0);
    wait_sem(Sem2, 0);
    if (flag == 0)
    {
        errno = ENOBUFS;
        return -1;
    }
    else
    {
        return SM[index].sock_id;
    }

    
}

int m_bind(int sock_id, char *srcip, int srcport, char *destip, int destport)
{

    

    // running  a for loop to find the actual udp socket id from the SM Table
    key_t key1;
    key1 = 827;

    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    int index = 0;
    for (index = 0; index < 25; index++)
    {
        
        if (SM[index].sock_id == sock_id)
        {
            break;
        }
    }

    // Put the UDP socket ID, IP, and port in SOCK_INFO table.
    key_t key;
    key = 2708;
    // create a shared memory segment
    int shmid = shmget(key, sizeof(struct SOCK_INFO), IPC_CREAT | 0777);
    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);

    Sinfo->sock_id = sock_id;
    strcpy(Sinfo->ip, destip);
    Sinfo->port = destport;

    

    key_t semkeyA = ftok(".", 'A');
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0666);
    semctl(Sem1, 0, SETVAL, 0);
    // Signal on Sem1
    signal_sem(Sem1, 0);

    // Wait on Sem2
    key_t semkeyB = ftok(".", 'B');
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0666);
    semctl(Sem2, 0, SETVAL, 0);
    wait_sem(Sem2, 0);

    if (Sinfo->sock_id == -1)
    {
        errno = Sinfo->err_no;
        Sinfo->sock_id = 0;
        memset(Sinfo->ip, 0, sizeof(Sinfo->ip));
        Sinfo->port = 0;
        Sinfo->err_no = 0;
        return -1;
    }
    else
    {
        Sinfo->sock_id = 0;
        memset(Sinfo->ip, 0, sizeof(Sinfo->ip));
        Sinfo->port = 0;
        Sinfo->err_no = 0;

        //UPDATING THE SM TABLE
        strcpy(SM[index].destip, destip);
        SM[index].destport = destport;
        return 0;   
    }

}

int m_sendto(int sock_id, char *buf, size_t len, char *destip, int destport)
{
    int sockfd = sock_id;
    int index = 0;
    key_t key1;
    key1 = 827;
    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    for (index = 0; index < 25; index++)
    {
        if (SM[index].sock_id == sock_id)
        {
            break;
        }
    }

    if(strcmp(SM[index].destip, destip) != 0 || SM[index].destport != destport)
    {
        errno = ENOTCONN;  
        return -1;
    }

    //check if there is space in the send buffer
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        if (SM[index].sendbuf[i] == NULL)
        {
            strcpy(SM[index].sendbuf[i], buf);
            break;
        }
    }
    if (i == 10)
    {
        errno = ENOBUFS;
        return -1;
    }
    return 0;
}



int m_recvfrom(int sock_id, char *buf, size_t len, char *srcip, int srcport)
{
    int sockfd = sock_id;
    int index = 0;
    key_t key1;
    key1 = 827;
    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    
    for (index = 0; index < 25; index++)
    {
        if (SM[index].sock_id == sock_id)
        {
            break;
        }
    }

    int i = 0;
    for (i = 0; i < 5; i++)
    {
        if (SM[index].recvbuf[i] != NULL)
        {
            strcpy(buf, SM[index].recvbuf[i]);
            SM[index].recvbuf[i] = NULL;
            break;
        }
    }
    if (i == 5)
    {
        errno = ENOMSG;
        return -1;
    }
    return 0;
}


int m_close(int sock_id)
{
    int sockfd = sock_id;
    int index = 0;
    key_t key1;
    key1 = 827;
    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    
    for (index = 0; index < 25; index++)
    {
        if (SM[index].sock_id == sock_id)
        {
            break;
        }
    }
    SM[index].free = 0;
    SM[index].pid = 0;
    SM[index].sock_id = 0;
    memset(SM[index].destip, 0, sizeof(SM[index].destip));
    SM[index].destport = 0;
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        SM[index].sendbuf[i] = NULL;
    }
    for (i = 0; i < 5; i++)
    {
        SM[index].recvbuf[i] = NULL;
    }
    return close(sockfd);
}
