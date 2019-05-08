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
    if (win) {
        return PASS;
    }
    return FAIL;
}

response message_handler(char message[], int index) {
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
    char temp[PACKET_SIZE];
    strcpy(temp, message);
    char *token = strtok(temp, ",");
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
    int timeout = 2; // 2 seconds timeout
    int time = 0;

    // Check whether all the clients have sent their packet
    while (time < POLLING_RATE * timeout) {
        int ready = 0;
        for (int i = 0; i < game->max_players; i++) {
            if (clients[i].client_fd != -1) {
                if (clients[i].rec[0] == '\0') {
                    clients[i].result = TIMEOUT; // Indicate those who have not sent
                } else {
                    clients[i].result = message_handler(clients[i].rec, i);
                    ready++;
                }
            }
        }
        if (ready == ready_required) {
            break;
        }
        time++;
        usleep((int) (1E6 / POLLING_RATE));
    }

    // Elimination & player count handler
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            if (clients[i].result == TIMEOUT) {
                printf("Client %s failed to send a move\n", clients[i].client_id);
                game->players--;
                eliminate_client(i);
            } else if (clients[i].result == ELIM) {
                game->players--;
            } else if (clients[i].result == FAIL) {
                clients[i].lives--;
                printf("Client %s lost a life (%d lives)\n", clients[i].client_id, clients[i].lives);
                if (clients[i].lives == 0) {
                    clients[i].result = ELIM;
                    game->players--;
                }
            } else if (clients[i].result == PASS) {
                printf("Client %s made it through (%d lives)\n", clients[i].client_id, clients[i].lives);
            }
        }
        memset(clients[i].rec, '\0', PACKET_SIZE);
    }

    // Send packets accordingly
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd != -1) {
            if (game->players == 1 && clients[i].result != ELIM) {
                //printf("1 player remaining\n\nWinner! Client %s has won!\n", clients[i].client_id);
                // client is the winner
                continue;
            } else if (game->players == 0) { // Draw case
                send_packet(VICT, i); // send victory packet instead of ELIM
            } else {
                if (clients[i].result == ELIM) {
                    eliminate_client(i);
                }
                send_packet(clients[i].result, i);
            }
        }
    }
    printf("%d players remaining\n", game->players);

    if (game->players == 0) {
        game->status = FINISHED;
        int draw_players = 0;
        printf("\nDraw!");
        for (int i = 0; i < game->max_players; i++ ) {
            if (clients[i].client_fd != -1) {
                printf(" Client %s,", clients[i].client_id);
                draw_players++;
            }
        }
        if (draw_players == 0) { // Unusual case, remaining players have all left
            printf(" There are no winners\n");
        } else {
            printf("\b have won!\n");
        }
    } else if (game->players == 1) {
        game->status = FINISHED;
        for (int i = 0; i < game->max_players; i++) {
            if (clients[i].client_fd != -1) {
                printf("\nWinner! Client %s has won!\n", clients[i].client_id);
                return;
            }
        }
    }
}

void init_game() {
    printf("Starting game\n");
    usleep((int) (1E6 / POLLING_RATE));
    srand((int) (time(NULL) * getpid())); // Seeds rand

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
