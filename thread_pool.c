#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include <sys/types.h>
#include <unistd.h>
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

extern int sem_wait(m_sem_t *s);
extern int sem_post(m_sem_t *s);
extern void sem_init(m_sem_t *s);
extern void sem_destroy(m_sem_t *s);
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
//	printf("in standbylist_add_task \n");
	sem_wait(&pool->sema);
	int err = 0;
	if(isFull(&pool->standbylist)){
		sem_post(&pool->sema);
		return err;
	}
	pool->standbylist.data_end-> function = function;
	pool->standbylist.data_end-> argument = argument;	
	c_queue* temp = &pool->standbylist;
	err = addToQueue(temp);
//	printf("------>in standbylist job added, start:%p  end:%p\n",pool->standbylist.data_start,pool->standbylist.data_end);
	sem_post(&pool->sema);
	return err;
}

/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void* argument)
{
//	printf("-------------in pool_add_task %x\n",pthread_self());
//	printf("-----------function: %p\n",function);
	//TODO SET a global 
	pthread_mutex_lock(&pool->lock);
	int notify = 0;
	if(isEmpty(&pool->queue)){		
		notify = 1;
	}
	int err = 0;
	if(isFull(&pool->queue)){
		err = 1;
		pthread_mutex_unlock(&pool->lock);
		return err;
	}
	pool->queue.data_end-> function = function;
	pool->queue.data_end-> argument = argument;
	c_queue* temp = &pool->queue;
	err = addToQueue(temp);
//	printf("1.pool->queue.data_start: %p  data_end:%p\n",pool->queue.data_start,pool->queue.data_end);
	if(notify==1){
//		printf("before send conditon signal\n");
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
//	printf("Thread do work\n");
	pool_t* pool = (pool_t*)vpool;
	pool_task_t temp;
    while(1) {

	//check standby list
		//standbylist is empty, execute task in queue.
		pthread_mutex_lock(&pool->lock);
		//check pool->queue
	//	printf("2.pool->queue.data_start: %p  data_end:%p\n",pool->queue.data_start,pool->queue.data_end);
		if(!isEmpty(&pool->queue)){
//			printf("the queue is not empty!!\n");
			temp = *(pool->queue.data_start);
			delFromQueue(&pool->queue);
		}else{
			//wait for condition reach signal to continue
			while(pool->available==0){
//				printf("waiting for signal id: %x\n",pthread_self());
				pthread_cond_wait(&pool->notify, &pool->lock);
			}
//			printf("signal received id: %x\n",pthread_self());			
			pthread_mutex_unlock(&pool->lock);
			continue;
		}
		if(isEmpty(&pool->queue)){
			pool->available = 0;
		}
		pthread_mutex_unlock(&pool->lock);    		
    	
//		printf("executing the function\n");
	    (*(temp.function))((argu*)temp.argument);
    }
    pthread_exit(NULL);
    return(NULL);
}
int isEmpty(c_queue *queue){
//	printf("3.queue.data_start: %p  data_end:%p\n",queue->data_start,queue->data_end);
	if(queue->data_start == queue->data_end){
	//	printf("is empty return true\n");
		return 1;
	}
	//	printf("is empty return false\n");
	return 0;
}
int isFull(c_queue *queue){
	if(queue->data_end + 1 == queue->data_start){
		return 1;
	}
	if(queue->data_end == queue->buf_end && queue->data_start == queue->buf_start){
		return 1;
	}
	return 0;

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