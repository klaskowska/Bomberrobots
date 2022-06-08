#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include <cstdint>
#include <string>

#include "server-parser.h"
#include "server-tcp-handler.h"

class Game_engine {
private:
    static const size_t buf_size = 256;
    std::byte buf[buf_size];

    server_parameters_t game_params;
    Server_tcp_handler tcp_handler;

void manage_connections() {
    while (true) {
        tcp_handler.reset_revents();

        std::this_thread::sleep_for(std::chrono::milliseconds(game_params.turn_duration));

        int poll_status = tcp_handler.poll_exec();
        if (poll_status == -1 ) {
            exit_with_msg("Błąd w połączeniu.\n");            
        } 
        else if (poll_status > 0) {
            if (tcp_handler.is_new_connection()) {
                if (tcp_handler.accept_client()) {
                    // send hello
                }
            }
            for (size_t i = 1; i < tcp_handler.get_conn_max(); ++i) {
                if (tcp_handler.is_message_from(i)) {

                    Message_recv_status status = handle_msg(i);

                    tcp_handler.report_msg_status(status);
                }
            }
        } else {
            printf("%ld milliseconds passed without any events\n", game_params.turn_duration);
        }
    }
}

//TODO
Message_recv_status handle_msg(size_t i) {
    tcp_handler.set_current_client_pos(i);

    Message_recv message_recv = tcp_handler.read_msg_from();


    // TODO: handle status
    // TODO: handle specific message


    return message_recv.status;
}

public:
    Game_engine(server_parameters_t game_params, Server_tcp_handler &tcp_handler) 
        : game_params(game_params), tcp_handler(tcp_handler) {
    }

    void start_game() {
        tcp_handler.start_listening();

        manage_connections();
    }

    

};

#endif