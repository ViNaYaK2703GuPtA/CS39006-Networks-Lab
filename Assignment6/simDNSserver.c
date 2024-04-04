// simDNSserver.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ether.h>

#define MAX_QUERY_SIZE 32
#define MAX_RESPONSE_SIZE 33
#define MAX_DOMAINS 8
#define SERVER_PORT 8888

// Structure for simDNS query packet
struct SimDNSQuery
{
    int ID;
    int MessageType; // 0 for query, 1 for response
    int NumQueries;
    char Queries[MAX_DOMAINS][MAX_QUERY_SIZE];
};

// Structure for simDNS response packet
struct SimDNSResponse
{
    int ID;
    int MessageType;
    int NumResponses;
    char Responses[MAX_DOMAINS][MAX_RESPONSE_SIZE];
};

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    struct SimDNSQuery query;
    struct SimDNSResponse response;
    char recv_buffer[sizeof(struct iphdr) + sizeof(struct SimDNSQuery)];
    char send_buffer[sizeof(struct iphdr) + sizeof(struct SimDNSResponse)];

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, ETH_P_ALL);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the local IP address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Main loop to read and process queries
    while (1)
    {
        // Receive packet
        int recv_len = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0)
        {
            perror("Receive failed");
            continue;
        }
        printf("Received packet of length %d\n", sizeof(recv_buffer));

        // Check IP header protocol field
        struct iphdr *ip_hdr = (struct iphdr *)(recv_buffer);
        printf("Received packet with protocol %d\n", ip_hdr->protocol);
        if (ip_hdr->protocol != 254)
        {
            printf("Non-simDNS packet received. Discarding.\n");
            continue;
        }

        // Extract SimDNS query
        memcpy(&query, recv_buffer + sizeof(struct iphdr), sizeof(struct SimDNSQuery));

        // Prepare SimDNS response
        response.ID = query.ID;
        response.MessageType = 1; // Response
        response.NumResponses = query.NumQueries;

        // Resolve domain names and populate response
        for (int i = 0; i < query.NumQueries; i++)
        {
            struct hostent *host_entry = gethostbyname(query.Queries[i]);
            if (host_entry == NULL || host_entry->h_addr_list[0] == NULL)
            {
                strcpy(response.Responses[i], "N/A");
            }
            else
            {
                strcpy(response.Responses[i], inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]));
            }
        }

        // Construct and send response
        memset(send_buffer, 0, sizeof(send_buffer));
        memcpy(send_buffer, recv_buffer, sizeof(struct iphdr)); // Copy IP header
        memcpy(send_buffer + sizeof(struct iphdr), &response, sizeof(struct SimDNSResponse)); // Copy SimDNS response

        if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&client_addr, client_len) < 0)
        {
            perror("Send failed");
        }
    }

    return 0;
}
