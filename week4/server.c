//
// Created by ilya on 2/14/19.
//

//Taken from Abhishek Sagar

#include "common.h"
#include <errno.h>
#include <zconf.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT     2000

//test_struct_t test_struct;
//result_struct_t res_struct;
char data_buffer[1024];
struct student_t student;

void
setup_tcp_server_communication() {
    int master_sock_tcp_fd = 0,
        sent_recv_bytes = 0,
        addr_len = 0,
        opt = 1;
    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr,
            client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    addr_len = sizeof(struct sockaddr);
    if (bind(master_sock_tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));
    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("listen failed\n");
        return;
    }
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);
        printf("blocked on select System call...\n");
        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);
        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
            printf("New connection recieved recvd, accept the connection. Client and Server completes TCP-3 way handshake at this point\n");
            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {
                printf("accept error : errno = %d\n", errno);
                exit(0);
            }
            printf("Connection accepted from client : %s:%u\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            while (1) {
                printf("Server ready to service client msgs.\n");
                memset(data_buffer, 0, sizeof(data_buffer));
                sent_recv_bytes = recvfrom(comm_socket_fd, (char *) &student, sizeof(struct student_t), 0,
                                           (struct sockaddr *) &client_addr, &addr_len);
                printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                if (sent_recv_bytes == 0) {
                    close(comm_socket_fd);
                    break;
                }
                //change student...
                bzero(student.group,16);
                strcpy(student.group,"Dropped");
                //send him back
                sent_recv_bytes = sendto(comm_socket_fd, (char *) &student, sizeof(struct student_t), 0,
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));
                printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
            }
        }
    }
}

int main(int argc, char **argv) {
    setup_tcp_server_communication();
    return 0;
}


