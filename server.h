/**
 * CITS3002: Networks Project
 * RNG Battle Royale game simulation utilising a client-server approach.
 *
 * Name: Bruce How (22242664)
 * Date: 07/05/2019
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <poll.h>
#include <sys/mman.h> // used for memory sharing

#define PACKET_SIZE 14
#define POLLING_RATE 30
#define OPT_LIST "p:m:l:"

/**
 * Enumerations used for consistent packet messaging and server statuses
 */
typedef enum {WAITING, PLAYING, FINISHED, EXIT} server_status;
typedef enum {WELCOME, START, PASS, FAIL, ELIM, VICT, REJECT, CANCEL, TIMEOUT} response;

/**
 * Global variables used with mmap for memory sharing
 */
typedef struct {
    char client_id[4];
    int client_fd;
    int lives;
    response result;
    char rec[PACKET_SIZE];
    char send[PACKET_SIZE];
} CLIENTVAR;

typedef struct {
    int players;
    int max_players;
    int start_lives;
    server_status status;
    int die1;
    int die2;
} GLOBALVAR;

extern CLIENTVAR *clients;
extern GLOBALVAR *game;
extern int server_fd;

/**
 * Sends a packet to a specific client based on the given response enumeration
 * @param  msg_type response enum
 * @param  index the index of the client
 */
extern void send_packet(response msg_type, int index);

/**
 * Create a variable that has a shared memory. Allows for shared access
 * between the parent and any child processes.
 * @param  size size of shared memory variable
 * @return      the shared memory
 */
extern void *create_shared_memory(size_t size);

/**
 * Allocate memory according to the required size using malloc. Checks for
 * memory allocation failures and outputs the issue accordingly.
 * @param  size the required memory space
 * @return      pointer to the memory
 */
extern void *allocate_memory(size_t items, size_t size);

/**
 * Constantly listens for new client connection and handles them appropiately
 */
extern void connection_listener();

/**
 * Adds a client to the list of existing clients. Checks for any previously
 * disconnected clients and reuses its client ID
 * @param  client_fd the client file descriptor
 * @return           the index containing the client in clients
 */
extern int add_client(int client_fd);

/**
 * Disconnects a client from the list of existing clients. Disconnected clients
 * have their client FD set to -1 allowing for it to be easily reused
 * @param client_index the index of the client to disconnect
 */
extern void disconnect_client(int index);

/**
 * Eliminates a client from the list of existing clients. Eliminated clients
 * have their client FD set to -1, and an ELIM packet sent to them
 * @param client_index the index of the client to eliminate
 */
extern void eliminate_client(int index);

/**
 * Initialises the game and calls init_round to handle each game round
 */
extern void init_game();
