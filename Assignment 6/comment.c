// Set socket timeout
    timeout.tv_sec = 5; // 5 second timeout
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Set socket timeout failed");
        exit(EXIT_FAILURE);
    }

    // Prompt user for query strings
    while (1)
    {
        printf("Enter query string (e.g., 'getIP N domain1 domain2 ... domainN'): ");
        fgets(query_string, sizeof(query_string), stdin);

        // Check if user wants to exit
        if (strncmp(query_string, "EXIT", 4) == 0)
        {
            break;
        }

        // Send query to server
        sendQuery(sockfd, query_string);

        // Clear buffer
        memset(query_string, 0, sizeof(query_string));

        // Receive and process responses
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0)
        {
            // Response received
            struct SimDNSResponse response;
            recvfrom(sockfd, &response, sizeof(response), 0, NULL, NULL);

            // Process and print response
            printf("Query ID: %d\nTotal query strings: %d\n", response.ID, response.NumResponses);
            for (int i = 0; i < response.NumResponses; i++)
            {
                if (response.Responses[i] != 0)
                {
                    struct in_addr addr;
                    addr.s_addr = response.Responses[i];
                    printf("%s %s\n", query_string + 6 + (i * (MAX_QUERY_SIZE + 1)), inet_ntoa(addr));
                }
                else
                {
                    printf("%s NO IP ADDRESS FOUND\n", query_string + 6 + (i * (MAX_QUERY_SIZE + 1)));
                }
            }
        }
        else
        {
            // Timeout occurred
            printf("Timeout occurred. Retrying...\n");
        }
    }

    // Close socket
    close(sockfd);

    return 0;
}