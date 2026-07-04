define the location of the node.js project root
define how many threads you want to process the things from the application
define the port that the node.js server will be on
define how many queues you want, different queus represent different levels of priority and threads will pull tasks from high priority queus first
---all in the env.conf

compile it like: gcc Main.c scheduler.c config.c C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/CJSON_lib/cjson/cJSON.c -I"C:/Users/david/Documents/CodingProjects/C_MMO_RPG_rewrite/CJSON_lib/cjson" -o DistributedNodes -lpthread

