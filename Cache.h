//
// Created by mano on 10.12.16.
//

#ifndef PORTFORWARDERREDEAWROFTROP_CACHE_H
#define PORTFORWARDERREDEAWROFTROP_CACHE_H

#include "Includes.h"
#include "Downloader.h"

struct Record {
    char * data;
    size_t size;
    long long last_time_use;
};

class Cache {
    std::map<std::pair<std::string, std::string>, Downloader*> downloaders;

    pthread_mutex_t mtx;

    static void * start_new_downloader(void * pdownloader);

    Downloader * create_new_downloader(std::string host_name, std::string new_first_line, Buffer * buffer_to_server);

public:

    Cache();

    void delete_from_cache(std::pair<std::string, std::string> key);

    DownloadBuffer * get_from_cache(std::pair<std::string, std::string> key, Buffer * buffer_to_server);

    ~Cache();
};


#endif //PORTFORWARDERREDEAWROFTROP_CACHE_H
