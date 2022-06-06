#include "error-handler.h"

void exit_with_msg(std::string message) {
    std::cerr << message;
    exit(1);
}