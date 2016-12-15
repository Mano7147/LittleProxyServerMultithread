//
// Created by mano on 14.12.16.
//

#include "Downloader.h"

Downloader::Downloader() {
    buffer = new Buffer(DEFAULT_BUFFER_SIZE);

    server_socket = socket(PF_SOCKET, SOCK_STREAM, 0);

    if (socket <= 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    flag_finished = false;
    flag_finished_correct = true;
}

int Downloader::send_request_to_server() {
    while (buffer_to_server->get_data_size()) {
        ssize_t sent = send(server_socket, buffer_to_server, buffer_to_server->get_data_size(), 0);

        if (-1 == sent) {
            perror(sent);
            return RESULT_INCORRECT;
        }
        else if (0 == sent) {
            fprintf(stderr, "Server close connection\n");
            return (buffer_to_server->is_have_data() ? RESULT_INCORRECT : RESULT_CORRECT);
        }
        else {
            buffer_to_server->do_move_start(sent);
        }
    }
    return RESULT_CORRECT;
}

void Downloader::start_receive_from_server() {
    struct socklen_t sock_len = sizeof(struct sockaddr_in);

    if (connect(server_socket, &addr, sock_len)) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    while (1) {
        ssize_t received = recv(server_socket, buffer->get_end(), buffer->get_empty_size(), 0);

        if (-1 == received) {
            flag_finished = true;
            flag_finished_correct = false;
            break;
        }
        else if (0 == received) {
            flag_finished = true;
            flag_finished_correct = true;
            break;
        }
        else {
            buffer->do_move_end(received);
        }
    }
    fprintf(stderr, "Download finished: %d\n", flag_finished_correct);
}

Downloader::~Downloader() {
    delete buffer;
}
