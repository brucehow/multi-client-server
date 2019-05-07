#include "server.h"

void eliminate_client(int index) {
    send_packet(ELIM, index);
    clients[index].client_fd = -1;
    game->players--;
    printf("Client %s has been eliminated\n", clients[index].client_id);
}

void disconnect_client(int index) {
    close(clients[index].client_fd);
    clients[index].client_fd = -1;
    game->players--;
    printf("Client %s has prematurely disconnected (%d/%d)\n", clients[index].client_id, game->players, game->max_players);
}

int add_client(int client_fd) {
    for (int i = 0; i < game->max_players; i++) {
        if (clients[i].client_fd == -1) {
            sprintf(clients[i].client_id, "%03d", i);
            clients[i].client_fd = client_fd;
            clients[i].lives = game->start_lives;
            bzero(clients[i].send, PACKET_SIZE);
            bzero(clients[i].rec, PACKET_SIZE);
            game->players++;

            // Send welcome packet
            send_packet(WELCOME, i);
            if (send(client_fd, clients[i].send, strlen(clients[i].send), 0) < 0) {
                perror(__func__);
                fprintf(stderr, "Failed to send packet (%s) to new client\n", clients[i].send);
            }
            bzero(clients[i].send, PACKET_SIZE);

            printf("New client %s has connected (%d/%d)\n", clients[i].client_id, game->players, game->max_players);
            return i;
        }
    }
    fprintf(stderr, "Unable to add new client\n");
    return -1;
}
