#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_pool.h"
#include <stdio.h>


void sem_init(m_sem_t *s);

int sem_wait(m_sem_t *s);

int sem_post(m_sem_t *s);
void sem_destroy(m_sem_t *s);

void sem_init(m_sem_t *s)
{
	pthread_mutex_init(&s->guard,NULL);
	pthread_cond_init(&s->cond,NULL);
	s->value = 1;
}

int sem_wait(m_sem_t *s)
{
    //TODO
    pthread_mutex_lock(&s->guard);
    while(s->value == 0){
	    pthread_cond_wait(&s->cond,&s->guard);
    }
    s->value = s->value - 1;
    pthread_mutex_unlock(&s->guard);
    return 0;
}

int sem_post(m_sem_t *s)
{
    pthread_mutex_lock(&s->guard);
    s->value = s->value + 1;
    pthread_cond_signal( &s->cond);
    pthread_mutex_unlock(&s->guard);
    return 0;
}

void sem_destroy(m_sem_t *s){
	if(pthread_mutex_destroy(&s->guard)){
		printf(" mutex Destroy error!\n");
		exit(2);
	}

	if(pthread_cond_destroy(&s->cond)){
		printf("cond Destroy error!\n");
		exit(2);
	}
}