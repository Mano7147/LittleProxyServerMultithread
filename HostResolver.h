//
// Created by mano on 11.12.16.
//

#ifndef LITTLEPROXYSERVER_HOSTRESOLVER_H
#define LITTLEPROXYSERVER_HOSTRESOLVER_H

#include "Includes.h"


class HostResolver {
    pthread_mutex_t mtx_gethostbyname;

public:
    HostResolver();

    int get_server_address(std::string host_name, struct sockaddr_in &dest_addr);
};


#endif //LITTLEPROXYSERVER_HOSTRESOLVER_H
