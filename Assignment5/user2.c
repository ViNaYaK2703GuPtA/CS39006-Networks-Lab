#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msocket.h"

#define PORT1 6000
#define PORT2 5000
#define IP1 "127.0.0.1"
#define IP2 "127.0.0.1"
#define MAX_BUFFER_SIZE 1024

int main() {
    // Create MTP socket
    int sockfd = m_socket(AF_INET, SOCK_MTP, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created\n");
    printf("%d\n", sockfd);
    // Bind to local IP and Port
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT2);
    local_addr.sin_addr.s_addr = inet_addr(IP2);
    if (m_bind(sockfd,IP2, PORT2,IP1,PORT1) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket bound\n");

    // Set up remote IP and Port
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(PORT1);
    remote_addr.sin_addr.s_addr = inet_addr(IP1);

    // Open a new file to write received content
    FILE *file = fopen("received_file.txt", "w");
    if (file == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    // Receive file content over the MTP socket
    char buffer[MAX_BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = m_recvfrom(sockfd, buffer, MAX_BUFFER_SIZE,0, (struct sockaddr *) &remote_addr, &remote_addr)) > 0) {
        printf("Received %ld bytes\n", bytes_received);
        printf("%s",buffer);
        if (fputs(buffer, file) != bytes_received) {
            perror("File writing failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the file and socket
    fclose(file);
    close(sockfd);

    return 0;
}

