//
// Created by mano on 14.12.16.
//

#include "Downloader.h"

Downloader::Downloader() {
    buffer_from_server = new DownloadBuffer(DEFAULT_BUFFER_SIZE);

    server_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (server_socket <= 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
}

int Downloader::send_request_to_server() {
    fprintf(stderr, "Connect to server...\n");

    if (connect(server_socket, (struct sockaddr*) &addr_to_server, sizeof(struct sockaddr_in))) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Connection have been established\n");

    while (buffer_to_server->get_data_size()) {
        ssize_t sent = send(server_socket, buffer_to_server->get_start(), buffer_to_server->get_data_size(), 0);

        if (-1 == sent) {
            perror("send");
            return RESULT_INCORRECT;
        }
        else if (0 == sent) {
            fprintf(stderr, "Server close connection\n");
            return RESULT_INCORRECT;
        }
        else {
            buffer_to_server->do_move_start(sent);
        }
    }

    fprintf(stderr, "Sent all request to server:\n");
    Parser::print_buffer_data(buffer_to_server->get_buf(), buffer_to_server->get_i_end());

    return RESULT_CORRECT;
}

void Downloader::start_receive_from_server() {
    while (1) {
        char * buf = buffer_from_server->get_end();
        size_t size = buffer_from_server->get_empty_size();

        ssize_t received = recv(server_socket, buf, size, 0);

        if (-1 == received) {
            buffer_from_server->set_finished_incorrect();
            break;
        }
        else if (0 == received) {
            buffer_from_server->set_finished_correct();
            break;
        }
        else {
            buffer_from_server->do_move_end((size_t)received);
        }
    }

    fprintf(stderr, "Download finished: %d\n", buffer_from_server->is_finished_correct());
}

Downloader::~Downloader() {
    fprintf(stderr, "Destructor downloader\n");
    delete buffer_from_server;
}
