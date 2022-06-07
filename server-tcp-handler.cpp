#include "server-tcp-handler.h"
#include "error-handler.h"

Server_tcp_handler::Server_tcp_handler(uint16_t port, uint64_t turn_duration) 
    : port(port), turn_duration(turn_duration) {
    
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
    CHECK_ERRNO(close(poll_descriptors[0].fd));
}

int Server_tcp_handler::open_socket() {
    int socket_fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        exit_with_msg("Nie udało się otworzyć gniazda.\n");
    }

    return socket_fd;
}

void Server_tcp_handler::bind_socket(int socket_fd) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET6;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    CHECK_ERRNO(bind(socket_fd, (struct sockaddr *) &server_address,
                     (socklen_t) sizeof(server_address)));
}

void Server_tcp_handler::start_listening() {
    printf("Listening on port %u\n", port);
    CHECK_ERRNO(listen(poll_descriptors[0].fd, 3));
}

inline static int accept_connection(int socket_fd, struct sockaddr_in *client_address) {
    socklen_t client_address_length = (socklen_t) sizeof(*client_address);

    int client_fd = accept(socket_fd, (struct sockaddr *) client_address, &client_address_length);
    if (client_fd < 0) {
        PRINT_ERRNO();
    }

    return client_fd;
}

void Server_tcp_handler::manage_connections() {
    while (true) {
        for (size_t i = 0; i < conn_max; ++i) {
            poll_descriptors[i].revents = 0;
        }

        // TODO: czy w sleep sa milisekundy?
        sleep(turn_duration);

        int poll_status = poll(poll_descriptors, conn_max, 0);
        if (poll_status == -1 ) {
            if (errno == EINTR) 
                fprintf(stderr, "Interrupted system call\n");
            else    
                PRINT_ERRNO();
        } 
        else if (poll_status > 0) {
            if (poll_descriptors[0].revents & POLLIN) {
                /* Przyjmuję nowe połączenie */
                int client_fd = accept_connection(poll_descriptors[0].fd, NULL);

                bool accepted = false;
                for (int i = 1; i < conn_max; ++i) {
                    if (poll_descriptors[i].fd == -1) {
                        fprintf(stderr, "Received new connection (%d)\n", i);

                        poll_descriptors[i].fd = client_fd;
                        poll_descriptors[i].events = POLLIN;
                        active_clients++;
                        accepted = true;
                        break;
                    }
                }
                if (!accepted) {
                    CHECK_ERRNO(close(client_fd));
                    fprintf(stderr, "Too many clients\n");
                }
            }
            for (int i = 1; i < conn_max; ++i) {
                if (poll_descriptors[i].fd != -1 && (poll_descriptors[i].revents & (POLLIN | POLLERR))) {
                    ssize_t received_bytes = read(poll_descriptors[i].fd, buf, BUF_SIZE);
                    if (received_bytes < 0) {
                        fprintf(stderr, "Error when reading message from connection %d (errno %d, %s)\n", i, errno, strerror(errno));
                        CHECK_ERRNO(close(poll_descriptors[i].fd));
                        poll_descriptors[i].fd = -1;
                        active_clients -= 1;
                    } else if (received_bytes == 0) {
                        fprintf(stderr, "Ending connection (%d)\n", i);
                        CHECK_ERRNO(close(poll_descriptors[i].fd));
                        poll_descriptors[i].fd = -1;
                        active_clients -= 1;
                    } else {
                        printf("(%d) -->%.*s\n", i, (int) received_bytes, buf);
                    }
                }
            }
        } else {
            printf("%d milliseconds passed without any events\n", turn_duration);
        }
    }
}

