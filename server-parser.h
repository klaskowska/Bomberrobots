#ifndef SERVER_PARSER_H
#define SERVER_PARSER_H


#include <boost/program_options.hpp>
#include <chrono>
#include "error-handler.h"

namespace po = boost::program_options;

typedef struct server_parameters {
    uint16_t bomb_timer;
    uint8_t players_count;
    uint64_t turn_duration;
    uint16_t explosion_radius;
    uint16_t initial_blocks;
    uint16_t game_length;
    std::string server_name;
    uint16_t port;
    uint32_t seed;
    uint16_t size_x;
    uint16_t size_y;
} server_parameters_t;

class Parser {
public:
    Parser() = default;

    server_parameters_t parse_parameters(int ac, const char* av[]);

};

#endif