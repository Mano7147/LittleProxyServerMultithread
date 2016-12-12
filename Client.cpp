//
// Created by mano on 10.12.16.
//

#include "Client.h"

Client::Client(int my_socket, Cache * cache, HostResolver * host_resolver) {
    this->my_socket = my_socket;
    this->http_socket = -1;

    flag_correct_my_socket = true;
    flag_correct_http_socket = true;

    buffer_in = new Buffer(DEFAULT_BUFFER_SIZE);
    buffer_out = new Buffer(DEFAULT_BUFFER_SIZE);
    buffer_server_request = new Buffer(DEFAULT_BUFFER_SIZE);

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

    this->host_resolver = host_resolver;
}

void Client::add_result_to_cache() {
    if (!cache->is_in_cache(first_line_and_host)) {
        size_t size_data = buffer_out->get_i_end();

        char * data = (char*)malloc(size_data);
        memcpy(data, buffer_out->get_buf(), size_data);

        cache->push_to_cache(first_line_and_host, data, size_data);
    }
}

int Client::create_tcp_connection_to_request(std::string host_name) {
    struct sockaddr_in dest_addr;

    fprintf(stderr, "Before get address\n");
    int res = host_resolver->get_server_address(host_name, dest_addr);
    fprintf(stderr, "After get address\n");

    if (RESULT_INCORRECT == res) {
        return RESULT_INCORRECT;
    }

    http_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (-1 == http_socket) {
        perror("socket");
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "Before connect\n");

    if (connect(http_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) {
        perror("connect");
        return RESULT_INCORRECT;
    }

    flag_process_http_connecting = false;
    fprintf(stderr, "Connection established\n");

    return RESULT_CORRECT;
}

void Client::push_data_to_request_from_cache(std::pair<char *, size_t> data) {
    fprintf(stderr, "Put data from cache\n");
    set_data_cached();
    buffer_out->add_data_to_end(data.first, data.second);
}

int Client::handle_first_line_proxy_request(char *p_new_line, size_t i_next_line) {
    std::pair<std::string, std::string> parsed = Parser::get_new_first_line_and_hostname(buffer_in, p_new_line);

    std::string host_name = parsed.first;
    std::string new_first_line = parsed.second;

    if ("" == host_name || "" == new_first_line) {
        fprintf(stderr, "Can not correctly parse first line\n");
        return RESULT_INCORRECT;
    }

    fprintf(stderr, "Hostname: %s\n", host_name.c_str());
    fprintf(stderr, "New first line: %s\n", new_first_line.c_str());

    if (cache->is_in_cache(parsed)) {
        Record record = cache->get_from_cache(parsed);
        push_data_to_request_from_cache(std::make_pair(record.data, record.size));

        return RESULT_CORRECT;
    }
    else {
        Parser::push_first_data_request(buffer_server_request, buffer_in, new_first_line, i_next_line);
        int result_connection = create_tcp_connection_to_request(host_name);

        first_line_and_host = parsed;

        return result_connection;
    }
}

void Client::do_all() {
    while (!buffer_in->is_have_data() || NULL == strstr(buffer_in->get_start(), "\r\n\r\n")) {
        ssize_t received = recv(my_socket, buffer_in->get_end(), buffer_in->get_empty_space_size(), 0);
        fprintf(stderr, "Received from client %ld\n", received);

        if (-1 == received) {
            perror("recv");
            return;
        }

        if (0 == received && (!buffer_in->is_have_data() || NULL == strstr(buffer_in->get_start(), "\r\n\r\n"))) {
            return;
        }

        if (buffer_in->get_data_size() >= 3 && NULL == strstr(buffer_in->get_start(), "GET")) {
            fprintf(stderr, "Not GET request\n");
            return;
        }

        if (NULL != strchr(buffer_in->get_start(), '\n') && NULL == strstr(buffer_in->get_start(), "HTTP/1.0")) {
            fprintf(stderr, "Not http1.0 request:\n");
            Parser::print_buffer_data(buffer_in->get_start(), buffer_in->get_data_size());
            return;
        }

        if (buffer_in->get_data_size() > MAX_GET_REQUEST_SIZE) {
            fprintf(stderr, "Too big GET request\n");
            return;
        }

        buffer_in->do_move_end(received);
    }
    fprintf(stderr, "After recv client\n");

    char * p_new_line = strchr(buffer_in->get_start(), '\n');

    if (NULL == p_new_line) {
        fprintf(stderr, "Bad received data\n");
        return;
    }

    fprintf(stderr, "Before handle host and path\n");

    size_t i_next_line = (p_new_line - buffer_in->get_start()) + 1;
    int result = handle_first_line_proxy_request(p_new_line, i_next_line);

    fprintf(stderr, "After handle host and path\n");

    if (RESULT_INCORRECT == result) {
        return;
    }

    if (!is_data_cached()) {
        while (buffer_server_request->is_have_data()) {
            ssize_t sent = send(http_socket, buffer_server_request->get_start(), buffer_server_request->get_data_size(), 0);
            fprintf(stderr, "Sent to server: %ld\n", sent);

            if (-1 == sent) {
                perror("send");
                http_socket = -1;
                return;
            }

            if (0 == sent) {
                return;
            }

            buffer_server_request->do_move_start(sent);
        }

        while (1) {
            ssize_t received = recv(http_socket, buffer_out->get_end(), buffer_out->get_empty_space_size(), 0);
            fprintf(stderr, "Received from server: %ld\n", received);

            if (-1 == received) {
                perror("recv");
                http_socket = -1;
                return;
            }

            if (0 == received) {
                break;
            }

            buffer_out->do_move_end(received);
        }
    }
    fprintf(stderr, "After work with server\n");

    while (buffer_out->is_have_data()) {
        ssize_t sent = send(my_socket, buffer_out->get_start(), buffer_out->get_data_size(), 0);
        fprintf(stderr, "Sent to client: %ld\n", sent);

        if (-1 == sent) {
            perror("send");
            return;
        }

        if (0 == sent) {
            return;
        }

        buffer_out->do_move_start(sent);
    }

    pthread_mutex_lock(&mtx_execute_loop);
    flag_loop_finished = true;
    pthread_mutex_unlock(&mtx_execute_loop);
}

Client::~Client() {
    fprintf(stderr, "Destructor client start\n");

    if (-1 != my_socket && flag_correct_my_socket) {
        close(my_socket);
    }

    if (-1 != http_socket && flag_correct_http_socket) {
        close(http_socket);
    }

    if (flag_closed_correct && !is_data_cached()) {
        fprintf(stderr, "Add result to cache\n");
        add_result_to_cache();
    }

    delete buffer_in;
    delete buffer_server_request;
    delete buffer_out;

    fprintf(stderr, "Destructor done\n");
}
