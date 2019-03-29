/**
 * Created by Ilya Potemin on 3/7/19.
 * Last updated 3/24/19
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "log.h"
#include "node.h"
#include "storage.h"

map_t *storage;
pthread_t server_thread;
server_config_t server_config;
int server_sock_fd;
unsigned shutdown_required = 0;

typedef int (*process_command_func_t)(server_worker_t *, int, sockaddr_in_t *);

process_command_func_t processing_functions[32];

void *ensure_buffer(server_worker_t *worker, int min_size);

char *get_random_node_name() {
    char *name = (char *) malloc(16);
    sprintf(name, "name-%p", &name);
    return name;
}

int connect_to_addr(in_addr_t addr, uint16_t port) {
    sockaddr_in_t dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = addr;
    //create socket and connect
    int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(socket_fd, (sockaddr_t *) &dest, sizeof(sockaddr_t));
    return socket_fd;
}

int connect_to(const char *ip, uint16_t port) {
    struct hostent *host;
    host = gethostbyname(ip);
    if (!host)
        return -1;
    return connect_to_addr(*((in_addr_t *) host->h_addr), port);
}

int bind_to(uint16_t port) {
    //create server socket
    int master_sock_tcp_fd = 0;
    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        log(ERROR, "Server socket creation failed, exiting");
        return -1;
    }
    int optval = 1;
    setsockopt(master_sock_tcp_fd, SOL_SOCKET, SO_REUSEADDR,
               (const void *) &optval, sizeof(int));
    //bind server socket
    sockaddr_in_t server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    socklen_t sockaddr_len = sizeof(sockaddr_t);
    if (bind(master_sock_tcp_fd, (sockaddr_t *) &server_addr, sockaddr_len) == -1) {
        log(ERROR, "Server socket bind failed, exiting");
        return -2;
    }
    return master_sock_tcp_fd;
}

int dump_getsockname(int socket_fd) {
    sockaddr_in_t sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket_fd, (sockaddr_t *) &sin, &len) == -1) {
        log(ERROR, "Getsockname failed, something wrong");
        return -1;
    } else {
        logf(DEBUG, "Port number %u", ntohs(sin.sin_port));
        return 0;
    }
}

int read_config(server_config_t *server_config) {
    //read name
    printf("Write server name (default is random) : ");
    char name_buffer[256];
    bzero(name_buffer, 256);
    int name_scanned = scanf("%s", name_buffer);
    if (name_scanned) {
        size_t name_length = strlen(name_buffer);
        char *name_str = malloc(name_length + 1);
        strcpy(name_str, name_buffer);
        server_config->node_name = name_str;
    } else {
        server_config->node_name = get_random_node_name();
    }
    //read port
    printf("Write server port (default 22022) : ");
    char port_buffer[8], *buffer_end;
    bzero(port_buffer, 8);
    int port_scanned = scanf("%s", port_buffer);
    strcpy(server_config->port_str, port_buffer);
    if (port_scanned) {
        server_config->port = (in_port_t) strtoul(port_buffer, &buffer_end, 10);
    } else {
        server_config->port = (in_port_t) SERVER_PORT;
    }
    return 0;
}

int do_bootstrap(const char *ip, uint16_t port) {
    log(INFO, "Retrieving nodes");
    int socket = connect_to(ip, port);
    if (socket == -1) {
        log(ERROR, "Bootstrap failed: unable to connect to server");
        return -1;
    }
    //retrieving nodes
    uint32_t command = htonl(COMMAND_RETRIEVE_NODES);
    send(socket, &command, 4, 0);
    int list_size = 0;
    recv(socket, &list_size, 4, 0);
    logf(DEBUG, "Nodes amount : %d", list_size);
    if (list_size == 0)
        goto do_bootstrap_connect;
    node_t *node_buffer = (node_t *) malloc(sizeof(node_t) * list_size);
    for (int i = 0; i < list_size; i++) {
        recv(socket, &node_buffer[i], sizeof(node_t), 0);
    }
    //workaround
    node_buffer[0].address = *((in_addr_t *) gethostbyname(ip)->h_addr);
    for (int i = 0; i < list_size; i++) {
        char node_address[20];
        inet_ntop(AF_INET, &(node_buffer[i].address), node_address, 20);
        logf(DEBUG, "Receiving node %s at %s:%d", node_buffer[i].name, node_address, node_buffer[i].port);
        storage_node_added(storage, node_buffer[i]);
    }
    do_bootstrap_connect:
    close(socket);
    //connect to this network
    log(INFO, "Connecting to network");
    node_t node;
    strcpy(node.name, server_config.node_name);
    node.name_length = (uint8_t) strlen(node.name);
    node.port = server_config.port;
    struct hostent *host;
    host = gethostbyname("0.0.0.0");
    node.address = *((in_addr_t *) host->h_addr);
    socket = connect_to(ip, port);
    command = COMMAND_CONNECT;
    send(socket, &command, 4, 0);
    send(socket, &node, sizeof(node_t), 0);
    close(socket);
    log(INFO, "Bootstrap done");
    return 0;
}

//COMMAND_SYN; lab9
int do_syn(const char *ip, uint16_t port) {
    int socket = connect_to(ip, port);
    if (socket == -1) {
        log(ERROR, "Syn failed: unable to connect to target server");
        return -1;
    } else {
        log(INFO, "Syn started");
    }
    uint32_t command = htonl(COMMAND_SYN);
    send(socket, &command, 4, 0);
    //1: send info about us
    char buffer_start[1024];
    char *buffer = buffer_start;
    bzero(buffer_start, 1024);
    int size = sprintf(buffer, "%s:%s:%hu:", server_config.node_name, server_config.node_address, server_config.port);
    buffer += size;
    //list files
    DIR *dir;
    struct dirent *dirent;
    dir = opendir(".");
    if (dir) {
        readdir(dir);
        readdir(dir);
        dirent = readdir(dir);
        int limit = 0;
        while (dirent && limit < 1000) {
            buffer += sprintf(buffer, ",%s", dirent->d_name);
            //logf(DEBUG, "Send file name %s", dirent->d_name);
            dirent = readdir(dir);
            limit++;
        }
    }
    //send count of our nodes
    send(socket, buffer_start, 1024, 0);
    logf(DEBUG, "pushing %s to syn server", buffer_start);
    //2: send nodes count
    process_syn_stage2:
    closedir(dir);
    uint32_t stor_size = htonl(storage->size);
    send(socket, &stor_size, 4, 0);
    usleep(50000);
    //3: send nodes one by one
    buffer = buffer_start;
    storage_iter_t iterator = storage_new_iterator(storage);
    node_t *current_node = 0;
    while ((current_node = storage_next(&iterator)) != 0) {
        char *node_addr = inet_ntoa(*((struct in_addr *) &current_node->address));
        bzero(buffer, 1024);
        sprintf(buffer, "%s:%s:%hu", current_node->name, node_addr, current_node->port);
        send(socket, buffer, 1024, 0);
    }
}

//COMMAND_REQUEST: lab9
int do_request(node_t *node, const char *dst_file_name, const char *remote_file_name) {
    int socket = connect_to_addr(node->address, node->port);
    FILE *file = fopen(dst_file_name, "wb");
    //send request file name
    uint32_t command = htonl(COMMAND_REQUEST);
    send(socket, &command, 4, 0);
    char buffer[1024];
    bzero(buffer, 1024);
    strcpy(buffer, remote_file_name);
    send(socket, buffer, 1024, 0);
    logf(DEBUG, "requesting %s", buffer);
    int32_t words_count = 0;
    //receive words count
    recv(socket, &words_count, 4, 0);
    words_count = (int32_t) ntohl((uint32_t) words_count);
    //receive words
    for (int i = 0; i < words_count; i++) {
        bzero(buffer, 1024);
        recv(socket, buffer, 1024, 0);
        fprintf(file, "%s ", (char *) buffer);
    }
    fclose(file);
    return words_count;
}

int do_pingall() {
    log(INFO, "Pinging known nodes");
    int start_size = storage->size;
    storage_iter_t iterator = storage_new_iterator(storage);
    node_t *current_node = 0;
    while ((current_node = storage_next(&iterator)) != 0) {
        int socket = connect_to_addr(current_node->address, current_node->port);
        if (socket == -1) {
            storage_node_removed(storage, *current_node);
            continue;
        }
        uint32_t command = htonl(COMMAND_PING);
        send(socket, &command, 4, MSG_NOSIGNAL);
        command = 0;
        recv(socket, &command, 4, MSG_NOSIGNAL);
        if (command == 0) {
            storage_node_removed(storage, *current_node);
            continue;
        }
        logf(DEBUG, "Node %s is alive", current_node->name);
    }
    int finish_size = storage->size;
    int cleaned_up = start_size - finish_size;
    logf(INFO, "%d nodes pinged, %d alive, %d were cleaned up", start_size, finish_size, cleaned_up);
    return 0;
}

int do_get_remote_file_info(node_t *node, const char *file_name) {
    int socket = connect_to_addr(node->address, node->port);
    size_t file_name_length = (short) strlen(file_name);
    uint32_t command = htonl(COMMAND_GET_FILE_INFO);
    short int fn_len_short = (short int) file_name_length;
    //send get info request
    send(socket, &command, 4, 0);
    send(socket, &fn_len_short, 2, 0);
    send(socket, file_name, (size_t) file_name_length, 0);

    int file_length = 0;
    recv(socket, &file_length, 4, 0);
    return file_length;
}

int do_pull_file_part(node_t *node, const char *file_name, int offset, int length, void *buffer) {
    int socket = connect_to_addr(node->address, node->port);
    size_t file_name_length = (short) strlen(file_name);
    uint32_t command = htonl(COMMAND_TRANSFER);
    short int fn_len_short = (short int) file_name_length;
    //send transfer request
    send(socket, &command, 4, 0);
    send(socket, &fn_len_short, 2, 0);
    send(socket, file_name, (size_t) file_name_length, 0);
    send(socket, &offset, 4, 0);
    send(socket, &length, 4, 0);

    int bytes_received = 0;
    recv(socket, &bytes_received, 4, 0);
    recv(socket, buffer, (size_t) bytes_received, 0);
    return bytes_received;
}

int do_pull_file(node_t *node, const char *dst_file_name, const char *remote_file_name) {
    int file_size = do_get_remote_file_info(node, remote_file_name);
    if (file_size == -1) {
        log(ERROR, "File was not found");
        return -1;
    } else {
        logf(INFO, "File found with length %d", file_size);
    }
    int parts = (file_size / MAX_PAYLOAD_LENGTH) +
                (file_size % MAX_PAYLOAD_LENGTH != 0 ? 1 : 0);
    FILE *file = fopen(dst_file_name, "wb");
    void *buffer = malloc(MAX_PAYLOAD_LENGTH);
    int bytes_left = file_size;
    int bytes_written = 0;
    for (int i = 0; i < parts; i++, bytes_left -= MAX_PAYLOAD_LENGTH) {
        bzero(buffer, MAX_PAYLOAD_LENGTH);
        int read_bytes = bytes_left > MAX_PAYLOAD_LENGTH ? MAX_PAYLOAD_LENGTH : bytes_left;
        do_pull_file_part(node, remote_file_name, i * MAX_PAYLOAD_LENGTH, read_bytes, buffer);
        bytes_written += fwrite(buffer, 1, (size_t) read_bytes, file);
    }
    fflush(file);
    fclose(file);
    return bytes_written;
}

void *ensure_buffer(server_worker_t *worker, int min_size) {
    if (worker->buffer_size < min_size) {
        worker->buffer = worker->buffer ? realloc(worker->buffer, (size_t) min_size)
                                        : malloc((size_t) min_size);
    }
    if (worker->buffer) {
        bzero(worker->buffer, (size_t) min_size);
    }
    return worker->buffer;
}

//COMMAND_SYN, lab 9
int process_command_syn(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    char buffer_holder[1024];
    char *buffer = buffer_holder;
    bzero(buffer, 1024);
    ssize_t msg1size = recv(comm_socket, buffer, 1024, 0);
    if (msg1size <= 0) {
        log(ERROR, "Unable to parse 1st message");
        return 1;
    }
    node_t node;
    bzero(&node, sizeof(node_t));
    char address_str[20], port_str[6], *port_end;
    bzero(address_str, 20);
    int read_chars = 0;
    sscanf(buffer, "%127[^:]:%19[^:]:%5[^:]:%n", node.name, address_str, port_str, &read_chars);
    buffer += read_chars;
    //add or update node
    struct hostent *host = gethostbyname(address_str);
    if (!host) {
        log(ERROR, "Unable to parse hostent");
        return 11;
    }
    node.address = *((in_addr_t *) host->h_addr);
    node.port = (uint16_t) strtoul(port_str, &port_end, 10);
    int ret = storage_node_added(storage, node);
    if (ret != -1) {
        logf(INFO, "Received syn from new node %s at %s:%hu", node.name, address_str, node.port);
    }
    //read node files
    char file_buffer[256];
    bzero(file_buffer, 256);
    read_chars = 0;
    int limit = 0;
    while (limit < 100 && sscanf(buffer, "%255[^,]%n", file_buffer, &read_chars)) {
        logf(INFO, "Node %s has file %s", node.name, file_buffer);
        buffer += read_chars;
        if (*buffer != ',') break;
        limit++;
        buffer++;
    }
    buffer = buffer_holder;
    uint32_t peers_count = 0;
    ssize_t msg2size = recv(comm_socket, &peers_count, 4, 0);
    if (msg2size != 4) {
        logf(ERROR, "Invalid peers value size: %lu", msg2size);
        return 2;
    } else {
        logf(INFO, "Peers count: %lu", peers_count);
    }
    peers_count = ntohl(peers_count);
    for (uint32_t i = 0; i < peers_count && i < 1000; i++) {
        node_t peer_node;
        recv(comm_socket, buffer, 1024, 0);
        sscanf(buffer, "%127[^:]:%19[^:]:%5[^:]:", peer_node.name, address_str, port_str);
        struct hostent *peer_host = gethostbyname(address_str);
        peer_node.address = *((in_addr_t *) peer_host->h_addr);
        peer_node.port = (uint16_t) strtoul(port_str, &port_end, 10);
        int res = storage_node_added(storage, peer_node);
        if (res == -1) continue;
        logf(DEBUG, "New node retrieved: %s at %s:%hu", peer_node.name, address_str, peer_node.port);
    }
    return 0;
}

//COMMAND_REQUEST, lab 9
int process_command_request(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    char buffer[1024];
    recv(comm_socket, buffer, 1024, 0);
    char *file_name = buffer;
    FILE *file = fopen(file_name, "rb");
    if (file == 0) {
        int32_t words_count = -1;
        words_count = (int32_t) htonl((uint32_t) words_count);
        send(comm_socket, &words_count, 4, 0);
        return -1;
    }
    fseek(file, 0, SEEK_SET);
    //count words
    int32_t words_count = 0;
    size_t part_read_size;
    while ((part_read_size = fread(buffer, 1, 1024, file)) > 0) {
        for (int i = 0; i < part_read_size; i++) {
            if (buffer[i] == ' ' || buffer[i] == '\0') words_count++;
        }
    }
    int32_t words_count_n = (int32_t) htonl((uint32_t) words_count);
    send(comm_socket, &words_count_n, 4, 0);
    //send word by word
    fseek(file, 0, SEEK_SET);
    int word_length = 0;
    for (int i = 0; i < words_count; i++) {
        bzero(buffer, 1024);
        fscanf(file, "%s%n", buffer, &word_length);
        send(comm_socket, buffer, 1024, 0);
        usleep(20000);
    }
    return 0;
}

//COMMAND_CONNECT
int process_command_connect(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    node_t node;
    recv(comm_socket, &node, sizeof(node_t), 0);
    int res = storage_node_added(storage, node);
    char answer = (char) 0;
    send(comm_socket, &answer, 1, 0);
    char node_address[20];
    inet_ntop(AF_INET, &(node.address), node_address, 20);
    logf(DEBUG, "New node have connected: %s at %s:%d", node.name, node_address, node.port);
    if (res == -1)
        return -1;
    storage_iter_t iterator = storage_new_iterator(storage);
    node_t *current_node = 0;
    while ((current_node = storage_next(&iterator)) != 0) {
        if (current_node->address == node.address && current_node->port == node.port)
            continue;
        int32_t command_connect = COMMAND_CONNECT;
        int socket = connect_to_addr(current_node->address, current_node->port);
        send(socket, &command_connect, 4, 0);
        send(socket, &node, sizeof(node_t), 0);
        close(socket);
    }
    return 0;
}

//COMMAND_RETRIEVE_NODES
int process_command_retrieve_nodes(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    int length = storage->size + 1;
    send(comm_socket, &length, 4, 0);
    //send this node
    node_t this_node;
    this_node.port = server_config.port;
    strcpy(this_node.name, server_config.node_name);
    send(comm_socket, &this_node, sizeof(node_t), 0);
    //send other nodes
    storage_iter_t iterator = storage_new_iterator(storage);
    node_t *current_node = 0;
    while ((current_node = storage_next(&iterator)) != 0) {
        send(comm_socket, current_node, sizeof(node_t), 0);
    }
    logf(DEBUG, "%d nodes are written to %u", storage->size, client_addr->sin_addr.s_addr);
    return 0;
}

//COMMAND_PING
int process_command_ping(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    char *pong_str = "pong";
    send(comm_socket, pong_str, 4, 0);
    log(DEBUG, "Ponged");
    return 0;
}

//COMMAND_DISCONNECT
int process_command_disconnect(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    node_t node;
    recv(comm_socket, &node, sizeof(node_t), 0);
    int result = storage_node_removed(storage, node);
    if (result == -1)
        return -1;
    storage_iter_t iterator = storage_new_iterator(storage);
    node_t *current_node = 0;
    while ((current_node = storage_next(&iterator)) != 0) {
        int socket = connect_to_addr(current_node->address, current_node->port);
        if (socket == -1) continue;
        send(socket, &node, sizeof(node_t), 0);
        close(socket);
    }
    return 0;
}

//COMMAND_GET_FILE_INFO
int process_command_get_file_info(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    short int fn_length = 0;
    char *filename = 0;
    recv(comm_socket, &fn_length, 2, 0);
    filename = (char *) ensure_buffer(worker, fn_length + 1);
    recv(comm_socket, filename, (size_t) fn_length, 0);
    filename[fn_length] = '\0';
    FILE *file = fopen(filename, "rb");
    long file_size;
    if (file != 0) {
        fseek(file, 0L, SEEK_END);
        file_size = ftell(file);
    } else {
        file_size = -1;
    }
    send(comm_socket, &file_size, 4, 0);
    return 0;
}

//COMMAND_TRANSFER
int process_command_transfer(server_worker_t *worker, int comm_socket, sockaddr_in_t *client_addr) {
    short int fn_length = 0;
    char *filename = 0;
    int offset = 0;
    int length = 0;
    void *buffer = 0;
    recv(comm_socket, &fn_length, 2, 0);
    filename = (char *) malloc((size_t) (fn_length + 1));
    filename[fn_length] = '\0';
    recv(comm_socket, filename, (size_t) fn_length, 0);
    recv(comm_socket, &offset, 4, 0);
    recv(comm_socket, &length, 4, 0);
    buffer = ensure_buffer(worker, length);
    FILE *file = fopen(filename, "rb");
    int read = 0;
    if (file != 0) {
        fseek(file, offset, SEEK_SET);
        read = (int) fread(buffer, 1, (size_t) length, file);
    }
    send(comm_socket, &read, 4, 0);
    send(comm_socket, buffer, (size_t) read, 0);
    logf(DEBUG, "Send %d bytes of file %s", read, filename);
    fclose(file);
    free(buffer);
    return 0;
}

//Command processing entry point
int process_comm_socket(server_worker_t *worker, int comm_socket) {
    uint32_t command = 0;
    sockaddr_in_t client_addr;
    socklen_t addr_len = sizeof(sockaddr_t);
    ssize_t read_bytes = recvfrom(comm_socket, &command, 4, 0, (sockaddr_t *) &client_addr, &addr_len);
    command = ntohl(command);
    if (read_bytes == 0)
        return -1;
    switch (command) {
        case COMMAND_SYN:
        case COMMAND_REQUEST:
        case COMMAND_CONNECT:
        case COMMAND_DISCONNECT:
        case COMMAND_RETRIEVE_NODES:
        case COMMAND_GET_FILE_INFO:
        case COMMAND_TRANSFER:
        case COMMAND_PING: {
            logf(DEBUG, "received command %u", command);
            process_command_func_t func = processing_functions[command];
            (*func)(worker, comm_socket, &client_addr);
            break;
        }
        default: {
            logf(WARN, "command not recognised: %u", command);
            process_command_ping(worker, comm_socket, &client_addr);
        }
    }
    return 0;
}

void *server_main(void *data) {
    server_config_t *cfg = (server_config_t *) data;
    int server_socket_fd = 0;
    if ((server_socket_fd = bind_to(cfg->port)) < 0) {
        exit(server_socket_fd);
    }
    if (dump_getsockname(server_socket_fd) < 0) {
        exit(server_socket_fd);
    }
    storage = storage_create();
    if (listen(server_socket_fd, 5) < 0) {
        log(ERROR, "Listen failed");
        exit(1);
    }
    server_sock_fd = server_socket_fd;
    log(INFO, "Server is ready to accept connections");
    sockaddr_in_t client_addr;
    socklen_t addr_len = sizeof(sockaddr_t);
    server_worker_t main_worker;
    while (shutdown_required == 0) {
        int socket = accept(server_socket_fd, (struct sockaddr *) &client_addr, &addr_len);
        if (socket < 0) {
            logf(ERROR, "Invalid communication socket: %i", socket);
            continue;
        }
        process_comm_socket(&main_worker, socket);
        //close(socket);
    }
    storage_destroy(storage);
    return 0;
}

void *client_main(void) {
    char buffer[512];
    while (1) {
        bzero(buffer, 512);
        scanf("%s", buffer);
        if (strcmp(buffer, "exit") == 0) {
            log(OUT, "Exiting");
            shutdown_required = 1;
            close(server_sock_fd);
            pthread_cancel(server_thread);
            break;
        }
        if (strcmp(buffer, "pingall") == 0) {
            log(OUT, "Trying to refresh database...");
            do_pingall();
            continue;
        }
        if (strcmp(buffer, "help") == 0) {
            const char *help = "Available commands:\n"\
            " syn        - starting synchronization via compatible protocol\n" \
            " receive    - pulls required file via compatible protocol\n"\
            " bootstrap  - starting synchronization with other server regions\n" \
            " help       - this message\n"\
            " pingall    - ping all servers to refresh local database\n"\
            " pull       - pulls required file from required node\n"\
            " exit       - close this node\n";
            printf("%s", help);
            continue;
        }
        if (strcmp(buffer, "syn") == 0) {
            char ip_buffer[16];
            printf("Syn server ip  : ");
            scanf("%s", ip_buffer);
            char port_buffer[8], *buffer_end;
            printf("Syn server port: ");
            scanf("%s", port_buffer);
            uint16_t port = (uint16_t) strtoul(port_buffer, &buffer_end, 10);
            do_syn(ip_buffer, port);
            continue;
        }
        if (strcmp(buffer, "bootstrap") == 0) {
            char ip_buffer[16];
            printf("Bootstrap server ip  : ");
            scanf("%s", ip_buffer);
            char port_buffer[8], *buffer_end;
            printf("Bootstrap server port: ");
            scanf("%s", port_buffer);
            uint16_t port = (uint16_t) strtoul(port_buffer, &buffer_end, 10);
            do_bootstrap(ip_buffer, port);
            continue;
        }
        if (strcmp(buffer, "request") == 0) {
            char node_name[128];
            printf("Source node name      : ");
            scanf("%s", node_name);
            storage_iter_t iterator = storage_new_iterator(storage);
            node_t *node = 0;
            while ((node = storage_next(&iterator)) != 0) {
                if (strcmp(node->name, node_name) == 0)
                    break;
            }
            if (node == 0) {
                log(ERROR, "Node not found in local database");
                continue;
            }
            char src_filename[256];
            printf("Source file name      : ");
            scanf("%255s", src_filename);
            char dst_filename[256];
            printf("Destination file name : ");
            scanf("%255s", dst_filename);
            int ret = do_request(node, dst_filename, src_filename);
            logf(INFO, "File pull completed: received %d words", ret);
            continue;
        }
        if (strcmp(buffer, "pull") == 0) {
            char node_name[128];
            printf("Source node name      : ");
            scanf("%s", node_name);
            storage_iter_t iterator = storage_new_iterator(storage);
            node_t *node = 0;
            while ((node = storage_next(&iterator)) != 0) {
                if (strcmp(node->name, node_name) == 0)
                    break;
            }
            if (node == 0) {
                log(ERROR, "Node not found in local database");
                continue;
            }
            char src_filename[128];
            printf("Source file name      : ");
            scanf("%s", src_filename);
            char dst_filename[128];
            printf("Destination file name : ");
            scanf("%s", dst_filename);
            int ret = do_pull_file(node, dst_filename, src_filename);
            logf(INFO, "File pull completed: received %d bytes", ret);
            continue;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    log(INFO, "Starting new node");
    processing_functions[COMMAND_SYN] = &process_command_syn;
    processing_functions[COMMAND_REQUEST] = &process_command_request;
    processing_functions[COMMAND_CONNECT] = &process_command_connect;
    processing_functions[COMMAND_DISCONNECT] = &process_command_disconnect;
    processing_functions[COMMAND_RETRIEVE_NODES] = process_command_retrieve_nodes;
    processing_functions[COMMAND_GET_FILE_INFO] = &process_command_get_file_info;
    processing_functions[COMMAND_TRANSFER] = &process_command_transfer;
    processing_functions[COMMAND_PING] = &process_command_ping;
    read_config(&server_config);
    char hostname[64];
    printf("Write server ip: ");
    scanf("%s", hostname);
    struct hostent *host_entry = gethostbyname(hostname);
    char *address = inet_ntoa(*((struct in_addr *) host_entry->h_addr_list[0]));
    strcpy(server_config.node_address, address);
    pthread_create(&server_thread, 0, server_main, &server_config);
    client_main();
    pthread_join(server_thread, 0);
}
