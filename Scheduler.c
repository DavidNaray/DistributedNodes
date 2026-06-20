#include "scheduler.h"

//so this is the importing from scheduler.h
//no need to malloc scheduler as everything is known types inside it
//--other than the queue, so malloc the queue
Scheduler scheduler;

void push_task(Queue *q, Task task) {
    TaskNode *node = malloc(sizeof(TaskNode));
    node->task = task;
    node->next = NULL;//since itll be at the top

    pthread_mutex_lock(&q->lock); //lock the queue because we need to update it

    if (q->head == NULL) { //if no head then init the array
        q->head = node;
        q->tail = node;
    }
    else {//put it at the back of the queue, setting the last tail to the new tail
        q->tail->next = node;
        q->tail = node;
    }

    pthread_mutex_unlock(&q->lock);//finished updating so unlock

    // update global job count
    pthread_mutex_lock(&scheduler.lock);
    scheduler.job_count++;
    pthread_cond_signal(&scheduler.cond);
    pthread_mutex_unlock(&scheduler.lock);
}


TaskNode* pop_task(Queue *q) {
    pthread_mutex_lock(&q->lock);

    TaskNode *node = q->head;
    if (node == NULL) {//theres nothing to get so bail
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    //it wasnt null so move the head back, node holds the task we want
    q->head = node->next;
    if (q->head == NULL) {//if the node after next is null, tail must be null
        q->tail = NULL;
    }

    pthread_mutex_unlock(&q->lock);//finished the grab so unlock queue

    // update global job count
    pthread_mutex_lock(&scheduler.lock);
    scheduler.job_count--;
    pthread_mutex_unlock(&scheduler.lock);

    //sleep occurs in the worker thread, putting itself to sleep if job_count==0
    return node;
}


void init_queue(Queue *q){
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->lock, NULL);
}


void init_scheduler(const Config *setup) {
    scheduler.queue_count = setup->queue_order;
    scheduler.job_count = 0;

    pthread_mutex_init(&scheduler.lock, NULL);
    pthread_cond_init(&scheduler.cond, NULL);

    scheduler.queues = malloc(sizeof(Queue) * scheduler.queue_count);

    int i;
    for (i = 0; i < scheduler.queue_count; i++) {
        //send pointer, need to define the queue
        init_queue(&scheduler.queues[i]);
    }
}