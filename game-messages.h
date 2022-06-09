#ifndef GAME_MESSAGES_H
#define GAME_MESSAGES_H

#include <cinttypes>
#include <string>
#include <set>
#include <map>
#include <iostream>


using player_id = uint8_t;
using bomb_id = uint32_t;
using score = uint32_t;


enum Server_message_code : uint8_t {
    Hello = 0,
    AcceptedPlayer = 1,
    GameStarted = 2,
    Turn = 3,
    GameEnded = 4,
};

enum Draw_message_code : uint8_t {
    Lobby = 0,
    Game = 1,
};

enum Input_message_code : uint8_t {
    Place_bomb = 0,
    Place_block = 1,
    Move = 2,
};

enum Client_message_code : uint8_t {
    Join = 0,
    Place_bomb_client = 1,
    Place_block_client = 2,
    Move_client = 3,
};

enum Direction : uint8_t {
    Up = 0,
    Right = 1,
    Down = 2,
    Left = 3,
};

enum Event_code : uint8_t {
    Bomb_placed = 0,
    Bomb_exploded = 1,
    Player_moved = 2,
    Block_placed = 3,
};

typedef struct player {
    std::string name;
    std::string address;

} player_t;

typedef struct position {
    uint16_t x;
    uint16_t y;
} position_t;

bool operator<(const position_t& first, const position_t& second);

bool operator==(const position_t& first, const position_t& second);

typedef struct bomb {
    position_t position;
    uint16_t timer;
} bomb_t;

bool operator<(const bomb_t& first, const bomb_t& second);

class Lobby_info {
    public:
        Lobby_info() = default;

        Lobby_info(std::string server_name, uint8_t players_count, uint16_t size_x, uint16_t size_y, 
            uint16_t game_length, uint16_t explosion_radius, uint16_t bomb_timer) :
            server_name{server_name}, players_count{players_count}, size_x{size_x}, size_y{size_y},
            game_length{game_length}, explosion_radius{explosion_radius}, bomb_timer{bomb_timer} {}
        
        void add_player(player_id id, player_t player) {
            players.insert(std::pair(id, player));
        }

        void clear_players() {
            players.clear();
        }

        // gettery
        std::string get_server_name() {
            return server_name;
        }
        uint8_t get_players_count() {
            return players_count;
        }
        uint16_t get_size_x() {
            return size_x;
        }
        uint16_t get_size_y() {
            return size_y;
        }
        uint16_t get_game_length() {
            return game_length;
        }
        uint16_t get_explosion_radius() {
            return explosion_radius;
        }
        uint16_t get_bomb_timer() {
            return bomb_timer;
        }
        std::map<player_id, player> get_players() {
            return players;
        }
    private:
        std::string server_name;
        uint8_t players_count;
        uint16_t size_x;
        uint16_t size_y;
        uint16_t game_length;
        uint16_t explosion_radius;
        uint16_t bomb_timer;
        std::map<player_id, player_t> players;
};

class Game_info {
    private:
        std::string server_name;
        uint16_t size_x;
        uint16_t size_y;
        uint16_t game_length;
        uint16_t bomb_timer;
        uint16_t turn;
        std::map<player_id, player_t> players;
        std::map<player_id, position_t> player_positions;
        std::set<position_t> blocks;
        std::map<bomb_id, bomb_t> bombs;
        std::set<position_t> explosions;
        std::map<player_id, score> scores;
    public:
        Game_info() = default;

        Game_info(std::string server_name, uint16_t size_x, uint16_t size_y, uint16_t game_length, 
            std::map<player_id, player_t> _players, uint16_t bomb_timer, std::map<player_id, score> scores);

        void new_turn(uint16_t _turn) {
            explosions.clear();
            turn = _turn;
        }

        void add_explosion(position_t position) {
            explosions.insert(position);
        }

        void add_bomb(bomb_id id, position_t position) {
            bomb_t bomb{position, bomb_timer};
            bombs.insert(std::pair(id, bomb));
        }

        void add_block(position_t position) {
            blocks.insert(position);
        }

        void remove_bomb(bomb_id id) {
            bombs.erase(id);
        }

        void destroy_player(player_id id) {
            player_positions.erase(id);
            scores.at(id)++;
        }

        void remove_block(position_t position) {
            blocks.erase(position);
        }

        void change_player_position(player_id id, position_t position) {
            std::cout << "position\n";
            player_positions.insert_or_assign(id, position);
        }

        // gettery
        std::string get_server_name() {
            return server_name;
        }
        uint16_t get_size_x() {
            return size_x;
        }
        uint16_t get_size_y() {
            return size_y;
        }
        uint16_t get_game_length() {
            return game_length;
        }
        uint16_t get_bomb_timer() {
            return bomb_timer;
        }
        uint16_t get_turn() {
            return turn;
        }
        std::map<player_id, player> get_players() {
            return players;
        }
        std::map<player_id, position_t> get_player_positions() {
            return player_positions;
        }
        std::set<position_t> get_blocks() {
            return blocks;
        }
        std::map<bomb_id, bomb_t> get_bombs() {
            return bombs;
        }
        std::set<position_t> get_explosions() {
            return explosions;
        }
        std::map<player_id, score> get_scores() {
            return scores;
        }        
};


// added with server implementation

class Client_message {
protected:
    Client_message_code code;
public:
    Client_message_code get_code() {return code;}
};

class Join_msg : public Client_message {
private:
    std::string name;
public:
    Join_msg(std::string name) :  name(name) {
        code = Client_message_code::Join;
    }

    std::string get_name() {return name;}
};


class Place_bomb_msg : public Client_message {
public:
    Place_bomb_msg() {
        code = Client_message_code::Place_bomb_client;
    }
};

class Place_block_msg : public Client_message {
public:
    Place_block_msg() {
        code = Client_message_code::Place_block_client;
    }
};

class Move_msg : public Client_message {
private:
    Direction direction;
public:
    Move_msg(Direction direction) : direction(direction) {
        code = Client_message_code::Move_client;
    }
};


#endif