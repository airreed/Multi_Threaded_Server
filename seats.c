#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "seats.h"
#include "thread_pool.h"
//#include "semaphore.c"
#include "util.h"


extern struct pool_t* g_thread_pool;
extern int sem_wait(m_sem_t *s);
extern int sem_post(m_sem_t *s);
extern struct pool_t* g_thread_pool;
extern int delFromQueue(c_queue *queue);
extern int addToQueue(c_queue *queue);
seat_t* seat_header = NULL;

char seat_state_to_char(seat_state_t);

void list_seats(char* buf, int bufsize)
{


    seat_t* curr = seat_header;
    int index = 0;
    while(curr != NULL && index < bufsize+ strlen("%d %c,"))
    {
    	pthread_mutex_lock(&curr->seat_lock);
        int length = snprintf(buf+index, bufsize-index, 
                "%d %c,", curr->id, seat_state_to_char(curr->state));
    	pthread_mutex_unlock(&curr->seat_lock);
        if (length > 0)
            index = index + length;
        curr = curr->next;
    }
    if (index > 0)
        snprintf(buf+index-1, bufsize-index-1, "\n");
    else
        snprintf(buf, bufsize, "No seats not found\n\n");
}

int view_seat(char* buf, int bufsize,  int seat_id, int customer_id, int customer_priority)
{
	int res = 0;
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
    	printf("------before lock seat lock\n");
	   	pthread_mutex_lock(&curr->seat_lock);
        if(curr->id == seat_id)
        {
            if(curr->state == AVAILABLE || (curr->state == PENDING && curr->customer_id == customer_id))
            {
                snprintf(buf, bufsize, "Confirm seat: %d %c ?\n\n",curr->id, seat_state_to_char(curr->state));
                //TODO: set the start time of the seat to 5 

                curr->state = PENDING;
                curr->customer_id = customer_id;
                printf("set %d from %d to 5\n",curr->id,curr->time_left);
                printf("seat_available : --- %d\n",seat_available);
                curr->time_left = 5;
               	pthread_mutex_lock(&seat_flag_lock);
               	seat_available -= 1;
 //.              	printf("Seat available: %d", seat_available);
               	pthread_mutex_unlock(&seat_flag_lock);

            }
            else if(curr->state == PENDING && curr->customer_id != customer_id){
            	//TODO: add the task to standby list //todo not right
            	//check if the seats are all pending and occupied. if yes, add to standbylist
            	// if not, return to customer infomation
              	pthread_mutex_lock(&seat_flag_lock);
              	if(seat_available == 0){
              		snprintf(buf,bufsize,"Seat unavailable\n\n");
   //.           		printf("add to standbylist!!!!!!!\n");
              		res = 1;
              	}else{
              		snprintf(buf,bufsize,"Seat unavailable\n\n");
              	}
               	pthread_mutex_unlock(&seat_flag_lock);
            }
            else 
            {
                snprintf(buf, bufsize, "Seat unavailable\n\n");
            }
	    	pthread_mutex_unlock(&curr->seat_lock);        
            return res;
        }
        printf("release the lock\n");
    	pthread_mutex_unlock(&curr->seat_lock);        
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
    return res;
}

void confirm_seat(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
    seat_t* curr = seat_header;
    while(curr != NULL)
    {
        if(curr->id == seat_id)
        {
	    	pthread_mutex_lock(&curr->seat_lock);            	
            if(curr->state == PENDING && curr->customer_id == customer_id )
            {
                snprintf(buf, bufsize, "Seat confirmed: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
                curr->state = OCCUPIED;
            }
            else if(curr->customer_id != customer_id )
            {
            	//TODO: add to standby list  ???
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
	    	pthread_mutex_unlock(&curr->seat_lock);        
            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Requested seat not found\n\n");
    
    return;
}

void cancel(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
	// AFTER every cancel, increse the seat available flag, send a signal to all the standbylist
    seat_t* curr = seat_header;
    while(curr != NULL)
    {	
        if(curr->id == seat_id)
        {
		   	pthread_mutex_lock(&curr->seat_lock);        
            if(curr->state == PENDING && curr->customer_id == customer_id )
            {
                snprintf(buf, bufsize, "Seat request cancelled: %d %c\n\n",
                        curr->id, seat_state_to_char(curr->state));
                //TODO: SET TIME of the seat to be default value -1
   //.             printf("seat cancelled.\n");
                //TODO: check the the standbylist, if empty,set the seat is available. If not, give the seat to the the first person in the list.
               	sem_wait(&g_thread_pool->sema);
    //.           	printf("----standbylist: data_start :%p  data_end:%p\n",g_thread_pool->standbylist.data_start,g_thread_pool->standbylist.data_end);
                if(!isEmpty(&g_thread_pool->standbylist)){

					argu* temp = g_thread_pool->standbylist.data_start->argument;
   	//.                printf("[cancelled] SET table :%d from %d to customer:%d \n",curr->id,curr->customer_id,temp->req.user_id);
                	curr->customer_id = temp->req.user_id;
                	curr->time_left = 5;
                	delFromQueue(&g_thread_pool->standbylist);
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
            else if(curr->customer_id != customer_id )
            {
                snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
            }
            else if(curr->state != PENDING)
            {
                snprintf(buf, bufsize, "No pending request\n\n");
            }
		   	pthread_mutex_unlock(&curr->seat_lock);        
            return;
        }
        curr = curr->next;
    }
    snprintf(buf, bufsize, "Seat not found\n\n");
    
    return;
}

void load_seats(int number_of_seats)
{
	pthread_cond_init(&seat_not_empty,NULL);

	pthread_mutex_lock(&seat_flag_lock);
	seat_available = number_of_seats;
	pthread_mutex_unlock(&seat_flag_lock);	

    seat_t* curr = NULL;
    int i;
    for(i = 0; i < number_of_seats; i++)
    {   
        seat_t* temp = (seat_t*) malloc(sizeof(seat_t));
        pthread_mutex_init(&temp->seat_lock, NULL);
        temp->id = i;
        temp->customer_id = -1;
        temp->state = AVAILABLE;
        temp->time_left = -1;
        temp->next = NULL;
        
        if (seat_header == NULL)
        {
            seat_header = temp;
        }
        else
        {
            curr-> next = temp;
        }
        curr = temp;
    }
}

void unload_seats()
{
    seat_t* curr = seat_header;
    pthread_cond_destroy(&seat_not_empty);
	pthread_mutex_destroy(&seat_flag_lock);
    while(curr != NULL)
    {
        seat_t* temp = curr;
        pthread_mutex_destroy(&curr->seat_lock);
        curr = curr->next;
        free(temp);
    }
}

char seat_state_to_char(seat_state_t state)
{
    switch(state)
    {
        case AVAILABLE:
            return 'A';
        case PENDING:
            return 'P';
        case OCCUPIED:
            return 'O';
    }
    return '?';
}
