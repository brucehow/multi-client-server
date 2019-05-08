#include "server.h"

void send_packet(response msg_type, int index) {
    memset(clients[index].send, '\0', PACKET_SIZE);

    switch (msg_type) {
        case WELCOME: sprintf(clients[index].send, "WELCOME,%s", clients[index].client_id); break;
        case START: sprintf(clients[index].send, "START,%d,%d", game->players, game->start_lives); break;
        case PASS: sprintf(clients[index].send, "%s,PASS", clients[index].client_id); break;
        case FAIL: sprintf(clients[index].send, "%s,FAIL", clients[index].client_id); break;
        case ELIM: sprintf(clients[index].send, "%s,ELIM", clients[index].client_id); break;
        case VICT: sprintf(clients[index].send, "VICT"); break;
        case REJECT: sprintf(clients[index].send, "REJECT"); break;
        case CANCEL: sprintf(clients[index].send, "CANCEL"); break;
        default: fprintf(stderr, "Unexpected msg_type\n"); exit(EXIT_FAILURE);
    }
}

void connection_listener() {
    // Listen to connections while the game is waiting or playing
    while (game->status == WAITING || game->status == PLAYING) {
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
                // If new connection is not an INIT packet ignore
                if (strcmp(buf, "INIT") != 0) {
                    close(client_fd);
                    free(buf);
                    exit(EXIT_FAILURE);
                }
                // Reject connections whilst the server is playing
                if (game->status == PLAYING) {
                    if (send(client_fd, "REJECT", strlen("REJECT"), 0) < 0) {
                        perror(__func__);
                        fprintf(stderr, "Failed to send packet (REJECT) to new client\n");
                    }
                    printf("New client connection rejected as game as started!\n");
                    close(client_fd);
                    free(buf);
                    exit(EXIT_FAILURE);
                }

                // Add the client to the global clients list
                int index = add_client(client_fd);

                // Loop that handles rec and send packets to the client
                while (true) {
                    // Check if the client is the winner
                    if (game->status == FINISHED && clients[index].client_fd != -1) {
                        send_packet(VICT, index);
                        if (send(client_fd, clients[index].send, strlen(clients[index].send), 0) < 0) {
                            perror(__func__);
                            fprintf(stderr, "Failed to send packet (%s) to client %s\n", clients[index].send, clients[index].client_id);
                        }
                        break;
                    }

                    // Check if the client is eliminated
                    if (clients[index].client_fd == -1) {
                        send_packet(ELIM, index);
                        if (send(client_fd, clients[index].send, strlen(clients[index].send), 0) < 0) {
                            perror(__func__);
                            fprintf(stderr, "Failed to send packet (%s) to client %s\n", clients[index].send, clients[index].client_id);
                        }
                        break;
                    }

                    // Checks for when the game can't start
                    if (game->status == EXIT) {
                        send_packet(CANCEL, index);
                        if (send(client_fd, clients[index].send, strlen(clients[index].send), 0) < 0) {
                            perror(__func__);
                            fprintf(stderr, "Failed to send packet (%s) to client %s\n", clients[index].send, clients[index].client_id);
                        }
                        break;
                    }

                    // Checks for packets to send
                    if (clients[index].send[0] != '\0') {
                        if (send(client_fd, clients[index].send, strlen(clients[index].send), 0) < 0) {
                            perror(__func__);
                            fprintf(stderr, "Failed to send packet (%s) to client %s\n", clients[index].send, clients[index].client_id);
                        }
                        memset(clients[index].send, '\0', PACKET_SIZE);
                    }

                    // Set a timeout for recv so it is not constantly blocked
                    struct pollfd fd;
                    fd.fd = client_fd;
                    fd.events = POLLIN;
                    switch (poll(&fd, 1, (int) (1E3 / POLLING_RATE))) {
                        case -1: {
                            fprintf(stderr, "Poll failed\n");
                            break;
                        } case 0: {
                            continue;
                        }
                    }
                    memset(buf, '\0', PACKET_SIZE);
                    read = recv(client_fd, buf, PACKET_SIZE, 0);

                    if (read < 0) {
                        fprintf(stderr, "Failed to read from client %s\n", clients[index].client_id);
                        game->players--;
                        eliminate_client(index);
                    } else if (read == 0) { // Client disconnected
                        if (game->status == WAITING) {
                            game->players--;
                            disconnect_client(index);
                            break;
                        } else if (clients[index].client_fd != -1) {
                            printf("Client %s has left the game\n", clients[index].client_id);
                            game->players--;
                            eliminate_client(index);
                            break;
                        }
                    } else { // Received a packet
                        if (clients[index].rec[0] != '\0') { // Not expecting packet
                            fprintf(stderr, "Client %s sent an unexcepted packet\n", clients[index].client_id);
                        }
                        strcpy(clients[index].rec, buf);
                    }
                    usleep((int) (1E6 / POLLING_RATE));
                }

                close(client_fd);
                free(buf);
                exit(EXIT_SUCCESS);
            }
        }
    }
}
