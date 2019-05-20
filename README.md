# Multi-Client-Server &middot; [![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/brucehow/Multi-Client-Server/blob/master/LICENSE) [![mac version](https://img.shields.io/badge/macOS-10.12.6-blue.svg)]()
<b>Authors:</b> Bruce How ([22242664](https://github.com/brucehow/))

This program is simple RNG Battle Royale game (server) that is coded in C and is based on the fundamentals of socket programming.

It is apart of the 2019 CITS3002 Computer Networks Project.

## Installation
Use the provided `Makefile` to compile the program which will create an executable `server` program.
```bash
make install
```

## Usage
The program can be ran with or without parameters. There are 3 optional parameters which can be changed when running the program.
* `-p` to set the port number. If not specified, the port is `4444`.
* `-l` to set the starting number of lives for all players. This must be a value between 1 and 99 inclusive. If not specified, the starting number of lives for all players is 3.
* `-m` to specify the maximum number of players allowed in the game. Must be a value between 4 and 99 inclusive. If not specified, the maximum number of players is 10.

Please run this program on a Mac as it has only been tested on Mac environments.

**Example**
```bash
./server -p 8888 -m 50 -l 10
```

## Implementations
All four tiers in the project specification have been implemented into this program with the exception of tier 1's single player requirement as the game can only start when there are more than 4 players. Tier breakdown on this program's implementations can be found below.

### Tier 1 - If everything works as you expect
The program is able to receive client connections and send game initialisation messages. Clients are updated when the game has started as the server will send a '*START,%d,%d*' packet (%d being a 1 or 2 digit number). A unique client ID can be easily generated based on the client's index in an array. 0s are pre-padded to this number to ensure that

The server is able to receive moves from players and has a timeout value of 3 seconds; if a player fails to make a move, they lose a life. The game-state is updated based on the moves from all players and the server is able to gracefully tear down when a game has finished.

### Tier 2 - Scaling up
The game will only start if there are at least 4 players in the lobby. Packets are sent to players to update them only about their own state (e.g PASS, FAIL etc.). These packets can be modified accordingly based on the state of the game. For example, if a player loses a life but is the only remaining player, then a *VICT* packet is sent instead.

### Tier 3 - Connection issues
Players that exit mid-game are automatically eliminated from the server. If the winning player or all remaining players exit mid-game, there are no winners (the output for this scenario can be found below). Players attempting to join mid-game are automatically rejected and disconnected.

### Tier 4 - Game logic extended
Players who send incorrectly structured packets are eliminated from the game. The server is also able to identify cheaters, to a certain extent.
* **Client ID Spoofing** - If a player sends a packet with a client ID that differs to their assigned ID, they will be eliminated.
* **Impossible Packets** - If a player sends a packet that is 'valid' but impossible (e.g *'100,MOVE,CON,9'*), they will be eliminated.
* **Multi-Client** - Using multiple "dummy" clients to increase the chances of winning is useless as players are only updated on their outcomes once the round has finished, i.e i.e. when all players have made a move.

The game will only start when either one of these conditions are met:
* **Full Lobby** - If the number of players waiting for the game to start is equal to the maximum number of players allowed.
* **Waiting Period** - If the waiting period has passed and there are at least 4 players waiting for the game to start. The waiting period is set to 30 seconds by default, and increases to 60 seconds if the maximum allowed players is >= 45. This allows sufficient time for players to join.

If the game fails to start, all waiting players will be notified and their connection to the server will be gracefully closed.

### Additional Implementations
As *recv* is constantly running for each player, the server will need to take into account unexpected packets.
If a player sends an unexpected packet (e.g. whilst the round is still in progress), the server will hold onto this packet and store it appropriately once the previous packet has been processed. If a player sends more than 3 unexpected packets (defined by *MAX_PACKET_OVERFLOW*), they will be eliminated from the game. This ensures that there is a layer of protection which prevents clients from spamming packets to the server.


## Output
The program's output is displayed through print statements. The server and game configurations will be displayed once the program is run. This allows server creators to verify that their parameters are as expected. An example is shown below.
```
Configurations
Server port: 8888 (specified)
Player lives: 10 (specified)
Maximum players: 10 (default)
```
Each round is displayed in a similar format to the example provided below. Players will either make it through the round without losing a life, lose a life, and or be eliminated. The dice rolls are also displayed beside the round number, and the remaining player count is displayed at the end of every round.
```
Round 19 (Dice rolled 5 and 6 - Total 11)
Client 601 made it through (1 lives)
Client 703 lost a life (1 lives)
Client 204 lost a life (0 lives)
Client 207 made it through (3 lives)
Client 608 made it through (4 lives)
Client 204 has been eliminated
4 players remaining
```

At the end of every round, the server will check for a winner and output the appropriate message. In the case where there is a draw, all remaining clients are winners.

```
Round 8 (Dice rolled 3 and 6 - Total 9)
Client 007 lost a life (0 lives)
Client 109 lost a life (0 lives)
0 players remaining

Draw! Client 007, Client 109 have won!
```

In the edge case where all remaining players have disconnected, there will be no winners.

```
Round 81 (Dice rolled 1 and 3 - Total 4)
Client 103 has left the game
Client 701 has left the game
Client 103 has been eliminated
Client 701 has been eliminated
0 players remaining

Draw! There are no winners
```
