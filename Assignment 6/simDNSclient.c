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
#include <sys/time.h>   // for timeval
#include <sys/select.h> // for fd_set
#include <netinet/ethernet.h>

#define MAX_QUERY_SIZE 32
#define MAX_RESPONSE_SIZE 33
#define MAX_DOMAINS 8
#define SERVER_IP "127.0.0.1" // Assuming server is running locally
#define SERVER_PORT 8888

// Structure for simDNS query packet
struct SimDNSQuery
{
    int ID;
    int MessageType; // 0 for query 1 for response
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

// Function to construct and send simDNS query
void sendQuery(int sockfd, char *query_string)
{
    struct SimDNSQuery query;
    struct sockaddr_in server_addr;
    char *token;
    int i = 0;

    // Initialize query packet
    query.ID = rand() % 65536; // Random ID
    query.MessageType = 0;     // Query type

    // Parse query string
    token = strtok(query_string, " ");
    query.NumQueries = atoi(token);
    token = strtok(NULL, " ");
    while (token != NULL && i < MAX_DOMAINS)
    {
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

int main()
{
    int sockfd;
    struct timeval timeout;
    fd_set readfds;
    char query_string[256];

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct iphdr *ip_hdr;
    ip_hdr = (struct iphdr *)malloc(sizeof(struct iphdr));
    ip_hdr->protocol = 254;
    ip_hdr->saddr = inet_addr("127.0.0.1");
    ip_hdr->daddr = inet_addr("127.0.0.1");
    ip_hdr->tot_len = sizeof(struct iphdr);
    ip_hdr->check = 0;
    ip_hdr->ihl = 5;
    ip_hdr->version = 4;
    ip_hdr->tos = 0;
    ip_hdr->id = 0;
    ip_hdr->frag_off = 0;
    ip_hdr->ttl = 255;

    // Main loop to handle user queries
    while (1)
    {
        // Get query string from user
        printf("Enter query string: ");
        fgets(query_string, sizeof(query_string), stdin);

        // Remove newline character from query_string
        if ((strlen(query_string) > 0) && (query_string[strlen(query_string) - 1] == '\n'))
            query_string[strlen(query_string) - 1] = '\0';

        // Parse query string
        char *token = strtok(query_string, " ");
        int numQueries = atoi(token);

        // Check if numQueries <= 8
        if (numQueries > MAX_DOMAINS)
        {
            printf("Error: Number of queries should be less than or equal to 8.\n");
            continue;
        }

        // Validate domain names
        int valid = 1;
        for (int i = 0; i < numQueries; i++)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Error: Insufficient number of queries provided.\n");
                valid = 0;
                break;
            }
            if (strlen(token) < 3 || strlen(token) > 31)
            {
                printf("Error: Domain name '%s' does not meet length requirements.\n", token);
                valid = 0;
                break;
            }
            for (int j = 0; j < strlen(token); j++)
            {
                if (!(isalnum(token[j]) || token[j] == '-' || token[j] == '.'))
                {
                    printf("Error: Domain name '%s' contains invalid characters.\n", token);
                    valid = 0;
                    break;
                }
                if (token[j] == '-' && (j == 0 || j == strlen(token) - 1 || token[j - 1] == '-'))
                {
                    printf("Error: Domain name '%s' contains consecutive hyphens or starts/ends with hyphen.\n", token);
                    valid = 0;
                    break;
                }
            }
            if (!valid)
                break;
        }

        // If any validation fails, prompt user for next query
        if (!valid)
            continue;

        // Send query to server

        char packet[65536];
        memset(packet, 0, sizeof(packet));

        // Copy IP header to packet
        memcpy(packet, ip_hdr, sizeof(struct iphdr));

        // Copy data to packet
        // Assuming you have the data in a variable called 'data'
        memcpy(packet + sizeof(struct iphdr), data, data_length);

        // Send packet to server
        sendto(sockfd, packet, sizeof(struct iphdr) + data_length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    return 0;
}
