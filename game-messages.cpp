#include "game-messages.h"

bool operator<(const position_t& first, const position_t& second) {
    return (first.x < second.y) || (first.x == second.x && first.y < second.y);
}

bool operator==(const position_t& first, const position_t& second) {
    return first.x == second.x && first.y == second.y;
}

bool operator<(const bomb_t& first, const bomb_t& second) {
    return (first.position < second.position) || (first.position == second.position && first.timer < second.timer);
}

Game_info::Game_info(std::string server_name, uint16_t size_x, uint16_t size_y, uint16_t game_length, 
    std::map<player_id, player_t> _players, uint16_t bomb_timer, std::map<player_id, score> scores) :
    server_name{server_name}, size_x{size_x}, size_y{size_y}, game_length{game_length}, 
    bomb_timer {bomb_timer}, scores{scores} {
    
    players = _players;

    for (auto score : this->scores) {
        if (players.count(score.first) == 0) {
            this->scores.erase(score.first);
        }
    }

    for (auto player : players) {
        if (this->scores.count(player.first) == 0) {
            std::cout << "iii\n";
            this->scores.insert(std::pair(player.first, 0));
        }
    }
}