#include "server.h"

typedef enum {EVEN, ODD, DOUB, CON} move_type;

void roll_dice() {
    game->die1 = 1 + (rand() % 6);
    game->die2 = 1 + (rand() % 6);
}

void game_handler(move_type type, int index, int val) {
    bool win = false;

    switch (type) {
        case EVEN: {
            if ((game->die1 + game->die2) % 2 == 0) {
                win = true;
                break;
            }
        } case ODD: {
            if ((game->die1 + game->die2) % 2 != 0) {
                win = true;
                break;
            }
        } case DOUB: {
            if (game->die1 == game->die2) {
                win = true;
                break;
            }
        } case CON: {
            if (game->die1 == val || game->die2 == val) {
                win = true;
                break;
            }
        }
    }
    if (!win) {
        clients[index].lives--;
        printf("Client %s lost a life (%d lives remaining)\n", clients[index].client_id, clients[index].lives);
        send_packet(FAIL, clients[index].client_fd, clients[index].client_id);
    } else {
        printf("Client %s made it through this round (%d lives)\n", clients[index].client_id, clients[index].lives);
        send_packet(PASS, clients[index].client_fd, clients[index].client_id);
    }
}

bool message_handler(char *message, int index) {
    int len = strlen(message);

    // Check for invalid message lengths
    if (len < 11 || len > 13) {
        printf("Invalid message length from client %s\n", clients[index].client_id);
        return false;
    }

    // Check for tailing ',' which is not taken into account using strtok
    if (message[strlen(message)-1] == ',') {
        printf("Invalid message format from client %s\n", clients[index].client_id);
        return false;
    }

    // Tokenise the message with the ',' delimiter
    char *token = strtok(message, ",");
    int sections = 1;

    while (token != NULL) {
        switch (sections) {
            case 1: // Client ID
                if (strcmp(token, clients[index].client_id) != 0) { // Checks if the client_id is spoofed
                    printf("Cheating detected from client %s (sent a packet with another ID %s)\n", clients[index].client_id, token);
                    return false;
                }
                break;
            case 2: // Asset equals to MOV
                if (strcmp(token, "MOV") != 0) {
                    printf("Invalid MOV syntax from client %s\n", clients[index].client_id);
                    return false;
                }
                break;
            case 3: // Move type
                if (strcmp(token, "EVEN") == 0) {
                    game_handler(EVEN, index, 0);
                    return true;
                } else if (strcmp(token, "DOUB") == 0) {
                    game_handler(DOUB, index, 0);
                    return true;
                } else if (strcmp(token, "ODD") == 0 && len == 11) {
                    game_handler(ODD, index, 0);
                    return true;
                } else if (strcmp(token, "CON") == 0 && len == 13) {
                    token = strtok(NULL, ",");
                    if (token == NULL) { // Missing the contains value
                        printf("Missing CON value from client %s\n", clients[index].client_id);
                        return false;
                    }
                    int val = atoi(token);
                    if (val < 1 || val > 6) {
                        printf("Invalid CON value from client %s\n", clients[index].client_id);
                        return false;
                    }
                    game_handler(CON, index, val);
                    return true;
                } else { // Invalid type, does not match required enums
                    printf("Invalid move type from client %s\n", clients[index].client_id);
                    return false;
                }
        }
        sections++;
        token = strtok(NULL, ",");
    }
    printf("Invalid message format from client %s\n", clients[index].client_id);
    return false;
}

void init_round() {
    game->ready_players = 0;
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            switch(fork()) {
                case -1: {
                    fprintf(stderr, "Fork failed\n");
                    exit(EXIT_FAILURE);
                } case 0: {
                    char *buf = allocate_memory(PACKET_SIZE * sizeof(char));

                    int read = recv(clients[i].client_fd, buf, PACKET_SIZE, 0);
                    if (read < 0) {
                        fprintf(stderr, "Failed to read from client %s\n", clients[i].client_id);
                        eliminate_client(i);
                    } else if (read == 0) {
                        printf("Client %s has left the game\n", clients[i].client_id);
                        eliminate_client(i);
                    } else {
                        if(!message_handler(buf, i) || clients[i].lives == 0) {
                            eliminate_client(i);
                        }
                    }
                    game->ready_players++;
                    free(buf);
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }
    // Wait until all the players have sent their packet
    while (game->ready_players != game->players);

    // If there is a winner
    if (game->players == 1) {
        for (int i = 0; i < game->max_players; i++) {
            if (clients[i].lives > 0) {
                printf("Client %s has won!\n", clients[i].client_id);
                game->status = FINISHED;
                break;
            }
        }
    }

}

void init_game() {
    printf("Initialising game\n");
    srand(time(NULL));
    sendall_packet(START);

    int round = 1;
    while (game->status != FINISHED) {
        roll_dice();
        printf("\nRound %d (Dice rolled %d and %d)\n", round++, game->die1, game->die2);
        init_round();
    }
}
