#include "server.h"

void connection_handler() {
    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);

        // Will block until a connection is made
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

                char *buf = allocate_memory(PACKET_SIZE * sizeof(char));

                // Try to read from the incoming client
                int read = recv(client_fd, buf, PACKET_SIZE, 0);
                if (read < 0) {
                    fprintf(stderr, "Client read failed\n");
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
                    int client_index = add_client(client_fd);
                    printf("IGNORE#%d\n", client_index);

                    // Check if the required player count is satisfied
                    if (game->players == game->max_players) {
                        printf("Sufficient players have joined\n");
                        game->status = PLAYING;
                        init_game();
                    }

                    /* NEED FIX Watch for disconnects before the game starts
                    while (game->status == WAITING) {
                        read = recv(client_fd, buf, PACKET_SIZE, 0);
                        printf("break");

                        // Client disconnected
                        if (read == 0) {
                            remove_client(client_index);
                            free(buf);
                            exit(EXIT_FAILURE);
                        }
                    }*/
                    free(buf);
                } else {
                    close(client_fd);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}
