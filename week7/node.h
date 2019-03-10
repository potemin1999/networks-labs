/**
 * Created by ilya on 3/7/19.
 */

#ifndef NETWORKS_LABS_NODE_H
#define NETWORKS_LABS_NODE_H

#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 22022

/**
 * Entry:
 *      address     = 4 bytes
 *      port        = 2 bytes
 *      name_length = 1 byte
 *      name        = name_length bytes, 128 at max
 */

/**
 * Request:
 *      command = 4 bytes
 *      entry   = entry_bytes
 *
 * Response: 0 (1 byte)
 *
 * If you do not have this node in local database,
 *  then add and notify all others
 * If you do, just ignore this message and send response
 */
#define COMMAND_CONNECT 10

/**
 * Response: List
 *
 * List:
 *      list_size   = 4 bytes
 *      entries     = list_size * sizeof(Entry)
 *
 * First entry will be the one you are asking
 */
#define COMMAND_RETRIEVE_NODES 20

/**
 * Response: "pong" (4 bytes)
 */
#define COMMAND_PING 30

/**
 * Request:
 *      command = 4 bytes
 *      entry   = entry_bytes
 *
 * No response should be given
 *
 * If this node
 */
#define COMMAND_DISCONNECT 40

//typedef unsigned char uint8_t;


struct node {
    //uint32_t = 4 bytes
    in_addr_t address;
    //uint16_t = 2 bytes
    in_port_t port;
    //1 bytes
    uint8_t name_length;
    //name_length bytes
    char name[128];
};

#define NODE_HASH(node) \
    ((uint32_t) (node)->address & (node)->port)

struct node_list {
    uint32_t list_size;
    struct node_t *nodes;
};


struct server_config {
    in_port_t port;
    char *node_name;
};

typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_in sockaddr_in_t;

typedef struct node node_t;
typedef struct node_list node_list_t;
typedef struct server_config server_config_t;

char *get_random_node_name();

int connect_to(const char *ip, uint16_t port);

int bind_to(uint16_t port);

int dump_getsockname(int socket_fd);

int read_config(server_config_t *server_config);

int do_bootstrap(const char *ip, uint16_t port);

int process_comm_socket(int comm_socket);

#endif //NETWORKS_LABS_NODE_H
