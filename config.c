#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include <string.h>

void trimEnd(char *value){
    size_t len = strlen(value);

    if (len > 0 && value[len - 1] == '\n'){value[--len] = '\0';}

    if (len > 0 && value[len - 1] == '\r'){value[--len] = '\0';}
}

bool process_line(char *line, Config *cfg){
    char *key = strtok(line, "=");
    char *value = strtok(NULL, "=");

    if (!key || !value){return false;}; //  line of wrong form detector

    //value is a pointer and essentially you cant just assign it to the struct param
    //since the param might have different size and thats a nono for strings of a buffer size
    //hence the strncpy to copy over the value into the buffer
    //atoi turns a number string into an actual number
    
    trimEnd(value);

    if (strcmp(key, "NODE_ROOT") == 0) {strncpy(cfg->node_root, value, sizeof(cfg->node_root));}

    else if (strcmp(key, "NODE_START") == 0) {strncpy(cfg->node_start, value, sizeof(cfg->node_start));}

    else if (strcmp(key, "NODE_PORT") == 0) {cfg->node_port = atoi(value);}

    else if (strcmp(key, "WORKER_THREADS") == 0) {cfg->worker_threads = atoi(value);}

    else if (strcmp(key, "QUEUE_ORDER") == 0) {cfg->queue_order = atoi(value);}

    return true;
}

void print_config(const Config *cfg) {
    printf("--- CONFIG ---\n");
    printf("NODE_ROOT:      %s\n", cfg->node_root);
    printf("NODE_START:     %s\n", cfg->node_start);
    printf("NODE_PORT:      %d\n", cfg->node_port);
    printf("WORKER_THREADS: %d\n", cfg->worker_threads);
    printf("QUEUE_ORDER:    %d\n", cfg->queue_order);
    printf("--------------\n");
}


bool load_config(const char *filepath, Config *setup) {
    FILE * f;
    char buffer[512];

    f = fopen(filepath, "r");
    // !f means the pointer to the file, points to nothing
    if (!f) {printf("%s", "Failed to open config file:");return false;}

    memset(setup, 0, sizeof(Config));//zeros the struct, ints=0, strings=\0 ... etc
    setup->worker_threads = 1;//default threadcount is 1

    //read the file line by line
    while (fgets(buffer, sizeof(buffer), f)) { process_line(buffer,setup); }

    fclose(f);

    print_config(setup);

    return true;
}