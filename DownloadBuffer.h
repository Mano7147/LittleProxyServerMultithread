//
// Created by mano on 14.12.16.
//

#ifndef LITTLEPROXYSERVER_DOWNLOADBUFFER_H
#define LITTLEPROXYSERVER_DOWNLOADBUFFER_H

#include "Includes.h"

class DownloadBuffer {
    char * buf;
    size_t end;
    size_t size;

    bool flag_finished;
    bool flag_finished_correct;

    pthread_mutex_t mtx;
    pthread_cond_t cond;

public:

    DownloadBuffer(size_t size);

    DownloadBuffer();

    void resize(size_t size);

    void do_move_end(size_t size);

    char * get_data_from_buffer(size_t offs, ssize_t &size);

    char * get_end() {
        return buf + end;
    }

    size_t get_empty_size() {
        return size - end;
    }

    void set_finished_correct() {
        flag_finished = true;
        flag_finished_correct = true;
    }

    void set_finished_incorrect() {
        flag_finished = true;
        flag_finished_correct = false;
    }

    bool is_finished_correct() {
        return flag_finished && flag_finished_correct;
    }

    ~DownloadBuffer();
};


#endif //LITTLEPROXYSERVER_DOWNLOADBUFFER_H
