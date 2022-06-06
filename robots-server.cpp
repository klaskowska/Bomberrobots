// ./robots-server -n server_name -p 18739 -c 2 -x 10 -y 10 -l 5 -d 500 -e 2 -b 2
// ./robots-client -d localhost:46284 -n anna -p 36725 -s localhost:18739
// ./robots-client -d localhost:46284 -n anna -p 36725 -s 193.0.96.129:10011
// cargo run --bin gui -- --client-address localhost:36725 --port 46284

// TODO: server-parser: co jeśli ujemne parametry?
// TODO: clean w makefile


#include "server-parser.h"

int main(int argc, const char** argv) {
    Parser parser;
    parser.parse_parameters(argc, argv);

    return 0;
}