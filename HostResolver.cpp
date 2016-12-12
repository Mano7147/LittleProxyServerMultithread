//
// Created by mano on 11.12.16.
//

#include "HostResolver.h"

HostResolver::HostResolver() {
    pthread_mutex_init(&mtx_gethostbyname, 0);
}

int HostResolver::get_server_address(std::string host_name, struct sockaddr_in &dest_addr) {
    pthread_mutex_lock(&mtx_gethostbyname);

    struct hostent * host_info = gethostbyname(host_name.c_str());

    if (NULL == host_info) {
        perror("gethostbyname");

        pthread_mutex_unlock(&mtx_gethostbyname);

        return RESULT_INCORRECT;
    }

    dest_addr.sin_family = PF_INET;
    dest_addr.sin_port = htons(DEFAULT_PORT);
    memcpy(&dest_addr.sin_addr, host_info->h_addr, host_info->h_length);

    pthread_mutex_unlock(&mtx_gethostbyname);

    return RESULT_CORRECT;
}