//
// Created by ilya on 2/14/19.
//

#ifndef NETWORKS_LABS_COMMON_H
#define NETWORKS_LABS_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>

#define STUDENT_NAME_MAX_SIZE 108
#define STUDENT_GROUP_MAX_SIZE 16

struct student_t{
    char name[STUDENT_NAME_MAX_SIZE];
    int age;
    char group[STUDENT_GROUP_MAX_SIZE];
};

#endif //NETWORKS_LABS_COMMON_H
