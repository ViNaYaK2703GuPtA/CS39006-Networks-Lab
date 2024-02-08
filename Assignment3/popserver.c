/*
            NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* THE SERVER PROCESS */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <my_port>\n", argv[0]);
        exit(0);
    }
    int sockfd, newsockfd; /* Socket descriptors */
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;

    int i;
    char buf[100]; /* We will use this buffer for communication */

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Cannot create socket\n");
        exit(0);
    }

    int my_port = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(my_port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        printf("Unable to bind local address\n");
        exit(0);
    }

    listen(sockfd, 10);
    while (1)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                           &clilen);

        if (newsockfd < 0)
        {
            printf("Accept error\n");
            exit(0);
        }

        if (fork() == 0)
        {

            close(sockfd); /* Close the old socket since all
                      communications will be through
                      the new socket.
                   */

            /* We initialize the buffer, copy the message to it,
               and send the message to the client.
            */

            strcpy(buf, "+OK POP3 server ready\r\n");
            send(newsockfd, buf, strlen(buf) + 1, 0);

            // receiving USER <username> from client
            memset(buf, 0, 100);
            recv(newsockfd, buf, 100, 0);

            char username[100];
            memset(username, 0, 100);
            sscanf(buf, "USER %s", username);
            if (strcmp(username, "ishaan26") == 0 || strcmp(username, "vinayak27") == 0)
            {
                sprintf(buf, "+OK %s is a valid mailbox\r\n", username);
                send(newsockfd, buf, strlen(buf) + 1, 0);
            }
            else
            {
                sprintf(buf, "-ERR never heard of mailbox %s\r\n", username);
                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            // count the number of messages in the mailbox of vinayak27 and ishaan26 by checking the number of \n.\r\n in the file
            char filepath[100];
            memset(filepath, 0, sizeof(filepath));
            strcat(filepath, "./");
            strcat(filepath, username);
            strcat(filepath, "/mymailbox.txt");

            FILE *file = fopen(filepath, "w");
            if (file == NULL)
            {
                printf("Unable to open mailbox file\n");
                return 0;
            }
            int count = 0;
            char temp[5000];
            while (fgets(temp, 5000, file) != NULL)
            {
                if (strcmp(temp, ".\r\n") == 0)
                {
                    count++;
                }
            }

            // receiving PASS <password> from client
            memset(buf, 0, 100);
            recv(newsockfd, buf, 100, 0);
            // the received message is PASS <password> now extract password from it and compare with iitkgp123 if username is vinayak27 and with vmcsje456 if username is ishaan26
            char password[100];
            memset(password, 0, 100);
            sscanf(buf, "PASS %s", password);
            if (strcmp(username, "vinayak27") == 0)
            {
                if (strcmp(password, "iitkgp123") == 0)
                {
                    sprintf(buf, "+OK %s's maildrop has %d messages\r\n", username, count);
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                }
                else
                {
                    sprintf(buf, "-ERR invalid password\r\n");
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                    close(newsockfd);
                    exit(0);
                }
            }
            else if (strcmp(username, "ishaan26") == 0)
            {
                if (strcmp(password, "vmcsje456") == 0)
                {
                    sprintf(buf, "+OK %s's maildrop has %d messages\r\n", username, count);
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                }
                else
                {
                    sprintf(buf, "-ERR invalid password\r\n");
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                    close(newsockfd);
                    exit(0);
                }
            }

            // receiving STAT command from client
            memset(buf, 0, 100);
            recv(newsockfd, buf, 100, 0);

            // In response to the STAT command, the server sends a line of the form +OK nn mm, where nn is the number of messages in the maildrop and mm is the total number of octets in the maildrop.
            memset(buf, 0, 100);
            fseek(file, 0, SEEK_END); // seek to end of file
            int size = ftell(file);   // get current file pointer
            fseek(file, 0, SEEK_SET); // seek back to beginning of file
            sprintf(buf, "+OK %d %d\r\n", count, size);
            send(newsockfd, buf, strlen(buf) + 1, 0);

            // declaring an array named delete of size count+1 and initializing all elements to 0
            int delete[count + 1];
            for (int i = 0; i < count + 1; i++)
            {
                delete[i] = 0;
            }

            // file pointer file is currently at the end of the file, so we need to seek to the beginning of the file
            fseek(file, 0, SEEK_SET);

            while (1)
            {
                memset(buf, 0, 100);
                recv(newsockfd, buf, 100, 0);
                if (strcmp(buf, "QUIT\r\n") == 0)
                {
                    sprintf(buf, "+OK POP3 server signing off\r\n");
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                    close(newsockfd);
                    exit(0);
                }
                else if (strcmp(buf, "LIST\r\n") == 0)
                {
                    char str[100];
                    memset(str, 0, sizeof(str));

                    int sno = 1;
                    char sender[50], received[50], subject[100];

                    memset(sender, 0, sizeof(sender));
                    memset(received, 0, sizeof(received));
                    memset(subject, 0, sizeof(subject));

                    while (fgets(str, sizeof(str), file) != NULL)
                    {
                        fflush(stdin);
                        fflush(stdout);

                        if (strncmp(str, "From: ", 6) == 0)
                        {
                            sscanf(str + 6, "%49[^\n]", sender);
                        }
                        else if (strncmp(str, "Subject: ", 9) == 0)
                        {
                            sscanf(str + 9, "%99[^\n]", subject);
                        }
                        else if (strncmp(str, "Received: ", 10) == 0)
                        {
                            sscanf(str + 10, "%49[^\n]", received);
                            fflush(stdout);

                            // concatenate sno sender received subject and send it to the client
                            memset(buf, 0, sizeof(buf));
                            sprintf(buf, "%d %s %s %s\r\n", sno, sender, received, subject);
                            send(newsockfd, buf, strlen(buf) + 1, 0);

                            memset(sender, 0, sizeof(sender));
                            memset(received, 0, sizeof(received));
                            memset(subject, 0, sizeof(subject));
                            memset(str, 0, sizeof(str));
                            sno++;
                        }
                        else
                            continue;
                    }
                }
                else if (strncmp(buf, "RETR ", 5) == 0)
                {
                    // In response to the RETR command with a message-number argument, the server sends the message corresponding to the given message-number, being careful to byte-stuff the termination character (as with all multi-line responses).
                    int msgno;
                    memset(buf, 0, 100);
                    sscanf(buf, "RETR %d", &msgno);

                    // sending +OK message to client
                    memset(buf, 0, 100);
                    sprintf(buf, "+OK\r\n");
                    send(newsockfd, buf, strlen(buf) + 1, 0);

                    fseek(file, 0, SEEK_SET);
                    char str[100];
                    memset(str, 0, sizeof(str));
                    int sno = 1;
                    while (fgets(str, sizeof(str), file) != NULL)
                    {
                        if (strcmp(str, ".\n") == 0)
                        {
                            send(newsockfd, str, strlen(str) + 1, 0);
                            sno++;
                            if (sno == msgno + 1)
                                break;
                        }
                        if (sno == msgno)
                        {
                            // replace \n in str with \r\n and send it to the client
                            for (int i = 0; i < strlen(str); i++)
                            {
                                if (str[i] == '\n')
                                {
                                    str[i] = '\r';
                                    str[i + 1] = '\n';
                                    break;
                                }
                            }
                            send(newsockfd, str, strlen(str) + 1, 0);
                        }
                    }
                }
                else if (strncmp(buf, "DELE ", 5) == 0)
                {
                    int msgno;
                    memset(buf, 0, 100);
                    sscanf(buf, "DELE %d", &msgno);
                    if (delete[msgno] == -1)
                    {
                        sprintf(buf, "-ERR message %d already deleted\r\n", msgno);
                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }
                    else
                    {
                        delete[msgno] = -1;
                        memset(buf, 0, 100);
                        sprintf(buf, "+OK message %d deleted\r\n", msgno);
                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }
                }
                else
                {
                    sprintf(buf, "-ERR invalid command\r\n");
                    send(newsockfd, buf, strlen(buf) + 1, 0);
                }
            }

            // close(newsockfd);
            // exit(0);
        }

        close(newsockfd);
    }
    return 0;
}
