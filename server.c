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

void sendall_packet(response msg_type) {
    for (int i = 0; i < game->max_players; i++) {
        send_packet(msg_type, clients[i].client_fd, clients[i].client_id);
    }
}

void send_packet(response msg_type, int client_fd, char *client_id) {
    char *buf = allocate_memory(PACKET_SIZE, sizeof(char));

    switch (msg_type) {
        case WELCOME: sprintf(buf, "WELCOME,%s", client_id); break;
        case START: sprintf(buf, "START,%d,%d", game->players, game->start_lives); break;
        case PASS: sprintf(buf, "%s,PASS", client_id); break;
        case FAIL: sprintf(buf, "%s,FAIL", client_id); break;
        case ELIM: sprintf(buf, "%s,ELIM", client_id); break;
        case VICT: sprintf(buf, "VICT"); break;
        case REJECT: sprintf(buf, "REJECT"); break;
        case CANCEL: sprintf(buf, "CANCEL"); break;
    }

    if (send(client_fd, buf, strlen(buf), 0) < 0) {
        fprintf(stderr, "Failed to send packet (%s) to client %s\n", buf, client_id);
    }
    free(buf);
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,"Usage: %s [port] [lives] [max_players]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    int lives = atoi(argv[2]);
    int max_players = atoi(argv[3]);

    // Declare global variables "game" and "clients:
    game = create_shared_memory(sizeof(GLOBALVAR));
    game->players = 0;
    game->start_lives = lives;
    game->max_players = max_players;
    game->status = WAITING;
    clients = create_shared_memory(game->max_players * sizeof(CLIENTVAR));

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

    printf("Server created on port %d\n", port);

    // Enters the infinite connection handling loop
    connection_handler();
    close(server_fd);
    exit(EXIT_SUCCESS);
}
