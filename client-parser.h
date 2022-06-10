#ifndef PARSER_H
#define PARSER_H

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include "error-handler.h"

namespace po = boost::program_options;
namespace asio = boost::asio;

using boost::lexical_cast;
using asio::ip::tcp;
using asio::ip::udp;


typedef struct client_parameters {
    udp::endpoint gui_address;
    std::string name;
    uint16_t port;
    tcp::resolver::results_type server_address;
} client_parameters_t;

typedef struct address {
    asio::ip::address ip_address;
    uint16_t port;
} address_t;

class Parser {
    private:
        size_t find_split_position(std::string str);

        tcp::resolver::results_type parse_address_tcp(std::string endpoint_str);

        udp::endpoint parse_address_udp(std::string endpoint_str) {
            size_t position = find_split_position(endpoint_str);
            if (position == std::string::npos)
                exit_with_msg("Nieprawidłowo podane parametry. 4\n");

            // check if port is in uint16
            try {
                lexical_cast<uint16_t>(endpoint_str.substr(position + 1));
            }
            catch (std::exception &e) {
                exit_with_msg("Nieprawidłowo podane parametry. 3\n");       
            }


            asio::io_service io_service;
            udp::resolver resolver(io_service);
            udp::resolver::query query(endpoint_str.substr(0, position), endpoint_str.substr(position + 1));
            udp::resolver::iterator iter = resolver.resolve(query);

            return iter->endpoint();
        }
    public:
        Parser() = default;

        client_parameters_t parse_parameters(int ac, char* av[]);

};

#endif