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
	q->proc[q->size] = proc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose priority is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (q == NULL || q->size == 0)
		return NULL;
	
	struct pcb_t *result = NULL;
	uint32_t max_prior = 0, index=0;

	for (int i = 0;i < q->size;i++){
		if (q->proc[i]->priority > max_prior){
			max_prior = q->proc[i]->priority;
			index = i;
		}
	}
	result = q->proc[index];
	for (int i=index;i<q->size-1;i++){
		q->proc[i] = q->proc[i+1];
	}
	q->proc[q->size-1] = NULL;
	q->size--;
	return result;
}	

