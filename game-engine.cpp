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

        int timeout;
        if (gameplay_started) {
            begin_turn();
            timeout = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(game_params.turn_duration));
        }
        else {
            timeout = -1;
        }

        
        int poll_status = tcp_handler.poll_exec(timeout);

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

        if (gameplay_started)
            summarise_turn();
    }

    tcp_handler.close_conn();
}

void Game_engine::begin_turn() {
    current_turn++;
    turns.push_back(Turn_info(current_turn));

    // handle explosions for bombs
    for (auto &bomb : bombs) {
        bomb.second.timer--;

        if (bomb.second.timer == 0) {
            handle_explosion(bomb.first, bomb.second);
        }
    }

    // erase bombs
    for (auto bomb_it = bombs.cbegin(); bomb_it != bombs.cend();) {
        if (bomb_it->second.timer == 0)
            bombs.erase(bomb_it++);
        else
            ++bomb_it;    
    }

    // handle blocks destruction
    for (auto pos : current_blocks_to_erase) {
        blocks_positions.erase(pos);
    }
}

void Game_engine::handle_explosion(bomb_id id, bomb_t bomb) {
    std::cout << id << bomb.position.x;
    std::vector<player_id> robots_destroyed;
    std::vector<position_t> blocks_destroyed;
    bool was_block = false;
    bool go_up = true;
    bool go_down = true;
    bool go_left = true;
    bool go_right = true;

    for (auto &player : players_positions) {
        if (player.second == bomb.position) {
            robots_destroyed.push_back(player.first);
            current_players_to_destroy.insert(player.first);
        }
    }
    if (blocks_positions.count(bomb.position) == 1) {
        blocks_destroyed.push_back(bomb.position);
        current_blocks_to_erase.push_back(bomb.position);
        was_block = true;
    }
    if (!was_block) {
        uint16_t x = bomb.position.x;
        uint16_t y = bomb.position.y;

        for (uint16_t i = 1; i <= game_params.explosion_radius; i++) {
            if (go_down) {
                position_t down = position_t(x, y - i);
                if (check_position(down)) {
                    for (auto &player : players_positions) {
                        if (player.second == down) {
                            robots_destroyed.push_back(player.first);
                            current_players_to_destroy.insert(player.first);
                        }
                    }
                    if (blocks_positions.count(down) == 1) {
                        blocks_destroyed.push_back(down);
                        current_blocks_to_erase.push_back(down);
                        go_down = false;
                    }
                }
                else {
                    go_down = false;
                }    
            }

            if (go_up) {
                position_t up = position_t(x, y + i);
                if (check_position(up)) {
                    for (auto &player : players_positions) {
                        if (player.second == up) {
                            robots_destroyed.push_back(player.first);
                            current_players_to_destroy.insert(player.first);
                        }
                    }
                    if (blocks_positions.count(up) == 1) {
                        blocks_destroyed.push_back(up);
                        current_blocks_to_erase.push_back(up);
                        go_up = false;
                    }
                }
                else {
                    go_up = false;
                }    
            }

            if (go_left) {
                position_t left = position_t(x - i, y);
                if (check_position(left)) {
                    for (auto &player : players_positions) {
                        if (player.second == left) {
                            current_players_to_destroy.insert(player.first);
                            robots_destroyed.push_back(player.first);
                        }
                    }
                    if (blocks_positions.count(left) == 1) {
                        blocks_destroyed.push_back(left);
                        current_blocks_to_erase.push_back(left);
                        go_left = false;
                    }
                }
                else {
                    go_left = false;
                }    
            }

            if (go_right) {
                const position_t right = position_t(x + i, y);
                if (check_position(right)) {
                    for (auto &player : players_positions) {
                        if (player.second == right) {
                            current_players_to_destroy.insert(player.first);
                            robots_destroyed.push_back(player.first);
                        }
                    }
                    if (blocks_positions.count(right) == 1) {
                        blocks_destroyed.push_back(right);
                        current_blocks_to_erase.push_back(right);
                        go_right = false;
                    }
                }
                else {
                    go_right = false;
                }    
            }
        }
    }

    std::shared_ptr<Event> event(new Bomb_exploded_event(id, robots_destroyed, blocks_destroyed));
    turns.at(current_turn).add_event(event);
}

void Game_engine::summarise_turn() {
    // respawn robots
    for (auto p_id : current_players_to_destroy) {
        scores.at(p_id) += 1;
        seed_player(p_id);
    }

    tcp_handler.send_message_to_all(turns.at(current_turn).to_bytes());
    std::cout << "Wysłano do wszystkich Turn\n";


    current_blocks_to_erase.clear();
    current_players_to_destroy.clear();

    if (current_turn == game_params.game_length)
        end_gameplay();
}

void Game_engine::end_gameplay() {
    // send Game_ended message
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);
    msg->push_back(uint8_to_byte(Server_message_code::GameEnded));
    insert_vector(msg, score_map_to_bytes(scores));
    tcp_handler.send_message_to_all(*msg);

    // clear fields
    gameplay_started = false;
    players.clear();
    scores.clear();
    players_positions.clear();
    blocks_positions.clear();
    player_ids.clear();
    next_player_id = 0;
    current_turn = 0;
    turns.clear();
    bombs.clear();
    next_bomb_id = 0;


}

Message_recv_status Game_engine::handle_msg(size_t i) {
    Message_recv_status status = tcp_handler.read_from(i);
    std::shared_ptr<Client_message> msg;
    
    do {
        if (status != Message_recv_status::POTENTIALLY_SUCCESS) {
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
    engine.scores.insert(std::pair(id, 0));


    engine.tcp_handler.send_message_to_all(*(engine.accepted_player_msg(id, player)));
    std::cout << "Wysłano do wszystkich klientów Accepted_client\n";

    if (engine.players.size() == engine.game_params.players_count)
        engine.start_gameplay();
}

void Game_engine::Place_bomb_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Place_bomb od klienta " << i << "\n";

    player_id p_id;
    try {
        p_id = engine.player_ids.at(i);
    }
    catch (std::out_of_range& e) {
        std::cout << "Ten klient nie jest graczem\n";
        return;
    }
    if (engine.current_players_to_destroy.count(p_id) == 1) {
        std::cout << "Gracz " << i << "został zniszczony w tej turze i nie może dokonać ruchu.\n";
        return;
    }


    bomb_id id = engine.next_bomb_id;
    engine.next_bomb_id++;

    position_t position = engine.players_positions.at(p_id);

    bomb_t new_bomb(position, engine.game_params.bomb_timer);

    engine.bombs.insert(std::pair(id, new_bomb));

    std::shared_ptr<Event> event(new Bomb_placed_event(id, position));
    engine.turns.at(engine.current_turn).add_event(event);

}

void Game_engine::Place_block_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Place_block od klienta " << i << "\n";

    player_id p_id;
    try {
        p_id = engine.player_ids.at(i);
    }
    catch (std::out_of_range& e) {
        std::cout << "Ten klient nie jest graczem\n";
        return;
    }

    if (engine.current_players_to_destroy.count(p_id) == 1) {
        std::cout << "Gracz " << i << "został zniszczony w tej turze i nie może dokonać ruchu.\n";
        return;
    }

    position_t position = engine.players_positions.at(p_id);

    engine.blocks_positions.insert(position);

    std::shared_ptr<Event> event(new Block_placed_event(position));
    engine.turns.at(engine.current_turn).add_event(event);
}

void Game_engine::Move_msg::handle_msg(size_t i) {
    std::cout << "Odebrano Move_msg od klienta " << i << "\n";

    player_id p_id;
    try {
        p_id = engine.player_ids.at(i);
    }
    catch (std::out_of_range& e) {
        std::cout << "Ten klient nie jest graczem\n";
        return;
    }

    if (engine.current_players_to_destroy.count(p_id) == 1) {
        std::cout << "Gracz " << i << "został zniszczony w tej turze i nie może dokonać ruchu.\n";
        return;
    }

    position_t position = engine.players_positions.at(p_id);
    position_t new_position;

    switch (direction) {
    case Direction::Up:
        new_position = position_t(position.x, position.y + 1);
        break;
    case Direction::Down:
        new_position = position_t(position.x, position.y - 1);
        break;
    case Direction::Left:
        new_position = position_t(position.x - 1, position.y);
        break;
    case Direction::Right:
        new_position = position_t(position.x + 1, position.y);
        break;
    default:
        break;
    }

    if (!engine.check_position_to_move(new_position)) {
        std::cout << "Niedozwolony ruch dla gracza" << i << "\n";
        return;
    }

    engine.players_positions.insert_or_assign(p_id, new_position);

    std::shared_ptr<Event> event(new Player_moved_event(p_id, new_position));
    engine.turns.at(engine.current_turn).add_event(event);
}

bool Game_engine::check_position(position_t pos) {
    return pos.x < game_params.size_x && pos.y < game_params.size_y;
}

bool Game_engine::check_position_to_move(position_t pos) {
    return check_position(pos) && blocks_positions.count(pos) == 0;
}

void Game_engine::start_gameplay() {
    std::cout << "Rozpoczęto rozgrywkę\n";

    gameplay_started = true;

    turns.push_back(Turn_info(0));

    tcp_handler.send_message_to_all(game_started_msg());

    seed_players();

    seed_blocks();
}

std::vector<std::byte> Game_engine::game_started_msg() {
    std::shared_ptr<std::vector<std::byte>> msg(new std::vector<std::byte>);

    msg->push_back(uint8_to_byte(Server_message_code::GameStarted));

    insert_vector(msg, player_map_to_bytes(players));

    return *msg;
}

void Game_engine::seed_player(player_id id) {
    uint16_t x = (uint16_t)(random.next() % (uint32_t) game_params.size_x);
    uint16_t y = (uint16_t)(random.next() % (uint32_t) game_params.size_y);
    position_t position(x, y);

    std::shared_ptr<Event> player_moved_event(new Player_moved_event(id, position));
    turns.at(current_turn).add_event(player_moved_event);
    players_positions.insert_or_assign(id, position);

    std::cout << "Ustawiono robota gracza o id" << id << " na pozycji (" << x << ", " << y << ")\n"; 
}

void Game_engine::seed_players() {
    for (auto &player : players) {
        seed_player(player.first);
    }
}

void Game_engine::seed_blocks() {
    for (uint16_t i = 0; i < game_params.initial_blocks; i++) {
        uint16_t x = (uint16_t)(random.next() % (uint32_t) game_params.size_x);
        uint16_t y = (uint16_t)(random.next() % (uint32_t) game_params.size_y);
        position_t position(x,y);
        std::shared_ptr<Event> block_placed_event(new Block_placed_event(position));
        turns.at(0).add_event(block_placed_event);
        blocks_positions.insert(position);  

        std::cout << "Ustawiono blok na pozycji (" << x << ", " << y << ")\n"; 
      
    }
}