#ifndef _log_h
#define _log_h

#define LOG(...) {                              \
    time_t t = time(NULL);                      \
    struct tm *tm = localtime(&t);              \
    char buff[32];                              \
    strftime(buff, 32, "[%F %T]", tm);          \
    printf("[INFO] \e[36m%s\e[0m ", buff);      \
    printf(__VA_ARGS__);                        \
    putchar('\n'); }

#define WARN(...) {                                     \
    time_t t = time(NULL);                              \
    struct tm *tm = localtime(&t);                      \
    char buff[32];                                      \
    strftime(buff, 32, "[%F %T]", tm);                  \
    printf("\e[35m[WARN]\e[0m \e[36m%s\e[0m ", buff);   \
    printf(__VA_ARGS__);                                \
    putchar('\n'); }

#define ERR(...) {                                      \
    time_t t = time(NULL);                              \
    struct tm *tm = localtime(&t);                      \
    char buff[32];                                      \
    strftime(buff, 32, "[%F %T]", tm);                  \
    printf("\e[31m[ERRO]\e[0m \e[36m%s\e[0m ", buff);   \
    printf(__VA_ARGS__);                                \
    putchar('\n'); }

#endif
