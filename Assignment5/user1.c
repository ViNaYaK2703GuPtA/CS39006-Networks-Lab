#include "msocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT1 5000
#define PORT2 6000
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

    // Bind to local IP and Port
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(PORT1);
    local_addr.sin_addr.s_addr = inet_addr(IP1);
    if (m_bind(sockfd,IP1, PORT1,IP2,PORT2) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Set up remote IP and Port
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(PORT2);
    remote_addr.sin_addr.s_addr = inet_addr(IP2);

    // Open the file to send
    FILE *file = fopen("file.txt", "rb");
    if (file == NULL) {
        perror("File opening failed");
        exit(EXIT_FAILURE);
    }

    // Send file content over the MTP socket
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, MAX_BUFFER_SIZE, file)) > 0) {
        if (m_sendto(sockfd, buffer, bytes_read, IP2, PORT2) == -1) {
            perror("Sendto failed");
            exit(EXIT_FAILURE);
        }
    }

    // Close the file and socket
    fclose(file);
    close(sockfd);

    return 0;
}
