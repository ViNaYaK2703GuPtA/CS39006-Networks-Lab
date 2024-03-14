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
#include <sys/shm.h>
#include <pthread.h>
#include <sys/select.h>
#include "msocket.h"

int  sem_mtx;

// create a shared memory chunk SM containing the information about 25 MTP sockets(using shmget)
// struct SOCK_INFO
// {
//     int sock_id;
//     int err_no;
//     int port;
//     char *ip;
// };

// typedef struct swnd
// {
//     int seq_number[5];
//     int window_size;
//     time_t send_time;
// } swnd;

// typedef struct rwnd
// {
//     int seq_number[5];
//     int window_size;
//     time_t recv_time;
// } rwnd;

// struct Socket
// {
//     int free;
//     int pid;
//     int sock_id;
//     char *destip;
//     int destport;
//     char *sendbuf[10];
//     char *recvbuf[5];
//     swnd sender_window;
//     rwnd receiver_window;
// };

#define MAX_SOCKETS 25

/*The  thread  R  behaves  in  the  following  manner.  It  waits  for  a  message  to  come  in  a
recvfrom() call from any of the UDP sockets (you need to use select() to keep on checking
whether there is any incoming message on any of the UDP sockets, on timeout check whether
a new MTP socket has been created and include it in the read/write set accordingly). When it
receives a message, if it is a data message, it stores it in the receiver-side message buffer for
the  corresponding  MTP  socket  (by  searching  SM  with  the  IP/Port),  and  sends  an  ACK
message  to  the  sender.  In  addition  it  also  sets  a  flag  nospace  if  the  available  space  at  the
receive  buffer  is  zero.  On  a  timeout  over  select(),  it  additionally  checks  whether  the  flag
nospace was set but now there is space available in the receive buffer. In that case, it sends
a duplicate ACK message with the last acknowledged sequence number but with the updated
rwnd size, and resets the flag (there might be a problem here – try to find it out and resolve!).
If  the  received  message  is  an  ACK  message  in  response  to  a  previously  sent  message,  it
updates  the  swnd  and  removes  the  message  from  the  sender-side  message  buffer  for  the
corresponding  MTP  socket.  If  the  received  message  is  a  duplicate  ACK  message,  it  just
updates the swnd size. */
void *R(void *arg)
{
    // Get the shared memory segment containing socket information
    key_t key = 2708;
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0);
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
    key1 = 827;

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    // Create a file descriptor set to hold the UDP sockets
    fd_set readfds;
    int maxfd = 0;
    //int i;

    // Initialize the file descriptor set
    FD_ZERO(&readfds);

    // Add the UDP sockets to the file descriptor set
    for (i = 0; i < MAX_SOCKETS; i++)
    {
        if (SM[i].free == 0)
        {
            FD_SET(SM[i].sock_id, &readfds);
            if (SM[i].sock_id > maxfd)
            {
                maxfd = SM[i].sock_id;
            }
        }
    }

    // Set the timeout for select() to NULL for now
    struct timeval timeout;
    timeout.tv_sec = 5; // check
    timeout.tv_usec = 0;

    // Loop to wait for incoming messages
    while (1)
    {
        // Check if a new MTP socket has been created
        if (Sinfo->sock_id != 0)
        {
            FD_SET(Sinfo->sock_id, &readfds);
            if (Sinfo->sock_id > maxfd)
            {
                maxfd = Sinfo->sock_id;
            }
            Sinfo->sock_id = 0;
        }

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
            for (i = 0; i < MAX_SOCKETS; i++)
            {
                if (SM[i].free == 1)
                {
                    FD_SET(SM[i].sock_id, &readfds);
                    if (SM[i].sock_id > maxfd)
                    {
                        maxfd = SM[i].sock_id;
                    }
                }
            }
        }

        // Check if there is any incoming message on any of the UDP sockets
        for (i = 0; i < MAX_SOCKETS; i++)
        {
            if (FD_ISSET(SM[i].sock_id, &readfds))
            {
                // Receive the message using recvfrom()
                char buffer[1024];
                struct sockaddr_in sender_addr;
                socklen_t sender_len = sizeof(sender_addr);
                ssize_t recv_len = recvfrom(SM[i].sock_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &sender_len);
                if (recv_len == -1)
                {
                    perror("recvfrom failed");
                    exit(1);
                }
                // Store the message in the receiver-side message buffer
                int j;
                for (j = 0; j < 5; j++)
                {
                    if (SM[i].recvbuf[j] == NULL)
                    {
                        strcpy(SM[i].recvbuf[j], buffer);
                        break;
                    }
                }
                // Send an ACK message to the sender
                // char ack[1024];
                // strcpy(ack, "ACK");
                // ssize_t send_len = sendto(SM[i].sock_id, ack, sizeof(ack), 0, (struct sockaddr *)&sender_addr, sender_len);
                // Update the swnd and remove the message from the sender-side message buffer
            }
        }

        // Check if the available space at the receive buffer is zero
        // If so, set the flag nospace

        // Check if the flag nospace was set but now there is space available in the receive buffer
        // If so, send a duplicate ACK message with the last acknowledged sequence number but with the updated rwnd size
        // Reset the flag
    }

    // Detach the shared memory segment
    if (shmdt(Sinfo) == -1)
    {
        perror("shmdt failed");
        exit(1);
    }

    return NULL;
}

/*The thread S behaves in the following manner. It sleeps for some time ( < T/2 ), and wakes
up periodically. On waking up, it first checks whether the message timeout period (T) is over
(by computing the time difference between the current time and the time when the messages
within the window were sent last) for the messages sent over any of the active MTP sockets.
If  yes,  it  retransmits  all  the  messages  within  the  current  swnd  for  that  MTP  socket.  It  then
checks  the  current  swnd  for  each  of  the  MTP  sockets  and  determines  whether  there  is  a
pending message from the sender-side message buffer that can be sent. If so, it sends that
message through the UDP sendto() call for the corresponding UDP socket and updates the
send timestamp.*/
void *S(void *arg)
{

    // Get the shared memory segment containing socket information
    key_t key = 2708;
    int shmid = shmget(key, sizeof(struct SOCK_INFO), 0);
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
    key1 = 827;

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

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
            if (SM[i].free)
            {
                // Compute the time difference between the current time and the time when the messages within the window were sent last
                time_t time_diff = current_time.tv_sec - SM[i].sender_window.send_time;

                // If the timeout period (T) is over, retransmit all messages within the current swnd for that MTP socket
                if (time_diff >= 5)
                {
                    int j;
                    for (j = 0; j < 5; j++)
                    {
                        if (SM[i].sender_window.seq_number[j] != -1)
                        {
                            struct sockaddr_in destaddr;
                            destaddr.sin_family = AF_INET;
                            destaddr.sin_port = htons(SM[i].destport);
                            int err = inet_aton(SM[i].destip, &destaddr.sin_addr);

                            // Retransmit the message
                            ssize_t send_len = sendto(SM[i].sock_id, SM[i].sendbuf[j], strlen(SM[i].sendbuf[j]), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
                            if (send_len == -1)
                            {
                                perror("sendto failed");
                                exit(1);
                            }
                        }
                    }
                    // Update the send timestamp
                    gettimeofday(&current_time, NULL);
                    SM[i].sender_window.send_time = current_time.tv_sec;
                }
            }
        }

        // Check the current swnd for each MTP socket
        for (i = 0; i < MAX_SOCKETS; i++)
        {
            wait_sem(sem_mtx, 0);
            if (SM[i].free == 1)
            {
                // Determine whether there is a pending message from the sender-side message buffer that can be sent
                if (SM[i].sender_window.window_size > 0)
                {
                    int j;
                    for (j = 0; j < 10; j++)
                    {
                        if (SM[i].sendbuf[j] != NULL)
                        {
                            struct sockaddr_in destaddr;
                            destaddr.sin_family = AF_INET;
                            destaddr.sin_port = htons(SM[i].destport);
                            int err = inet_aton(SM[i].destip, &destaddr.sin_addr);

                            // Send the pending message through the UDP sendto() call for the corresponding UDP socket
                            ssize_t send_len = sendto(SM[i].sock_id, SM[i].sendbuf[j], strlen(SM[i].sendbuf[j]), 0, (struct sockaddr *)&destaddr, sizeof(destaddr));
                            if (send_len == -1)
                            {
                                perror("sendto failed");
                                exit(1);
                            }
                            // Update the send timestamp
                            gettimeofday(&current_time, NULL);
                            SM[i].sender_window.send_time = current_time.tv_sec;

                            // Remove the sent message from the sender-side message buffer
                            free(SM[i].sendbuf[j]);
                            SM[i].sendbuf[j] = NULL;
                            SM[i].sender_window.window_size--;
                            break; // Exit loop after sending one message
                        }
                    }
                }
            }
            signal_sem(sem_mtx, 0);
        }
    }

    // Detach the shared memory segment
    if (shmdt(Sinfo) == -1)
    {
        perror("shmdt failed");
        exit(1);
    }

    return NULL;
}

/*The  init  process  should  also  start  a  garbage  collector  process  G  to  clean  up  the
corresponding entry in the MTP socket if the corresponding process is killed and the
socket has not been closed explicitly. */
void *G(void *arg)
{
    key_t key;
    key = 2708;
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
    key1 = 827;

    int shmid1 = shmget(key1, MAX_SOCKETS * sizeof(struct Socket), IPC_CREAT | 0777);
    SM = (struct Socket *)shmat(shmid1, NULL, 0);

    // Loop to clean up the corresponding entry in the MTP socket if the corresponding process is killed and the socket has not been closed explicitly
    while (1)
    {
        wait_sem(sem_mtx, 0);
        // Check if the corresponding process is killed and the socket has not been closed explicitly
        int i;
        for (i = 0; i < MAX_SOCKETS; i++)
        {
            if (SM[i].free == 1)
            {
                int status;
                int result = waitpid(SM[i].pid, &status, NULL);
                if (result == -1)
                {
                    perror("waitpid failed");
                    exit(1);
                }
                else if (result == 0)
                {
                    // The corresponding process is still running
                    continue;
                }
                else
                {
                    // The corresponding process is killed
                    // Clean up the corresponding entry in the MTP socket
                    SM[i].free = 0;
                    SM[i].pid = 0;
                    SM[i].sock_id = 0;
                    SM[i].destip = NULL;
                    SM[i].destport = 0;
                    memset(SM[i].sendbuf, 0, sizeof(SM[i].sendbuf));
                    memset(SM[i].recvbuf, 0, sizeof(SM[i].recvbuf));
                    memset(&SM[i].sender_window.seq_number, 0, sizeof(SM[i].sender_window.seq_number));
                    memset(&SM[i].receiver_window.seq_number, 0, sizeof(SM[i].receiver_window.seq_number));
                    SM[i].sender_window.window_size = 0;
                    SM[i].receiver_window.window_size = 0;
                }
            }
        }
        signal_sem(sem_mtx, 0);
    }

    return NULL;
}

int main()
{


    key_t key;
    key = 2708;
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
    key1 = 827;

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
        SM[i].destip = NULL;
        SM[i].destport = 0;
        memset(SM[i].sendbuf, 0, sizeof(SM[i].sendbuf));
        memset(SM[i].recvbuf, 0, sizeof(SM[i].recvbuf));
        memset(&SM[i].sender_window.seq_number, 0, sizeof(SM[i].sender_window.seq_number));
        memset(&SM[i].receiver_window.seq_number, 0, sizeof(SM[i].receiver_window.seq_number));
        SM[i].sender_window.window_size = 0;
        SM[i].receiver_window.window_size = 0;
    }

    key_t semkeyA = ftok(".", 'A');
    int Sem1;
    Sem1 = semget(semkeyA, 1, IPC_CREAT | 0666);
    semctl(Sem1, 0, SETVAL, 0);

    key_t semkeyB = ftok(".", 'B');
    int Sem2;
    Sem2 = semget(semkeyB, 1, IPC_CREAT | 0666);
    semctl(Sem2, 0, SETVAL, 0);

    // use the shared memory for the sockets
    pthread_t sender, receiver, g_collector;
    pthread_create(&receiver, NULL, R, NULL);
    pthread_create(&sender, NULL, S, NULL);
    pthread_create(&g_collector, NULL, G, NULL);

    while (1)
    {
        wait_sem(Sem1, 0);
        // check SOCK_INFO whether all fields are 0
        if (Sinfo->sock_id == 0 && Sinfo->err_no == 0 && Sinfo->port == 0)
        {
            int sock_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_id < 0)
            {
                Sinfo->sock_id = -1;
                Sinfo->err_no = errno;
            }
            else
            {
                Sinfo->sock_id = sock_id;
                int flag = 0;
                int index = 0;

                for (index = 0; index < 25; index++)
                {

                    if (SM[index].free == 0)
                    {
                        flag = 1;
                        break;
                    }
                }

                if (flag == 0)
                {
                    errno = ENOBUFS;
                    return -1;
                }
                else
                {
                    SM[index].free = 1;
                    SM[index].sock_id = Sinfo->sock_id;
                    SM[index].pid = getpid();
                }
            }
            signal_sem(Sem2, 0);
        }
        else if (Sinfo->sock_id != 0 && Sinfo->port != 0)
        {
            int err;
            struct sockaddr_in servaddr;
            int sockfd = Sinfo->sock_id;
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(Sinfo->port);
            if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                Sinfo->sock_id = -1;
                Sinfo->err_no = errno;
            }
            signal_sem(Sem2, 0);
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
