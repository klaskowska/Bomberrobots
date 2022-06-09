#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <cstdint>
#include <string>

#include "server-tcp-handler.h"
#include "byte-parser.h"
#include "server-parser.h"


static bool finish_server;

class Game_engine {
private:
    static const size_t buf_size = 256;
    std::byte buf[buf_size];

    server_parameters_t game_params;
    Server_tcp_handler tcp_handler;

void manage_connections();


Message_recv_status handle_msg(size_t i);

void handle_join();

void handle_hello(size_t i);

public:
    Game_engine(server_parameters_t game_params, Server_tcp_handler &tcp_handler) 
        : game_params(game_params), tcp_handler(tcp_handler) {
            finish_server = false;
        }

    void start_game();    

};

#endif