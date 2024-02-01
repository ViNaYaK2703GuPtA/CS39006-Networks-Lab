
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
        else
        {
            // In response server sends 220 <domain> Service ready
            // use a function to get domain name
            char domain[100];
            gethostname(domain, sizeof(domain));
            sprintf(buf, "220 %s Service ready\r\n", domain);
            send(newsockfd, buf, strlen(buf) + 1, 0); // check
        }

        if (fork() == 0)
        {

            close(sockfd); 

            printf("Server is ready!\n");
            char *response;
            response = (char *)malloc(5000 * sizeof(char));
            char *username;
            username = (char *)malloc(100 * sizeof(char));
            char *extra;
            extra = (char *)malloc(100 * sizeof(char));
            int flag = 0;
            memset(username, 0, sizeof(username));

            // for command HELO
            memset(buf, 0, sizeof(buf));
            recv(newsockfd, buf, sizeof(buf), 0);
            printf("C: %s\n", buf);

            char domain_name[100];
            gethostname(domain_name, sizeof(domain_name));
            sprintf(response, "250 OK Hello %s\r\n", domain_name);
            send(newsockfd, response, strlen(response) + 1, 0);

            // for command MAIL FROM:
            memset(buf, 0, sizeof(buf));
            recv(newsockfd, buf, sizeof(buf), 0);
            printf("C: %s\n", buf);

            char *ptr = strstr(buf, "MAIL FROM:");
            if (ptr != NULL)
            {
                sprintf(response, "250 %s... Sender ok\r\n", ptr + 10);
                send(newsockfd, response, strlen(response) + 1, 0);
            }

            // for command RCPT TO:
            memset(buf, 0, sizeof(buf));
            recv(newsockfd, buf, sizeof(buf), 0);
            printf("C: %s\n", buf);

            sscanf(buf, "RCPT TO: %100[^@]@%100[^\n]%s", username, extra, extra);

            if (username != NULL && (strcmp(username, "ishaan26") == 0 || strcmp(username, "vinayak27") == 0))
            {
                sprintf(response, "250 root... Recipient ok\r\n");
                send(newsockfd, response, strlen(response) + 1, 0);
            }
            else
            {
                sprintf(response, "550 No such user here\r\n");
                send(newsockfd, response, strlen(response), 0);
            }

            // for command DATA
            memset(buf, 0, sizeof(buf));
            recv(newsockfd, buf, sizeof(buf), 0);
            printf("C: %s\n", buf);

            // copy 354 Start mail input; end with <CRLF>.<CRLF> to response
            sprintf(response, "354 Enter mail, end with \".\" on a line by itself\r\n");
            send(newsockfd, response, strlen(response) + 1, 0);

            char filepath[100];
            memset(filepath, 0, sizeof(filepath));
            strcat(filepath, "./");
            strcat(filepath, username);
            strcat(filepath, "/mymailbox.txt");

            FILE *file = fopen(filepath, "a");
            if (file == NULL)
            {
                printf("Unable to open mailbox file\n");
                return 0;
            }

            while (1)
            {
                /* We again initialize the buffer, and receive a
                   message from the client.
                */
                memset(buf, 0, sizeof(buf));
                recv(newsockfd, buf, sizeof(buf), 0);
                printf("C: %s\n", buf);
                memset(response, 0, sizeof(response));
                strcpy(response, "#");
                send(newsockfd, response, strlen(response) + 1, 0);

                // Get current time
                time_t now = time(NULL);
                struct tm *timeinfo = localtime(&now);
                char time_str[20];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", timeinfo);

                // Append message to mailbox file line by line
                
                if (strncmp(buf, "Subject:", 8) == 0)
                {
                    fprintf(file, "%s\n", buf);
                    fprintf(file, "Received: %s\n", time_str);
                }
                else
                {
                    fprintf(file, "%s\n", buf);
                }

                if (strcmp(buf, ".") == 0)
                {
                    fclose(file);
                    sprintf(response, "250 OK Message accepted for delivery\r\n");
                    send(newsockfd, response, strlen(response) + 1, 0);
                    break;
                }
                
                
            }
           

            // For Command QUIT
            memset(buf, 0, sizeof(buf));
            recv(newsockfd, buf, sizeof(buf), 0);
            printf("C: %s\n", buf);

            // copy 221 <domain> Service closing transmission channel to response
            char domain__name[100];
            gethostname(domain__name, sizeof(domain__name));
            sprintf(response, "221 %s Service closing transmission channel\r\n", domain__name);
            send(newsockfd, response, strlen(response), 0);

            close(newsockfd);
            exit(0);
        }

        close(newsockfd);
    }
    return 0;
}
