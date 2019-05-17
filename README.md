# Multi-Client-Server
<b>Authors:</b> Bruce How ([22242664](https://github.com/brucehow/))

This application is simple RNG Battle Royale game (server) that is coded in C and is based on the fundamentals of socket programming. It is apart of the 2019 CITS3002 Computer Networks Project.

## Installation
Use the provided `Makefile` to compile the program. This will create an executable `server` file which will run the program.
```bash
make install
./server
```

## Usage
The program can be ran with or without parameters. There are 3 optional parameters which can be changed when running the program.
* `-p` to set the port number. If not specified, the port is `4444`.
* `-l` to set the starting number of lives for all players. This must be a value between 1 and 99 inclusive. If not specified, the starting number of lives for all players is 3.
* `-m` to specify the maximum number of players allowed in the game. Must be a value between 4 and 99 inclusive. If not specified, the maximum number of players is 10.

**Example**
```bash
./server -p 8888 -m 50 -l 10
```

## Implementations
All four tiers in the project specification have been implemented into this program with the exception of tier 1's single player gameplay (>= 4 players required). Below is a breakdown on this program's implementations in tiers.

### Tier 1 - If everything works as you expect
 The program is able to receive client connections and send game initialisation messages. Clients are updated when the game has started. The server is able to receive moves from players and has a 3 seconds timeout; if a player fails to make a move, they lose a life. The game-state is updated based on the moves from all players and the server is able to gracefully tear down when a game has finished.

### Tier 2 - Scaling up
The game will only start if there are at least 4 players in the lobby. Players are also only updated about their own gate state (e.g PASS, FAIL etc.).

### Tier 3 - Connection issues
Players that exit mid-game are automatically eliminated from the server. If the winning player or all remaining players exit mid-game, there are no winners. Players attempting to join mid-game are automatically rejected and disconnected.

### Tier 4 - Game logic extended
Players who send incorrectly formed move packets are eliminated from the game. The server is also able to identify cheaters, to a certain extent. Players are eliminated on the following conditions:
* **Client ID Spoofing** - If a player sends a packet with a client ID that differs to their assigned ID.
* **Impossible Packets** - If a player sends a packet that is 'valid' but impossible (e.g *'100,MOVE,CON,9'*).

The game will only start when either one of these conditions are met:
* **Full Lobby** - If the number of players waiting for the game to start is equal to the maximum number of players allowed.
* **Waiting Period** - If the waiting period has passed and there are at least 4 players waiting for the game to start. The waiting period is set to 30 seconds by default, and increases to 60 seconds if the maximum players allowed is >= 45. This allows sufficient time for the players to join.
