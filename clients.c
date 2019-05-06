#include "server.h"

void victory_client(int index) {
    send_packet(VICT, clients[index].client_fd, clients[index].client_id);

    close(clients[index].client_fd);

    printf("\nWinner! Client %s has won!\n", clients[index].client_id);
}

void eliminate_client(int index) {
    send_packet(ELIM, clients[index].client_fd, clients[index].client_id);

    close(clients[index].client_fd);

    clients[index].client_fd = -1;
    game->players--;
    printf("Client %s has been eliminated\n", clients[index].client_id);
}

void remove_client(int index) {
    close(clients[index].client_fd);

    clients[index].client_fd = -1;
    game->players--;

    printf("Client %s has prematurely disconnected (%d/%d)\n", clients[index].client_id, game->players, game->max_players);
}

int add_client(int client_fd) {
    // Check for previously removed clients (if any)
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd == -1) {
            clients[i].client_fd = client_fd;
            clients[i].lives = game->start_lives;
            game->players++;
            send_packet(WELCOME, client_fd, clients[i].client_id);
            printf("New client %s has connected (%d/%d)\n", clients[i].client_id, game->players, game->max_players);
            return i;
        }
    }
    // Adding a new client
    int i = game->players;
    sprintf(clients[i].client_id, "%03d", game->players);
    clients[i].client_fd = client_fd;
    clients[i].lives = game->start_lives;
    game->players++;

    send_packet(WELCOME, client_fd, clients[i].client_id);
    printf("New client %s has connected (%d/%d)\n", clients[i].client_id, game->players, game->max_players);

    // Index of new client
    return i;
}
