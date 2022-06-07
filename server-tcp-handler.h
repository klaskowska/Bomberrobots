#ifndef SERVER_TCP_HANDLER_H
#define SERVER_TCP_HANDLER_H

#include <cstdint>
#include <cstdlib>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

class Server_tcp_handler {
private:
    static const size_t conn_max = 25;
    static const size_t queue_length = 27;

    uint16_t port;

    uint64_t turn_duration;

    size_t active_clients;


    struct pollfd poll_descriptors[conn_max];

    static int open_socket();

    void bind_socket(int socket_fd);

    inline static int accept_connection(int socket_fd, struct sockaddr_in *client_address);

public:
    Server_tcp_handler(uint16_t port, uint64_t turn_duration);

    ~Server_tcp_handler();

    void start_listening();

    void manage_connections();

};

#endif