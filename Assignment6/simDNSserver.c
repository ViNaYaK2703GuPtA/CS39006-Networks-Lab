// simDNSserver.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define MAX_QUERY_SIZE 32
#define MAX_RESPONSE_SIZE 33
#define MAX_DOMAINS 8
#define SERVER_PORT 8888
#define PROTOCOL_SIMDNS 254

// Structure for simDNS query packet
struct SimDNSQuery
{
    int ID;
    int MessageType;
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

// Function to process simDNS query and generate response
// void processQuery(struct SimDNSQuery *query, struct sockaddr_in *client_addr, int sockfd)
// {
//     struct SimDNSResponse response;
//     response.ID = query->ID;
//     response.MessageType = 1; // Response type

//     for (int i = 0; i < query->NumQueries; i++)
//     {
//         struct hostent *host_entry;
//         host_entry = gethostbyname(query->Queries[i]);

//         if (host_entry == NULL || host_entry->h_addr_list[0] == NULL)
//         {
//             response.Responses[i] = 0; // Flag bit set to 0 for invalid response
//         }
//         else
//         {
//             // Convert IP address to network byte order
//             memcpy(&response.Responses[i], host_entry->h_addr_list[0], sizeof(uint32_t));
//         }
//     }

//     // Send response back to client
//     sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)client_addr, sizeof(*client_addr));
// }

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    struct SimDNSQuery query;

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Receive and process queries
    while (1)
    {
        char buffer[65536]; // Buffer to store packet data
        int data_size;

        data_size = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_addr_len);
        if (data_size < 0)
        {
            perror("Packet receive failed");
            exit(EXIT_FAILURE);
        }

        // Parse IP header
        struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));

        // Check if the protocol is simDNS (protocol number 254)
        if (ip_header->protocol == PROTOCOL_SIMDNS)
        {
            // Process the packet further
            // Add your code here to handle simDNS packets
            printf("Received a simDNS packet of size %d bytes\n", data_size);

            // Check if it's a simDNS query
            // if (query.MessageType == 0)
            // {
            //     processQuery(&query, &client_addr, sockfd);
            // }
        }
        else
        {
            // Discard the packet
            printf("Discarded a non-simDNS packet\n");
        }
    }

    // Close socket
    close(sockfd);

    return 0;
}
