
#include "queue.h"
#include "sched.h"
#include <pthread.h>

static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

int queue_empty(void) {
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

/* Get process from ready_queue */
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	
	if (queue_empty()){
		/* Both queue are empty. */
		return proc;
	}
	else{		
		/* At least one queue contains process(es). */
		pthread_mutex_lock(&queue_lock); // Lock queue.

		if (empty(&ready_queue)){	
			/* Empty ready_queue. */
			while (!empty(&run_queue)){
				
				/* Move all processes in run_queue to ready_queue. */
				enqueue(&ready_queue, dequeue(&run_queue));
			}
		}

		/* Get process from ready_queue. */
		proc = dequeue(&ready_queue);  
		
		pthread_mutex_unlock(&queue_lock); // Unlock queue. 
	}
	return proc;
}
/* Push a process to run_queue. */
void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}
/* Push a process to ready_queue. */
void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}


