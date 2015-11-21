#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
#include "seats.h"
//#include "semaphore.c"
#include "thread_pool.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *	
 *  Feel free to make any modifications you want to the function prototypes and structs
 *	
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 20
#define STANDBY_SIZE 10
static void *thread_do_work(void *vpool);
void* checkPending(void* num_of_seats);

pthread_mutex_t quitlock;
int quit;

extern struct pool_t* g_thread_pool;
extern int sem_wait(m_sem_t *s);
extern int sem_post(m_sem_t *s);
extern void sem_init(m_sem_t *s);
extern void sem_destroy(m_sem_t *s);
extern seat_t* seat_header;
extern int seat_available;
extern pthread_mutex_t seat_flag_lock;
//add a condition value for empty seats
extern pthread_cond_t seat_not_empty;
/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
	pthread_mutex_init (&quitlock, NULL);

	struct pool_t* resPool = (pool_t*)malloc(sizeof(pool_t));
	pthread_mutex_init(&(resPool->lock), NULL);
	pthread_cond_init (&(resPool->notify), NULL);
	resPool->threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
	// initQueue(&resPool->queue,queue_size);
	sem_init(&resPool->sema);
	// initQueue(&resPool->standbylist,STANDBY_SIZE);

	resPool->queue.queue = (pool_task_t*)malloc(sizeof(pool_task_t) * queue_size);
	resPool->queue.size = 0;
	resPool->standbylist.queue= (pool_task_t*)malloc(sizeof(pool_task_t) * STANDBY_SIZE);
	resPool->standbylist.size = 0;

	resPool->thread_count = num_threads;  	//?
	resPool->task_queue_size_limit = queue_size;  //?
	int i;
	for ( i=0;i<num_threads;i++){
		pthread_create(&resPool->threads[i],NULL,thread_do_work,(void*)resPool);
	}
    return resPool;
}
// void initQueue(c_queue* queue,int queue_size){
// 	queue->buf_start = (pool_task_t*)malloc(sizeof(pool_task_t)*queue_size);
// 	queue->buf_end = queue->buf_start + queue_size;
// 	queue->data_start = queue->buf_start;
// 	queue->data_end = queue->buf_start;
// }

void cleanupPending(){

}
/*
 * Add a task to the standbylist
 *
 */
int standbylist_add_task(pool_t *pool, void (*function)(void *), void* argument)
{
	sem_wait(&pool->sema);
	int err = 0;
	if(isFull(&pool->standbylist, STANDBY_SIZE)){
		sem_post(&pool->sema);
		return err;
	}
	err = addToQueue(function, argument, &pool->standbylist, STANDBY_SIZE);
	sem_post(&pool->sema);
	return err;
}

/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void* argument)
{
	// SET a global 
	pthread_mutex_lock(&pool->lock);

	int notify = 0;
	if(isEmpty(&pool->queue, pool->task_queue_size_limit)){		
		notify = 1;
	}
	int err = 0;
	if(isFull(&pool->queue, pool->task_queue_size_limit)){
		err = 1;
		pthread_mutex_unlock(&pool->lock);
		return err;
	}
	err = addToQueue(function, argument, &pool->queue, pool->task_queue_size_limit);
	if(notify==1){
		pool->available = 1;
		pthread_cond_signal(&pool->notify);
	}
	pthread_mutex_unlock(&pool->lock);
	return err;
}
/*
 * Destroy the threadpool, free all memory, destroy threads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
	pthread_mutex_lock(&quitlock);
	quit = 1;
	pthread_mutex_unlock(&quitlock);

    int err = 0;
	pthread_cond_broadcast(&pool->notify);
	sleep(1);

	if(pthread_mutex_destroy(&pool->lock)){
		printf(" mutex Destroy error!\n");
		exit(2);
	}

	if(pthread_cond_destroy(&pool->notify)){
		printf(" cond Destroy error!\n");
		exit(2);
	}
	sem_destroy(&pool->sema);
	pthread_mutex_destroy(&quitlock);
	free(pool->threads);
	free(pool);

    return err;
}
/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *vpool)
{ 
//	printf("Thread do work\n");
	pool_t* pool = (pool_t*)vpool;
	pool_task_t temp;
    while(1) {

	//check standby list
		//standbylist is empty, execute task in queue.
		pthread_mutex_lock(&pool->lock);
		if(!isEmpty(&pool->queue, pool->task_queue_size_limit)){
			temp = popFromQueue(&pool->queue, pool->task_queue_size_limit);
		}else{
			//wait for condition reach signal to continue
			while(pool->available==0){
				pthread_cond_wait(&pool->notify, &pool->lock);
				pthread_mutex_lock(&quitlock);
				if(quit == 1){
					pthread_mutex_unlock(&quitlock);
					pthread_mutex_unlock(&pool->lock);
					pthread_cond_signal(&pool->notify);
					pthread_exit(NULL);
					return(NULL);
				}
				pthread_mutex_unlock(&quitlock);

			}
			pthread_mutex_unlock(&pool->lock);

			continue;
		}
		if(isEmpty(&pool->queue, pool->task_queue_size_limit)){
			pool->available = 0;
		}
		pthread_mutex_unlock(&pool->lock);
    		
	    (*(temp.function))((argu*)temp.argument);
    }
    pthread_exit(NULL);
    return(NULL);
}


int isEmpty(priority_queue* queue, int limit){
	if(queue->size == 0)return 1;
	else return 0;
}

int isFull(priority_queue* queue, int limit){
	if(queue->size == limit) return 1;
	else return 0;
}

int addToQueue(void (*function)(void *), void* argument, priority_queue* queue, int limit){
	int err = 0;
	if(isFull(queue, limit)){
		return err;
	}else{
		queue->queue[queue->size].function = function;
		queue->queue[queue->size].argument = argument;
		queue->size = queue->size + 1;
		percolateUp(queue->size - 1, queue, limit);
		return err;	
	}
}

pool_task_t popFromQueue(priority_queue* queue, int limit){

	pool_task_t task = queue->queue[0];
	swap(0, queue->size - 1, queue, limit);
	percolateDown(0, queue, limit);
	queue->size = queue->size - 1;
	return task;	
}

void swap(int i, int j, priority_queue* queue, int limit) {
	pool_task_t temp = queue->queue[i];
	queue->queue[i] = queue->queue[j];
	queue->queue[j] = temp;
}

void percolateUp(int i, priority_queue* queue, int limit) {
	while (i >=0) {
		argu* iargu =  queue->queue[i].argument;
		argu* parentargu =  queue->queue[(i - 1) / 2].argument;
		if (iargu->customer_priority < parentargu->customer_priority) {
			swap(i, (i - 1) / 2, queue, limit);
			i = (i - 1) / 2;
		}
		else {
			break;
		}
	}
}

void percolateDown(int i, priority_queue* queue, int limit) {
	int left = 2 * i + 1;
	int right = 2 * i + 2;
	while (left < queue->size) {
		int smaller = left;
		argu* rightargu =  queue->queue[right].argument;
		argu* leftargu =  queue->queue[left].argument;
		argu* iargu =  queue->queue[i].argument;
		if (right < queue->size && rightargu->customer_priority < leftargu->customer_priority) {
			smaller = right;
		}
		argu* smallerargu =  queue->queue[smaller].argument;
		if (iargu->customer_priority > smallerargu->customer_priority) {
			swap(i, smaller, queue, limit);
			i = smaller;
		} else {
			break;
		}
		left = 2 * i + 1;
		right = 2 * i + 2;
	}
}
void* checkPending(void* num_of_seats){
	while(1){
		//check if the all seats empty
		sleep(1);
		//walk through the seats to see if reached time limit
		seat_t* curr = seat_header;
		while(curr!=NULL){
			pthread_mutex_lock(&curr->seat_lock);
		

			//  if curr->time !=-1,ignore the empty seats
			if(curr->time_left != -1 && curr->state != OCCUPIED){
				curr->time_left-=1;
				if(curr->time_left == 0){
					//set it available
	               	sem_wait(&g_thread_pool->sema);
                if(!isEmpty(&g_thread_pool->standbylist, STANDBY_SIZE)){

					argu* temp = popFromQueue(&g_thread_pool->standbylist, STANDBY_SIZE).argument;
                	curr->customer_id = temp->req.user_id;
                	curr->time_left = 5;
                }else{
					pthread_mutex_lock(&seat_flag_lock);
					seat_available += 1;
					pthread_mutex_unlock(&seat_flag_lock);
					curr->state = AVAILABLE;
					curr->customer_id = -1;
					curr->time_left = -1;
				}
					sem_post(&g_thread_pool->sema);
				}				
			}
			pthread_mutex_unlock(&curr->seat_lock);
			curr = curr->next;
		}
		
	}
	pthread_exit(NULL);
}




// int isEmpty(c_queue *queue){
// 	if(queue->data_start == queue->data_end){
// 		return 1;
// 	}
// 	return 0;
// }
// int isFull(c_queue *queue){
// 	if(queue->data_end + 1 == queue->data_start){
// 		return 1;
// 	}
// 	if(queue->data_end == queue->buf_end && queue->data_start == queue->buf_start){
// 		return 1;
// 	}
// 	return 0;

// }
// int addToQueue(c_queue *queue){
// 	int err = 0;
// 	//move pointer
// 	if(queue->data_end == queue->buf_end){
// 		queue->data_end = queue->buf_start;
// 	}else{
// 		queue->data_end = queue->data_end + 1;
// 	}
// 	return err;	
// }
// int delFromQueue(c_queue *queue){
// 	int err = 0;
// 	if(queue->data_start == queue->buf_end){
// 		queue->data_start = queue->buf_start;
// 	}else{
// 		queue->data_start = queue->data_start + 1;
// 	}
// 	return err;	
// }


