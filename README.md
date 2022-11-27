# Bomberrobots
Net multiplayer game - simple version of `Bomberman`.

## Game rules
The game take place on a rectangular board.  
Every player controls their robot.  
The game comprises given number of rounds.
During every round each player can:
- do nothing,
- move a robot to an adjacent field,
- put a bomb under their robot,
- put a block under their robot.  
The game is cyclic - if there is a given number of available players, the next gameplay begins.  

## Architecture
- Server
- Client
- GUI server (provided by external sources)
Server communicates with clients, manages the game state, receives from clients information about their actions, sends clients information about the game state.  
Client communicates with server and with GUI server.

## Run components: parameters
Server:
```
    -b, --bomb-timer <u16>
    -c, --players-count <u8>
    -d, --turn-duration <u64, miliseconds>
    -e, --explosion-radius <u16>
    -h, --help
    -k, --initial-blocks <u16>
    -l, --game-length <u16>
    -n, --server-name <String>
    -p, --port <u16>
    -s, --seed <u32, optional>
    -x, --size-x <u16>
    -y, --size-y <u16>
```
Client:
```
    -d, --gui-address <(host):(port) lub (IPv4):(port) lub (IPv6):(port)>
    -h, --help
    -n, --player-name <String>
    -p, --port <u16>                           Port for listening from GUI server
    -s, --server-address <(host):(port) lub (IPv4):(port) lub (IPv6):(port)>
```

## Protocol of communication between server and clients
The data is sent by TCP protocol.  
The data is in binary form.
- Binary representation of a string: 
`[string length in bytes(1 byte)][string bytes without the last zero byte]`
- Binary representation of a list:
`[list length (4 bytes)][list's elements]`
- Binary representation of a map:
`[map length (4 bytes)][key][value][key][value]...`

## Communication form
```
Event:
[0] BombPlaced { id: BombId, position: Position },
[1] BombExploded { id: BombId, robots_destroyed: List<PlayerId>, blocks_destroyed: List<Position> },
[2] PlayerMoved { id: PlayerId, position: Position },
[3] BlockPlaced { position: Position },

BombId: u32
Bomb: { position: Position, timer: u16 },
PlayerId: u8
Position: { x: u16, y: u16 }
Player: { name: String, address: String }
Score: u32
```
- From client to server
```
enum ClientMessage {
    [0] Join { name: String },
    [1] PlaceBomb,
    [2] PlaceBlock,
    [3] Move { direction: Direction },
}
```
```
enum Direction {
    [0] Up,
    [1] Right,
    [2] Down,
    [3] Left,
}
```
- From server to client
```
enum ServerMessage {
    [0] Hello {
        server_name: String,
        players_count: u8,
        size_x: u16,
        size_y: u16,
        game_length: u16,
        explosion_radius: u16,
        bomb_timer: u16,
    },
    [1] AcceptedPlayer {
        id: PlayerId,
        player: Player,
    },
    [2] GameStarted {
            players: Map<PlayerId, Player>,
    },
    [3] Turn {
            turn: u16,
            events: List<Event>,
    },
    [4] GameEnded {
            scores: Map<PlayerId, Score>,
    },
}
```
- From client to GUI server
```
enum DrawMessage {
    [0] Lobby {
        server_name: String,
        players_count: u8,
        size_x: u16,
        size_y: u16,
        game_length: u16,
        explosion_radius: u16,
        bomb_timer: u16,
        players: Map<PlayerId, Player>
    },
    [1] Game {
        server_name: String,
        size_x: u16,
        size_y: u16,
        game_length: u16,
        turn: u16,
        players: Map<PlayerId, Player>,
        player_positions: Map<PlayerId, Position>,
        blocks: List<Position>,
        bombs: List<Bomb>,
        explosions: List<Position>,
        scores: Map<PlayerId, Score>,
    },
}
```
- From GUI server to client
```
enum InputMessage {
    [0] PlaceBomb,
    [1] PlaceBlock,
    [2] Move { direction: Direction },
}
```