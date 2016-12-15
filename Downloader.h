//
// Created by mano on 14.12.16.
//

#ifndef LITTLEPROXYSERVER_DOWNLOADER_H
#define LITTLEPROXYSERVER_DOWNLOADER_H

#include "Includes.h"
#include "Buffer.h"
#include "DownloadBuffer.h"

class Downloader {
    int server_socket;
    struct sockaddr_in addr_to_server;

    Buffer * buffer_to_server;
    DownloadBuffer * buffer_from_server;

    bool flag_finished;
    bool flag_finished_correct;

    void set_finished_correct() {
        flag_finished = true;
        flag_finished_correct = true;
    }

    void set_finished_incorrect() {
        flag_finished = true;
        flag_finished_correct = false;
    }

    void set_finished_incorrect();

public:

    Downloader();

    DownloadBuffer * get_buffer() {
        return buffer;
    }

    void set_addr_to_server(struct sockaddr_in addr_to_server) {
        this->addr_to_server = addr_to_server;
    }

    void set_buffer_to_server(Buffer * buffer_to_server) {
        this->buffer_to_server = buffer_to_server;
    }

    void send_request_to_server();

    void start_receive_from_server();

    ~Downloader();

};


#endif //LITTLEPROXYSERVER_DOWNLOADER_H
