//
// Created by mano on 14.12.16.
//

#include "DownloadBuffer.h"

DownloadBuffer::DownloadBuffer(size_t size) {
    buf = (char*) malloc(size);
    this->size = size;
    this->end = 0;

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&cond, NULL);
}

DownloadBuffer::DownloadBuffer() {
    buf = (char*) malloc(DEFAULT_BUFFER_SIZE);
    this->size = DEFAULT_BUFFER_SIZE;
    this->end = 0;

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&cond, NULL);
}

void DownloadBuffer::resize(size_t size) {
    char * new_buf = (char *)realloc(buf, size);

    if (NULL == new_buf) {
        perror("realloc");
        free(buf);
        exit(EXIT_FAILURE);
    }

    buf = new_buf;
    this->size = size;
}

void DownloadBuffer::do_move_end(size_t size) {
    pthread_mutex_lock(&mtx);

    while (end + size >= this->size) {
        resize(this->size * 2);
    }

    end += size;

    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&mtx);
}

char * DownloadBuffer::get_data_from_buffer(size_t offs, ssize_t &size) {
    pthread_mutex_lock(&mtx);

    if (offs == end && flag_finished) {
        size = 0;
        pthread_mutex_unlock(&mtx);
        return NULL;
    }

    while (offs >= end) {
        pthread_cond_wait(&cond, &mtx);
    }

    size = end - offs;
    char * new_data = buf + offs;

    pthread_mutex_unlock(&mtx);

    return new_data;
}

DownloadBuffer::~DownloadBuffer() {
    fprintf(stderr, "Destructor download buffer\n");

    free(buf);

    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond);
}
