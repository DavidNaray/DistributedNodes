#include "scheduler.h"
#include "config.h"
#include <stdio.h> 
#include "../C_MMO_RPG_rewrite/LoginRegister/LoginRegister.h"
#include <cJSON.h>
#include "../C_MMO_RPG_rewrite/MongoDBReadWriteCache/Cache.h"
#include "../C_MMO_RPG_rewrite/noiseLib/TerrainGeneration.h"

#include "../C_MMO_RPG_rewrite/serverComm/ReadWriteServ.h"

#include "../C_MMO_RPG_rewrite/UserUpdates/UserUpdates.h"

void test_task(void *arg) {
    printf("Worker executed test task: %s\n", (char*)arg);
}

Task parse_json_to_task(char json[]){
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        printf("JSON parse error\n");
        return (Task){NULL, NULL};
    }
    
    const cJSON *type = cJSON_GetObjectItem(root, "type");
    printf("'%s'\n", type->valuestring);

    if (strcmp(type->valuestring, "Register") == 0) {
        RegisterArgs *args = malloc(sizeof(RegisterArgs));


        args->RId = atoi(cJSON_GetObjectItem(root, "RId")->valuestring);

        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        strcpy(args->password, cJSON_GetObjectItem(root, "password")->valuestring);

        cJSON_Delete(root);
        return (Task){RegisterTask, args};
    
    }
    else if (strcmp(type->valuestring, "Login") == 0) {
        RegisterArgs *args = malloc(sizeof(RegisterArgs));

        args->RId = atoi(cJSON_GetObjectItem(root, "RId")->valuestring);

        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        strcpy(args->password, cJSON_GetObjectItem(root, "password")->valuestring);

        cJSON_Delete(root);
        return (Task){LoginTask, args};
    
    }
    else if (strcmp(type->valuestring, "TechTreeUpdate") == 0){
        //what tech is available to the user
        UUpdate *args = malloc(sizeof(UUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);
        strcpy(args->sockid, cJSON_GetObjectItem(root, "sockid")->valuestring);

        cJSON_Delete(root);
        return (Task){TechUpdateTask, args};
    }

    printf("mega failure\n");
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

        // if (!ok || bytesRead == 0) {continue; /*Node disconnected or no data*/ }
        if (!ok) {
            DWORD err = GetLastError();
            printf("ReadFile failed. err=%lu\n", err);
            continue;
        }

        if (bytesRead == 0) {
            printf("ReadFile returned 0 bytes\n");
            continue;
        }

        buffer[bytesRead] = '\0';

        printf("Read %lu bytes: %s\n", bytesRead, buffer);
        // Parse JSON into the appropriate Task
        Task t = parse_json_to_task(buffer);
        
        push_task(&scheduler.queues[0], t);
    }
    printf("[Reader] Thread terminated cleanly.\n");
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
    
    GlobalCache=cache_create();
    TSetup=SetupTerrainFields(512,512,"Cellular",0.011,123,"../C_MMO_RPG_rewrite/NodeServer/Tiles/");
    ApplyTerrainFields();

    mongoc_init();
    mongoClient = mongoc_client_new("mongodb://localhost:27019");
    if (!mongoClient) {
        printf("Failed to connect to MongoDB\n");
        return 1;
    }
    
    bool successConfig=load_config("env.conf", &setup);
    if (!successConfig) {printf("Failed to load config\n");return 1;}
    init_scheduler(&setup);

    setupPipe();
    // scheduler.hPipe=hPipe;

    //create an array of threads, can also use it to free it at program end.
    pthread_t *threads = malloc(sizeof(pthread_t) * (setup.worker_threads+1));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);   // 8 MB stack

    for (int i = 0; i < setup.worker_threads; i++) {
        pthread_create(&threads[i], &attr, worker_thread, NULL);
    }

    pthread_create(&threads[setup.worker_threads], &attr, Reader_thread, NULL);

    boot_node_server(setup.node_root,setup.node_start);
    
    ConnectNamedPipe(scheduler.hPipe, NULL);//wait for nodejs server to connect to pipe
    
    
    
    for (int j = 0; j < 30; j++) {
        char *msg = malloc(32);
        sprintf(msg, "Hello from task %d", j);

        Task t = { test_task, msg };
        push_task(&scheduler.queues[0], t);
    }

    //makes sure threads finish their work before closure
    for (int i = 0; i < (setup.worker_threads+1); i++) {pthread_join(threads[i], NULL);}
    free(threads);
    pthread_attr_destroy(&attr);

    cache_free(GlobalCache);
    // free(TSetup);
}