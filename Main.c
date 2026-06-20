#include "scheduler.h"
#include "config.h"
#include <stdio.h> 

void test_task(void *arg) {
    printf("Worker executed test task: %s\n", (char*)arg);
}

void* worker_thread(void *arg){
    while (1){
        
        //access scheduler
        pthread_mutex_lock(&scheduler.lock);
        //if there are no tasks to do then you freeze the thread
        while (scheduler.job_count == 0) {pthread_cond_wait(&scheduler.cond, &scheduler.lock);}
        pthread_mutex_unlock(&scheduler.lock);


        //go through the different queus to get a task
        //queue 0 has highest priority
        for (int i = 0; i < scheduler.queue_count; i++) {
            TaskNode *node = pop_task(&scheduler.queues[i]);
            if(node!=NULL){
                
                //do the tasks function call with its params
                node->task.func(node->task.arg);

                free(node);

                break;
            }
        }
    }
}

int main() {
    Config setup;// no need to malloc, all are known types

    bool successConfig=load_config("env.conf", &setup);
    if (!successConfig) {printf("Failed to load config\n");return 1;}

    init_scheduler(&setup);

    //create an array of threads, can also use it to free it at program end.
    pthread_t *threads = malloc(sizeof(pthread_t) * setup.worker_threads);

    for (int i = 0; i < setup.worker_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    for (int j = 0; j < 30; j++) {
        char *msg = malloc(32);
        sprintf(msg, "Hello from task %d", j);

        Task t = { test_task, msg };
        push_task(&scheduler.queues[0], t);
    }

    //makes sure threads finish their work before closure
    for (int i = 0; i < setup.worker_threads; i++) {pthread_join(threads[i], NULL);}
    free(threads);
}