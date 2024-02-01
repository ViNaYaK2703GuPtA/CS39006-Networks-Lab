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
#include <time.h>

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
    char buf[5000]; /* We will use this buffer for communication */

    /* The following system call opens a socket. The first parameter
       indicates the family of the protocol to be followed. For internet
       protocols we use AF_INET. For TCP sockets the second parameter
       is SOCK_STREAM. The third parameter is set to 0 for user
       applications.
    */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Cannot create socket\n");
        exit(0);
    }

    int my_port = atoi(argv[1]);

    /* The structure "sockaddr_in" is defined in <netinet/in.h> for the
       internet family of protocols. This has three main fields. The
       field "sin_family" specifies the family and is therefore AF_INET
       for the internet family. The field "sin_addr" specifies the
       internet address of the server. This field is set to INADDR_ANY
       for machines having a single IP address. The field "sin_port"
       specifies the port number of the server.
    */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(my_port);

    /* With the information provided in serv_addr, we associate the server
       with its port using the bind() system call.
    */
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    {
        printf("Unable to bind local address\n");
        exit(0);
    }

    listen(sockfd, 10); /* This specifies that up to 5 concurrent client
                  requests will be queued up while the system is
                  executing the "accept" system call below.
               */

    /* In this program we are illustrating a concurrent server -- one
       which forks to accept multiple client connections concurrently.
       As soon as the server accepts a connection from a client, it
       forks a child which communicates with the client, while the
       parent becomes free to accept a new connection. To facilitate
       this, the accept() system call returns a new socket descriptor
       which can be used by the child. The parent continues with the
       original socket descriptor.
    */
    while (1)
    {

        /* The accept() system call accepts a client connection.
           It blocks the server until a client request comes.

           The accept() system call fills up the client's details
           in a struct sockaddr which is passed as a parameter.
           The length of the structure is noted in clilen. Note
           that the new socket descriptor returned by the accept()
           system call is stored in "newsockfd".
        */
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                           &clilen);

        if (newsockfd < 0)
        {
            printf("Accept error\n");
            exit(0);
        }

        /* Having successfully accepted a client connection, the
           server now forks. The parent closes the new socket
           descriptor and loops back to accept the next connection.
        */
        if (fork() == 0)
        {

            /* This child process will now communicate with the
               client through the send() and recv() system calls.
            */
            close(sockfd); /* Close the old socket since all
                      communications will be through
                      the new socket.
                   */

            /* We initialize the buffer, copy the message to it,
               and send the message to the client.
            */

            // strcpy(buf,"Server is ready!");
            // send(newsockfd, buf, strlen(buf) + 1, 0);
            printf("Server is ready!\n");
            char *response;
            response = (char *)malloc(5000 * sizeof(char));
            char *username;
            username = (char *)malloc(100 * sizeof(char));
            int flag = 0;

            while (1)
            {
                /* We again initialize the buffer, and receive a
                   message from the client.
                */
                memset(buf, 0, sizeof(buf));
                recv(newsockfd, buf, 5000, 0);
                memset(response, 0, sizeof(response));
                memset(username, 0, sizeof(username));

                if (flag == 1)
                {
                    // if (username != NULL && (strcmp(username, "ishaan26") == 0 || strcmp(username, "vinayak27") == 0))
                    // {
                        // Append message to mailbox file
                        char filepath[100];
                        sprintf(filepath, "./%s/mymailbox.txt", username);

                        FILE *file = fopen(filepath, "a");
                        if (file == NULL)
                        {
                            printf("Unable to open mailbox file\n");
                            return;
                        }

                        // Get current time
                        time_t now = time(NULL);
                        struct tm *timeinfo = localtime(&now);
                        char time_str[20];
                        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);

                        // Append message to mailbox file line by line
                        char *token = strtok(buf, "\n");
                        while (token != NULL)
                        {
                            if (strncmp(token, "Subject:", 8) == 0)
                            {
                                fprintf(file, "%s\n", token);
                                fprintf(file, "Received: %s\n", time_str);
                            }
                            else
                            {
                                fprintf(file, "%s\n", token);
                            }
                            token = strtok(NULL, "\n");
                        }

                        fclose(file);
                    // }
                    // else
                    // {
                    //     sprintf(response, "550 No such user here\n");
                    //     send(newsockfd, response, strlen(response), 0);
                    // }
                    flag = 0;
                    sprintf(response, "250 OK Message accepted for delivery\n");
                    send(newsockfd, response, strlen(response)+1, 0);
                    continue;
                }

                if (strcmp(buf, "client connects to SMTP port") == 0)
                {
                    // In response server sends 220 <domain> Service ready
                    // use a function to get domain name
                    char domain[100];
                    gethostname(domain, sizeof(domain));
                    sprintf(buf, "220 %s Service ready\n", domain);
                    send(newsockfd, buf, strlen(buf) + 1, 0); // check
                }

                else if (strncmp(buf, "HELO", 4) == 0)
                {
                    // copy 250 OK Hello <domain> to response
                    char domain_name[100];
                    gethostname(domain_name, sizeof(domain_name));
                    sprintf(response, "250 OK Hello %s\n", domain_name);
                    send(newsockfd, response, strlen(response) + 1, 0);
                }
                else if (strncmp(buf, "MAIL FROM:", 10) == 0)
                {
                    // copy 250 and afterwards the content of buf after MAIL FROM:  to response
                    char *ptr = strstr(buf, "MAIL FROM:");
                    if (ptr != NULL)
                    {
                        sprintf(response, "250 %s... Sender ok\n", ptr + 10);
                        send(newsockfd, response, strlen(response) + 1, 0);
                    }
                }
                else if (strncmp(buf, "RCPT TO:", 8) == 0)
                {
                    // store the username which is after RCPT TO: in username
                    char *ptr = strstr(buf, "RCPT TO:");
                    strcpy(username, ptr + 8);

                    if (username != NULL && (strcmp(username, "ishaan26") == 0 || strcmp(username, "vinayak27") == 0))
                    {
                        sprintf(response, "250 root... Recipient ok\n", username);
                        send(newsockfd, response, strlen(response) + 1, 0);
                    }
                    else
                    {
                        sprintf(response, "550 No such user here\n");
                        send(newsockfd, response, strlen(response), 0);
                        break;
                    }
                }
                else if (strncmp(buf, "DATA", 4) == 0)
                {
                    // copy 354 Start mail input; end with <CRLF>.<CRLF> to response
                    sprintf(response, "354 Enter mail, end with \".\" on a line by itself\n");
                    send(newsockfd, response, strlen(response), 0);
                    flag = 1;
                }
                else if (strncmp(buf, "QUIT", 4) == 0)
                {
                    // copy 221 <domain> Service closing transmission channel to response
                    char domain_name[100];
                    gethostname(domain_name, sizeof(domain_name));
                    sprintf(response, "221 %s Service closing transmission channel\n", domain_name);
                    send(newsockfd, response, strlen(response), 0);
                    break;
                }

                // char *token = strtok(buf, "\n");
                // while (token != NULL)
                // {
                //     if (strncmp(token, "To:", 3) == 0)
                //     {
                //         // Extract username from "To: <username>"
                //         char *start = strchr(token, '<');
                //         char *end = strchr(token, '>');
                //         if (start != NULL && end != NULL)
                //         {
                //             username = malloc(end - start);
                //             strncpy(username, start + 1, end - start - 1);
                //             username[end - start - 1] = '\0';
                //             break;
                //         }
                //     }
                //     token = strtok(NULL, "\n");
                // }
            }
            //     char filepath[100];
            //     sprintf(filepath, "./%s/mymailbox.txt", username);

            //     FILE* file = fopen(filepath, "a");
            //     if (file == NULL) {
            //     printf("Unable to open mailbox file\n");
            //     return;
            // }

            //  // Get current time
            // time_t now = time(NULL);
            // struct tm* timeinfo = localtime(&now);
            // char time_str[20];
            // strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);

            // // Append message to mailbox file line by line
            // char* token = strtok(buf, "\n");
            // while (token != NULL) {
            //     if (strncmp(token, "Subject:", 8) == 0) {
            //         fprintf(file, "%s\n", token);
            //         fprintf(file, "Received: %s\n", time_str);
            //     } else {
            //         fprintf(file, "%s\n", token);
            //     }
            //     token = strtok(NULL, "\n");
            // }

            // fclose(file);

            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }
    return 0;
}
