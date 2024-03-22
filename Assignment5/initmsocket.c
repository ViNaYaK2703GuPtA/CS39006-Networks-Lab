#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/select.h>
#include "msocket.h"

int sem_mtx;

#define MAX_SOCKETS 25

void *R(void *arg)
{
    // Get the shared memory segment containing socket information
    key_t key = ftok(".", 'b');
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0777 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (Sinfo == (struct SOCK_INFO *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    struct Socket *SM;
    key_t key1;
    key1 = ftok(".", 'a');

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);
    fd_set readfds;
    printf("\t\t\tReceiver process started\n");

    int last_inorder_seq_no = -1;
    // Loop to wait for incoming messages
    while (1)
    {

        int maxfd = 0;

        // Initialize the file descriptor set
        FD_ZERO(&readfds);

        for (int i = 0; i < MAX_SOCKETS; i++)
        {
            if (SM[i].free == 1)
            {
                printf("sock_id for FDSET: %d\n", SM[i].sock_id);
                FD_SET(SM[i].sock_id, &readfds);
                if (SM[i].sock_id > maxfd)
                {
                    maxfd = SM[i].sock_id;
                }
            }
        }

        int ns = -1;       // nospace flag, three values: 1, 0, -1

        // Set the timeout for select() to NULL for now
        struct timeval timeout;
        timeout.tv_sec = 10; // check
        timeout.tv_usec = 0;

        // Call select() to check for incoming messages
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity == -1)
        {
            perror("select failed");
            exit(1);
        }

        if (activity == 0)
        {
            // on timeout check whether a new MTP socket has been created and include it in the read/write set accordingly
            // for (int i = 0; i < MAX_SOCKETS; i++)
            // {
            //     if (SM[i].free == 1)
            //     {
            //         //printf("sock_id: %d\n", SM[i].sock_id);
            //         FD_SET(SM[i].sock_id, &readfds);
            //         if (SM[i].sock_id > maxfd)
            //         {
            //             maxfd = SM[i].sock_id;
            //         }
            //     }
            // }
        }
        else
        {
            for (int i = 0; i < MAX_SOCKETS; i++)
            {

                if (SM[i].free == 1 && FD_ISSET(SM[i].sock_id, &readfds))
                {
                    struct sockaddr_in dest_addr;
                    socklen_t len = sizeof(dest_addr);
                    char buffer[1024];
                    int n = recvfrom(SM[i].sock_id, buffer, 1024, 0, (struct sockaddr *)&dest_addr, &len);

                    if (n == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            continue;
                        }
                        else
                        {
                            perror("recvfrom() failed");
                            exit(1);
                        }
                    }
                    
                    if (buffer[0] == 'A' && buffer[1] == 'C' && buffer[2] == 'K')
                    {
                        char number[2];
                        memset(number, 0, sizeof(number));
                        number[0] = buffer[3];
                        number[1] = '\0';

                        int num = atoi(number);
                        int t = SM[i].sender_window.seq_number[0];
                        SM[i].sendbuf_size[t] = 1;

                        SM[i].notyetack[num] = 0;
                        SM[i].sendbuf_size[num] = 1;
                        SM[i].sender_window.window_size += 1;

                        memset(SM[i].sendbuf[num], 0, sizeof(SM[i].sendbuf[num]));

                        printf("Received ACK for %d\n", num);
                        printf("Sender window size: %d\n", SM[i].sender_window.window_size);
                    }
                    else
                    {
                        int num,j,x=0;
                        for (j = 0; j < SM[i].receiver_window.window_size; j++)
                        {
                            int temp = SM[i].receiver_window.seq_number[j];
                            if (SM[i].recvbuf_size[temp] == 1)
                            {
                                printf("S.no: %d\n", SM[i].receiver_window.seq_number[j]);
                                ns = 0;
                                int next = SM[i].receiver_window.seq_number[j];
                                last_inorder_seq_no = SM[i].receiver_window.seq_number[j];
                                SM[i].recvbuf_size[next] = 0;

                                char message[1024];
                                memset(message, 0, sizeof(message));
                                char number[2];
                                memset(number, 0, sizeof(number));

                                number[0]=buffer[3];
                                number[1]='\0';
                                num = atoi(number);

                                strcpy(message, buffer + 5);
                                // message[strlen(buffer) - 5] = '\0';   ---check this
                                strcpy(SM[i].recvbuf[next], message);
                                SM[i].receiver_window.window_size--;

                                printf("Window Size(Receiver) = %d\n", SM[i].receiver_window.window_size);
                                break;
                            }
                        }

                        if(ns == -1)
                        {
                            ns = 1;
                        }
                        else
                        {
                            // Space available in buffer and message received, now sending an ACK
                            char acknowledge[1024];
                            memset(acknowledge, 0, sizeof(acknowledge));
                            sprintf(acknowledge, "ACK%d, rwnd size = %d", num, SM[i].receiver_window.window_size);
                            sendto(SM[i].sock_id, acknowledge, strlen(acknowledge), 0, (struct sockaddr *)&dest_addr, &len);
                            sleep(1);
                        }
                    }
                }
            }
        }
    }

    // Detach the shared memory segment
    if (shmdt(Sinfo) == -1)
    {
        perror("shmdt failed");
        exit(1);
    }

    if(shmdt(SM) == -1){
        perror("shmdt failed");
        exit(1);
    }

    return NULL;
}

void *S(void *arg)
{

    // Get the shared memory segment containing socket information
    key_t key = ftok(".", 'b');
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0777 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (Sinfo == (struct SOCK_INFO *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    struct Socket *SM;
    int i;
    key_t key1;
    key1 = ftok(".", 'a');

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    printf("\t\t\tSender process started\n");
    // Loop to sleep for some time and wake up periodically
    while (1)
    {
        // Sleep for some time (less than T/2)
        usleep(2500000); // Sleep for 2.5 seconds (2500 milliseconds)

        // Get the current time for checking message timeout period
        struct timeval current_time;
        gettimeofday(&current_time, NULL);

        // Check whether the message timeout period (T) is over for messages sent over any active MTP sockets
        for (i = 0; i < MAX_SOCKETS; i++)
        {
            wait_sem(sem_mtx);
            if (SM[i].free)
            {
                // Compute the time difference between the current time and the time when the messages within the window were sent last
                time_t time_diff = current_time.tv_sec - SM[i].sender_window.send_time;

                // If the timeout period (T) is over, retransmit all messages within the current swnd for that MTP socket
                // if (time_diff >= 5)
                // {
                //     int j;
                //     for (j = 0; j < 5; j++)
                //     {
                //         if (SM[i].sender_window.seq_number[j] != -1)
                //         {
                //             struct sockaddr_in destaddr;
                //             destaddr.sin_family = AF_INET;
                //             destaddr.sin_port = htons(SM[i].destport);
                //             int err = inet_aton(SM[i].destip, &destaddr.sin_addr);

                //             // Retransmit the message

                //             ssize_t send_len = sendto(SM[i].sock_id, SM[i].sendbuf[j], strlen(SM[i].sendbuf[j]), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
                //             if (send_len == -1)
                //             {
                //                 perror("sendto failed 1");
                //                 exit(1);
                //             }
                //         }
                //     }
                //     // Update the send timestamp
                //     gettimeofday(&current_time, NULL);
                //     SM[i].sender_window.send_time = current_time.tv_sec;
                // }
            }
            signal_sem(sem_mtx);
        }

        // Check the current swnd for each MTP socket

        for (i = 0; i < MAX_SOCKETS; i++)
        {
            // printf("\t\t\tGoing into wait state\n");
            // wait_sem(sem_mtx);
            // printf("\t\t\tOut of wait stat\n");
            if (SM[i].free == 1)
            {

                if (SM[i].sender_window.window_size > 0)
                {
                    int j;
                    for (j = 0; j < 10; j++)
                    {
                        if (strcmp(SM[i].sendbuf[j], "") != 0)
                        {
                            struct sockaddr_in destaddr;
                            destaddr.sin_family = AF_INET;
                            destaddr.sin_port = htons(SM[i].destport);
                            int err = inet_aton(SM[i].destip, &destaddr.sin_addr);

                            // Send the pending message through the UDP sendto() call for the corresponding UDP socket
                            // printf("%d\n", SM[i].sock_id);
                            printf("Sending message: %s\n", SM[i].sendbuf[j]);
                            printf("Sending to: %s:%d\n", SM[i].destip, SM[i].destport);
                            ssize_t send_len = sendto(SM[i].sock_id, SM[i].sendbuf[j], strlen(SM[i].sendbuf[j]), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
                            if (send_len == -1)
                            {
                                perror("sendto failed 2");
                                exit(1);
                            }
                            // Update the send timestamp
                            gettimeofday(&current_time, NULL);
                            SM[i].sender_window.send_time = current_time.tv_sec;

                            // Remove the sent message from the sender-side message buffer
                            // free(SM[i].sendbuf[j]);
                            memset(SM[i].sendbuf[j], 0, sizeof(SM[i].sendbuf[j]));
                            SM[i].sender_window.window_size--;
                            break; // Exit loop after sending one message
                        }
                    }
                }
            }
            // signal_sem(sem_mtx);
        }
    }

    // Detach the shared memory segment
    if (shmdt(Sinfo) == -1)
    {
        perror("shmdt failed");
        exit(1);
    }

    if(shmdt(SM) == -1){
        perror("shmdt failed");
        exit(1);
    }

    return NULL;
}

void *G(void *arg)
{
    key_t key;
    key = ftok(".", 'b');
    // create a shared memory segment
    int shmid = shmget(key, sizeof(struct SOCK_INFO), IPC_CREAT | 0777);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    // attach the shared memory segment
    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);
    if (Sinfo == (struct SOCK_INFO *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    // initialize the shared memory
    Sinfo->sock_id = 0;
    Sinfo->err_no = 0;
    Sinfo->port = 0;
    memset(Sinfo->ip, 0, sizeof(Sinfo->ip));

    // use the shared memory
    struct Socket *SM;
    int i;
    key_t key1;
    key1 = ftok(".", 'a');

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    // Loop to clean up the corresponding entry in the MTP socket if the corresponding process is killed and the socket has not been closed explicitly
    while (1)
    {
        // wait_sem(sem_mtx);
        // Check if the corresponding process is killed and the socket has not been closed explicitly
        int i;
        for (i = 0; i < MAX_SOCKETS; i++)
        {
            if (SM[i].free == 1)
            {
                int status;
                int result = waitpid(SM[i].pid, &status, NULL);
                if (result == SM[i].pid)
                {
                    // The corresponding process is killed
                    // Clean up the corresponding entry in the MTP socket
                    close(SM[i].sock_id);
                    SM[i].free = 0;
                    SM[i].pid = 0;
                    SM[i].sock_id = 0;
                    SM[i].destport = 0;
                    memset(SM[i].destip, 0, sizeof(SM[i].destip));
                    memset(SM[i].sendbuf, 0, sizeof(SM[i].sendbuf));
                    memset(SM[i].recvbuf, 0, sizeof(SM[i].recvbuf));
                    memset(SM[i].sendbuf_size, 1, sizeof(SM[i].sendbuf_size)); // if empty, 1 else 0
                    memset(SM[i].recvbuf_size, 1, sizeof(SM[i].recvbuf_size));
                    memset(SM[i].sender_window.seq_number, 0, sizeof(SM[i].sender_window.seq_number));
                    memset(SM[i].receiver_window.seq_number, 0, sizeof(SM[i].receiver_window.seq_number));
                }
            }
        }
        // signal_sem(sem_mtx);
    }

    return NULL;
}

int main()
{

    key_t key;
    key = ftok(".", 'b');
    // create a shared memory segment
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0777 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(1);
    }

    // attach the shared memory segment
    struct SOCK_INFO *Sinfo = (struct SOCK_INFO *)shmat(shmid, NULL, 0);

    if (Sinfo == (struct SOCK_INFO *)-1)
    {
        perror("shmat failed");
        exit(1);
    }

    // initialize the shared memory
    Sinfo->sock_id = 0;
    Sinfo->err_no = 0;
    Sinfo->port = 0;
    memset(Sinfo->ip, 0, sizeof(Sinfo->ip));

    // use the shared memory
    struct Socket *SM;
    int i;
    key_t key1;
    key1 = ftok(".", 'a');

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    for (i = 0; i < MAX_SOCKETS; i++)
    {
        if (SM == (struct Socket *)-1)
        {
            perror("shmat failed");
            exit(1);
        }
        // initialize the shared memory for each socket
        SM[i].free = 0;
        SM[i].pid = 0;
        SM[i].sock_id = 0;
        SM[i].destport = 0;

        memset(SM[i].destip, 0, sizeof(SM[i].destip));
        memset(SM[i].sendbuf, 0, sizeof(SM[i].sendbuf));
        memset(SM[i].recvbuf, 0, sizeof(SM[i].recvbuf));

        for (int j = 0; j < 10; j++)
        {
            SM[i].sendbuf_size[j] = 0;
            SM[i].notyetack[j] = 0;
        }
        for (int j = 0; j < 5; j++)
        {
            SM[i].recvbuf_size[j] = 0;
        }
        for (int j = 0; j < 10; j++)
        {
            SM[i].sender_window.seq_number[j] = j; // max window size = 10 and buffer size also 10? change it in future
        }
        for (int j = 0; j < 5; j++)
        {
            SM[i].receiver_window.seq_number[j] = j;
        }
    }

    key_t semkeyA = ftok(".", 'A');
    int Sem1;
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0777);
    semctl(Sem1, 0, SETVAL, 0);

    key_t semkeyB = ftok(".", 'B');
    int Sem2;
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0777);
    semctl(Sem2, 0, SETVAL, 0);

    semctl(sem_mtx, 0, SETVAL, 0);
    // use the shared memory for the sockets
    pthread_t sender, receiver, g_collector;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&receiver, &attr, R, NULL);
    pthread_create(&sender, &attr, S, NULL);
    // pthread_create(&g_collector, &attr, G, NULL);

    while (1)
    {
        wait_sem(Sem1);

        // check SOCK_INFO whether all fields are 0
        if (Sinfo->sock_id == 0 && Sinfo->err_no == 0 && Sinfo->port == 0)
        {
            int sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            Sinfo->sock_id = sock_id;
            if (sock_id < 0)
            {
                Sinfo->err_no = errno;
            }

            signal_sem(Sem2);
        }
        else
        {
            int err;
            struct sockaddr_in servaddr;
            int sockfd = Sinfo->sock_id;
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(Sinfo->port);
            servaddr.sin_addr.s_addr = inet_addr(Sinfo->ip);

            if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                Sinfo->sock_id = -1;
                Sinfo->err_no = errno;
            }
            signal_sem(Sem2);
        }
    }

    // detach the shared memory segments
    for (i = 0; i < MAX_SOCKETS; i++)
    {
        if (shmdt(SM) == -1)
        {
            perror("shmdt failed");
            exit(1);
        }
    }

    // remove the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl failed");
        exit(1);
    }

    // detach the shared memory segment
    if (shmdt(Sinfo) == -1)
    {
        perror("shmdt failed");
        exit(1);
    }

    // remove the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl failed");
        exit(1);
    }

    return 0;
}
