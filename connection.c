#include "server.h"

void connection_listener() {
    while (game->status != FINISHED) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        // Block until a connection is made or if it times out
        struct timeval timeout = {0, (int) (1E6 / POLLING_RATE)};
        fd_set client_fd_set;
        FD_ZERO(&client_fd_set);
        FD_SET(server_fd, &client_fd_set);
        if(select(server_fd + 1, &client_fd_set, NULL, NULL, &timeout) == 0){
            continue;
        }
        int client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);
        if (client_fd < 0) {
            fprintf(stderr, "Could not establish connection with new client\n");
            continue;
        }

        switch (fork()) {
            case -1: {
                fprintf(stderr, "Fork failed\n");
                exit(EXIT_FAILURE);
            } case 0: {

                char *buf = allocate_memory(PACKET_SIZE, sizeof(char));

                // Try to read from the incoming client
                int read = recv(client_fd, buf, PACKET_SIZE, 0);
                if (read < 0) {
                    fprintf(stderr, "Client read failed\n");
                    free(buf);
                    close(client_fd);
                    exit(EXIT_FAILURE);
                }
                if (strcmp(buf, "INIT") == 0) {
                    // Reject connections whilst the server is playing
                    if (game->status == PLAYING) {
                        send_packet(REJECT, client_fd, NULL);
                        printf("New client connection rejected as game as started!\n");
                        close(client_fd);
                        exit(EXIT_FAILURE);
                    }
                    // Accept the connection
                    int index = add_client(client_fd);

                    // Check for player count requirements
                    if (game->players == game->max_players && game->status == WAITING) {
                        switch (fork()) {
                            case -1: {
                                fprintf(stderr, "Fork failed\n");
                                exit(EXIT_FAILURE);
                            } case 0: {
                                game->status = PLAYING;
                                printf("Sufficient players have joined\n");
                                init_game();
                                exit(EXIT_SUCCESS);
                            }
                        }
                    }

                    while (game->status != FINISHED) {
                        bzero(buf, PACKET_SIZE);
                        read = recv(client_fd, buf, PACKET_SIZE, 0);

                        if (read < 0) {
                            fprintf(stderr, "Failed to read from client %s\n", clients[index].client_id);
                            game->players--;
                            eliminate_client(index);
                        } else if (read == 0) {
                            if (game->status == WAITING) {
                                remove_client(index);
                                free(buf);
                                exit(EXIT_FAILURE);
                            } else if (game->status == PLAYING && clients[index].client_fd != -1) {
                                printf("Client %s has left the game\n", clients[index].client_id);
                                game->players--;
                                eliminate_client(index);
                                free(buf);
                                exit(EXIT_FAILURE);
                            }
                        } else if (clients[index].expect_packet) {
                            strcpy(clients[index].packet, buf);
                            clients[index].read_packet = true;
                            clients[index].sent_packet = true;
                            clients[index].expect_packet = false;
                        } else {
                            fprintf(stderr, "Client %s sent an unexcepted packet\n", clients[index].client_id);
                            game->players--;
                            eliminate_client(index);
                        }
                        usleep((int) (1E6 / POLLING_RATE));
                    }
                    close(client_fd);
                    free(buf);
                    exit(EXIT_SUCCESS);
                } else {
                    free(buf);
                    close(client_fd);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}
