#include "scheduler.h"
#include "config.h"
#include <stdio.h> 

int main() {
    Config setup;// no need to malloc, all are known types

    bool successConfig=load_config("env.conf", &setup);
    if (!successConfig) {printf("Failed to load config\n");return 1;}

    init_scheduler(&setup);

    printf("YAYAYAYAY\n");
    // start_workers();
}