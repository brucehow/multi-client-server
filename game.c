#include "server.h"

typedef enum {EVEN, ODD, DOUB, CON} move_type;

void roll_dice() {
    game->die1 = 1 + (rand() % 6);
    game->die2 = 1 + (rand() % 6);
}

response game_handler(move_type type, int index, int val) {
    bool win = false;

    switch (type) {
        case EVEN: {
            int sum = game->die1 + game->die2;
            if (sum % 2 == 0 && game->die1 != game->die2) {
                win = true;
                break;
            }
        } case ODD: {
            int sum = game->die1 + game->die2;
            if (sum % 2 != 0 && sum > 5) {
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
        printf("Client %s lost a life (%d lives)\n", clients[index].client_id, clients[index].lives);
        if (clients[index].lives == 0) {
            return ELIM;
        }
        return FAIL;
    } else {
        printf("Client %s made it through (%d lives)\n", clients[index].client_id, clients[index].lives);
        return PASS;
    }
}

response message_handler(char *message, int index) {
    int len = strlen(message);

    // Check for invalid message lengths
    if (len < 11 || len > 13) {
        printf("Invalid message length from client %s\n", clients[index].client_id);
        return ELIM;
    }

    // Check for tailing ',' which is not taken into account using strtok
    if (message[strlen(message)-1] == ',') {
        printf("Invalid message format from client %s\n", clients[index].client_id);
        return ELIM;
    }

    // Tokenise the message with the ',' delimiter
    char *token = strtok(message, ",");
    int sections = 1;

    while (token != NULL) {
        switch (sections) {
            case 1: // Client ID
                if (strcmp(token, clients[index].client_id) != 0) { // Checks if the client_id is spoofed
                    printf("Cheating detected from client %s (sent a packet with another ID %s)\n", clients[index].client_id, token);
                    return ELIM;
                }
                break;
            case 2: // Asset equals to MOV
                if (strcmp(token, "MOV") != 0) {
                    printf("Invalid MOV syntax from client %s\n", clients[index].client_id);
                    return ELIM;
                }
                break;
            case 3: // Move type
                if (strcmp(token, "EVEN") == 0) {
                    return game_handler(EVEN, index, 0);
                } else if (strcmp(token, "DOUB") == 0) {
                    return game_handler(DOUB, index, 0);
                } else if (strcmp(token, "ODD") == 0 && len == 11) {
                    return game_handler(ODD, index, 0);
                } else if (strcmp(token, "CON") == 0 && len == 13) {
                    token = strtok(NULL, ",");
                    if (token == NULL) { // Missing the contains value
                        printf("Missing CON value from client %s\n", clients[index].client_id);
                        return ELIM;
                    }
                    int val = atoi(token);
                    if (val < 1 || val > 6) {
                        printf("Invalid CON value from client %s\n", clients[index].client_id);
                        return ELIM;
                    }
                    return game_handler(CON, index, val);
                } else { // Invalid type, does not match required enums
                    printf("Invalid move type from client %s\n", clients[index].client_id);
                    return ELIM;
                }
        }
        sections++;
        token = strtok(NULL, ",");
    }
    printf("Invalid message format from client %s\n", clients[index].client_id);
    return ELIM;
}

void init_round() {
    game->ready_players = 0; // Amount of players that have sent their packets
    int round_players = game->players; // Players this round

    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            switch(fork()) {
                case -1: {
                    fprintf(stderr, "Fork failed\n");
                    exit(EXIT_FAILURE);
                } case 0: {
                    char *buf = allocate_memory(PACKET_SIZE, sizeof(char));

                    int read = recv(clients[i].client_fd, buf, PACKET_SIZE, 0);
                    if (read < 0) {
                        fprintf(stderr, "Failed to read from client %s\n", clients[i].client_id);
                        eliminate_client(i);
                    } else if (read == 0) {
                        printf("Client %s has left the game\n", clients[i].client_id);
                        eliminate_client(i);
                    } else {
                        printf("### DEBUG ### received %s from client %s\n", buf, clients[i].client_id);
                        clients[i].result = message_handler(buf, i);
                    }
                    free(buf);
                    game->ready_players++;
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }
    // Wait until all the players have sent their packet
    printf("Need %d have ", round_players);
    while (game->ready_players != round_players) {
        printf("\b%d", game->ready_players);
    }

    if (game->players == 1) {
        printf("%d player remaining\n", game->players);
    } else {
        printf("%d players remaining\n", game->players);
    }

    // Send our responses back
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].result == PASS || clients[i].result == FAIL) {
            if (game->players == 1) {
                victory_client(i);
                game->status = FINISHED;
                return;
            }
            send_packet(clients[i].result, clients[i].client_fd, clients[i].client_id);
        } else if (clients[i].result == ELIM) {
            eliminate_client(i);
        } else {
            printf("Error reading response type for client %s\n", clients[i].client_id);
            eliminate_client(i);
        }
    }

    // There is a draw (no winner)
    if (game->status == PLAYING && game->players == 0) {
        printf("\nDraw! There are no winners\n");
        game->status = FINISHED;
    }
}

void init_game() {
    printf("Initialising game\n");
    srand(time(NULL));
    sendall_packet(START);

    int round = 1;
    while (game->status != FINISHED) {
        roll_dice();
        printf("\nRound %d (Dice rolled %d and %d - Total %d)\n", round++, game->die1, game->die2, game->die1+game->die2);
        init_round();
    }
}
