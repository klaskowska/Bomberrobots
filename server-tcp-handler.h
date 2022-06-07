#ifndef SERVER_TCP_HANDLER_H
#define SERVER_TCP_HANDLER_H

#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <netinet/tcp.h>
#include <vector>

enum class Message_recv_status {
    ERROR_CONN,
    ERROR_MSG,
    END_CONN,
    SUCCESS,
};

typedef struct byte_msg {
    size_t received_bytes_length;
    std::vector<std::byte> buf;
} byte_msg;

class Server_tcp_handler {
private:
    static const size_t conn_max = 25;
    static const size_t queue_length = 27;
    static const size_t buf_size = 256;

    uint16_t port;

    size_t active_clients;


    struct pollfd poll_descriptors[conn_max];

    std::byte buf[buf_size];

    static int open_socket();

    void bind_socket(int socket_fd);

    int accept_connection(int socket_fd, struct sockaddr_in *client_address);

public:
    Server_tcp_handler(uint16_t port);

    ~Server_tcp_handler();

    size_t get_conn_max() { return conn_max; }

    void start_listening();

    void reset_revents();

    // returns poll status
    int poll_exec();

    bool is_new_connection();

    // returns if client has successfully connected
    bool accept_client();

    // is message from client whom descriptor is at i position
    bool is_message_from(size_t i);

    void report_msg_status(size_t i, Message_recv_status status);

    byte_msg read_from(size_t i);


};

#endif