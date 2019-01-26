/**
 * Created by ilya on 1/26/19.
 */

#ifndef NETWORKS_LABS_EX1_H
#define NETWORKS_LABS_EX1_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_OUTPUT_BUFFER_SIZE 1024

#define TRACE_CLIENT(){ \
        printf("[T] %s:%d\n",__func__,__LINE__);\
        }

//Server to client message
// 4 bytes for code
// 4 bytes for length
// length bytes for payload
#define SERVER_CODE_OK      0
#define SERVER_CODE_ERROR   1
#define SERVER_CODE_EXIT    2

//Client functions

int CLIENT_INPUT_FD  = 0;
int CLIENT_OUTPUT_FD = 0;

#define CLIENT_USAGE_STRING "Commands: \n"\
                   " push <int_value>\n"\
                   " pop \n"\
                   " empty \n"\
                   " display \n"\
                   " create \n"\
                   " stack_size \n"

#define CLIENT_INPUT_BUFFER_SIZE 32
#define CLIENT_INPUT_PIPE_BUFFER_SIZE 128

#define COMMAND_NOP     0
#define COMMAND_PEEK    1
#define COMMAND_PUSH    2
#define COMMAND_POP     3
#define COMMAND_EMPTY   4
#define COMMAND_DISPLAY 5
#define COMMAND_CREATE  6
#define COMMAND_SIZE    7
#define COMMAND_STOP    8

#define WRITE_BYTE(fd, b){    \
        char byte = (char) b; \
        write(fd,&byte,1);    \
        }

#define WRITE_INTEGER(fd, i){  \
        int integer = (int) i; \
        write(fd,&integer,4);  \
        }

//Server functions and structures

struct integer_stack {
    int                  last;
    int                  value;
    struct integer_stack *next;
};

struct server_state {
    struct integer_stack *stack;
    int                  stack_size;
};

int  SERVER_INPUT_FD  = 0;
int  SERVER_OUTPUT_FD = 0;
char server_output_buffer[SERVER_OUTPUT_BUFFER_SIZE];

#define CREATE_STACK_NODE(stack_node_ptr){ \
        stack_node_ptr = (struct integer_stack*)malloc(sizeof(struct integer_stack)); \
        stack_node_ptr->value = 0;         \
        stack_node_ptr->next = 0;          \
        stack_node_ptr->last = 0;          \
        }

#define WRITE_TO_CLIENT(code, length, str){         \
        int code_value = code;                      \
        int length_value = length;                  \
        write(SERVER_OUTPUT_FD,&code_value, 4);     \
        write(SERVER_OUTPUT_FD,&length_value, 4);   \
        write(SERVER_OUTPUT_FD, str, length);       \
        }

#define WRITE_ERROR(str){              \
        printf("[E] %s\n",str);        \
        int length = strlen(str);      \
        WRITE_TO_CLIENT(SERVER_CODE_ERROR,length,str); \
        }

#define WRITE_LINE(str){               \
        printf("[S] %s\n",str);        \
        int length = strlen(str);      \
        WRITE_TO_CLIENT(SERVER_CODE_OK,length,str); \
        }

#define WRITE_LINE_INT(format_str, integer){ \
        snprintf(server_output_buffer,SERVER_OUTPUT_BUFFER_SIZE,format_str, integer); \
        WRITE_LINE(server_output_buffer);    \
        }

#define ENSURE_STATE(state){ \
        if (!state){         \
            WRITE_LINE_INT("Server state assertion failed at %d",__LINE__); \
            return 10;       \
        }}

#define ENSURE_STACK(state){   \
        ENSURE_STATE(state)    \
        if (!(state->stack)){  \
            WRITE_LINE_INT("Stack assertion failed at %d",__LINE__); \
            return 11;         \
        }}

#endif //NETWORKS_LABS_EX1_H
