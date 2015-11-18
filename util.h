#ifndef _UTIL_H_
#define _UTIL_H_

struct request{
    int seat_id;
    int user_id;
    int customer_priority;
    char* resource;
};
typedef struct arguments
{
	int connfd;
	struct request req;
	int customer_priority;
}argu;



void parse_request(argu* arguments);
void process_request(argu* arguments);
extern int standbylist_add_task(pool_t *pool, void (*function)(void *), void* argument);

#endif
