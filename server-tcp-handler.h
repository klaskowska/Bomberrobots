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
#include <exception>
#include "game_messages.h"

enum class Message_recv_status {
    ERROR_CONN,
    ERROR_MSG,
    END_CONN,
    SUCCESS,
};

typedef struct Message_recv {
    Message_recv_status status;
    Client_message client_message;
};

typedef struct current_buf {
    std::byte *buf;
    size_t buf_length;
} current_buf_t;

struct MsgException : public std::exception
{
	const char * what () const throw ()
    {
    	return "Błąd w wysłanej wiadomości od klienta.";
    }
};

class Server_tcp_handler {
private:
    static const size_t conn_max = 25;
    static const size_t queue_length = 27;
    static const size_t buf_size = 256;
    std::byte buf[buf_size];
    current_buf_t current_buf;

    uint16_t port;

    size_t active_clients;

    // current client's position in poll_descriptors
    size_t current_client_pos;

    struct pollfd poll_descriptors[conn_max];

    static int open_socket();

    void bind_socket(int socket_fd);

    int accept_connection(int socket_fd, struct sockaddr_in *client_address);

    uint8_t read_uint8();

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

    void report_msg_status(Message_recv_status status);

    void read_from();

    Message_recv read_msg_from();

    void set_current_client_pos(size_t i);

    uint8_t read_uint8();

    std::string read_string();

    void current_buf_update(size_t bytes_count);

    Client_message read_client_msg();


};

#endif