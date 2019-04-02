/**
 * Created by ilya on 3/7/19.
 * Last updated 4/3/19
 */

#ifndef NETWORKS_LABS_LOG_H
#define NETWORKS_LABS_LOG_H

#include <stdio.h>
#include <stdarg.h>

#define DEBUG 10
#define INFO  20
#define WARN  25
#define OUT   27
#define ERROR 30

#define FILE_LOG_LEVEL    18
#define CONSOLE_LOG_LEVEL 26

#define __COLOR_YELLOW "\e[33;1m"
#define __COLOR_WHITE  "\e[37;1m"
#define __COLOR_RESET  "\e[0m"

#define LOG(level, str)                                 \
    if ((level)>=CONSOLE_LOG_LEVEL) {                   \
        __log_console_impl(#level, str);                \
    }                                                   \
    if ((level)>=FILE_LOG_LEVEL) {                      \
        __log_file_impl(#level, str);                   \
    }                                                   \

#define LOGf(level, str, ...)                           \
    if ((level)>=CONSOLE_LOG_LEVEL) {                   \
        __log_console_impl(#level, str, __VA_ARGS__);   \
    }                                                   \
    if ((level)>=FILE_LOG_LEVEL) {                      \
        __log_file_impl(#level, str, __VA_ARGS__);      \
    }

FILE *__log_file;

void __log_console_impl(const char *level, const char *format, ...) {
    time_t timer;
    char buffer[26];
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%20.26s ", buffer);
    printf("%s%5.5s%s : %s", __COLOR_YELLOW, level, __COLOR_RESET, __COLOR_WHITE);
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
    printf("%s\n", __COLOR_RESET);
}

void __log_file_impl(const char *level, const char *format, ...) {
    time_t timer;
    char buffer[26];
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(__log_file,"[%5.5d]", getpid());
    fprintf(__log_file,"[%8.8lu] ", pthread_self());
    fprintf(__log_file,"%20.26s ", buffer);
    fprintf(__log_file,"%5.5s : ", level);
    va_list list;
    va_start(list, format);
    vfprintf(__log_file,format, list);
    va_end(list);
    fprintf(__log_file,"\n");
    fflush(__log_file);
}

__attribute((unused))
__attribute((constructor(101)))
int __init_log(){
    __log_file = fopen("./node.log","ab+");
    return 0;
}

__attribute((unused))
__attribute((destructor(101)))
int __destroy_log(){
    fclose(__log_file);
    return 0;
}

#define UNUSED(expr) ((void)(expr));

#endif //NETWORKS_LABS_LOG_H
