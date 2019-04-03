/**
 * Created by Ilya Potemin on 3/7/19.
 * Last updated 4/3/19
 */

#ifndef NETWORKS_LABS_NODE_H
#define NETWORKS_LABS_NODE_H

#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 22022

#define MAX_SERVER_WORKERS 8
#define CLIENT_WARN_CONNECTIONS 5
#define CLIENT_MAX_CONNECTIONS 10
#define CLIENT_MAX_SYN_FAILS 5
#define CLIENT_MAX_REQ_FAILS 5

/**
 * Request:
 *      filename = null terminated string
 * Response:
 *      N - integer, 4 bytes, number of words
 *      word (N times) - string, word
 *
 * if file not found return N = -1
 */
#define COMMAND_REQUEST 0x0

/**
 * Request:
 *      command = 4 bytes
 *      name:ip_address:port:[files1,file2,...] - first null terminated string
 *      N - integer, 4 bytes, number of peers
 *      name:ip_address:port (N times) - peer
 *
 *  No response is required
 */
#define COMMAND_SYN 0x1


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
#define COMMAND_CONNECT 16

/**
 * Response: List
 *
 * List:
 *      list_size   = 4 bytes
 *      entries     = list_size * sizeof(Entry)
 *
 * First entry will be the one you are asking
 */
#define COMMAND_RETRIEVE_NODES 17

/**
 * Response: "pong" (4 bytes)
 */
#define COMMAND_PING 18

/**
 * Request:
 *      command = 4 bytes
 *      entry   = entry_bytes
 *
 * No response should be given
 *
 * If this node
 */
#define COMMAND_DISCONNECT 19

/**
 * Request:
 *      command         = 4 bytes
 *      filename_length = 2 bytes
 *      filename        = filename_length
 *
 * Response:
 *      file_size       = 4 bytes
 */
#define COMMAND_GET_FILE_INFO 20

/**
 * Request:
 *      command         = 4 bytes
 *      filename_length = 2 bytes
 *      filename        = filename_length
 *      offset          = 4 bytes
 *      length          = 4 bytes
 *
 * Response:
 *      payload_length  = 4 bytes
 *      payload         = payload_length
 */
#define COMMAND_TRANSFER 21


#define MAX_PAYLOAD_LENGTH 0x00100000 //1 MB

#ifdef __GNUC__
# define __PURE __attribute((pure))
# define __PACKED __attribute((packed))
# define __UNUSED __attribute((unused))
# define __DESTRUCTOR(priority) __attribute((destructor(priority)))
# define __CONSTRUCTOR(priority) __attribute((constructor(priority)))
#else
# define __PURE
# define __PACKED
# define __UNUSED
# define __DESTRUCTOR
# define __CONSTRUCTOR
#endif

#define __CLI_FUNC
#define __FLAG_FUNC
#define __SERV_FUNC
#define __UTIL_FUNC
#define __THREAD_FUNC


struct node {
    //uint32_t = 4 bytes
    in_addr_t address;
    //uint16_t = 2 bytes
    in_port_t port;
    //2 bytes
    union {
        uint16_t name_length;
        uint16_t thread_count;
    };
    //name_length bytes
    char name[128];
};

#define NODE_HASH(node) \
    ((uint32_t) (node)->address & (node)->port)

#define NEW_NODE() ((node_t*) malloc(sizeof(node_t)))

struct client_info {
    in_addr_t address;
    in_port_t port;
    int32_t cur_conn;
    int32_t failed_syn;
    int32_t failed_req;
    uint8_t trusted;
    pthread_mutex_t lock;
};

#define CLIENT_HASH(address, port) \
    ((uint32_t) (address))

struct server_config {
    in_addr_t addr;
    in_port_t port;
    char port_str[6];
    char node_address[20];
    char *node_name;
    char enable_hton;
};

struct server_worker {
    void *buffer;
    in_addr_t client_addr;
    in_port_t client_port;
    int client_socket;
    size_t buffer_size;
    pthread_t thread;
};

typedef struct sockaddr sockaddr_t;
typedef struct sockaddr_in sockaddr_in_t;

typedef struct node node_t;
typedef struct client_info client_info_t;
typedef struct server_config server_config_t;
typedef struct server_worker server_worker_t;


#define PROCESSING_FUNC_LENGTH 32
#define FLAG_FUNC_LENGTH 64

typedef int (*process_command_func_t)(server_worker_t * /* context of call */, int /* command */,
                                      client_info_t * /* client info */);

//returns count of additionally read arguments
typedef int (*flag_func_t)(int /* flag_index */, int /* argc */, char ** /* argv */);

#endif //NETWORKS_LABS_NODE_H
