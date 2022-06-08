#include "server-tcp-handler.h"
#include "error-handler.h"

Server_tcp_handler::Server_tcp_handler(uint16_t port) 
    : port(port) {
    
    active_clients = 0;

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
        CHECK_ERRNO(close(poll_descriptors[0].fd));
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

    CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &server_address,
                     (socklen_t) sizeof(server_address)));
}

void Server_tcp_handler::start_listening() {
    printf("Listening on port %u\n", port);
    CHECK_ERRNO(listen(poll_descriptors[0].fd, 3));
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

bool Server_tcp_handler::accept_client() {
    int client_fd = accept_connection(poll_descriptors[0].fd, NULL);

    bool accepted = false;
    for (size_t i = 1; i < conn_max; ++i) {
        if (poll_descriptors[i].fd == -1) {
            fprintf(stderr, "Przyjęto połączenie (%ld).\n", i);

            poll_descriptors[i].fd = client_fd;
            poll_descriptors[i].events = POLLIN;
            active_clients++;
            accepted = true;
            break;
        }
    }
    if (!accepted) {
        CHECK_ERRNO(close(client_fd));
        fprintf(stderr, "Zbyt dużo klientów.\n");
    }
    return accepted;
}

bool Server_tcp_handler::is_message_from(size_t i) {
    return poll_descriptors[i].fd != -1 && (poll_descriptors[i].revents & (POLLIN | POLLERR));
}

void Server_tcp_handler::report_msg_status(size_t i, Message_recv_status status) {
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

byte_msg Server_tcp_handler::read_from(size_t i) {
    byte_msg msg;

    msg.received_bytes_length = read(poll_descriptors[i].fd, buf, buf_size);
    msg.buf = std::vector<std::byte>(buf, buf + msg.received_bytes_length);
    return msg;
}