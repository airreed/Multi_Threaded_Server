#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

typedef struct pool_t pool_t;

typedef struct standbylist_element{
  int userid;
  int priority;
}standby_element;

typedef struct pool{
    void (*function)(void *);
    void *argument;
} pool_task_t;

typedef struct circular_array{
	pool_task_t* buf_start;
	pool_task_t* buf_end;
	pool_task_t* data_start;
	pool_task_t* data_end;
} c_queue;

typedef struct standby_circular_array{
  standby_element* buf_start;
  standby_element* buf_end;
  standby_element* data_start;
  standby_element* data_end;
} stand_queue;

struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  int available;
  pthread_t *threads;
  c_queue queue;
  m_sem_t sema;
  c_queue standbylist;
  int thread_count;
  int task_queue_size_limit;
};

pool_t *pool_create(int thread_count, int queue_size);

int pool_add_task(pool_t *pool, void (*routine)(void *), void *arg);
int standbylist_add_task(pool_t *pool, void (*function)(void *), void* argument);
int pool_destroy(pool_t *pool);
int addToQueue(c_queue *queue);
int delFromQueue(c_queue *queue);
void initQueue(c_queue* queue,int queue_size);

#endif
