/**
 * Created by ilya on 1/26/19.
 */

#include <pthread.h>
#include <string.h>

#include "ex1.h"

int pipe_fd[4];

int client_main();

int server_main();

int main() {
    // client -> server
    pipe(pipe_fd + 0);
    // server -> client
    pipe(pipe_fd + 2);
    SERVER_INPUT_FD  = pipe_fd[0];
    CLIENT_OUTPUT_FD = pipe_fd[1];
    CLIENT_INPUT_FD  = pipe_fd[2];
    SERVER_OUTPUT_FD = pipe_fd[3];
    pid_t pid        = fork();
    if (pid == -1)
        exit(-1);
    if (pid) {
        return server_main();
    } else {
        return client_main();
    }
}

int client_main() {
    printf(CLIENT_USAGE_STRING);
    char input_buffer[CLIENT_INPUT_BUFFER_SIZE];
    char command_buffer[16];
    int  int_buffer[1];
    char buffer[CLIENT_INPUT_PIPE_BUFFER_SIZE];
    for (;;) {
        //write phase
        bzero(command_buffer, 16);
        bzero(input_buffer, CLIENT_INPUT_BUFFER_SIZE);
        scanf("%[^\n]%*c", input_buffer);
        sscanf(input_buffer, "%s", command_buffer);
        if (strcmp(command_buffer, "peek") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_PEEK)
        } else if (strcmp(command_buffer, "push") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_PUSH)
            char *end = 0;
            int_buffer[0] = (int) strtol(input_buffer + 4, &end, 10);
            WRITE_INTEGER(CLIENT_OUTPUT_FD, int_buffer[0]);
        } else if (strcmp(command_buffer, "pop") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_POP)
        } else if (strcmp(command_buffer, "empty") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_EMPTY)
        } else if (strcmp(command_buffer, "display") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_DISPLAY)
            while (1) {
                read(CLIENT_INPUT_FD, int_buffer, 4);
                bzero(buffer, CLIENT_INPUT_PIPE_BUFFER_SIZE);
                if (int_buffer[0] == SERVER_CODE_OK) {
                    read(CLIENT_INPUT_FD, int_buffer, 4);
                    if (int_buffer[0] == 0)
                        break;
                    read(CLIENT_INPUT_FD, buffer, (size_t) int_buffer[0]);
                    printf("%s \n", buffer);
                }
            }
            continue;
        } else if (strcmp(command_buffer, "create") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_CREATE)
        } else if (strcmp(command_buffer, "stack_size") == 0 |
                   strcmp(command_buffer, "size") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_SIZE)
        } else if (strcmp(command_buffer, "exit") == 0 |
                   strcmp(command_buffer, "stop") == 0) {
            WRITE_BYTE(CLIENT_OUTPUT_FD, COMMAND_STOP)
            printf("[C] Client exiting\n");
            exit(0);
        } else {
            printf("Unknown command : '%s'\n", command_buffer);
            continue;
        }
        //read phase
        read(CLIENT_INPUT_FD, int_buffer, 4);
        if (int_buffer[0] == SERVER_CODE_EXIT) {
            printf("Client exiting due to server exit message");
            exit(0);
        }
        bzero(buffer, CLIENT_INPUT_PIPE_BUFFER_SIZE);
        if (int_buffer[0] == SERVER_CODE_ERROR) {
            read(CLIENT_INPUT_FD, int_buffer, 4);
            read(CLIENT_INPUT_FD, buffer, (size_t) int_buffer[0]);
            printf("%s \n", buffer);
        }
        if (int_buffer[0] == SERVER_CODE_OK) {
            read(CLIENT_INPUT_FD, int_buffer, 4);
            read(CLIENT_INPUT_FD, buffer, (size_t) int_buffer[0]);
            printf("%s \n", buffer);
        }
    }
}

int server_nop(struct server_state *state, void *data) {
    return 0;
}

int server_peek(struct server_state *state, void *data) {
    ENSURE_STACK(state)
    WRITE_LINE_INT("Peek top element: %d", state->stack->value)
    return 0;
}

int server_push(struct server_state *state, void *data) {
    ENSURE_STACK(state)
    struct integer_stack *node;
    CREATE_STACK_NODE(node)
    node->value  = *((int *) data);
    node->next   = state->stack;
    state->stack = node;
    state->stack_size++;
    WRITE_LINE_INT("Push top element: %d", state->stack->value)
    return 0;
}

int server_pop(struct server_state *state, void *data) {
    ENSURE_STATE(state)
    if (state->stack) {
        WRITE_LINE_INT("Pop top element: %d", state->stack->value)
        struct integer_stack *current = state->stack;
        struct integer_stack *next    = state->stack->next;
        state->stack = next;
        state->stack_size--;
        free(current);
        return 0;
    } else {
        WRITE_LINE("Pop failed : stack is empty");
        return 1;
    }
}

int server_empty(struct server_state *state, void *data) {
    ENSURE_STATE(state)
    if (state->stack) {
        WRITE_LINE("Stack is not empty")
    } else {
        WRITE_LINE("Stack is empty")
    }
    return 0;
}

int server_display(struct server_state *state, void *data) {
    if (!state) {
        WRITE_LINE("No server state")
        WRITE_LINE("")
        return 10;
    }
    struct integer_stack *current = state->stack;
    if (!current) {
        WRITE_LINE("There is no stack")
        WRITE_LINE("")
        return 11;
    }
    while (!(current->last)) {
        int value = current->value;
        WRITE_LINE_INT(" %d", value);
        current = current->next;
    }
    WRITE_LINE("")
    return 0;
}

int server_create(struct server_state *state, void *data) {
    ENSURE_STATE(state)
    state->stack_size = 0;
    if (state->stack) {
        struct integer_stack *current = state->stack;
        while (current) {
            void *to_delete = current;
            current = current->next;
            free(to_delete);
        }
        CREATE_STACK_NODE(state->stack)
        state->stack->last = 1;
        WRITE_LINE("Current stack was replaced by the new one")
        return 1;
    } else {
        CREATE_STACK_NODE(state->stack)
        state->stack->last = 1;
        WRITE_LINE("Current stack was created")
        return 0;
    }
}

int server_size(struct server_state *state, void *data) {
    ENSURE_STATE(state);
    WRITE_LINE_INT("Stack size: %d", state->stack_size)
    return 0;
}

int server_main() {
    struct server_state state;
    state.stack      = 0;
    state.stack_size = 0;
    //array of 8 functions which returns int;
    int (*cmd_funcs[8])(struct server_state *, void *data);
    cmd_funcs[COMMAND_NOP]     = &server_nop;
    cmd_funcs[COMMAND_PEEK]    = &server_peek;
    cmd_funcs[COMMAND_PUSH]    = &server_push;
    cmd_funcs[COMMAND_POP]     = &server_pop;
    cmd_funcs[COMMAND_EMPTY]   = &server_empty;
    cmd_funcs[COMMAND_DISPLAY] = &server_display;
    cmd_funcs[COMMAND_CREATE]  = &server_create;
    cmd_funcs[COMMAND_SIZE]    = &server_size;
    char command_code = 0;
    int  data         = 0;
    while (1) {
        read(SERVER_INPUT_FD, &command_code, 1);
        if (command_code > 8) {
            WRITE_ERROR("Invalid command");
        }
        if (command_code == COMMAND_STOP) {
            WRITE_LINE("Server stopping")
            exit(0);
        }
        if (command_code == COMMAND_PUSH) {
            read(SERVER_INPUT_FD, &data, 4);
        }
        int exec_code = cmd_funcs[command_code](&state, &data);
        if (exec_code != 0) {
            printf("[W] function execution returned %d\n", exec_code);
        }
    }
}


