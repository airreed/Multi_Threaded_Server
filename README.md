# Multi_Threaded_Server

Thread Pool Implementation: 
	
	When this server is running, MAX_THREAD (20 in this case) of threads is running. As they are executing
thread_do_work function, they can get task from queue whenever there are tasks available. Since data race may happen
during multithreading, mutex is used to protect data from modified by multiple threads at the same time. If there is a
thread reading or writing the queue, a mutex "lock" is used to lock the queue. When a thread is viewing, confirming,
canceling seat, "seat_lock" is used to protect each seat.

	A condition variable is used to put threads into waiting when the queue is empty, or signal thread when new
task is added.

Standby List Implementation: 

	When all the seats are occupied, a request for seat task is added to a standby list. Whenever there is a seat
got cancelled, the user waiting in the standby list has the priority to confirm the seat. A binary semaphore is used to
lock the standby list when a thread is working on it. There is a condition variable implemented in the semaphore so
that the thread can be waked up if any seat got cancelled.

Benchmarking Implementation:

	Output the time consumed.


Extra Credit:
Priority Scheduling;

	The task queue and standby list is implemented using priority queue. Every user has a priority ranged from
0(highest) to 2(lowest). Each time a task is added into the queue or standby list, the priority queue is sorted
according to the customer priority number. Therefore, every task poped out from the queue is with the highest priority
among all the tasks in the queue.

Cleanup pending requests:
	"time_left", defined in "seat_t" structure, is used for each seat to track the pending time. One particular
thread is created to check the pending time of every pending seats every one second. Each time the thread checks seats,
the "time_left" is decreased by 1. If there is any pending seat with "time_left" equaling 0, this seat is set to be
"AVAILABLE". Then other threads were waked to get seat.

Variables:

	threads: array of the working threads;

	lock: a mutex to protect queue;

	queue: priority queue of user requests;

	notify: condition variable to work as sighnal if any request task is added;

	standbylist: priority queue of user requests when all seats are not available;

	sema: a semaphore to protect standbylist.

Server Performance:

	There is little performance improvement that I can see from the Benchmarking output. The performace of Multi-threaded server should be much better than that of single-threaded one. However, When I change the thread number, the time from the 
