#ifndef BYTE_PARSER_H
#define BYTE_PARSER_H

#include "game-messages.h"
#include <vector>
#include <cstring>
#include <iostream>
#include <memory>

// inserts bytes from src to the end of dest
void insert_vector(std::shared_ptr<std::vector<std::byte>> dest, std::vector<std::byte> src);

std::byte uint8_to_byte(uint8_t u8);

std::vector<std::byte> uint16_to_bytes(uint16_t u16);

std::vector<std::byte> uint32_to_bytes(uint32_t u32);

std::vector<std::byte> string_to_bytes(std::string str);

std::vector<std::byte> player_to_bytes(player_t player);

std::vector<std::byte> position_to_bytes(position_t position);

std::vector<std::byte> bomb_to_bytes(bomb_t bomb);

std::vector<std::byte> position_set_to_bytes(std::set<position_t> positions);

std::vector<std::byte> bomb_set_to_bytes(std::map<bomb_id, bomb_t> bombs);

std::vector<std::byte> player_map_to_bytes(std::map<player_id, player> player_map);

std::vector<std::byte> player_position_map_to_bytes(std::map<player_id, position_t> positions);

std::vector<std::byte> score_map_to_bytes(std::map<player_id, score> scores);

std::vector<std::byte> uint8_list_to_bytes(std::vector<uint8_t> elements);

std::vector<std::byte> position_list_to_bytes(std::vector<position_t> positions);


#endif