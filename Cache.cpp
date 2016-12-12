//
// Created by mano on 10.12.16.
//

#include "Cache.h"
#include <iostream>
#include <fstream>

Cache::Cache() {
    pthread_mutex_init(&mtx, 0);
}

bool Cache::is_in_cache(std::pair<std::string, std::string> key) {
    pthread_mutex_lock(&mtx);

    fprintf(stderr, "Check cache: %s %s\n", key.first.c_str(), key.second.c_str());
    bool result = (bool) cache.count(key);

    pthread_mutex_unlock(&mtx);

    fprintf(stderr, "Result: %d\n", result);

    return result;
}

Record Cache::get_from_cache(std::pair<std::string, std::string> key) {
    Record result;

    pthread_mutex_lock(&mtx);

    if (!cache.count(key)) {
        Record record;
        record.data = NULL;
        result =  record;
    }
    else {
        result =  cache[key];
    }

    pthread_mutex_unlock(&mtx);

    return result;
}

void Cache::delete_from_cache(std::pair<std::string, std::string> key) {
    pthread_mutex_lock(&mtx);

    if (!cache.count(key)) {
        return;
    }

    free(cache[key].data);
    cache.erase(key);

    pthread_mutex_unlock(&mtx);
}

int Cache::push_to_cache(std::pair<std::string, std::string> key, char *value, size_t size_value) {
    pthread_mutex_lock(&mtx);

    fprintf(stderr, "!!!!!!!!!!! PUSH !!!: %s %s\n", key.first.c_str(), key.second.c_str());

    if (cache.count(key)) {
        free(cache[key].data);
    }

    cache[key].data = (char*) malloc(size_value);
    memcpy(cache[key].data, value, size_value);

    cache[key].size = size_value;

    pthread_mutex_unlock(&mtx);
}

Cache::~Cache() {
    for (auto elem : cache) {
        free(elem.second.data);
    }
}