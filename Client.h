//
// Created by mano on 10.12.16.
//

#ifndef LITTLEPROXYSERVER_CLIENT_H
#define LITTLEPROXYSERVER_CLIENT_H

#include "Cache.h"
#include "Buffer.h"
#include "Parser.h"

class Client {
    int my_socket;

    bool flag_correct_my_socket;
    bool flag_correct_http_socket;

    Buffer * buffer_in;

    bool flag_received_get_request;

    bool flag_closed;
    bool flag_closed_correct;
    bool flag_closed_http_socket;

    bool flag_process_http_connecting;
    int http_socket_flags;

    Cache * cache;

    bool flag_data_cached;

    pthread_mutex_t mtx_execute_loop;
    bool flag_execute_loop;
    bool flag_loop_finished;

    int receive_request_from_client();

    int send_response_to_client(DownloadBuffer *buffer_from_server);

    DownloadBuffer * get_buffer_from_server(char *p_new_line, size_t i_next_line);

public:

    Client(int my_socket, Cache * cache);

    int get_my_socket() {
        return my_socket;
    }

    void set_data_cached() {
        flag_data_cached = true;
    }

    bool is_data_cached() {
        return flag_data_cached;
    }

    void set_closed_correct() {
        flag_closed = true;
        flag_closed_correct = true;
    }

    void set_closed_incorrect() {
        flag_closed = true;
        flag_closed_correct = false;
    }

    bool is_process_http_connecting() {
        return flag_process_http_connecting;
    }

    void stop_loop_execute() {
        pthread_mutex_lock(&mtx_execute_loop);
        flag_execute_loop = false;
        pthread_mutex_unlock(&mtx_execute_loop);
    }

    bool is_loop_finished() {
        pthread_mutex_lock(&mtx_execute_loop);
        bool res = flag_loop_finished;
        pthread_mutex_unlock(&mtx_execute_loop);

        return res;
    }

    int do_all();

    ~Client();

};


#endif //LITTLEPROXYSERVER_CLIENT_H
