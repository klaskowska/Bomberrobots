#include "client-parser.h"

size_t Parser::find_split_position(std::string str) {
    size_t position = std::string::npos;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == ':')
            position = i;
    }
    return position;
}

tcp::resolver::results_type Parser::parse_address_tcp(std::string endpoint_str) {
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
            boost::system::error_code error;
            asio::io_service io_service;
            tcp::resolver resolver(io_service);
            
            tcp::resolver::results_type results = resolver.resolve(endpoint_str.substr(0, position), endpoint_str.substr(position + 1), error);

            return results;
        }

client_parameters_t Parser::parse_parameters(int ac, char* av[]) {
	
            client_parameters_t parameters;
            
            try {
                
                po::options_description description("Allowed options");
                
                description.add_options()
                    ("gui-address,d", po::value<std::string>(), "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>")
                    ("help,h", "Wypisuje jak używać programu")
                    ("player-name,n", po::value<std::string>(&parameters.name), "<String>")
                    ("port,p", po::value<uint16_t>(&parameters.port), "<u16>")
                    ("server-address,s", po::value<std::string>(), "<(nazwa hosta):(port) lub (IPv4):(port) lub (IPv6):(port)>")
                ;

                po::variables_map vm;
                po::store(po::parse_command_line(ac, av, description), vm);
                
                po::notify(vm);    

                if (vm.count("help")) {
                    std::cout << description << "\n";
                    exit(0);
                }

                parameters.gui_address = parse_address_udp(vm["gui-address"].as<std::string>());

                parameters.server_address = parse_address_tcp(vm["server-address"].as<std::string>());
                
            }
            catch(std::exception &e) {
                exit_with_msg("Nieprawidłowo podane parametry. 1\n");
            }
            

            return parameters;

        }