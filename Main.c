#include "scheduler.h"
#include "config.h"
#include <stdio.h> 
#include "Structs.h"
#include "../C_MMO_RPG_rewrite/LoginRegister/Register.c"
#include <cJSON.h>


void test_task(void *arg) {
    printf("Worker executed test task: %s\n", (char*)arg);
}


// void RegisterTask(void *arg){
//     printf("REGISTER REG REG\n");
// }

Task parse_json_to_task(char json[]){
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        printf("JSON parse error\n");
        return (Task){NULL, NULL};
    }
    
    const cJSON *type = cJSON_GetObjectItem(root, "type");

    if (strcmp(type->valuestring, "Register") == 0) {
        RegisterArgs *args = malloc(sizeof(RegisterArgs));

        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        strcpy(args->password, cJSON_GetObjectItem(root, "password")->valuestring);

        cJSON_Delete(root);
        return (Task){RegisterTask, args};
    
    }

    cJSON_Delete(root);
    return (Task){NULL, NULL};
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

void* Reader_thread(void* arg) {

    char buffer[1024];
    DWORD bytesRead;

    while (1) {

        //read and write file are thread safe according to google for windows
        BOOL ok = ReadFile(
            scheduler.hPipe,
            buffer,
            sizeof(buffer) - 1,
            &bytesRead,
            NULL
        );

        if (!ok || bytesRead == 0) {continue; /*Node disconnected or no data*/ }

        buffer[bytesRead] = '\0';

        // Parse JSON into the appropriate Task
        Task t = parse_json_to_task(buffer);
        
        push_task(&scheduler.queues[0], t);
    }
}


void boot_node_server(const char *root, const char *command){
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    char cmdline[512];
    snprintf(
        cmdline, 
        sizeof(cmdline), 
        "cmd.exe /k title NodeServer && %s", 
        command);

    BOOL ok = CreateProcessA(
        NULL,               // application
        cmdline,            // command line (modifiable buffer!)
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        root,               // working directory
        &si,
        &pi
    );

    if (!ok) {
        printf("CreateProcess failed: %lu\n", GetLastError());
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}


int main() {
    Config setup;// no need to malloc, all are known types
    
    bool successConfig=load_config("env.conf", &setup);
    if (!successConfig) {printf("Failed to load config\n");return 1;}
    init_scheduler(&setup);

    HANDLE hPipe = CreateNamedPipe(
        TEXT("\\\\.\\pipe\\SchedulerPipe"),//simply the name of the pipe
        PIPE_ACCESS_DUPLEX,//two way communication
        
        // stream of messages, read from it as messages, and its blocking so read/write 
            //of it dont return immediately
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
        1,//nMaxInstances
        1024,//nOutBufferSize
        1024,//nInBufferSize
        0,//nDefaultTimeOut
        NULL//[in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

    scheduler.hPipe=hPipe;

    //create an array of threads, can also use it to free it at program end.
    pthread_t *threads = malloc(sizeof(pthread_t) * (setup.worker_threads+1));

    for (int i = 0; i < setup.worker_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    pthread_create(&threads[setup.worker_threads], NULL, Reader_thread, NULL);

    boot_node_server(setup.node_root,setup.node_start);
    
    ConnectNamedPipe(hPipe, NULL);//wait for nodejs server to connect to pipe
    
    
    
    for (int j = 0; j < 30; j++) {
        char *msg = malloc(32);
        sprintf(msg, "Hello from task %d", j);

        Task t = { test_task, msg };
        push_task(&scheduler.queues[0], t);
    }

    //makes sure threads finish their work before closure
    for (int i = 0; i < (setup.worker_threads+1); i++) {pthread_join(threads[i], NULL);}
    free(threads);
}