#include "server-parser.h"

server_parameters_t Parser::parse_parameters(int ac, const char* av[]) {
	
            server_parameters_t parameters;
            u_int16_t players_count_u16;
            
            try {
                po::options_description description("Allowed options");
                
                description.add_options()
                    ("bomb-timer,b", po::value<uint16_t>(&parameters.bomb_timer), "<u16>")
                    ("players-count,c", po::value<u_int16_t>(&players_count_u16), "<u8>")
                    ("turn-duration,d", po::value<uint64_t>(&parameters.turn_duration), "<u64, milisekundy>")
                    ("explosion-radius,e", po::value<uint16_t>(&parameters.explosion_radius), "<u16>")
                    ("help,h", "Wypisuje jak używać programu")
                    ("initial-blocks,k", po::value<uint16_t>(&parameters.initial_blocks), "<u16>")
                    ("game-length,l", po::value<uint16_t>(&parameters.game_length), "<u16>")
                    ("server-name,n", po::value<std::string>(&parameters.server_name), "<String>")
                    ("port,p", po::value<uint16_t>(&parameters.port), "<u16>")
                    ("seed,s", po::value<uint32_t>(&parameters.seed)->default_value(
                        (uint32_t)std::chrono::system_clock::now().time_since_epoch().count()), "<u32, parametr opcjonalny>")
                    ("size-x,x", po::value<uint16_t>(&parameters.size_x), "<u16>")
                    ("size-y,y", po::value<uint16_t>(&parameters.size_y), "<u16>")
                ;

                po::variables_map vm;
                po::store(po::parse_command_line(ac, av, description), vm);
                
                po::notify(vm);    

                if (vm.count("help")) {
                    std::cout << description << "\n";
                    exit(0);
                }

                if (players_count_u16 < 1)
                    exit_with_msg("Liczba graczy powinna wynosić co najmniej 1\n.");
                else
                    parameters.players_count = (uint8_t) players_count_u16;
            }
            catch(std::exception &e) {
                exit_with_msg("Nieprawidłowo podane parametry. 1\n");
            }
            

            return parameters;

        }