// ./robots-server -n servername -p 18739 -c 2 -x 10 -y 10 -l 5 -d 500 -e 2 -b 2
// ./robots-client -d localhost:46284 -n anna -p 36725 -s localhost:18739
// ./robots-client -d localhost:46284 -n anna -p 36725 -s 193.0.96.129:10011
// cargo run --bin gui -- --client-address localhost:36725 --port 46284

// TODO: server-parser: co jeśli ujemne parametry?
// TODO: czytanie jakiekolwiek

#include "server-parser.h"
#include "server-tcp-handler.h"
#include "game-engine.h"

int main(int argc, const char** argv) {
    Parser parser;
    server_parameters_t game_params = parser.parse_parameters(argc, argv);

    Server_tcp_handler tcp_handler(game_params.port);

    Game_engine game_engine(game_params, tcp_handler);

    // game_engine.start_game();

    return 0;
}