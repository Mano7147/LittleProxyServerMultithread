//
// Created by mano on 10.12.16.
//

#include "Cache.h"
#include <iostream>
#include <fstream>

Cache::Cache() {
    pthread_mutex_init(&mtx, 0);
}

void * Cache::start_new_downloader(void * pdownloader) {
    Downloader * downloader = (Downloader*)pdownloader;
    downloader->send_request_to_server();
    downloader->start_receive_from_server();
}

void Cache::delete_from_cache(std::pair<std::string, std::string> key) {
    pthread_mutex_lock(&mtx);

    if (downloaders.count(key)) {
        delete downloaders[key];
        downloaders.erase(key);
    }

    pthread_mutex_unlock(&mtx);
}

Downloader * Cache::create_new_downloader(std::string host_name, std::string new_first_line, Buffer * buffer_to_server) {
    struct hostent * host_info = gethostbyname(host_name.c_str());

    if (NULL == host_info) {
        perror("gethostbyname");
        return NULL;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = PF_INET;
    dest_addr.sin_port = htons(DEFAULT_PORT);
    memcpy(&dest_addr.sin_addr, host_info->h_addr, host_info->h_length);

    Downloader * downloader = new Downloader();
    downloader->set_addr_to_server(dest_addr);
    downloader->set_buffer_to_server(buffer_to_server, new_first_line);

    pthread_t thread;
    pthread_create(&thread, 0, start_new_downloader, (void*)downloader);

    return downloader;
}

DownloadBuffer * Cache::get_from_cache(std::pair<std::string, std::string> key, Buffer * buffer_to_server) {
    DownloadBuffer * buffer;

    pthread_mutex_lock(&mtx);

    if (!downloaders.count(key)) {
        Downloader * downloader = create_new_downloader(key.first, key.second, buffer_to_server);
        downloaders[key] = downloader;

        if (NULL == downloader) {
            fprintf(stderr, "Can not to resolve host\n");
            pthread_mutex_unlock(&mtx);
            return NULL;
        }

        buffer = downloader->get_buffer_from_server();
    }
    else {
        fprintf(stderr, "Have data in cache\n");
        buffer = downloaders[key]->get_buffer_from_server();
    }

    pthread_mutex_unlock(&mtx);

    return buffer;
}

Cache::~Cache() {
    for (auto elem : downloaders) {
        delete elem.second;
    }

    pthread_mutex_destroy(&mtx);
}