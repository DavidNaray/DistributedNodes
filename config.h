#ifndef CONFIG_H
#define CONFIG_H   // these form a guard

#include <stdbool.h>//makes the boolean type

typedef struct {
    char node_root[256];  //location of the application
    char node_start[128]; // the command that starts up the node.js
    int  node_port;       // the port you want the app to run on, required
    int  worker_threads;  // how many threads, default=1
} Config;


bool load_config(const char *filepath, Config *setup);
void print_config(const Config *config);

#endif
