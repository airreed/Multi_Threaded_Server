#ifndef _UTIL_H_
#define _UTIL_H_

struct request {
    int seat_id;
    int user_id;
    int customer_priority;
    char* resource;
};
typedef struct arguments {
    int connfd;
    struct request req;
    int customer_priority;
    clock_t start_time;
} argu;
void parse_request(argu* arguments);
void process_request(argu* arguments);

#endif
