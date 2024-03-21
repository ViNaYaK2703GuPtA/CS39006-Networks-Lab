#include "msocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <semaphore.h>

#define MAX_SOCKETS 25

//A shared memory chunk SM containing the information about 25 MTP sockets

void wait_sem(int sem_id) {
    //printf("Init process waiting\n");

    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    semop(sem_id, &sops, 1);
    printf("Init process waiting\n");
}

// Function to signal a semaphore
void signal_sem(int sem_id) {

    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = 0;   
    semop(sem_id, &sops, 1);
    printf("Init process signaling\n");
}

int m_socket(int domain, int type, int protocol)
{

    printf("msocket called\n");
    if (type != SOCK_MTP)
    {
        fprintf(stderr, "Invalid socket type\n");
        // errno = 1;
        exit(1);
    }
    

    int flag = 0;
    int index = 0;

    key_t key1;
    key1 = ftok(".",'a');

    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    key_t key;
    key = ftok(".", 'b');
    // create a shared memory segment
    int shmid = shmget(key, sizeof(struct SOCK_INFO), IPC_CREAT | 0777);
    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);

    printf("msocket called\n");

    for (index = 0; index < 25; index++)
    {
        printf("index\n");
        if (SM[index].free == 0)
        {
            printf("%d\n", index);
            SM[index].free = 1;
            flag = 1;
            break;
        }
    }
    
    key_t semkeyA = ftok(".", 'A');
    int Sem1;
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0777);
    //semctl(Sem1, 0, SETVAL, 0);
    // Signal on Sem1
    printf("gcjcjgjgcf\n");
    signal_sem(Sem1);

    // Wait on Sem2
    key_t semkeyB = ftok(".", 'B');
    int Sem2;
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0777);
    // semctl(Sem2, 0, SETVAL, 0);

    printf("msocket called\n");

    wait_sem(Sem2);
    printf("msocket exiting\n");    
    if (flag == 0)
    {
        errno = ENOBUFS;
        return -1;
    }
    else
    {
        return index;
    }

    
}

int m_bind(int sockfd, char *srcip, int srcport, char *destip, int destport)
{
    // running  a for loop to find the actual udp socket id from the SM Table
    key_t key1;
    key1 = ftok(".",'a');

    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    

    // Put the UDP socket ID, IP, and port in SOCK_INFO table.
    key_t key;
    key = ftok(".", 'b');
    // create a shared memory segment
    int shmid = shmget(key, sizeof(struct SOCK_INFO), IPC_CREAT | 0777);
    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    printf("m_bind called\n");

   // Sinfo->ip = (char *)malloc(16 * sizeof(char));

    strcpy(Sinfo->ip, destip);
    Sinfo->port = destport;
    printf("%d %d\n", destport, Sinfo->port);
    printf("%d\n", Sinfo->sock_id);

     printf("m_bind called\n");

    key_t semkeyA = ftok(".", 'A');
    int Sem1;
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0777);
    // semctl(Sem1, 0, SETVAL, 0);
    key_t semkeyB = ftok(".", 'B');
    int Sem2;
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0777);
    // semctl(Sem2, 0, SETVAL, 0);
    // Signal on Sem1
    signal_sem(Sem1);
    printf("mbind signal\n");

    // Wait on Sem2
    
    
    wait_sem(Sem2);
    printf("mbind wait\n");

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
        //SM[index].destip = (char *)malloc(16 * sizeof(char));
        strcpy(SM[sockfd].destip, destip);
        SM[sockfd].destport = destport;
        return 0;   
    }
    //printf("m_bind exiting\n");

}

int m_sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t dest_len)
{
    
    int index = sockfd;
    key_t key1;
    key1 = ftok(".",'a');
    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    
    struct sockaddr_in *dest = (struct sockaddr_in *)dest_addr;

    printf("m_sendto called\n");

    printf("%s\n", inet_ntoa((dest->sin_addr)));
    printf("%s\n", SM[index].destip);
    printf("%d\n", ntohs(dest->sin_port));
    printf("%d\n", SM[index].destport);
    char *temp = (char *)malloc(17 * sizeof(char));
    strcpy(temp, SM[index].destip);

    if(strcmp(temp,inet_ntoa((dest->sin_addr))) || SM[index].destport != ntohs(dest->sin_port))
    {
        errno = ENOTCONN;          
        return -1;
    }
    printf("m_sendto called\n");
    //check if there is space in the send buffer
    int i = 0;
    char *buffer = (char *)buf;
    printf("index = %d\n", index);
    for (i = 0; i < 10; i++)
    {
        if (strcmp(SM[index].sendbuf[i], "") == 0)
        {   
            printf("m_sendto called in the loop\n");    
            strcpy(SM[index].sendbuf[i], buffer);
            SM[index].sender_window.window_size++;
            break;
        }
    }
    if (i == 10)
    {
        errno = ENOBUFS;
        return -1;
    }
    printf("%s\n", SM[index].sendbuf[i]);
    return 0;
}



int m_recvfrom(int sockfd, void  *restrict buf, size_t len, int flags, struct sockaddr *restrict address, socklen_t *restrict address_len)
{
    
    int index = sockfd;
    key_t key1;
    key1 = ftok(".",'a');
    int shmid1 = shmget(key1, MAX_SOCKETS*sizeof(struct Socket), IPC_CREAT | 0777);
    struct Socket *SM;
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    
    
    int i = 0;
    for (i = 0; i < 5; i++)
    {
        if (SM[index].recvbuf[i] != NULL)
        {
            strcpy(buf, SM[index].recvbuf[i]);
            memset(SM[index].recvbuf[i], 0, sizeof(SM[index].recvbuf[i]));
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
    key1 = ftok(".",'a');
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
        memset(SM[index].sendbuf[i], 0, sizeof(SM[index].sendbuf[i]));
    }
    for (i = 0; i < 5; i++)
    {
        memset(SM[index].recvbuf[i], 0, sizeof(SM[index].recvbuf[i]));
    }
    return close(sockfd);
}
