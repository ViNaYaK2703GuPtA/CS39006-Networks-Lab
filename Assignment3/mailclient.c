
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_LINES 50
#define MAX_CHAR_PER_LINE 80
int checkEmailFormat(char *email) {
    char *lines[MAX_LINES];
    int numLines = 0;

    // Tokenize the email into lines
    char *line = strtok(email, "\n");
    while (line != NULL && numLines < MAX_LINES) {
        lines[numLines++] = line;
        line = strtok(NULL, "\n");
    }

    // Check for the required fields and their format
    if (numLines < 3) {
        printf("Incorrect format: Missing fields\n");
        return 0;
    }

    // Check From field
    int cnt=0, space=0;
    for(int i = 0; i < strlen(lines[0]); i++) {
        
        if(lines[0][i]==' ')
        {
            space++;
            if(space>1)
            {
                printf("Incorrect format: Invalid 'From' field\n");
                return 0;
            }
        }

        if (lines[0][i] == '@') {
            cnt++;
            if (i == 0 || i == strlen(lines[0]) - 1 || cnt>1) {
                printf("Incorrect format: Invalid 'From' field\n");
                return 0;
            }
        }
        // if(cnt==0)
        // {
        //     printf("Incorrect format: Invalid 'From' field\n");
        //     return 0;
        // }
    }

    // Check To field
    cnt=0;space=0;
    for(int i = 0; i < strlen(lines[1]); i++) {
        
        if(lines[1][i]==' ')
        {
            space++;
            if(space>1)
            {
                printf("Incorrect format: Invalid 'To' field\n");
                return 0;
            }
        }
        if (lines[1][i] == '@') {
            cnt++;
            if (i == 0 || i == strlen(lines[1]) - 1 || cnt>1) {
                printf("Incorrect format: Invalid 'To' field\n");
                return 0;
            }
        }
        // if(cnt==0)
        // {
        //     printf("Incorrect format: Invalid 'To' field\n");
        //     return 0;
        // }
    }

    // Check Subject field
    if (sscanf(lines[2], "Subject: %50[^\n]", lines[2]) != 1) {
        printf("Incorrect format: Invalid 'Subject' field\n");
        return 0;
    }

    // Check Message body (if present)
    for (int i = 3; i < numLines; i++) {
        if (strcmp(lines[i], ".") == 0) {
            if (i != numLines - 1) {
                printf("Incorrect format: Message body should be terminated by a line with only a fullstop character\n");
                return 0;
            }
            break; // Valid termination of message body
        }
        if (strlen(lines[i]) > MAX_CHAR_PER_LINE) {
            printf("Incorrect format: Message body line exceeds maximum characters\n");
            return 0;
        }
    }

    return 1; // Format is correct
}


int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Enter 3 command line arguments: <server_IP> <smtp_port> <pop3_port>\n");
        exit(1);
    }

    char username[100], password[100];
    printf("Enter username: ");
    scanf(" %s", username);
    printf("Enter password: ");
    scanf(" %s", password);

    while (1)
    {
        printf("1. Manage Mail\n2. Send Mail\n3. Quit\nEnter your choice: ");
        int choice;
        scanf(" %d", &choice);
        if (choice == 1)
        {
            // Manage Mail
            // 1. Connect to POP3 server
            // 2. Send username and password
            // 3. Receive mails
            // 4. Display mails

            // char filepath[100];
            // memset(filepath, 0, sizeof(filepath));
            // strcat(filepath, "./");
            // strcat(filepath, username);
            // strcat(filepath, "/mymailbox.txt");

            // FILE *file = fopen(filepath, "r");
            // if (file == NULL)
            // {
            //     printf("Unable to open mailbox file\n");
            //     return 0;
            // }
            // char str[100];
            // memset(str, 0, sizeof(str));

   
            // int sno = 1;
            // char sender[50], received[50], subject[100];
            
    
            // int i = 1;
            // memset(sender,0,sizeof(sender));
            // memset(received, 0, sizeof(received));
            // memset(subject, 0, sizeof(subject));
           

            // while (fgets(str, sizeof(str), file) != NULL) {
            //     fflush(stdin);
            //     fflush(stdout); 

            //     //printf("%s", str);
            //     if (strncmp(str, "From: ", 6) == 0) {
            //         sscanf(str + 6, "%49[^\n]", sender);
            //     } else if (strncmp(str, "Subject: ", 9) == 0) {
            //         sscanf(str + 9, "%99[^\n]", subject);
            //     } else if (strncmp(str, "Received: ", 10) == 0) {
            //         sscanf(str + 10, "%49[^\n]", received);
            //         fflush(stdout);
            //         printf("%d ", sno);
            //         printf("%s %s %s\n", sender, received, subject);
            //         memset(sender, 0, sizeof(sender));
            //         memset(received, 0, sizeof(received));
            //         memset(subject, 0, sizeof(subject));
            //         memset(str,0,sizeof(str));
            //         sno++;
            //     }
            //     else continue;
            // }

            // fclose(file);



            int sockfd;
            struct sockaddr_in servaddr;
            int n;

            if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Unable to create socket\n");
                exit(0);
            }

            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(atoi(argv[2]));
            servaddr.sin_addr.s_addr = inet_addr(argv[1]);

            if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                perror("Unable to connect to server\n");
                exit(0);
            }

             char response[100];
            memset(response, 0, sizeof(response));
            recv(sockfd, response, sizeof(response), 0);

            char buffer[100];
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "USER %s\r\n", username);
            send(sockfd, buffer, sizeof(buffer), 0);
            memset(response, 0, sizeof(response));
            recv(sockfd, response, sizeof(response), 0);

            if(strncmp(response, "-ERR",4)==0)
            {
                printf("Invalid username\n");
                close(sockfd);
                return 0;
            }
            
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "PASS %s\r\n", password);
            send(sockfd, buffer, sizeof(buffer), 0);
            memset(response, 0, sizeof(response));
            recv(sockfd, response, sizeof(response), 0);

            if(strncmp(response, "-ERR",4)==0)
            {
                printf("Invalid password\n");
                close(sockfd);
                return 0;
            }

            // sending STAT command
            memset(buffer, 0, sizeof(buffer));
            sprintf(buffer, "STAT\r\n");
            send(sockfd, buffer, sizeof(buffer), 0);
            memset(response, 0, sizeof(response));
            recv(sockfd, response, sizeof(response), 0);

            int mails, size;
            sscanf(response, "+OK %d %d", &mails, &size);

            // sending LIST command
            while(1){
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "LIST\r\n");
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                if(strncmp(response, "+OK", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }

                for(int i=1;i<=mails;i++)
                {
                    char temp[200];
                    memset(temp, 0, sizeof(temp));
                    recv(sockfd, temp, sizeof(temp), 0);
                    printf("%s\n", temp);
                }

                printf("Enter mail no. to see: ");
                int mail_no;
                scanf("%d", &mail_no);
                if(mail_no==-1)
                {
                    send(sockfd, "QUIT\r\n", 6, 0);
                    close(sockfd);
                    break;
                }
                while(mail_no<1 || mail_no>=mails)
                {
                    printf("Mail no. out of range, give again\n");
                    scanf("%d", &mail_no);
                }

                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "RETR %d\r\n", mail_no);
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                if(strncmp(response, "+OK", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }
                while(1)
                {
                    memset(buffer, 0, sizeof(buffer));
                    recv(sockfd, buffer, sizeof(buffer), 0);
                    printf("%s ", buffer);
                    if(strncmp(buffer, ".\r\n", 3)==0)
                    {
                        break;
                    }
                }
                  
                char c = getchar();
                if(c=='d')
                {
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "DELE %d\r\n", mail_no);
                    send(sockfd, buffer, sizeof(buffer), 0);
                    memset(response, 0, sizeof(response));
                    recv(sockfd, response, sizeof(response), 0);
                    if(strncmp(response, "+OK", 3)!=0)
                    {
                        printf("Error in connection\n");
                        close(sockfd);
                        continue;
                    }
                }             
            }
        }
        else if (choice == 2)
        {
            int sockfd;
            struct sockaddr_in servaddr;
            
            int n;

            // Creating socket file descriptor
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Unable to create socket\n");
                exit(0);
            }

            // Filling server information
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(atoi(argv[2]));
            servaddr.sin_addr.s_addr = inet_addr(argv[1]);

            // Connect to server
            if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                perror("Unable to connect to server\n");
                exit(0);
            }

            // Receive acknowledgement
            

            // Write mail in given format
            char mail[5000];
            memset(mail, 0, sizeof(mail));
            
            printf("Enter mail in given format:\nFrom: <username>@<domain name>\nTo: <username>@<domain name>\nSubject: <subject string, max 50 characters>\n<Message body : one or more lines, terminated by a final line with only a fullstop character>\n");
                char str[80];
                //fflush(stdin);
                while(1)
                {
                    
                    memset(str, 0, sizeof(str));
                    fflush(stdin);
                    int c;

                   fgets(str, 80, stdin);
                    if (strcmp(str, ".\n") == 0)
                    {
                        strcat(mail, str);
                       // strcat(mail, "\n");
                        break;
                    }
                    //printf("%s\n", str);
                    strcat(mail, str);
                   // strcat(mail, "\n");
                    //  printf("%s\n", mail);
                }
                char temp1[5000];
                char temp[5000];
                //memset(temp, 0, sizeof(temp));
                strcpy(temp1, mail);
                strcpy(temp,mail);

                //printf("%s\n", mail);
            
            if (checkEmailFormat(temp)==0)
            {
                continue;
            }
            else
            {
                // 1. Connect to SMTP server
                // 2. Send mail
                // 3. Receive acknowledgement
                // 4. Display acknowledgement
                char *lines[MAX_LINES];
                //dynamically allocate memory for each line
                for (int i = 0; i < MAX_LINES; i++) {
                    lines[i] = (char *)malloc(MAX_CHAR_PER_LINE * sizeof(char));
                }
                int numLines = 0;

                //Tokenize the email into lines
                char *line = strtok(temp1, "\n");
                while (line != NULL && numLines < MAX_LINES) {
                    strcpy(lines[numLines++],line);
                    //printf("%s", lines[numLines-1]);
                    line = strtok(NULL, "\n");
                }
                char response[100];
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "220", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }



                // HELO <domain name>
                char domain_name[50];
                char username[50];
                sscanf(lines[0], "From: %50[^@]@%50[^\n]", username, domain_name);
                char buffer[100];
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "HELO %s\r\n", domain_name);
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
               // printf("S: %s\n", response);
                if(strncmp(response, "250", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }



                // MAIL FROM: username@domain_name
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "MAIL FROM: ");
                strcat(buffer, username);
                strcat(buffer, "@");
                strcat(buffer, domain_name);
                strcat(buffer, "\r\n");    //check
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "250", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;  
                }



                // RCPT TO: username@domain_name
                char to_username[50];
                char to_domain_name[50];
                sscanf(lines[1], "To: %50[^@]@%50[^\n]", to_username, to_domain_name);
                memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "RCPT TO: ");
                strcat(buffer, to_username);
                strcat(buffer, "@");
                strcat(buffer, to_domain_name);
                strcat(buffer, "\r\n");         //check
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "250", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }


                // DATA
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "DATA\r\n");
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "354", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }

                
                // Send mail
                for (int i = 0; i < numLines; i++) {
                    //printf("C: %s\n", lines[i]);
                    strcat(lines[i], "\r\n");
                    //printf("%s", lines[i]);
                    send(sockfd, lines[i], strlen(lines[i]), 0);
                    // memset(response, 0, sizeof(response));
                    // recv(sockfd, response, sizeof(response), 0);
                }
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "250", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue;
                }

                    
                // QUIT
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "QUIT\r\n");
                send(sockfd, buffer, sizeof(buffer), 0);
                memset(response, 0, sizeof(response));
                recv(sockfd, response, sizeof(response), 0);
                //printf("S: %s\n", response);
                if(strncmp(response, "221", 3)!=0)
                {
                    printf("Error in connection\n");
                    close(sockfd);
                    continue; 
                }
            }
            close(sockfd);
            printf("Quitting...\n");
            break;

        }

        else if (choice == 3)
        {
            // Quit
            exit(0);
        }
        else
        {
            printf("Invalid choice\n");
            exit(1);
        }
    }

    return 0;
}
