#include "game-engine.h"

void catch_int(int sig) {
    finish_server = true;
    fprintf(stderr,
            "Złapano sygnał %d. Koniec pracy serwera.\n", sig);
}

void install_signal_handler(int signal, void (*handler)(int), int flags) {
    struct sigaction action;
    sigset_t block_mask;

    sigemptyset(&block_mask);
    action.sa_handler = handler;
    action.sa_mask = block_mask;
    action.sa_flags = flags;

    CHECK_ERRNO(sigaction(signal, &action, NULL));
}

void Game_engine::start_game() {

    install_signal_handler(SIGINT, catch_int, SA_RESTART);

    tcp_handler.start_listening();

    manage_connections();
}

void Game_engine::manage_connections() {
    while (!finish_server) {
        tcp_handler.reset_revents();

        std::this_thread::sleep_for(std::chrono::milliseconds(game_params.turn_duration));

        int poll_status = tcp_handler.poll_exec();
        if (poll_status == -1 ) {
            exit_with_msg("Błąd w połączeniu.\n");            
        } 
        else if (!finish_server && poll_status > 0) {
            if (tcp_handler.is_new_connection()) {
                if (int i = tcp_handler.accept_client() >= 0) {
                    send_hello(i);
                    send_hello_state(i);
                }
            }
            for (size_t i = 1; i < tcp_handler.get_conn_max(); ++i) {
                if (tcp_handler.is_message_from(i)) {

                    Message_recv_status status = handle_msg(i);

                    tcp_handler.report_msg_status(status, i);
                }
            }
        } else {
            printf("Żaden klient nie wykonał ruchu\n");
        }
    }

    tcp_handler.close_conn();
}

//TODO
Message_recv_status Game_engine::handle_msg(size_t i) {
    Message_recv_status status = tcp_handler.read_from(i);
    std::shared_ptr<Client_message> msg;
    
    do {
        if (status != Message_recv_status::POTENTIALLY_SUCCESS) {

            // TODO: erase client from clients/players

            return status;
        }
        try {

            Client_message_code code = (Client_message_code)tcp_handler.read_uint8();

            switch (code)
            {
            case Client_message_code::Join:
                msg.reset(new Join_msg(*this, tcp_handler.read_string()));
                break;
            case Client_message_code::Place_bomb_client:
                msg.reset(new Place_bomb_msg(*this));
                break;
            case Client_message_code::Place_block_client:
                msg.reset(new Place_block_msg(*this));
                break;
            case Client_message_code::Move_client:
                msg.reset(new Move_msg(*this, (Direction)tcp_handler.read_uint8()));
                break;
            default:
                throw MsgException();
            }
        } catch (MsgException &e) {
            return Message_recv_status::ERROR_MSG;
        }


    } while (!tcp_handler.is_buf_empty());

    msg->handle_msg(i);

    return Message_recv_status::SUCCESS;
}

void Game_engine::send_hello(size_t i) {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

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

    std::cout << "Wysłano Hello do klienta " << i << "\n";  

}

void Game_engine::send_hello_state(size_t i) {

    if (gameplay_started) {
        send_game_started(i);
        send_all_turns(i);
    }
    else {
        send_all_accepted_player(i);
    }
}

void Game_engine::send_game_started(size_t i) {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    // prepare message
    msg->push_back(uint8_to_byte(Server_message_code::GameStarted));
    insert_vector(msg, player_map_to_bytes(players));

    // send message
    tcp_handler.send_message(i, *msg);

    std::cout << "Wysłano GameStarted do klienta " << i << "\n";  
}

void Game_engine::send_turn(size_t i, Turn_info turn) {
    tcp_handler.send_message(i, turn.to_bytes());
    std::cout << "Wysłano turę nr " << turn.get_turn() << "do klienta " << i << "\n";

}

void Game_engine::send_all_turns(size_t i) {
    for (auto &turn : turns) {
        send_turn(i, turn);
    }
}

std::shared_ptr<std::vector<std::byte>> Game_engine::accepted_player_msg(player_id id, player_t player) {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    // prepare message
    msg->push_back(uint8_to_byte(Server_message_code::AcceptedPlayer));
    msg->push_back(uint8_to_byte(id));
    insert_vector(msg, player_to_bytes(player));

    return msg;
}

void Game_engine::send_accepted_player(size_t i, player_id id, player_t player) {
    
    tcp_handler.send_message(i, *accepted_player_msg(id, player));

    std::cout << "Wysłano AcceptedPlayer do klienta " << i << "\n"; 

}

void Game_engine::send_all_accepted_player(size_t i) {
    for (auto &player : players) {
        send_accepted_player(i, player.first, player.second);
    }
}

void Game_engine::Join_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Join od klienta " << i << "\n";

    if (engine.gameplay_started) {
        std::cout << "Gra już się zaczęła\n";
        return;
    }

    // add as a player
    player_id id = engine.next_player_id;
    engine.next_player_id++;
    player_t player (name, engine.tcp_handler.get_address(i));

    engine.player_ids.insert_or_assign(i, id);
    engine.players.insert(std::pair(id, player));


    engine.tcp_handler.send_message_to_all(*(engine.accepted_player_msg(id, player)));
    std::cout << "Wysłano do wszystkich klientów Accepted_client\n";

    if (engine.players.size() == engine.game_params.players_count)
        engine.start_gameplay();
}

void Game_engine::Place_bomb_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Place_bomb od klienta " << i << "\n";
}

void Game_engine::Place_block_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Place_block od klienta " << i << "\n";
}

void Game_engine::Move_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Move_msg od klienta " << i << "\n";
}

void Game_engine::start_gameplay() {
    std::cout << "Rozpoczęto rozgrywkę\n";
}