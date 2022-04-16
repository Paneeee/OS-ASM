#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	if (q->size == MAX_QUEUE_SIZE){
		return;	
	}
	if (q->size==0){
		q->proc[0] = proc;
		q->size++;
		return;
	}
	for (int i = q->size-1;i>=0;i--){
		if (proc->priority <= q->proc[i]->priority){
			i++;
			for (int j = q->size; j>i;j--){
				q->proc[j] = q->proc[j-1];
			}
			q->proc[i] = proc;
			q->size++;
			return;
		}
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose priority is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (q == NULL || q->size == 0)
		return NULL;
	
	struct pcb_t *to_run = q->proc[q->size-1];
	q->proc[q->size-1] = NULL;
	q->size--;
	return to_run;
}	

