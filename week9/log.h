/**
 * Created by ilya on 3/7/19.
 */

#ifndef NETWORKS_LABS_LOG_H
#define NETWORKS_LABS_LOG_H

#include <stdio.h>
#include <stdarg.h>

#define DEBUG 10
#define OUT   15
#define INFO  20
#define WARN  25
#define ERROR 30

#define LOG_LEVEL 10

#define __COLOR_YELLOW "\e[33;1m"
#define __COLOR_WHITE  "\e[37;1m"
#define __COLOR_RESET  "\e[0m"

#define log(level, str) if ((level)>=LOG_LEVEL) { __log_impl(#level, str); }
#define logf(level, str, ...) if ((level)>=LOG_LEVEL) { __log_impl(#level, str, __VA_ARGS__); }

void __log_impl(const char *level, const char *format, ...) {
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

#endif //NETWORKS_LABS_LOG_H
