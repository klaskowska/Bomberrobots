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
#include <unistd.h>
#include <memory>
#include "game-messages.h"

enum class Message_recv_status {
    ERROR_CONN,
    ERROR_MSG,
    END_CONN,
    POTENTIALLY_SUCCESS,
    SUCCESS,
};

typedef struct current_buf {
    std::byte *buf;
    ssize_t buf_length;
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
    static const ssize_t conn_max = 25;
    static const size_t queue_length = 27;
    static const size_t buf_size = 256;
    std::byte buf[buf_size];
    current_buf_t current_buf;

    uint16_t port;

    size_t active_clients;

    struct pollfd poll_descriptors[conn_max];

    static int open_socket();

    void bind_socket(int socket_fd);

    int accept_connection(int socket_fd, struct sockaddr_in *client_address);

    void current_buf_update(size_t bytes_count);

public:
    Server_tcp_handler(uint16_t port);

    size_t get_conn_max() { return conn_max; }

    void start_listening();

    void reset_revents();

    // returns poll status
    int poll_exec();

    bool is_new_connection();

    // returns descriptor position if client has successfully connected
    // or -1 if not
    ssize_t accept_client();

    // is message from client whom descriptor is at i position
    bool is_message_from(size_t i);

    void report_msg_status(Message_recv_status status, size_t i);

    Message_recv_status read_from(size_t i);

    void send_message(size_t i, std::vector<std::byte> msg);

    void close_conn();

    uint8_t read_uint8();

    std::string read_string();

    bool is_buf_empty() {return current_buf.buf_length == 0;}

};

#endif