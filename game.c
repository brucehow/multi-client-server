#include "server.h"

typedef enum {EVEN, ODD, DOUB, CON} move_type;


void roll_dice() {
    game->die1 = 1 + (rand() % 6);
    game->die2 = 1 + (rand() % 6);
    printf("(Dice rolled %d and %d - Total %d)\n", game->die1, game->die2, game->die1+game->die2);
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

void new_round() {
    int ready_required = game->players;
    int timeout = 3; // 3 seconds timeout
    int time = 0;

    // Check whether all the clients have sent their packet
    while (true) {
        int ready = 0;
        for (int i = 0; i < game->max_players; i++) {
            if (clients[i].client_fd != -1) {
                if (clients[i].rec[0] == '\0') {
                    clients[i].result = TIMEDOUT; // Indicate those who have not sent
                } else {
                    clients[i].result = message_handler(clients[i].rec, i);
                    ready++;
                }
            }
        }
        if (ready == ready_required || time >= POLLING_RATE * timeout) {
            break;
        }
        time++;
        usleep((int) (1E6 / POLLING_RATE));
    }

    // Eliminate clients and check for missing expected packets
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            if (clients[i].result == TIMEDOUT) {
                printf("Client %s failed to send a move\n", clients[i].client_id);
                eliminate_client(i);
            } else if (clients[i].result == ELIM) {
                eliminate_client(i);
            }
            bzero(clients[i].send, PACKET_SIZE);
        }
    }
    // Elimination done separately to check if VICT is required
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            if (game->players == 1) {
                printf("1 player remaining\n\nWinner! Client %s has won!\n", clients[i].client_id);
                game->status = FINISHED;
                return;
            } else {
                send_packet(clients[i].result, i);
            }
        }
    }

    printf("%d players remaining\n", game->players);

    // In the case where there is a draw
    if (game->players == 0) {
        printf("\nDraw! There are no winners\n");
        game->status = FINISHED;
    }
}

void init_game() {
    printf("Starting game\n");
    srand(time(NULL));
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            send_packet(START, i);
        }
    }

    int round = 1;
    while (game->status != FINISHED) {
        printf("\nRound %d ", round++);
        roll_dice();
        new_round();
    }
}
