//
// Created by ilya on 2/14/19.
//

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>

#define DEST_PORT            2000
#define SERVER_IP_ADDRESS   "127.0.0.1"

struct student_t student;

void setup_tcp_communication() {
    int sockfd = 0;
    int sent_recv_bytes = 0;
    int addr_len = 0;
    addr_len = sizeof(struct sockaddr);
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = DEST_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));
    //press Ctrl+C to stop, that's easy
    while(1) {
        printf("Enter name : \n");
        scanf("%s", student.name);
        printf("Enter age : \n");
        scanf("%i", &student.age);
        printf("Enter group : \n");
        scanf("%s",student.group);
        sent_recv_bytes = sendto(sockfd,&student,sizeof(struct student_t),0,
                                 (struct sockaddr *)&dest,sizeof(struct sockaddr));
        printf("No of bytes sent = %d\n", sent_recv_bytes);
        sent_recv_bytes =  recvfrom(sockfd, (char *)&student, sizeof(struct student_t), 0,
                                    (struct sockaddr *)&dest, &addr_len);
        printf("No of bytes received = %d\n", sent_recv_bytes);
        printf("Student received:\n");
        printf(" student.name  = %s\n",student.name);
        printf(" student.age   = %i\n",student.age);
        printf(" student.group = %s\n",student.group);
        printf("\n");
    }
}


int main(int argc, char **argv) {
    setup_tcp_communication();
    printf("application quits\n");
    return 0;
}

