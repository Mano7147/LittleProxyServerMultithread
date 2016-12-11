#include "Includes.h"
#include "Client.h"

std::ofstream out_to_file("output.txt");

Cache * cache;

HostResolver * host_resolver;

int my_server_socket;

// thread and flag is finished
pthread_mutex_t mtx_threads;
int global_id = 0;
pthread_mutex_t mtx_ids;
std::vector<int> ids;
std::map<int, std::pair<pthread_t, Client*>> threads;

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

void * new_client_thread_function(void * data) {
    fprintf(stderr, "I am new thread\n");

    pthread_mutex_lock(&mtx_threads);

    pthread_mutex_lock(&mtx_ids);
    int id_in_map = ids.back();
    ids.pop_back();
    pthread_mutex_unlock(&mtx_ids);

    fprintf(stderr, "Id in map: %d\n", id_in_map);
    Client * client = threads[id_in_map].second;

    pthread_mutex_unlock(&mtx_threads);

    fprintf(stderr, "Start main loop\n");
    client->do_all();
    delete client;
    fprintf(stderr, "Client done\n");
    fprintf(stderr, "Loop finished\n");
}

void accept_incoming_connection() {
    struct sockaddr_in client_address;
    int address_size = sizeof(sockaddr_in);

    fprintf(stderr, "Before accept\n");
    int client_socket = accept(my_server_socket, (struct sockaddr *)&client_address, (socklen_t *)&address_size);
    fprintf(stderr, "After accept\n");

    if (client_socket <= 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    Client * new_client = new Client(client_socket, cache, host_resolver);
    pthread_t new_thread;

    pthread_mutex_lock(&mtx_threads);
    threads[global_id] = std::make_pair(new_thread, new_client);
    pthread_mutex_unlock(&mtx_threads);

    pthread_mutex_lock(&mtx_ids);
    ids.push_back(global_id);
    pthread_mutex_unlock(&mtx_ids);

    pthread_create(&new_thread, 0, new_client_thread_function, NULL);
    threads[global_id].first = new_thread;

    ++global_id;
}

void delete_finished_threads() {
    fprintf(stderr, "Delete clients\n");

    pthread_mutex_lock(&mtx_threads);

    fprintf(stderr, "Before delete: %ld\n", threads.size());

    std::map<int, std::pair<pthread_t, Client*>> new_threads;
    for (auto t : threads) {
        if (t.second.second->is_loop_finished()) {
            delete t.second.second;
            pthread_join(t.second.first, 0);
        }
        else {
            new_threads[t.first] = std::make_pair(t.second.first, t.second.second);
        }
    }
    threads = new_threads;

    fprintf(stderr, "After delete: %ld\n", threads.size());

    pthread_mutex_unlock(&mtx_threads);
}

void start_main_loop() {
    bool flag_execute = true;

    for ( ; flag_execute ; ) {
        accept_incoming_connection();
        fprintf(stderr, "Between\n");
        //delete_finished_threads();
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

    pthread_mutex_init(&mtx_threads, 0);
    pthread_mutex_init(&mtx_ids, 0);

    start_main_loop();

    delete cache;

    close(my_server_socket);

    return 0;
}
