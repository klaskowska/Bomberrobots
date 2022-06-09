#include "game-engine.h"

void Game_engine::start_game() {
    tcp_handler.start_listening();

    manage_connections();
}

void Game_engine::manage_connections() {
    while (true) {
        tcp_handler.reset_revents();

        std::this_thread::sleep_for(std::chrono::milliseconds(game_params.turn_duration));

        int poll_status = tcp_handler.poll_exec();
        if (poll_status == -1 ) {
            exit_with_msg("Błąd w połączeniu.\n");            
        } 
        else if (poll_status > 0) {
            if (tcp_handler.is_new_connection()) {
                if (int i = tcp_handler.accept_client() >= 0) {
                    handle_hello(i);
                }
            }
            for (size_t i = 1; i < tcp_handler.get_conn_max(); ++i) {
                if (tcp_handler.is_message_from(i)) {

                    Message_recv_status status = handle_msg(i);

                    tcp_handler.report_msg_status(status, i);
                }
            }
        } else {
            printf("%ld milliseconds passed without any events\n", game_params.turn_duration);
        }
    }
}

//TODO
Message_recv_status Game_engine::handle_msg(size_t i) {

    Message_recv message_recv = tcp_handler.read_msg_from(i);

    if (message_recv.status != Message_recv_status::SUCCESS) {

        // TODO: erase client from clients/players

        return message_recv.status;
    }

    switch (message_recv.client_message.get_code()) {
        // TODO
        case Client_message_code::Join:
            handle_join();
            break;
        case Client_message_code::Place_bomb_client:
            break;
        case Client_message_code::Place_block_client:
            break;
        case Client_message_code::Move_client:
            break;
    }

    // TODO: handle specific message


    return message_recv.status;
}

// TODO
void Game_engine::handle_join() {

}

void Game_engine::handle_hello(size_t i) {
    std::shared_ptr<std::vector<std::byte>> msg;

    // prepare message
    msg->push_back(uint8_to_byte(Server_message_code::Hello));
    insert_vector(msg, string_to_bytes(game_params.server_name));
    msg->push_back(uint8_to_byte(game_params.players_count));
    insert_vector(msg, uint16_to_bytes(game_params.size_x));
    insert_vector(msg, uint16_to_bytes(game_params.size_y));
    insert_vector(msg, uint16_to_bytes(game_params.game_length));
    insert_vector(msg, uint16_to_bytes(game_params.explosion_radius));
    insert_vector(msg, uint16_to_bytes(game_params.bomb_timer));

    // send message
    tcp_handler.send_message(i, *msg);

}