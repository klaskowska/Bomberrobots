#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <cstdint>
#include <string>

#include "server-tcp-handler.h"
#include "event.h"
#include "server-parser.h"
#include "random.h"


static bool finish_server;

class Game_engine {
private:
    static const size_t buf_size = 256;

    std::byte buf[buf_size];

    server_parameters_t game_params;

    Server_tcp_handler tcp_handler;

    bool gameplay_started;

    std::map<player_id, player_t> players;

    std::map<player_id, score> scores;

    std::map<player_id, position_t> players_positions;

    std::set<position_t> blocks_positions;

    // <position in descriptors arr, player_id>
    std::map<size_t, player_id> player_ids;

    player_id next_player_id;

    uint16_t current_turn;

    std::vector<position_t> current_blocks_to_erase;

    std::set<player_id> current_players_to_destroy;

    std::vector<Turn_info> turns;

    std::map<bomb_id, bomb_t> bombs;

    bomb_id next_bomb_id;

    Random random;



    void manage_connections();

    Message_recv_status handle_msg(size_t i);

    void send_hello(size_t i);

    void send_hello_state(size_t i);

    void send_game_started(size_t i);

    void send_turn(size_t i, Turn_info turn);

    void send_all_turns(size_t i);

    std::shared_ptr<std::vector<std::byte>> accepted_player_msg(player_id id, player_t player);

    void send_accepted_player(size_t i, player_id id, player_t player);

    void send_all_accepted_player(size_t i);

    void start_gameplay();

    std::vector<std::byte> game_started_msg();

    void seed_player(player_id id);

    void seed_players();

    void seed_blocks();

    // returns if position is on the board
    bool check_position(position_t position);

    // returns if player can walk into this position
    bool check_position_to_move(position_t position);

    void summarise_turn();

    void end_gameplay();

    void begin_turn();

    void handle_explosion(bomb_id id, bomb_t bomb);



    class Client_message {
    protected:
        Client_message_code code;
        
    public:
        Client_message_code get_code() {return code;}

        virtual void handle_msg(size_t i) = 0;
    };

    class Join_msg : public Client_message {
    private:
        Game_engine& engine;
        std::string name;
    public:
        Join_msg(Game_engine& engine, std::string name) : engine(engine), name(name) {
            code = Client_message_code::Join;
        }

        std::string get_name() {return name;}

        void handle_msg(size_t i);
    };


    class Place_bomb_msg : public Client_message {
    private:
        Game_engine& engine;
    public:
        Place_bomb_msg(Game_engine& engine) : engine(engine) {
            code = Client_message_code::Place_bomb_client;
        }

        void handle_msg(size_t i);
    };

    class Place_block_msg : public Client_message {
    private:
        Game_engine& engine;
    public:
        Place_block_msg(Game_engine& engine) : engine(engine) {
            code = Client_message_code::Place_block_client;
        }

        void handle_msg(size_t i);
    };

    class Move_msg : public Client_message {
    private:
        Game_engine& engine;
        Direction direction;
    public:
        Move_msg(Game_engine& engine, Direction direction) : engine(engine), direction(direction) {
            code = Client_message_code::Move_client;
        }

        Direction get_direction() {return direction;}

        void handle_msg(size_t i);
    };

public:
    Game_engine(server_parameters_t game_params, Server_tcp_handler &tcp_handler) 
        : game_params(game_params), tcp_handler(tcp_handler), random(Random(game_params.seed)) {
            finish_server = false;
            gameplay_started = false;
            next_player_id = 0;
            next_bomb_id = 0;
            current_turn = 0;
        }

    void start_game();    

};

#endif