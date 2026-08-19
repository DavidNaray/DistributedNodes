#include "scheduler.h"
#include "config.h"
#include <stdio.h> 
#include "../C_MMO_RPG_rewrite/LoginRegister/LoginRegister.h"
#include <cJSON.h>
#include "../C_MMO_RPG_rewrite/MongoDBReadWriteCache/Cache.h"

#include "../C_MMO_RPG_rewrite/noiseLib/TerrainGeneration.h"
#include "../C_MMO_RPG_rewrite/noiseLib/Spiral.h"

#include "../C_MMO_RPG_rewrite/serverComm/ReadWriteServ.h"

#include "../C_MMO_RPG_rewrite/UserUpdates/UserUpdates.h"

#include "../C_MMO_RPG_rewrite/noiseLib/Spiral.h"

#include "../C_MMO_RPG_rewrite/TickSystem/TickSystem.h"

#include <bcrypt.h>

void test_task(void *arg) {
    printf("Worker executed test task: %s\n", (char*)arg);
}

void generate_task_id(char out[17]) {
    unsigned long long r;
    BCryptGenRandom(NULL, (PUCHAR)&r, sizeof(r), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    sprintf(out, "%016llX", r);
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
        Task t = {
            .func = RegisterTask,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    
    }
    else if (strcmp(type->valuestring, "Login") == 0) {
        RegisterArgs *args = malloc(sizeof(RegisterArgs));

        args->RId = atoi(cJSON_GetObjectItem(root, "RId")->valuestring);

        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        strcpy(args->password, cJSON_GetObjectItem(root, "password")->valuestring);

        cJSON_Delete(root);
        // return (Task){LoginTask, args,generate_task_id()};
        Task t = {
            .func = LoginTask,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "TechTreeUpdate") == 0){
        //what tech is available to the user
        UUpdate *args = malloc(sizeof(UUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        cJSON_Delete(root);
        // return (Task){TechUpdateTask, args,generate_task_id()};
        Task t = {
            .func = TechUpdateTask,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "TrainableUpdate") == 0){
        //what tech is available to the user
        UUpdate *args = malloc(sizeof(UUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        cJSON_Delete(root);
        // return (Task){TrainingUpdateTask, args,generate_task_id()};
        Task t = {
            .func = TrainingUpdateTask,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "NewRegimen") == 0){
        RegUpdate *args = malloc(sizeof(RegUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);
        strcpy(args->regName, cJSON_GetObjectItem(root, "regName")->valuestring);

        cJSON_Delete(root);
        Task t = {
            .func = NewRegimenTask,
            .arg = args
        };
        generate_task_id(t.taskId);

        AddUserWithTrainingOrders(args->username);
        return t;        
    }
    else if (strcmp(type->valuestring, "TilesRequest") == 0){
        UUpdate *args = malloc(sizeof(UUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        cJSON_Delete(root);
        // return (Task){GetUserTiles, args,generate_task_id()};
        Task t = {
            .func = GetUserTiles,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "ConstructableUpdate") == 0){
        UUpdate *args = malloc(sizeof(UUpdate));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        cJSON_Delete(root);
        // return (Task){ConstructionUpdateTask, args,generate_task_id()};
        Task t = {
            .func = ConstructionUpdateTask,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "PlacementMovement") == 0){

        //check if id is real, if it is then go delete
        cJSON* taskIdItem = cJSON_GetObjectItem(root, "TaskId");
        if (!taskIdItem || !cJSON_IsString(taskIdItem)) {printf("TaskId missing or not a string\n");}
        else{
            char* taskId = taskIdItem->valuestring;
            // printf("thetask id previous:%s\n",taskId);
            remove_task_from_queue(&scheduler.queues[0],taskId);
        };
        


        BuildPlacement *args = malloc(sizeof(BuildPlacement));
        
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);
        strcpy(args->buildingname, cJSON_GetObjectItem(root, "building")->valuestring);
        
        generate_task_id(args->taskId);

        cJSON* posArr = cJSON_GetObjectItem(root, "position");
        args->position[0]=cJSON_GetArrayItem(posArr, 0)->valuedouble;
        args->position[1]=cJSON_GetArrayItem(posArr, 1)->valuedouble;
        args->position[2]=cJSON_GetArrayItem(posArr, 2)->valuedouble;

        cJSON_Delete(root);
        Task t = {
            .func = BuildingPosUpdateTask,
            .arg = args
        };
        // generate_task_id(t.taskId);
        strcpy(t.taskId, args->taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "BuildingPlacement") == 0){

        //should have pthread lock but because its just the origin tile, its whatever, that wont change
        char username[256];
        strcpy(username, cJSON_GetObjectItem(root, "username")->valuestring);
        User* u=cache_get_user(GlobalCache,username);


        double position[3]; // composition array
        cJSON* posArr = cJSON_GetObjectItem(root, "position");
        position[0]=cJSON_GetArrayItem(posArr, 0)->valuedouble;
        position[1]=cJSON_GetArrayItem(posArr, 1)->valuedouble;
        position[2]=cJSON_GetArrayItem(posArr, 2)->valuedouble;
        
        double pixelsPerUnit = 512.0 / 7.5;
        double px = (position[0]+3.75f) * pixelsPerUnit;
        double py = (position[2]+3.75f) * pixelsPerUnit;

        int pxf=(int)px;
        int pyf=(int)py;

        double xchunk=pxf/512.0;
        double ychunk=pyf/512.0;

        int xfloored=(int)xchunk;
        int yfloored=(int)ychunk;

        int tilepixelx=pxf - 512*xfloored;
        int tilepixely=pyf - 512*yfloored;
        Tile* focusTile = cache_get_tile(GlobalCache, xfloored, yfloored);
        
        char buildingname[32];
        strcpy(buildingname, cJSON_GetObjectItem(root, "building")->valuestring);

        bool enoughRoom=canplacebuilding(
            focusTile->Buffer,
            BuildingTemplates[bTypeFromString(buildingname)],
            tilepixelx,tilepixely
        );

        if(enoughRoom){
            cache_addbuilding_tile(focusTile,
                tilepixelx,
                tilepixely,
                BuildingTemplates[bTypeFromString(buildingname)]
            );
            int count=focusTile->buildings.count;
            int ServerId=focusTile->buildings.list[count-1]->base.ServerId;

            char uniquenames[9][256];
            int uniquecount=TileObservers(focusTile,uniquenames);

            char informPart[512];
            informPart[0] = '\0';  // start empty
            strcat(informPart, "\"inform\":[");
            for (int u = 0; u < uniquecount; u++) {
                strcat(informPart, "\"");
                strcat(informPart, uniquenames[u]);
                strcat(informPart, "\"");
                if (u < uniquecount - 1) strcat(informPart, ",");
            }
            strcat(informPart, "]");

            char detailsPart[512];
            snprintf(
                detailsPart, sizeof(detailsPart),
                "\"details\":{"
                    "\"px\":%d,"
                    "\"py\":%d,"
                    "\"cx\":%d,"
                    "\"cy\":%d,"
                    "\"ServerId\":%d,"
                    "\"building\":\"%s\""
                "}",
                tilepixelx,
                tilepixely,
                xfloored,
                yfloored,
                ServerId,
                buildingname
            );

            char msg[1024];
            snprintf(
                msg, sizeof(msg),
                "{\"type\":\"BuildingPlaced\",%s,%s}",
                informPart,
                detailsPart
            );

            send_message(msg);

            AddConstructionOrder(focusTile->buildings.count-1,xfloored,yfloored,tilepixelx,tilepixely);
        }
        // pthread_mutex_unlock(&GlobalCache->lock);

        
    }
    else if (strcmp(type->valuestring, "DeployCitySet") == 0){
        DCity *args = malloc(sizeof(DCity));
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);
        strcpy(args->buildingname, cJSON_GetObjectItem(root, "name")->valuestring);

        Task t = {
            .func = SetDeployCity,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }
    else if (strcmp(type->valuestring, "GetDeployLocations") == 0){
        UUpdate *args = malloc(sizeof(UUpdate));
        strcpy(args->username, cJSON_GetObjectItem(root, "username")->valuestring);

        Task t = {
            .func = GetCityCenters,
            .arg = args
        };
        generate_task_id(t.taskId);
        return t;
    }

    cJSON_Delete(root);
    Task toReturn= {0};
    toReturn.func=NULL;
    return toReturn;
}


void* tick_thread(void *arg) {
    while (1) {
        IncrementTickSystem();
        
        Sleep(200);// 5 ticks a second
    }
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

        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            // printf("JSON: %s\n", line);

            Task t = parse_json_to_task(line);
            if (t.func != NULL){push_task(&scheduler.queues[0], t);}

            line = strtok(NULL, "\n");
        }
        // printf("Read %lu bytes: %s\n", bytesRead, buffer);
        // Parse JSON into the appropriate Task
        // Task t = parse_json_to_task(buffer);
        
        // if(t.func!=NULL){push_task(&scheduler.queues[0], t);}
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
    initSpiral();

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
    pthread_t *threads = malloc(sizeof(pthread_t) * (setup.worker_threads+2));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);   // 8 MB stack

    for (int i = 0; i < setup.worker_threads-1; i++) {
        pthread_create(&threads[i], &attr, worker_thread, NULL);
    }

    pthread_create(&threads[setup.worker_threads-1], &attr, Reader_thread, NULL);
    pthread_create(&threads[setup.worker_threads], &attr, tick_thread, NULL);

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