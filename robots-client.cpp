#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <string.h>
#include <netinet/tcp.h>
#include <boost/asio.hpp>
#include <iostream>

#include "game-messages.h"
#include "client-parser.h"
#include "byte-parser.h"

#define BUFFER_SIZE 256


boost::array<std::byte, BUFFER_SIZE> buf;
std::byte *current_buf;
size_t current_buf_length;

boost::array<std::byte, BUFFER_SIZE> buf_udp;
std::byte *current_buf_udp;
size_t current_buf_length_udp;

bool is_lobby_status;
Game_info game_info;
Lobby_info lobby_info;
client_parameters_t parameters;
std::map<player_id, score> scores;



boost::shared_ptr<tcp::socket> connect_to_server(tcp::resolver::results_type endpoints) {
    asio::io_context io_context;
    boost::shared_ptr<tcp::socket> socket(new tcp::socket(io_context));
    boost::system::error_code error = boost::asio::error::host_not_found;
    
    tcp::no_delay option(true);
	
    asio::connect(*socket, endpoints, error);

    socket->set_option(option);

    if (error)
        exit_with_msg("Nie udało się połączyć z serwerem.\n");
	else
		std::cerr << "Połączono z serwerem.\n";

    return socket;
    
}


void read_server(boost::shared_ptr<tcp::socket> server_socket) {
    boost::system::error_code error;

    current_buf_length = server_socket->read_some(boost::asio::buffer(buf), error);

    if (error)
        exit_with_msg("Błąd w połączeniu z serwerem.\n"); 
    if (current_buf_length == 0)
        exit_with_msg("Serwer wysłał wiadomość niezgodną z regułami - pusta wiadomość.\n");

    current_buf = buf.data();    
}

void current_buf_update(size_t bytes_count) {
    current_buf += bytes_count;
    current_buf_length -= bytes_count;
}

void read_if_buf_empty(boost::shared_ptr<tcp::socket> server_socket) {
    if (current_buf_length == 0)
        read_server(server_socket);
}


uint8_t read_uint8(boost::shared_ptr<tcp::socket> server_socket) {
    read_if_buf_empty(server_socket);

    uint8_t result = (uint8_t)current_buf[0];
    current_buf_update(1);
    return result;
}

uint16_t read_uint16(boost::shared_ptr<tcp::socket> server_socket) {
    uint16_t result;

    read_if_buf_empty(server_socket);

    if (current_buf_length >= 2) {
        // result = ntohs(*(uint16_t*)(std::byte*)(current_buf));
        memcpy(&result, current_buf, sizeof result);
        result = ntohs(result);
        current_buf_update(2);
    }
    else {
        std::byte uint16_bytes[2];

        for (size_t i = 0; i < 2; i--) {
            read_if_buf_empty(server_socket);
            uint16_bytes[i] = current_buf[0];
            current_buf_update(1);
        }

        result = ntohs(*(uint16_t*)(std::byte*)(uint16_bytes));
    }

    std::cout << result << "read u16\n";
    return result;
}

uint32_t read_uint32(boost::shared_ptr<tcp::socket> server_socket) {
    uint32_t result;

    read_if_buf_empty(server_socket);

    if (current_buf_length >= 4) {
        result = ntohl(*(uint32_t*)(std::byte*)(current_buf));
        current_buf_update(4);
    }
    else {
        std::byte uint32_bytes[4];

        for (size_t i = 0; i < 4; i++) {
            read_if_buf_empty(server_socket);
            uint32_bytes[i] = current_buf[0];
            current_buf_update(1);  
        }

        result = ntohl(*(uint32_t*)(std::byte*)(uint32_bytes));
    }

    return result;
}

std::string read_string(boost::shared_ptr<tcp::socket> server_socket) {
    uint8_t str_length = read_uint8(server_socket);

    std::string str;

    size_t read_bytes = 0;
    while (read_bytes < str_length) {
        read_if_buf_empty(server_socket);
        size_t to_read = current_buf_length >= str_length ? str_length : current_buf_length;
        str.append(reinterpret_cast<const char *>(current_buf), to_read);
        read_bytes += to_read;
        current_buf_update(to_read);
    }

	return str;
}

player_t read_player(boost::shared_ptr<tcp::socket> server_socket) {
    player_t player;

    player.name = read_string(server_socket);
    player.address = read_string(server_socket);

    return player;
}

position_t read_position(boost::shared_ptr<tcp::socket> server_socket) {
    position_t position{read_uint16(server_socket), read_uint16(server_socket)};

    return position;
}

std::map<player_id, player_t> read_players_map(boost::shared_ptr<tcp::socket> server_socket) {
    std::map<player_id, player_t> players;

    uint32_t map_size = read_uint32(server_socket);

    for (uint32_t i = 0; i < map_size; i++) {
        player_id key = read_uint8(server_socket);
        player_t value = read_player(server_socket);

        players.insert(std::pair(key, value));
    }

    return players;
}

std::map<player_id, score> read_scores_map(boost::shared_ptr<tcp::socket> server_socket) {
    std::map<player_id, score> scores;

    uint32_t map_size = read_uint32(server_socket);

    for (uint32_t i = 0; i < map_size; i++) {
        player_id key = read_uint8(server_socket);
        uint32_t value = read_uint32(server_socket);

        scores.insert(std::pair(key, value));
    }

    return scores;
}

std::set<player_id> read_player_id_list(boost::shared_ptr<tcp::socket> server_socket) {
    std::set<player_id> player_ids;
    uint32_t list_size = read_uint32(server_socket);

    for (uint32_t i = 0; i < list_size; i++) {
        player_ids.insert(read_uint8(server_socket));
    }

    return player_ids;
}

std::set<position_t> read_position_list(boost::shared_ptr<tcp::socket> server_socket) {
    std::set<position_t> positions;
    uint32_t list_size = read_uint32(server_socket);

    std::cout << "List size " << list_size << "\n";

    for (uint32_t i = 0; i < list_size; i++) {
        positions.insert(read_position(server_socket));
    }

    return positions;
}

void read_bomb_placed(boost::shared_ptr<tcp::socket> server_socket) {
    bomb_id id = read_uint32(server_socket);
    position_t position = read_position(server_socket);

    game_info.add_bomb(id, position);
}

void read_bomb_exploded(boost::shared_ptr<tcp::socket> server_socket) {
    bomb_id id = read_uint32(server_socket);
    game_info.remove_bomb(id);

    std::set<player_id> robots_destroyed = read_player_id_list(server_socket);
    for (auto robot : robots_destroyed) {
        game_info.destroy_player(robot);
    }

    std::set<position_t> blocks_destroyed = read_position_list(server_socket);
    for (auto block : blocks_destroyed) {
        game_info.remove_block(block);
    }
}

void read_player_moved(boost::shared_ptr<tcp::socket> server_socket) {
    std::cout << "Read Player Moved\n";
    uint8_t id = read_uint8(server_socket);
    std::cout << (char)id << "\n";
    position_t pos = read_position(server_socket);
    std::cout << pos.x << ", " << pos.y << "\n";
    game_info.change_player_position(id, pos);
}

void read_block_placed(boost::shared_ptr<tcp::socket> server_socket) {
    game_info.add_block(read_position(server_socket));
}

void read_event(boost::shared_ptr<tcp::socket> server_socket) {
    std::cout << "Read Event\n";
    Event_code event_code = (Event_code) read_uint8(server_socket);

    switch (event_code)
    {
    case Event_code::Bomb_placed:
        std::cout << "Read Bomb placed\n";
        read_bomb_placed(server_socket);
        break;
    case Event_code::Bomb_exploded:
        std::cout << "Read Bomb exploded\n";
        read_bomb_exploded(server_socket);
        break;
    case Event_code::Player_moved:
        std::cout << "Read Player Moved\n";
        read_player_moved(server_socket);
        break;
    case Event_code::Block_placed:
        std::cout << "Read Block placed\n";
        read_block_placed(server_socket);
        break;
    default:
        exit_with_msg("Błąd w wiadomości od serwera: Nieprawidłowy kod wydarzenia.");
    }

}

void read_event_list(boost::shared_ptr<tcp::socket> server_socket) {
    uint32_t list_size = read_uint32(server_socket);

    for (uint32_t i = 0; i < list_size; i++) {
        read_event(server_socket);
    }
}

void read_hello(boost::shared_ptr<tcp::socket> server_socket) {
    std::string server_name = read_string(server_socket);

    uint8_t players_count = read_uint8(server_socket);
    
    uint16_t size_x = read_uint16(server_socket);

    uint16_t size_y = read_uint16(server_socket);

    uint16_t game_length = read_uint16(server_socket);

    uint16_t explosion_radius = read_uint16(server_socket);

    uint16_t bomb_timer = read_uint16(server_socket);

    lobby_info = Lobby_info(server_name, players_count, size_x, size_y, game_length, explosion_radius, bomb_timer);


}

void read_accepted_player(boost::shared_ptr<tcp::socket> server_socket) {
    player_id player_id = read_uint8(server_socket);

    player_t player;
    player.name = read_string(server_socket);
    player.address = read_string(server_socket);

    lobby_info.add_player(player_id, player);
}

void read_game_started(boost::shared_ptr<tcp::socket> server_socket) {
    game_info = Game_info(lobby_info.get_server_name(), lobby_info.get_size_x(), lobby_info.get_size_y(),
        lobby_info.get_game_length(), lobby_info.get_players(), lobby_info.get_bomb_timer(), scores);
    read_players_map(server_socket);
}

void read_turn(boost::shared_ptr<tcp::socket> server_socket) {
    game_info.new_turn(read_uint16(server_socket));

    read_event_list(server_socket);
}

void read_game_ended(boost::shared_ptr<tcp::socket> server_socket) {
    read_scores_map(server_socket);
}

void send_gui_message(boost::shared_ptr<udp::socket> socket, udp::endpoint endpoint, std::vector<std::byte> message);
std::vector<std::byte> lobby_message();
std::vector<std::byte> game_message();

void handle_hello_msg(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    read_hello(server_socket);

    send_gui_message(gui_socket, endpoint, lobby_message());

    is_lobby_status = true;
}

void handle_accepted_player_msg(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    read_accepted_player(server_socket);
    send_gui_message(gui_socket, endpoint, lobby_message());
}

void handle_game_started_msg(boost::shared_ptr<tcp::socket> server_socket) {
    read_game_started(server_socket);

    is_lobby_status = false;
}

void handle_turn_message(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    read_turn(server_socket);
    send_gui_message(gui_socket, endpoint, game_message());
}

void handle_game_ended_msg(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    read_game_ended(server_socket);

    lobby_info.clear_players();
    is_lobby_status = true;

    send_gui_message(gui_socket, endpoint, lobby_message());
}

void handle_server_message(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    
    uint8_t msg_code = read_uint8(server_socket);

    switch (msg_code) {
    case Server_message_code::Hello:
        handle_hello_msg(server_socket, gui_socket, endpoint);
        std::cerr << "Received Hello from server.\n";
        break;
    case Server_message_code::AcceptedPlayer:
        handle_accepted_player_msg(server_socket, gui_socket, endpoint);
        std::cerr << "Received AcceptedPlayer from server.\n";
        break;
    case Server_message_code::GameStarted:
        handle_game_started_msg(server_socket);
        std::cerr << "Received GameStarted from server.\n";
        break;
    case Server_message_code::Turn:
        handle_turn_message(server_socket, gui_socket, endpoint);
        std::cerr << "Received Turn from server.\n";
        break;
    case Server_message_code::GameEnded:
        handle_game_ended_msg(server_socket, gui_socket, endpoint);
        std::cerr << "Received GameEnded from server.\n";
        break;
    
    default:
        exit_with_msg("Nieprawidłowy kod wiadomości od serwera.\n");
    }
}


boost::shared_ptr<udp::socket> open_gui_socket(uint16_t port) { 

    boost::asio::io_service io_service;

    udp::endpoint local_endpoint(udp::v6(), port);

    boost::shared_ptr<udp::socket> socket(new udp::socket(io_service, local_endpoint));

    return socket;
}



std::vector<std::byte> lobby_message() {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Draw_message_code::Lobby));

    std::vector<std::byte> server_name = string_to_bytes(lobby_info.get_server_name());
    message.insert(message.end(), server_name.begin(), server_name.end());

    message.push_back(uint8_to_byte(lobby_info.get_players_count()));

    std::vector<std::byte> size_x = uint16_to_bytes(lobby_info.get_size_x());
    message.insert(message.end(), size_x.begin(), size_x.end());
    
    std::vector<std::byte> size_y = uint16_to_bytes(lobby_info.get_size_y());
    message.insert(message.end(), size_y.begin(), size_y.end());

    std::vector<std::byte> game_length = uint16_to_bytes(lobby_info.get_game_length());
    message.insert(message.end(), game_length.begin(), game_length.end());

    std::vector<std::byte> explosion_radius = uint16_to_bytes(lobby_info.get_explosion_radius());
    message.insert(message.end(), explosion_radius.begin(), explosion_radius.end());

    std::vector<std::byte> bomb_timer = uint16_to_bytes(lobby_info.get_bomb_timer());
    message.insert(message.end(), bomb_timer.begin(), bomb_timer.end());

    std::vector<std::byte> players = player_map_to_bytes(lobby_info.get_players());
    message.insert(message.end(), players.begin(), players.end());

    return message;
}

std::vector<std::byte> game_message() {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Draw_message_code::Game));

    std::vector<std::byte> server_name = string_to_bytes(game_info.get_server_name());
    message.insert(message.end(), server_name.begin(), server_name.end());

    std::vector<std::byte> size_x = uint16_to_bytes(game_info.get_size_x());
    message.insert(message.end(), size_x.begin(), size_x.end());
    
    std::vector<std::byte> size_y = uint16_to_bytes(game_info.get_size_y());
    message.insert(message.end(), size_y.begin(), size_y.end());

    std::vector<std::byte> game_length = uint16_to_bytes(game_info.get_game_length());
    message.insert(message.end(), game_length.begin(), game_length.end());

    std::vector<std::byte> turn = uint16_to_bytes(game_info.get_turn());
    message.insert(message.end(), turn.begin(), turn.end());

    std::vector<std::byte> players = player_map_to_bytes(game_info.get_players());
    message.insert(message.end(), players.begin(), players.end());

    std::vector<std::byte> player_positions = player_position_map_to_bytes(game_info.get_player_positions());
    message.insert(message.end(), player_positions.begin(), player_positions.end());

    std::vector<std::byte> blocks = position_set_to_bytes(game_info.get_blocks());
    message.insert(message.end(), blocks.begin(), blocks.end());

    std::vector<std::byte> bombs = bomb_set_to_bytes(game_info.get_bombs());
    message.insert(message.end(), bombs.begin(), bombs.end());

    std::vector<std::byte> explosions = position_set_to_bytes(game_info.get_explosions());
    message.insert(message.end(), explosions.begin(), explosions.end());

    std::vector<std::byte> scores = score_map_to_bytes(game_info.get_scores());
    message.insert(message.end(), scores.begin(), scores.end());

    return message;
}

void send_gui_message(boost::shared_ptr<udp::socket> socket, udp::endpoint endpoint, std::vector<std::byte> message) {
    boost::system::error_code error;
    socket->send_to(boost::asio::buffer(message), endpoint, 0, error);

    if (error) {
        exit_with_msg("Nie udało się wysłać wiadomości do GUI.");
    }
}

void current_buf_update_udp(size_t bytes_count) {
    current_buf_udp += bytes_count;
    current_buf_length_udp -= bytes_count;
}

uint8_t read_uint8_udp() {
    uint8_t result = (uint8_t) current_buf_udp[0];
    current_buf_update_udp(1);
    return result;
}

std::vector<std::byte> join_message();
std::vector<std::byte> place_bomb_message();
std::vector<std::byte> place_block_message();
std::vector<std::byte> move_message(Direction direction);
void send_server_message(boost::shared_ptr<tcp::socket> socket, std::vector<std::byte> message);

void handle_gui_message(boost::shared_ptr<udp::socket> gui_socket, boost::shared_ptr<tcp::socket> server_socket) {
    try {
        current_buf_length_udp = gui_socket->receive(boost::asio::buffer(buf_udp));
        current_buf_udp = buf_udp.data();
    } catch (std::exception& e)
    {
        exit_with_msg("Nie udało się odebrać wiadomości od GUI.");
    }

    uint8_t msg_code = read_uint8_udp();


    switch (msg_code) {
        case Input_message_code::Place_bomb:
            if (is_lobby_status)
                send_server_message(server_socket, join_message());
            else
                send_server_message(server_socket, place_bomb_message());
            std::cerr << "Received Place_bomb from GUI.\n";
            break;
        case Input_message_code::Place_block:
            if (is_lobby_status)
                send_server_message(server_socket, join_message());
            else
                send_server_message(server_socket, place_block_message());
            std::cerr << "Received Place_block from GUI.\n";
            break;
        case Input_message_code::Move:
            if (is_lobby_status)
                send_server_message(server_socket, join_message());
            else
                send_server_message(server_socket, move_message((Direction) read_uint8_udp()));
            std::cerr << "Received Move from GUI.\n";
            break;    
        default:
            std::cerr << "Nieprawidłowy kod wiadomości od serwera.\n";
    }

}


std::vector<std::byte> join_message() {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Client_message_code::Join));

    std::vector<std::byte> name = string_to_bytes(parameters.name);
    message.insert(message.end(), name.begin(), name.end());

    return message;
}

std::vector<std::byte> place_bomb_message() {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Client_message_code::Place_bomb_client));

    return message;
}

std::vector<std::byte> place_block_message() {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Client_message_code::Place_block_client));

    return message;
}

std::vector<std::byte> move_message(Direction direction) {
    std::vector<std::byte> message;

    message.push_back(uint8_to_byte(Client_message_code::Move_client));
    
    message.push_back(uint8_to_byte(direction));

    return message;
}

void send_server_message(boost::shared_ptr<tcp::socket> socket, std::vector<std::byte> message) {
    boost::system::error_code error;
    asio::write(*socket, boost::asio::buffer(message), error);

    if (error) {
        exit_with_msg("Nie udało się wysłać wiadomości do serwera.\n");
    }

}

void handle_server_messages(boost::shared_ptr<tcp::socket> server_socket, boost::shared_ptr<udp::socket> gui_socket, udp::endpoint endpoint) {
    while(true) {
        handle_server_message(server_socket, gui_socket, endpoint);
    }
}

void handle_gui_messages(boost::shared_ptr<udp::socket> gui_socket, boost::shared_ptr<tcp::socket> server_socket) {
    while (true) {
        handle_gui_message(gui_socket, server_socket);
    }
    
}

int main(int ac, char* av[])
{
    current_buf_length = 0;

    Parser parser;
    parameters = parser.parse_parameters(ac, av);

    udp::endpoint gui_endpoint = parameters.gui_address;

    boost::shared_ptr<tcp::socket> server_socket = connect_to_server(parameters.server_address);

    boost::shared_ptr<udp::socket> gui_socket = open_gui_socket(parameters.port);




    std::thread gui_receiver{[gui_socket, server_socket]{ handle_gui_messages(gui_socket, server_socket); }};
    std::thread server_receiver{[server_socket, gui_socket, gui_endpoint]{ handle_server_messages(server_socket, gui_socket, gui_endpoint); }};

    gui_receiver.join();
    server_receiver.join();

  
    return 0;
}
