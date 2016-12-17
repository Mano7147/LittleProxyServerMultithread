//
// Created by mano on 11.12.16.
//

#include "Parser.h"

std::pair<std::string, std::string> Parser::parse_hostname_and_path(char * uri) {
    char host_name[LITTLE_STRING_SIZE];
    char path[LITTLE_STRING_SIZE];

    char * protocolEnd = strstr(uri, "://");
    if (NULL == protocolEnd) {
        perror("Wrong protocol");
        return std::make_pair("", "");
    }

    char * host_end = strchr(protocolEnd + 3, '/');
    size_t host_length = 0;
    if (NULL == host_end) {
        host_length = strlen(protocolEnd + 3);
        path[0] = '/';
        path[1] = '\0';
    }
    else {
        host_length = host_end - (protocolEnd + 3);
        size_t path_size = strlen(uri) - (host_end - uri);
        strncpy(path, host_end, path_size);
        path[path_size] = '\0';
    }

    strncpy(host_name, protocolEnd + 3, host_length);
    host_name[host_length] = '\0';

    return std::make_pair(std::string(host_name), std::string(path));
}

std::pair<std::string, std::string> Parser::get_new_first_line_and_hostname(Buffer * buffer_in, char *p_new_line) {
    if (NULL == strstr(buffer_in->get_start(), "GET")) {
        fprintf(stderr, "Not GET request\n");
        return std::make_pair("Bad request", "");
    }

    if (NULL == strstr(buffer_in->get_start(), "HTTP/1.0")) {
        fprintf(stderr, "Not HTTP/1.0 request\n");
        return std::make_pair("Bad request", "");
    }

    char first_line[LITTLE_STRING_SIZE];

    size_t first_line_length = p_new_line - buffer_in->get_start();
    strncpy(first_line, buffer_in->get_start(), first_line_length);

    fprintf(stderr, "First line from client:\n");
    print_buffer_data(first_line, first_line_length);

    char * method = strtok(first_line, " ");
    char * uri = strtok(NULL, " ");
    char * version = strtok(NULL, "\n\0");

    fprintf(stderr, "method: %s\n", method);
    fprintf(stderr, "uri: %s\n", uri);
    fprintf(stderr, "version: %s\n", version);

    std::pair<std::string, std::string> parsed = parse_hostname_and_path(uri);

    std::string host_name = parsed.first;
    std::string path = parsed.second;

    if ("" == host_name || "" == path) {
        fprintf(stderr, "Hostname or path haven't been parsed\n");
        return std::make_pair("", "");
    }

    fprintf(stderr, "HostName: \'%s\'\n", host_name.c_str());
    fprintf(stderr, "Path: %s\n", path.c_str());

    std::string http10str = "HTTP/1.0";
    std::string new_first_line = std::string(method) + " " + path + " " + http10str;

    return std::make_pair(host_name, new_first_line);
}

Buffer * Parser::change_request(Buffer * old_buffer, std::string new_first_line) {
    size_t pos_new_line = (strchr(old_buffer->get_buf(), '\n') - old_buffer->get_buf()) + 1;
    size_t size_without_first_line = (old_buffer->get_data_size()) - pos_new_line;

    Buffer * new_buffer = new Buffer(DEFAULT_BUFFER_SIZE);

    new_buffer->add_data_to_end(new_first_line.c_str(), new_first_line.size());
    new_buffer->add_symbol_to_end('\n');
    new_buffer->add_data_to_end(old_buffer->get_start() + pos_new_line, size_without_first_line);

    fprintf(stderr, "New HTTP request:\n");
    print_buffer_data(new_buffer->get_start(), new_buffer->get_data_size());

    return new_buffer;
}

void Parser::print_buffer_data(char * data, size_t size) {
    static std::ofstream out_file("output.txt");

    char * to_print = (char*) malloc(size + 1);
    strncpy(to_print, data, size);
    to_print[size] = '\0';
    fprintf(stderr, "%s\n", to_print);

    // Print only GET requests
    if (NULL != strstr(to_print, "GET")) {
        out_file << to_print << std::endl;
    }

    free(to_print);
}
