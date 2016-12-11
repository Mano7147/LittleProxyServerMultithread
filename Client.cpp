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

    flag_received_get_request = false;

    flag_closed = false;
    flag_closed_http_socket = false;

    flag_process_http_connecting = false;

    flag_data_cached = false;

    pthread_mutex_init(&mtx_execute_loop, 0);
    flag_execute_loop = true;
    flag_loop_finished = false;

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
    int res = host_resolver->get_server_address(host_name, dest_addr);

    if (RESULT_INCORRECT == res) {
        return RESULT_INCORRECT;
    }

    http_socket = socket(PF_INET, SOCK_STREAM, 0);

    if (-1 == http_socket) {
        perror("socket");
        return RESULT_INCORRECT;
    }

    /*http_socket_flags = fcntl(http_socket, F_GETFL, 0);
    fcntl(http_socket, F_SETFL, http_socket_flags | O_NONBLOCK);*/

    fprintf(stderr, "Before connect\n");

    if (connect(http_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) {
        if (false && errno == EINPROGRESS) {
            fprintf(stderr, "Connect in progress\n");
            flag_process_http_connecting = true;

            return RESULT_CORRECT;
        }
        else {
            perror("connect");

            return RESULT_INCORRECT;
        }
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

void Client::receive_request_from_client() {
    ssize_t received = recv(my_socket, buffer_in->get_end(), buffer_in->get_empty_space_size(), 0);

    fprintf(stderr, "[%d] Received: %ld\n", my_socket, received);

    switch (received) {
        case -1:
            perror("Error while read()");
            set_closed_incorrect();
            break;

        case 0:
            fprintf(stderr, "Close client\n");
            set_closed_correct();
            break;

        default:
            int res = buffer_in->do_move_end(received);

            if (RESULT_INCORRECT == res) {
                set_closed_incorrect();
                break;
            }

            if (!flag_received_get_request) {
                char * p_new_line = strchr(buffer_in->get_start(), '\n');

                if (p_new_line != NULL) {
                    flag_received_get_request = true;

                    size_t i_next_line = (p_new_line - buffer_in->get_start()) + 1;

                    int result = handle_first_line_proxy_request(p_new_line, i_next_line);

                    if (RESULT_INCORRECT == result) {
                        set_closed_incorrect();
                        break;
                    }
                }
            }
            else {
                buffer_server_request->add_data_to_end(buffer_in->get_start(), buffer_in->get_data_size());
                buffer_in->do_move_start(buffer_in->get_data_size());
            }
    }
}

void Client::send_answer_to_client() {
    if (!is_closed() && buffer_out->is_have_data()) {
        fprintf(stderr, "\nHave data to send to client\n");

        ssize_t sent = send(my_socket, buffer_out->get_start(), buffer_out->get_data_size(), 0);

        fprintf(stderr, "[%d] Sent: %ld\n", my_socket, sent);

        switch (sent) {
            case -1:
                perror("Error while send to client");
                set_closed_incorrect();
                break;

            case 0:
                set_closed_correct();
                break;

            default:
                buffer_out->do_move_start(sent);
        }
    }
}

void Client::receive_server_response() {
    fprintf(stderr, "Have data from server\n");

    if (flag_process_http_connecting) {
        fprintf(stderr, "Connection established\n");

        fcntl(http_socket, F_SETFL, http_socket_flags);
        flag_process_http_connecting = false;

        return;
    }

    ssize_t received = recv(http_socket, buffer_out->get_end(), buffer_out->get_empty_space_size(), 0);

    fprintf(stderr, "[%d] Received: %ldn\n", my_socket, received);

    switch (received) {
        case -1:
            perror("recv(http)");
            set_closed_incorrect();
            break;

        case 0:
            fprintf(stderr, "Close http connection\n");
            fprintf(stderr, "Socket: %d\n", http_socket);

            set_close_http_socket();

            break;

        default:
            buffer_out->do_move_end(received);
    }
}

void Client::send_request_to_server() {
    if (!is_data_cached() && buffer_server_request->is_have_data()) {
        fprintf(stderr, "\nHave data to send to http server\n");

        ssize_t sent = send(http_socket, buffer_server_request->get_start(), buffer_server_request->get_data_size(), 0);

        fprintf(stderr, "[%d] Sent: %ld\n", my_socket, sent);

        switch (sent) {
            case -1:
                perror("Error while send(http)");
                set_closed_incorrect();
                break;

            case 0:
                set_close_http_socket();
                break;

            default:
                buffer_server_request->do_move_start(sent);
        }
    }
}

void Client::start_main_loop() {
    for ( ;  ; ) {
        pthread_mutex_lock(&mtx_execute_loop);
        if (!flag_execute_loop) {
            break;
        }
        pthread_mutex_unlock(&mtx_execute_loop);

        fd_set fds_read;
        fd_set fds_write;
        FD_ZERO(&fds_read);
        FD_ZERO(&fds_write);
        int max_fd = 0;

        FD_SET(my_socket, &fds_read);

        if (buffer_out->is_have_data()) {
            FD_SET(my_socket, &fds_write);
        }

        max_fd = my_socket;

        if (is_received_get_request() && -1 != http_socket && !is_closed_http_socket()) {
            fprintf(stderr, "Set http socket\n");
            FD_SET(http_socket, &fds_read);

            if (buffer_server_request->is_have_data()) {
                FD_ISSET(http_socket, &fds_write);
            }

            max_fd = std::max(max_fd, http_socket);
        }

        fprintf(stderr, "Before select\n");
        int activity = select(max_fd + 1, &fds_read, &fds_write, NULL, NULL);
        fprintf(stderr, "After select: %d\n", activity);

        if (activity <= 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(my_socket, &fds_read)) {
            fprintf(stderr, "Have data from client\n");
            receive_request_from_client();
        }

        if (!is_data_cached() && !is_closed_http_socket()) {
            if (FD_ISSET(http_socket, &fds_write)) {
                fprintf(stderr, "Send request to server\n");
                send_request_to_server();
            }

            if (FD_ISSET(http_socket, &fds_read)) {
                fprintf(stderr, "Receive server response\n");
                receive_server_response();
            }
        }

        if (FD_ISSET(my_socket, &fds_write)) {
            fprintf(stderr, "Send answer to client\n");
            send_answer_to_client();
        }

        if (is_closed_http_socket() && !get_buffer_out()->is_have_data()) {
            set_closed_correct();
            break;
        }

        if (is_data_cached() && !get_buffer_out()->is_have_data()) {
            set_closed_correct();
            break;
        }
    }

    pthread_mutex_lock(&mtx_execute_loop);
    flag_loop_finished = true;
    pthread_mutex_unlock(&mtx_execute_loop);
}

void Client::do_all() {
    while (!buffer_in->is_have_data() || NULL == strstr(buffer_in->get_start(), "\r\n\r\n")) {
        ssize_t received = recv(my_socket, buffer_in->get_end(), buffer_in->get_empty_space_size(), 0);

        if (-1 == received) {
            perror("recv");
            return;
        }

        if (0 == received && NULL == strstr(buffer_in->get_start(), "\r\n\r\n")) {
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

    size_t i_next_line = (p_new_line - buffer_in->get_start()) + 1;
    int result = handle_first_line_proxy_request(p_new_line, i_next_line);

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

            if (5 == received) {
                perror("recv");
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
        fprintf(stderr, "Sent: %ld\n", sent);

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
    fprintf(stderr, "Destructor client!!!!\n");

    if (-1 != my_socket && flag_correct_my_socket) {
        close(my_socket);
    }

    if (-1 != http_socket && flag_correct_http_socket) {
        close(http_socket);
    }

    if (flag_closed_correct) {
        fprintf(stderr, "Add result to cache\n");
        add_result_to_cache();
    }

    delete buffer_in;
    delete buffer_server_request;
    delete buffer_out;
}
