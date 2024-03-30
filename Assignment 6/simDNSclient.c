// simDNSclient.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define MAX_QUERY_SIZE 32
#define MAX_RESPONSE_SIZE 33
#define MAX_DOMAINS 8
#define SERVER_IP "127.0.0.1" // Assuming server is running locally
#define SERVER_PORT 8888

// Structure for simDNS query packet
struct SimDNSQuery {
    int ID;
    int MessageType;
    int NumQueries;
    char Queries[MAX_DOMAINS][MAX_QUERY_SIZE];
};

// Structure for simDNS response packet
struct SimDNSResponse {
    int ID;
    int MessageType;
    int NumResponses;
    char Responses[MAX_DOMAINS][MAX_RESPONSE_SIZE];
};

// Function to construct and send simDNS query
void sendQuery(int sockfd, char *query_string) {
    struct SimDNSQuery query;
    struct sockaddr_in server_addr;
    char *token;
    int i = 0;

    // Initialize query packet
    query.ID = rand() % 65536; // Random ID
    query.MessageType = 0; // Query type

    // Parse query string
    token = strtok(query_string, " ");
    query.NumQueries = atoi(token);
    token = strtok(NULL, " ");
    while (token != NULL && i < MAX_DOMAINS) {
        strncpy(query.Queries[i], token, MAX_QUERY_SIZE);
        token = strtok(NULL, " ");
        i++;
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    // Send query to server
    sendto(sockfd, &query, sizeof(query), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
}

int main() {
    int sockfd;
    struct timeval timeout;
    fd_set readfds;
    char query_string[256];

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket timeout
    timeout.tv_sec = 5; // 5 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Set socket timeout failed");
        exit(EXIT_FAILURE);
    }

    // Prompt user for query strings
    while (1) {
        printf("Enter query string (e.g., 'getIP N domain1 domain2 ... domainN'): ");
        fgets(query_string, sizeof(query_string), stdin);

        // Check if user wants to exit
        if (strncmp(query_string, "EXIT", 4) == 0) {
            break;
        }

        // Send query to server
        sendQuery(sockfd, query_string);

        // Clear buffer
        memset(query_string, 0, sizeof(query_string));

        // Receive and process responses
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            // Response received
            struct SimDNSResponse response;
            recvfrom(sockfd, &response, sizeof(response), 0, NULL, NULL);

            // Process and print response
            printf("Query ID: %d\nTotal query strings: %d\n", response.ID, response.NumResponses);
            for (int i = 0; i < response.NumResponses; i++) {
                if (response.Responses[i] != 0) {
                    struct in_addr addr;
                    addr.s_addr = response.Responses[i];
                    printf("%s %s\n", query_string + 6 + (i * (MAX_QUERY_SIZE + 1)), inet_ntoa(addr));
                } else {
                    printf("%s NO IP ADDRESS FOUND\n", query_string + 6 + (i * (MAX_QUERY_SIZE + 1)));
                }
            }
        } else {
            // Timeout occurred
            printf("Timeout occurred. Retrying...\n");
        }
    }

    // Close socket
    close(sockfd);

    return 0;
}
