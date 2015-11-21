#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include "thread_pool.h"
#include "seats.h"
#include "util.h"

#define BUFSIZE 1024
#define FILENAMESIZE 100
void shutdown_server(int);
extern void* checkPending(void* num_of_seats);
int listenfd;
int num_seats = 20;
struct pool_t* g_thread_pool;
int num_req;
double cumu_time;

int main(int argc,char *argv[])
{
    num_req = 0;
    cumu_time = 0;
    int flag = 20;
    int connfd = 0;
    struct sockaddr_in serv_addr;

    char send_buffer[BUFSIZE];
    
    listenfd = 0; 

    int server_port = 8080;

    if (argc > 1)
    {
        num_seats = atoi(argv[1]);
    } 

    if (server_port < 1500)
    {
        fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
        exit(-1);
    }
    
    if (signal(SIGINT, shutdown_server) == SIG_ERR) 
        printf("Issue registering SIGINT handler");

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( listenfd < 0 ){
        perror("Socket");
        exit(errno);
    }
    printf("Established Socket: %d\n", listenfd);
    flag = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) );

    // Load the seats;
    load_seats(num_seats);
    // todo:create the check thread
    
    pthread_t timeCheckthread;
 	pthread_create(&timeCheckthread,NULL,checkPending,(void*)((long*)&num_seats));
    // set server address 
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(send_buffer, '0', sizeof(send_buffer));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    // bind to socket
    if ( bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("socket--bind");
        exit(errno);
    }

    // listen for incoming requests
    listen(listenfd, 10);

    // Initialize your threadpool!
    g_thread_pool = pool_create(BUFSIZE,20);
    // This while loop "forever", handling incoming connections
    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        // printf("connfd:%d\n",connfd);
        /*********************************************************************
            You should not need to modify any of the code above this comment.
            However, you will need to add lines to declare and initialize your 
            threadpool!
            The lines below will need to be modified! Some may need to be moved
            to other locations when you make your server multithreaded.
        *********************************************************************/
        argu* temp= (argu*)malloc(sizeof(argu));//struct     {connfd,&req};
        temp->connfd = connfd;
        //set default priority to 0
        temp->customer_priority = 0;
        //remember the arrived time of a request
        temp->start_time = clock();
        num_req+=1;
        pool_add_task(g_thread_pool, (void*)&parse_request, (void*)temp);
    }
}

void shutdown_server(int signo){
    printf("Shutting down the server...\n");
    
    //  Teardown your threadpool
    int err = pool_destroy(g_thread_pool);
    if (err){
    	printf("Error !!! in pool destroy\n");
    }
    // TODO: Print stats about your ability to handle requests.
    printf("Average response time: %f second \n", cumu_time/num_req);
    unload_seats();
    close(listenfd);
    exit(0);
}
