//
// Created by mano on 10.12.16.
//

#include "Client.h"

Client::Client(int my_socket, Cache * cache) {
    this->my_socket = my_socket;

    flag_correct_my_socket = true;
    flag_correct_http_socket = true;

    buffer_in = new Buffer(DEFAULT_BUFFER_SIZE);

    pthread_mutex_init(&mtx_execute_loop, 0);
    flag_execute_loop = true;
    flag_loop_finished = false;

    flag_received_get_request = false;

    flag_data_cached = false;

    flag_process_http_connecting = false;

    flag_closed = false;
    flag_closed_correct = true;
    flag_closed_http_socket = false;

    this->cache = cache;
}

DownloadBuffer * Client::get_buffer_from_server(char *p_new_line, size_t i_next_line) {
    std::pair<std::string, std::string> parsed = Parser::get_new_first_line_and_hostname(buffer_in, p_new_line);

    std::string host_name = parsed.first;
    std::string new_first_line = parsed.second;

    if ("" == host_name || "" == new_first_line) {
        fprintf(stderr, "Can not correctly parse first line\n");
        return NULL;
    }

    fprintf(stderr, "Hostname: %s\n", host_name.c_str());
    fprintf(stderr, "New first line: %s\n", new_first_line.c_str());

    return cache->get_from_cache(parsed, buffer_in);
}

int Client::receive_request_from_client() {
    while (!buffer_in->is_have_data() || NULL == strstr(buffer_in->get_start(), "\r\n\r\n")) {
        ssize_t received = recv(my_socket, buffer_in->get_end(), buffer_in->get_empty_space_size(), 0);
        fprintf(stderr, "Received from client %ld\n", received);

        if (-1 == received) {
            perror("recv");
            return RESULT_INCORRECT;
        }

        if (0 == received && (!buffer_in->is_have_data() || NULL == strstr(buffer_in->get_start(), "\r\n\r\n"))) {
            return RESULT_INCORRECT;
        }

        if (buffer_in->get_data_size() >= 3 && NULL == strstr(buffer_in->get_start(), "GET")) {
            fprintf(stderr, "Not GET request\n");
            return RESULT_INCORRECT;
        }

        if (NULL != strchr(buffer_in->get_start(), '\n') && NULL == strstr(buffer_in->get_start(), "HTTP/1.0")) {
            fprintf(stderr, "Not http1.0 request:\n");
            Parser::print_buffer_data(buffer_in->get_start(), buffer_in->get_data_size());
            return RESULT_INCORRECT;
        }

        if (buffer_in->get_data_size() > MAX_GET_REQUEST_SIZE) {
            fprintf(stderr, "Too big GET request\n");
            return RESULT_INCORRECT;
        }

        buffer_in->do_move_end(received);
    }

    fprintf(stderr, "Received request to proxy server:\n");
    Parser::print_buffer_data(buffer_in->get_buf(), buffer_in->get_data_size());

    return RESULT_CORRECT;
}

int Client::send_response_to_client(DownloadBuffer * buffer_from_server) {
    size_t offs = 0;
    ssize_t size = 0;
    char * data;

    while (1) {
        data = buffer_from_server->get_data_from_buffer(offs, size);

        if (NULL == data) {
            if (-1 == size) {
                fprintf(stderr, "Error receive from buffer\n");
                return RESULT_INCORRECT;
            }
            if (0 == size) {
                fprintf(stderr, "Sent all response to client");
                return RESULT_CORRECT;
            }
            else {
                fprintf(stderr, "Not proper branch\n");
                return RESULT_INCORRECT;
            }
        }

        ssize_t sent = send(my_socket, data, (size_t)size, 0);

        if (-1 == sent) {
            perror("send");
            return RESULT_INCORRECT;
        }
        else if (0 == sent) {
            fprintf(stderr, "Client close connection\n");
            return RESULT_INCORRECT;
        }
        else {
            offs += sent;
        }
    }
}

int Client::do_all() {
    int result_recv_request = receive_request_from_client();

    if (RESULT_INCORRECT == result_recv_request) {
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "Received request from client\n");

    char * p_new_line = strchr(buffer_in->get_start(), '\n');

    if (NULL == p_new_line) {
        fprintf(stderr, "Bad received data\n");
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "Send request to cache\n");

    size_t i_next_line = (p_new_line - buffer_in->get_start()) + 1;
    DownloadBuffer * buffer_from_server = get_buffer_from_server(p_new_line, i_next_line);

    if (NULL == buffer_from_server) {
        fprintf(stderr, "Cache return NULL\n");
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "Cache return buffer read from\n");

    send_response_to_client(buffer_from_server);

    pthread_mutex_lock(&mtx_execute_loop);
    flag_loop_finished = true;
    pthread_mutex_unlock(&mtx_execute_loop);
}

Client::~Client() {
    fprintf(stderr, "Destructor client start\n");

    if (-1 != my_socket && flag_correct_my_socket) {
        close(my_socket);
    }

    delete buffer_in;

    fprintf(stderr, "Destructor client done\n");
}
