//
// Created by mano on 14.12.16.
//

#ifndef LITTLEPROXYSERVER_DOWNLOADER_H
#define LITTLEPROXYSERVER_DOWNLOADER_H

#include "Includes.h"
#include "Buffer.h"
#include "DownloadBuffer.h"
#include "Parser.h"

class Downloader {
    int server_socket;
    struct sockaddr_in addr_to_server;

    Buffer * buffer_to_server;
    DownloadBuffer * buffer_from_server;

public:

    Downloader();

    DownloadBuffer * get_buffer_from_server() {
        return buffer_from_server;
    }

    void set_addr_to_server(struct sockaddr_in addr_to_server) {
        this->addr_to_server = addr_to_server;
    }

    void set_buffer_to_server(Buffer * buffer_to_server, std::string new_first_line) {
        this->buffer_to_server = Parser::change_request(buffer_to_server, new_first_line);
    }

    int send_request_to_server();

    void start_receive_from_server();

    ~Downloader();

};


#endif //LITTLEPROXYSERVER_DOWNLOADER_H
