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
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>

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
struct responseType
{
    int validFlag;
    char IP[32];
};

struct SimDNSResponse
{
    int ID;
    int MessageType;
    int NumResponses;
    struct responseType Responses[MAX_DOMAINS];
};

int dropmessage(float p)
{
    float r = (float)rand() / (float)RAND_MAX;
    if (r < p)
    {
        return 1;
    }
    return 0;
}

int main()
{
    int sockfd;
    struct sockaddr_ll server_addr;
    struct sockaddr_ll client_addr;
    socklen_t client_len;

    char recv_buffer[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct SimDNSQuery)];
    char send_buffer[sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct SimDNSResponse)];

    // Create raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "lo", IFNAMSIZ - 1);

    // Obtain hardware (MAC) address of the network interface

    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0)
    {
        perror("Bind to interface failed");
        exit(EXIT_FAILURE);
    }

    // Main loop to read and process queries
    while (1)
    {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        struct SimDNSQuery query;
        struct SimDNSResponse response;



        int recv_len = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&client_addr, &client_len);

        if(dropmessage(0.3))
        {
            //printf("Dropped message\n");
            continue;
        }

        if (recv_len < 0)
        {
            perror("Receive failed");
            continue;
        }

        // Check IP header protocol field
        struct iphdr *ip_hdr = (struct iphdr *)(recv_buffer + sizeof(struct ethhdr));

        if (ip_hdr->protocol != 254)
            continue;

        // Extract SimDNS query
        memcpy(&query, recv_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr), sizeof(struct SimDNSQuery));

        if (query.MessageType == 1)
            continue;

        // Prepare SimDNS response
        // printf("Received query with ID %d\n", query.ID);
        response.ID = query.ID;
        response.MessageType = 1; // Response
        // printf("Received query with %d queries\n", query.NumQueries);
        response.NumResponses = query.NumQueries;

        // Resolve domain names and populate response
        for (int i = 0; i < query.NumQueries; i++)
        {
            struct hostent *host_entry = gethostbyname(query.Queries[i]);
            if (host_entry == NULL || host_entry->h_addr_list[0] == NULL)
            {
                response.Responses[i].validFlag = 0;
            }
            else
            {
                response.Responses[i].validFlag = 1;
                strcpy(response.Responses[i].IP, inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]));
                //printf("Resolved %s to %s\n", query.Queries[i], response.Responses[i]);
            }
        }

        // Construct and send response
        memset(send_buffer, 0, sizeof(send_buffer));

        // construct ethernet header
        struct ethhdr *eth = (struct ethhdr *)send_buffer;
        eth->h_proto = htons(ETH_P_ALL);
        memset(eth->h_dest, 0xff, ETH_ALEN);
        memset(eth->h_source, 0, ETH_ALEN);

        // construct ip header
        struct iphdr *ip = (struct iphdr *)(send_buffer + sizeof(struct ethhdr));
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct SimDNSResponse));
        ip->id = 0;
        ip->frag_off = 0;
        ip->ttl = 255;
        ip->protocol = 254;
        ip_hdr->saddr = inet_addr("127.0.0.1");
        ip_hdr->daddr = inet_addr("127.0.0.1");
        ip->check = 0;

        // construct SimDNS response
        memcpy(send_buffer + sizeof(struct ethhdr) + sizeof(struct iphdr), &response, sizeof(struct SimDNSResponse));


        if (sendto(sockfd, send_buffer, sizeof(send_buffer), 0, (struct sockaddr *)&client_addr, client_len) < 0)
        {
            perror("Send failed");
        }
    }

    return 0;
}
