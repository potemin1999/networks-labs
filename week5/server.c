//
// Created by ilya on 2/14/19.
//

#include "common.h"
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <zconf.h>

#define THREAD_COUNT    4
#define SERVER_PORT     2000

pthread_t        workers[THREAD_COUNT];
char             data_buffer[1024];
struct student_t student;
int              counter;

void *process_connection(void *data) {
    struct sockaddr_in client_addr;

    socklen_t addr_len  = sizeof(client_addr);
    pthread_t thread    = pthread_self();
    ssize_t   sent_recv_bytes;
    int       *data_i   = (int *) data;
    int       sockfd    = *(data_i + 0);
    int       worker_id = (int) thread;

    memset(&client_addr, 0, sizeof(client_addr));
    while (1) {
        printf("Worker %i ready to service client msgs.\n", worker_id);
        memset(data_buffer, 0, sizeof(data_buffer));
        sent_recv_bytes = recvfrom(sockfd, (char *) &student, sizeof(struct student_t), 0,
                                   (struct sockaddr *) &client_addr, &addr_len);
        printf("Worker %i recvd %zi bytes from client %s:%u\n", worker_id, sent_recv_bytes,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //change student...
        bzero(student.group, 16);
        strcpy(student.group, "Dropped");
        //sleep
        printf("Worker %i started hard work\n",worker_id);
        sleep(10);
        printf("Worker %i finished hard work\n",worker_id);
        //send him back
        sent_recv_bytes = sendto(sockfd, (char *) &student, sizeof(struct student_t), 0,
                                 (struct sockaddr *) &client_addr, addr_len);
        printf("Worker %i sent %zi bytes in reply to client\n", worker_id, sent_recv_bytes);
    }
}

int main(int argc, char **argv) {
    int                sockfd = 0;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("socket creation failed\n");
        exit(1);
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *) &optval, sizeof(int));
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = SERVER_PORT;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("socket bind failed\n");
        return 0;
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        int data[1];
        data[0] = sockfd;
        pthread_create(&workers[i], 0, process_connection, &data);
    }
    for (int j = 0; j < THREAD_COUNT; ++j) {
        pthread_join(workers[j], 0);
    }
    return 0;
}


