#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_pool.h"
#include "util.h"
#include "semaphore.c"
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


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
	struct pool_t* resPool = (pool_t*)malloc(sizeof(pool_t));
	pthread_mutex_init(&(resPool->lock), NULL);
	pthread_cond_init (&(resPool->notify), NULL);
	resPool->threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads);
	initQueue(&resPool->queue,queue_size);
	sem_init(&resPool->sema);
	initQueue(&resPool->standbylist,STANDBY_SIZE);
	resPool->thread_count = num_threads;  	//?
	resPool->task_queue_size_limit = queue_size;  //?
	int i;
	for ( i=0;i<num_threads;i++){
		pthread_create(&resPool->threads[i],NULL,thread_do_work,(void*)resPool);
	}
    return resPool;
}
void initQueue(c_queue* queue,int queue_size){
	queue->buf_start = (pool_task_t*)malloc(sizeof(pool_task_t)*queue_size);
	queue->buf_end = queue->buf_start + queue_size;
	queue->data_start = queue->buf_start;
	queue->data_end = queue->buf_start;
}
void cleanupPending(){

}


/*
 * Add a task to the standbylist
 *
 */
int standbylist_add_task(pool_t *pool, void (*function)(void *), void* argument)
{
	printf("in pool_add_task \n");
	int notify = 0;
	if(pool->standbylist.data_start==pool->standbylist.data_end){		
		notify = 1;
	}
	int err = 0;
	if(isFull(pool->standbylist)){
		return err;
	}else{
		pool->standbylist.data_end-> function = function;
		pool->standbylist.data_end-> argument = argument;
	}
	c_queue* temp = &pool->queue;
	err = addToQueue(temp);
	return err;
}

/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void* argument)
{
	printf("in pool_add_task \n");
	//TODO SET a global 
	int notify = 0;
	if(pool->queue.data_start==pool->queue.data_end){		
		notify = 1;
	}
	int err = 0;
	pthread_mutex_lock(&pool->lock);
	pool->queue.data_end-> function = function;
	pool->queue.data_end-> argument = argument;
	c_queue* temp = &pool->queue;
	err = addToQueue(temp);
	if(notify==1){
		printf("before send conditon signal\n");
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
    int err = 0;
	if(pthread_mutex_destroy(&pool->lock)){
		printf("mutex Destroy error!\n");
		exit(2);
	}

	if(pthread_cond_destroy(&pool->notify)){
		printf("cond Destroy error!\n");
		exit(2);
	}
	sem_destroy(&pool->sema);
	int i;
	for ( i=0;i<pool->thread_count;i++){
		pthread_cancel(pool->threads[i]);
	}

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
	printf("Thread do work\n");
	pool_t* pool = (pool_t*)vpool;
	pool_task_t temp;
    while(1) {

	//check standby list
		//standbylist is empty, execute task in queue.
		pthread_mutex_lock(&pool->lock);
		//check pool->queue
		if(!isEmpty(pool->queue)){
			temp = *(pool->queue.data_start);
			delFromQueue(&pool->queue);
		}else{
			//wait for condition reach signal to continue
			printf("waiting for signal\n");
			while(pool->available==0){
				pthread_cond_wait(&pool->notify, &pool->lock);
			}
			printf("signal received !\n");			
			pthread_mutex_unlock(&pool->lock);
			continue;
		}
		if(isEmpty(pool->queue)){
			pool->available = 0;
		}
		pthread_mutex_unlock(&pool->lock);    		
    	
		printf("executing the function\n");
	    (*(temp.function))((argu*)temp.argument);
    }
    pthread_exit(NULL);
    return(NULL);
}
int isEmpty(c_queue *queue){
	if(queue->data_end == queue->buf_end){
		return 1;
	}
}
int isFull(c_queue *queue){
	if(queue->data_end + 1 == queue->data_start){
		return 1;
	}
	if(queue->data_end == queue->buf_start && queue->data_start == queue->buf_end){
		return 1;
	}
	if(queue->data_end == queue->buf_end && queue->data_start == queue->buf_start){
		return 1;
	}

}
int addToQueue(c_queue *queue){
	int err = 0;
	//move pointer
	if(queue->data_end == queue->buf_end){
		queue->data_end = queue->buf_start;
	}else{
		queue->data_end = queue->data_end + 1;
	}
	return err;	
}
int delFromQueue(c_queue *queue){
	int err = 0;
	if(queue->data_start == queue->buf_end){
		queue->data_start = queue->buf_start;
	}else{
		queue->data_start = queue->data_start + 1;
	}
	return err;	
}