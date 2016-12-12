#include "Includes.h"
#include "Client.h"

Cache * cache;

HostResolver * host_resolver;

int my_server_socket;

void init_my_server_socket(unsigned short server_port) {
    my_server_socket = socket(PF_INET, SOCK_STREAM, 0);

    int one = 1;
    setsockopt(my_server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (my_server_socket <= 0) {
        perror("Error while creating serverSocket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = PF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server_port);

    if (bind(my_server_socket, (struct sockaddr *)&server_address, sizeof(server_address))) {
        perror("Error while binding");
        exit(EXIT_FAILURE);
    }

    if (listen(my_server_socket, 1024)) {
        perror("Error while listen()");
        exit(EXIT_FAILURE);
    }
}

void * new_client_thread_function(void * data_client) {
    fprintf(stderr, "New thread started\n");

    Client * client = (Client*)data_client;

    fprintf(stderr, "Start client's main function\n");

    client->do_all();

    delete client;

    fprintf(stderr, "New thread finished\n");
}

void accept_incoming_connection() {
    struct sockaddr_in client_address;
    int address_size = sizeof(sockaddr_in);

    int client_socket = accept(my_server_socket, (struct sockaddr *)&client_address, (socklen_t *)&address_size);

    if (client_socket <= 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Create new client\n");

    Client * new_client = new Client(client_socket, cache, host_resolver);
    pthread_t new_thread;

    pthread_create(&new_thread, 0, new_client_thread_function, (void*)(new_client));
}

void start_main_loop() {
    bool flag_execute = true;

    for ( ; flag_execute ; ) {
        accept_incoming_connection();
    }
}

void signal_handle(int sig) {
    perror("perror");
    fprintf(stderr, "Exit with code %d\n", sig);

    delete cache;

    fprintf(stderr, "Close my server socket\n");

    close(my_server_socket);

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handle);
    signal(SIGKILL, signal_handle);
    signal(SIGTERM, signal_handle);
    signal(SIGSEGV, signal_handle);

    // todo GET_OPT!!!

    if (2 != argc) {
        perror("Wrong number of arguments");
        exit(EXIT_FAILURE);
    }

    unsigned short server_port = (unsigned short)atoi(argv[1]);
    if (0 == server_port) {
        perror("Wrong port for listening");
        exit(EXIT_FAILURE);
    }
    init_my_server_socket(server_port);

    cache = new Cache();

    host_resolver = new HostResolver();

    start_main_loop();

    signal_handle(0);

    return 0;
}
