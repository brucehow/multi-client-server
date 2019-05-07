/**
 * CITS3002: Networks Project
 * RNG Battle Royale game simulation utilising a client-server approach.
 *
 * Name: Bruce How (22242664)
 * Date: 01/05/2019
**/

#include "server.h"

// Global variables used with mmap for memory sharing
CLIENTVAR *clients;
GLOBALVAR *game;
int server_fd;

int main(int argc, char *argv[]) {
    // Default configurations
    int port = 4444;
    int lives = 3;
    int max_players = 10;
    bool custom_port = false;
    bool custom_lives = false;
    bool custom_players = false;

    opterr = 0; // Use custom error reporting
    int opt;
    while ((opt = getopt(argc, argv, OPT_LIST)) != -1) {
        switch(opt) {
            case 'p': {
                port = atoi(optarg);
                if (port == 0) {
                    fprintf(stderr, "Illegal port value %s\n", optarg);
                    fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                custom_port = true;
                break;
            } case 'm': {
                max_players = atoi(optarg);
                if (max_players < 4) {
                    fprintf(stderr, "Illegal max players value %s\n", optarg);
                    fprintf(stderr, "Usage: %s [-m max_players]\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                custom_players = true;
                break;
            } case 'l': {
                lives = atoi(optarg);
                if (lives <= 0) {
                    fprintf(stderr, "Illegal lives value %s\n", optarg);
                    fprintf(stderr, "Usage: %s [-l lives]\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                custom_lives = true;
                break;
            } case '?': {
                if (optopt == 'p') {
                    fprintf(stderr, "Usage: %s [-p] [port]\n", argv[0]);
					exit(EXIT_FAILURE);
                } else if (optopt == 'm') {
                    fprintf(stderr, "Usage: %s [-m] [max players]\n", argv[0]);
					exit(EXIT_FAILURE);
                } else if (optopt == 'l') {
                    fprintf(stderr, "Usage: %s [-l] [lives]\n", argv[0]);
					exit(EXIT_FAILURE);
                } else {
                    fprintf(stderr, "Illegal argument -%c\nUsage: %s [-p port] [-m max_players] [-l lives]\n", optopt, argv[0]);
					exit(EXIT_FAILURE);
                }
            }
        }
    }

    // Declare global variables "game" and "clients:
    game = create_shared_memory(sizeof(GLOBALVAR));
    game->players = 0;
    game->start_lives = lives;
    game->max_players = max_players;
    game->status = WAITING;
    clients = create_shared_memory(game->max_players * sizeof(CLIENTVAR));
    for (int i = 0; i < game->max_players; i++) {
        clients[i].client_fd = -1;
    }

    // Create the socket to obtain the server file descriptor
    int err, opt_val;
    struct sockaddr_in server;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    opt_val = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    // Binds the server_fd to the specified port
    err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (err < 0) {
        fprintf(stderr, "Could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    err = listen(server_fd, 128);
    if (err < 0) {
        fprintf(stderr, "Could not listen on socket\n");
        exit(EXIT_FAILURE);
    }

    // Print the configurations
    printf("------- Server Configs -------\n");
    if (custom_port) {
        printf("Server port: %d (specified)\n", port);
    } else {
        printf("Server port: %d (default)\n", port);
    }
    if (custom_lives) {
        printf("Player lives: %d (specified)\n", game->start_lives);
    } else {
        printf("Player lives: %d (default)\n", game->start_lives);
    }
    if (custom_players) {
        printf("Maximum players: %d (specified)\n", game->max_players);
    } else {
        printf("Maximum players: %d (default)\n", game->max_players);
    }
    printf("------------------------------\n\nServer started with the above settings\n");

    // Listen for connections and start lobby timeout
    switch (fork()) {
        case -1: {
            fprintf(stderr, "Fork failed\n");
            exit(EXIT_FAILURE);
        } case 0: {
            connection_listener();
            exit(EXIT_SUCCESS);
        } default: {

        }
    }

    int timeout = 5; // 30 seconds timeout
    int time = 0;

    while (time < POLLING_RATE * timeout && game->players != game->max_players) {
        time++;
        usleep((int) (1E6 / POLLING_RATE));
    }
    if (game->players < 4) {
        printf("Insufficient players have joined (4 required)\n");
        game->status = EXIT;
    } else {
        printf("Sufficient players have joined\n");
        game->status = PLAYING;
        init_game();
    }

    close(server_fd);
    munmap(clients, sizeof(CLIENTVAR) * game->max_players);
    munmap(game, sizeof(GLOBALVAR));
    exit(EXIT_SUCCESS);
}
