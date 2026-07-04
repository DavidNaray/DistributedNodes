#ifndef SCHEDULE_H
#define SCHEDULE_H   // these form a guard


#include <windows.h>
#include <pthread.h>
#include <stdlib.h> 
#include "config.h"



//function pointer with a void pointer parameter, ie parameter can be anything
typedef void (*TaskFunction)(void *arg); 

typedef struct {
    TaskFunction func;   // function pointer to execute
    void *arg;           // data passed to the function
} Task;


typedef struct TaskNode{
    Task task;
    struct TaskNode *next;
} TaskNode;

typedef struct {
    TaskNode *head;
    TaskNode *tail;
    pthread_mutex_t lock; //prevents race conditions
    
} Queue;

typedef struct {
    Queue *queues;   // array of queues
    int queue_count;     // from config
    int job_count;     // accounts for the total jobs in queues

    pthread_mutex_t lock; //prevents race conditions
    pthread_cond_t cond;  //prevents busy waiting, (job_count=0 then sleep)

    HANDLE hPipe;
} Scheduler;

//kind of like creating the object and then exporting it in javascript 
//(global variable kinda, so all threads can access it)
extern Scheduler scheduler;

void push_task(Queue *q, Task task);

TaskNode* pop_task(Queue *q);

void init_queue(Queue *setup);

void init_scheduler(const Config *setup);


#endif