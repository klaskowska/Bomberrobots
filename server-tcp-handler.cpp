#include "server-tcp-handler.h"
#include "error-handler.h"

#define NO_FLAGS 0


Server_tcp_handler::Server_tcp_handler(uint16_t port) 
    : port(port) {
    
    active_clients = 0;
    current_buf.buf_length = 0;

    for (size_t i = 0; i < conn_max; ++i) {
        poll_descriptors[i].fd = -1;
        poll_descriptors[i].events = POLLIN;
        poll_descriptors[i].revents = 0;
    }

    poll_descriptors[0].fd = open_socket();

    bind_socket(poll_descriptors[0].fd);
}

Server_tcp_handler::~Server_tcp_handler() {
    if (poll_descriptors[0].fd >= 0)
        CHECK_ERRNO_EXIT(close(poll_descriptors[0].fd));
}

int Server_tcp_handler::open_socket() {
    int socket_fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        exit_with_msg("Nie udało się otworzyć gniazda.\n");
    }
    int yes;
    setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));

    return socket_fd;
}

void Server_tcp_handler::bind_socket(int socket_fd) {
    struct sockaddr_in6 server_address;
    server_address.sin6_family = AF_INET6;
    server_address.sin6_flowinfo = 0;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_port = htons(port);
    server_address.sin6_scope_id = 0;

    CHECK_ERRNO_EXIT(bind(socket_fd, (struct sockaddr *) &server_address,
                     (socklen_t) sizeof(server_address)));
}

void Server_tcp_handler::start_listening() {
    printf("Listening on port %u\n", port);
    CHECK_ERRNO_EXIT(listen(poll_descriptors[0].fd, 3));
}

int Server_tcp_handler::accept_connection(int socket_fd, struct sockaddr_in *client_address) {
    socklen_t client_address_length = (socklen_t) sizeof(*client_address);

    int client_fd = accept(socket_fd, (struct sockaddr *) client_address, &client_address_length);
    if (client_fd < 0) {
        PRINT_ERRNO();
    }

    return client_fd;
}

void Server_tcp_handler::reset_revents() {
    for (size_t i = 0; i < conn_max; ++i) {
        poll_descriptors[i].revents = 0;
    }
}

int Server_tcp_handler::poll_exec() {
    return poll(poll_descriptors, conn_max, 0);
}

bool Server_tcp_handler::is_new_connection() {
    return poll_descriptors[0].revents & POLLIN;
}

ssize_t Server_tcp_handler::accept_client() {
    int client_fd = accept_connection(poll_descriptors[0].fd, NULL);

    ssize_t accepted_pos = -1;
    for (ssize_t i = 1; i < conn_max; ++i) {
        if (poll_descriptors[i].fd == -1) {
            fprintf(stderr, "Przyjęto połączenie (%ld).\n", i);

            poll_descriptors[i].fd = client_fd;
            poll_descriptors[i].events = POLLIN;
            active_clients++;
            accepted_pos = i;
            break;
        }
    }
    if (accepted_pos < 0) {
        CHECK_ERRNO(close(client_fd));
        fprintf(stderr, "Zbyt dużo klientów.\n");
    }
    return accepted_pos;
}

bool Server_tcp_handler::is_message_from(size_t i) {
    return poll_descriptors[i].fd != -1 && (poll_descriptors[i].revents & (POLLIN | POLLERR));
}

void Server_tcp_handler::report_msg_status(Message_recv_status status, size_t i) {
    if (status != Message_recv_status::SUCCESS) {

        switch (status) {
            case Message_recv_status::ERROR_CONN:
                fprintf(stderr, "Problem podczas czytania wiadomości od klienta %ld (errno %d, %s)\n", i, errno, strerror(errno));
                break;
            case Message_recv_status::ERROR_MSG:
                fprintf(stderr, "Błąd w wiadomości odebranej od klienta %ld\n", i);
                break;
            case Message_recv_status::END_CONN:
                fprintf(stderr, "Klient %ld zakończył połączenie.\n", i);
                break;
            default:
                break;
        }

        CHECK_ERRNO(close(poll_descriptors[i].fd));
        poll_descriptors[i].fd = -1;
        active_clients -= 1;

        fprintf(stderr, "Zakończono połączenie z klientem %ld.\n", i);
    }
}

void Server_tcp_handler::read_from(size_t i) {
    current_buf.buf_length = read(poll_descriptors[i].fd, buf, buf_size);
    current_buf.buf = buf;
}

Message_recv Server_tcp_handler::read_msg_from(size_t i) {
    
    Message_recv message;

    read_from(i);

    if (current_buf.buf_length < 0) {
        message.status = Message_recv_status::ERROR_CONN;
        return message;
    }
    if (current_buf.buf_length == 0) {
        message.status = Message_recv_status::END_CONN;
        return message;        
    }

    try {
        message.client_message = read_client_msg(i);
        message.status = Message_recv_status::SUCCESS;
    } catch (MsgException &e) {
        message.status = Message_recv_status::ERROR_MSG;
    }

    return message;
}

Client_message Server_tcp_handler::read_client_msg(size_t i) {
    Client_message_code code = (Client_message_code)read_uint8();

    Client_message msg;

    switch (code)
    {
    case Client_message_code::Join:
        msg = Join_msg(read_string());
        break;
    case Client_message_code::Place_bomb_client:
        msg = Place_bomb_msg();
        break;
    case Client_message_code::Place_block_client:
        msg = Place_block_msg();
        break;
    case Client_message_code::Move_client:
        msg = Move_msg((Direction)read_uint8());
        break;
    default:
        throw MsgException();
    }
    return current_buf.buf_length == 0 ? msg : read_client_msg(i);
}

void Server_tcp_handler::current_buf_update(size_t bytes_count) {
    current_buf.buf += bytes_count;
    current_buf.buf_length -= bytes_count;
}

uint8_t Server_tcp_handler::read_uint8() {
    uint8_t result = (uint8_t)current_buf.buf[0];
    current_buf_update(1);
    return result;
}

std::string Server_tcp_handler::read_string() {
    uint8_t str_length = read_uint8();


    if (str_length > current_buf.buf_length) {
        throw MsgException();
    }

    std::string str(reinterpret_cast<const char *>(current_buf.buf), str_length);
    current_buf_update(str_length);

	return str;
}

void Server_tcp_handler::send_message(size_t i, std::vector<std::byte> msg) {
    errno = 0;
    ssize_t sent_length = send(poll_descriptors[i].fd, (std::byte*)&msg[0], msg.size(), NO_FLAGS);
    if (sent_length < 0) {
        printf("Nie udało się wysłać wiadomości do klienta %ld.\n", i);
        PRINT_ERRNO();
    }
    ENSURE(sent_length == (ssize_t) msg.size());

    printf("Wysłano wiadomość do klienta %ld.\n", i);
}