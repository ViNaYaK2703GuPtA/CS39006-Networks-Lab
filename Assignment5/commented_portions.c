// IN INITMSOCKET.C

// // SM[i].recv_buffer_empty[SM[i].rwnd.size] = 0;
                    // // SM[i].rwnd.sequence_numbers[SM[i].rwnd.size] = SM[i].rwnd.size;
                    // // SM[i].rwnd.size++;

                    // // Remove MTP header code left
                    // // code goes here

                    // int seq_no = -1;
                    // // printing received message
                    // printf("recv: %s\n", buffer);

                    // for(int j=0; j<5; j++){
                    //     if(SM[i].recvbuf_size[j] == 1){
                    //         ns = 0;
                    //         SM[i].recvbuf_size[j] = 0;
                    //         strcpy(SM[i].recvbuf_size[j], buffer);
                    //         if(SM[i].receiver_window.window_size == 5){
                    //             ns = 1;
                    //             break;
                    //         }
                    //         seq_no = j;
                    //         last_inorder_seq_no = j;
                    //         SM[i].receiver_window.window_size++;
                    //         break;
                    //     }
                    // }

                    // if(ns == -1) ns = 1;
                    // else{
                    //     // there was space in the buffer and message was received
                    //     // send ACK
                    //     char ACK[50];
                    //     sprintf(ACK, "ACK %d, rwnd size = %d", seq_no, SM[i].receiver_window.window_size);
                    //     sendto(SM[i].sock_id, ACK, strlen(ACK), MSG_DONTWAIT, (struct sockaddr*)&dest_addr, len);
                    // }


        // Check if there is any incoming message on any of the UDP sockets
        for (int i = 0; i < MAX_SOCKETS; i++)
        {

            if (SM[i].free == 1) // && FD_ISSET(SM[i].sock_id, &readfds)
            {
                // Receive the message using recvfrom()
                char buffer[1024];
                struct sockaddr_in sender_addr;
                socklen_t sender_len = sizeof(sender_addr);
                printf("%d\n", SM[i].sock_id);
                ssize_t recv_len = recvfrom(SM[i].sock_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &sender_len);
                printf("Received message: %s\n", buffer);
                if (recv_len == -1)
                {
                    perror("recvfrom failed");
                    exit(1);
                }
                // Store the message in the receiver-side message buffer
                int j;
                for (j = 0; j < 5; j++) // change it to 5
                {
                    if (strcmp(SM[i].recvbuf[j], "") == 0)
                    {
                        // printf("Received message in loop: %d\n", j);
                        strcpy(SM[i].recvbuf[j], buffer);
                        printf("Received message in loop: %s\n", SM[i].recvbuf[j]);
                        break;
                    }
                    memset(buffer, 0, sizeof(buffer));
                }
                // Send an ACK message to the sender
                // char ack[1024];
                // strcpy(ack, "ACK");
                // ssize_t send_len = sendto(SM[i].sock_id, ack, sizeof(ack), 0, (struct sockaddr *)&sender_addr, sender_len);
                // Update the swnd and remove the message from the sender-side message buffer
            }
        }