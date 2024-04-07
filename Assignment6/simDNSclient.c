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
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <ctype.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>

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

int cnt = 1;

struct SimDNSQuery pendingQueryTable[100000];

// Function to construct and send simDNS query
void fillSimDNSQuery(struct SimDNSQuery *query, char *query_string)
{
    char *token = strtok(query_string, " ");

    // Skip the first token ("getIP")
    token = strtok(NULL, " ");

    query->ID = cnt;

    query->MessageType = 0; // Query type

    int numQueries = atoi(token);
    query->NumQueries = numQueries;

    // Fill domain names into the struct
    for (int i = 0; i < numQueries; i++)
    {
        token = strtok(NULL, " ");
        strcpy(query->Queries[i], token);
    }

    pendingQueryTable[cnt] = *query;
    cnt++;
}

int main(int argc, char *argv[])
{
    if(argc !=2)
    {
        printf("Write the mac address of the interface\n");
        exit(EXIT_FAILURE);
    }

    char mac[50];
    strcpy(mac,argv[1]);

    memset(pendingQueryTable, 0, sizeof(pendingQueryTable));
    int sockfd;
    struct timeval timeout;
    fd_set readfds;
    // Create raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_ll server_addr;
    socklen_t server_len = sizeof(server_addr);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sll_family = AF_PACKET;
    server_addr.sll_protocol = htons(ETH_P_ALL);
    server_addr.sll_halen = ETH_ALEN;
    server_addr.sll_ifindex = if_nametoindex("lo");

    // Main loop to handle user queries
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Add stdin to readfds
        FD_SET(sockfd, &readfds);       // Add sockfd to readfds
        char query_string[256];

        // Set the timeout
        timeout.tv_sec = 30;  // 5 seconds
        timeout.tv_usec = 0; // 0 microseconds
        // Wait for activity on stdin or sockfd

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        // Get query string from user

        if (activity < 0)
        {
            perror("Error in select");
            exit(EXIT_FAILURE);
        }

        if (activity == 0)
        {
            printf("timeout\n");
            // Send the pending queries again
            for (int i = 1; i < cnt; i++)
            {
                if (pendingQueryTable[i].ID != 0)
                {
                    struct SimDNSQuery packet;
                    packet = pendingQueryTable[i];

                    // Construct send buffer
                    char send_buffer[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct SimDNSQuery)];
                    memset(send_buffer, 0, sizeof(send_buffer));

                    // Construct Ethernet header
                    struct ethhdr *eth_hdr = (struct ethhdr *)send_buffer;
                    // get the mac address of the interface from char mac[]
                    sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                           &eth_hdr->h_dest[0], &eth_hdr->h_dest[1], &eth_hdr->h_dest[2], &eth_hdr->h_dest[3], &eth_hdr->h_dest[4], &eth_hdr->h_dest[5]);


                    // 08:00:27:bc:cc:be this is dest and src mac address
                    

                    memset(eth_hdr->h_source, 0, 6);
                    eth_hdr->h_proto = htons(ETH_P_ALL);

                    struct iphdr *ip_hdr = (struct iphdr *)(send_buffer + sizeof(struct ethhdr));
                    ip_hdr->protocol = 254;
                    ip_hdr->saddr = inet_addr("127.0.0.1");
                    ip_hdr->daddr = inet_addr("127.0.0.1");
                    ip_hdr->check = 0;
                    ip_hdr->ihl = 5;
                    ip_hdr->version = 4;
                    ip_hdr->tos = 0;
                    ip_hdr->id = 0;
                    ip_hdr->frag_off = 0;
                    ip_hdr->ttl = 255;
                    ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct SimDNSQuery));

                    // Copy SimDNS query to packet buffer
                    memcpy(send_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr), &packet, sizeof(struct SimDNSQuery));

                    printf("Sending packet to server\n");

                    sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                }

                fflush(stdout);
                // break;
            }
        }
        // If activity on stdin (user input)
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            // printf("Enter query string: ");
            fgets(query_string, sizeof(query_string), stdin);
            // scanf("%255[^\n]", query_string);
            // Remove newline character from query_string
            if ((strlen(query_string) > 0) && (query_string[strlen(query_string) - 1] == '\n'))
                query_string[strlen(query_string) - 1] = '\0';

            char temp[256];
            strcpy(temp, query_string);

            // Parse query string
            char *token = strtok(temp, " ");
            token = strtok(NULL, " ");

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

            struct SimDNSQuery packet;

            fillSimDNSQuery(&packet, query_string);

            // Construct send buffer
            char send_buffer[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct SimDNSQuery)];
            memset(send_buffer, 0, sizeof(send_buffer));

            // Construct Ethernet header
            struct ethhdr *eth_hdr = (struct ethhdr *)send_buffer;
            // 08:00:27:bc:cc:be this is dest and src mac address
            eth_hdr->h_dest[0] = 0xff;
            eth_hdr->h_dest[1] = 0xff;
            eth_hdr->h_dest[2] = 0xff;
            eth_hdr->h_dest[3] = 0xff;
            eth_hdr->h_dest[4] = 0xff;
            eth_hdr->h_dest[5] = 0xff;

            memset(eth_hdr->h_source, 0, 6);
            eth_hdr->h_proto = htons(ETH_P_ALL);

            // Copy IP header to packet buffer
            struct iphdr *ip_hdr = (struct iphdr *)(send_buffer + sizeof(struct ethhdr));
            ip_hdr->protocol = 254;
            ip_hdr->saddr = inet_addr("127.0.0.1");
            ip_hdr->daddr = inet_addr("127.0.0.1");
            ip_hdr->check = 0;
            ip_hdr->ihl = 5;
            ip_hdr->version = 4;
            ip_hdr->tos = 0;
            ip_hdr->id = 0;
            ip_hdr->frag_off = 0;
            ip_hdr->ttl = 255;
            ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct SimDNSQuery));

            // Copy SimDNS query to packet buffer
            memcpy(send_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr), &packet, sizeof(struct SimDNSQuery));

            // Send packet to server

            printf("Sending packet to server\n");

            sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        }

        // If activity on sockfd (response from server)
        if (FD_ISSET(sockfd, &readfds))
        {
            // Receive response from server

            char recv_buffer[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct SimDNSResponse)];
            int recv_len = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&server_addr, &server_len);
            if (recv_len < 0)
            {
                perror("Receive failed");
                continue;
            }
            // printf("Received packet of length %d\n", recv_len);
            struct iphdr *ip_hdr2 = (struct iphdr *)(recv_buffer + sizeof(struct ethhdr));
            //printf("ip_hdr2->protocol: %d\n", ip_hdr2->protocol);

            // print message type
            struct SimDNSResponse *response = (struct SimDNSResponse *)(recv_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));
            // printf("response->MessageType: %d\n", response->MessageType);

            // // Check IP header protocol field
            if (ip_hdr2->protocol != 254)
            {
                // printf("Non-simDNS packet received. Discarding.\n");
                continue;
            }

            if (response->MessageType == 0)
                continue;

            // printf("response->ID: %d\n", response->ID);
            if (!(response->ID > 0 && response->ID < 100000))
                continue;

            if (pendingQueryTable[response->ID].ID == 0)
                continue;
            

            // Process and print response
            printf("\nQuery ID: %d\nTotal query strings: %d\n", response->ID, response->NumResponses);
            for (int i = 0; i < response->NumResponses; i++)
            {
                if (response->Responses[i] != 0)
                {
                    // struct in_addr addr;
                    // addr.s_addr = response->Responses[i];
                    printf("%s\t%s\n", pendingQueryTable[response->ID].Queries[i],response->Responses[i]);
                }
            }
            pendingQueryTable[response->ID].ID = 0;
        }
    }
    return 0;
}
