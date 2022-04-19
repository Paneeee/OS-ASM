#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}
/* Insert a process to queue's tail. */
void enqueue(struct queue_t * q, struct pcb_t * proc) {
	
	if (q->size == MAX_QUEUE_SIZE){
		/* Full queue. */
		return;	
	}

	/* Insert at queue's tail. */
	q->proc[q->size] = proc;

	/*Adjust queue's size. */
	q->size++;
}

/* Return process with highest priority. */
struct pcb_t * dequeue(struct queue_t * q) {
	if (q == NULL || q->size == 0){
		/* Empty queue. */
		return NULL;
	}
	
	struct pcb_t *return_pcb = NULL; //PCB to be returned.
	uint32_t max_prior = 0;			 //Max priority in queue.
	uint32_t index = 0;				 //Index of PCB with highest priority.

	/* Traverse the queue to find pcb with highest priority. */
	for (int i = 0;i < q->size;i++){
		if (q->proc[i]->priority > max_prior){
			/* Found new max, update max_prior and index. */
			max_prior = q->proc[i]->priority;
			index = i;
		}
	}

	/* Assign highest priority PCB to return. */
	return_pcb = q->proc[index];

	/* Remove above PCB from queue. */
	for (int i=index;i<q->size-1;i++){
		q->proc[i] = q->proc[i+1];
	}

	/* Adjust queue's size. */
	q->proc[q->size-1] = NULL;
	q->size--;

	return return_pcb;
}	

