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

void Server_tcp_handler::close_conn() {
    for (size_t i = 0; i < conn_max; ++i) {
        if (poll_descriptors[i].fd >= 0) {
            CHECK_ERRNO_EXIT(close(poll_descriptors[i].fd));
        }
    }
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

int Server_tcp_handler::accept_connection(int socket_fd, struct sockaddr_in6 *client_address) {
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

int Server_tcp_handler::poll_exec(int timeout) {
    return poll(poll_descriptors, conn_max, timeout);
}

bool Server_tcp_handler::is_new_connection() {
    return poll_descriptors[0].revents & POLLIN;
}

ssize_t Server_tcp_handler::accept_client() {
    struct sockaddr_in6 client_address;

    int client_fd = accept_connection(poll_descriptors[0].fd, &client_address);

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

    // char *client_ip = client_address.sin6_addr;
    char client_ip[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &client_address.sin6_addr, client_ip, INET6_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_address.sin6_port);

    std::string address("[");
    address.append(client_ip);
    address.append("]:");
    address.append(std::to_string(client_port));

    addresses.insert_or_assign(accepted_pos, address.data());

    std::cout << "Adres klienta: " << address << "\n";

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

Message_recv_status Server_tcp_handler::read_from(size_t i) {
    current_buf.buf_length = read(poll_descriptors[i].fd, buf, buf_size);
    current_buf.buf = buf;

    if (current_buf.buf_length < 0) {
        return Message_recv_status::ERROR_CONN;
    }
    if (current_buf.buf_length == 0) {
        return Message_recv_status::END_CONN;      
    }

    return Message_recv_status::POTENTIALLY_SUCCESS;
}


void Server_tcp_handler::current_buf_update(size_t bytes_count) {
    current_buf.buf += bytes_count;
    current_buf.buf_length -= bytes_count;
}

uint8_t Server_tcp_handler::read_uint8() {
    if (current_buf.buf_length < 1) {
        throw MsgException();
    }

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
        std::cout << "Nie udało się wysłać wiadomości do klienta" << i << "\n";
        PRINT_ERRNO();
    }
    ENSURE(sent_length == (ssize_t) msg.size());
}

void Server_tcp_handler::send_message_to_all(std::vector<std::byte> msg) {
    for (ssize_t i = 1; i < conn_max; ++i) {
        if (poll_descriptors[i].fd != -1) {
            send_message(i, msg);
        }
    }    
}

std::string Server_tcp_handler::get_address(size_t i) {
    return addresses.at(i);
}