#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/txt.h>
#include <stdbool.h>
#include <pthread.h>
#include "server.h"
#include "tests.h"
#include "shared_utils.h"
#include "memory_utils.h"

/* LOG CONST */
#define LOG_PATH "./cfg/memoria.log"
#define PROGRAM  "[MEMORIA]"

/* CONFIG CONST */

#define PORT_CONFIG "PUERTO"
#define SWAMP_IP    "IP_SWAMP"
#define SWAMP_PORT  "PUERTO_SWAMP"

pthread_mutex_t pid_global_mutex;

typedef struct{
    int entrada;
    char* status;
    char* pid;
    char* page;
    char* frame;
} Dump;

int pid_global;

void print_dump();
void write_dump(FILE* file, char* record);
void print_metrics();
void clean_tlb();
void handler(int fd, char* id, int opcode, void* payload, t_log* logger);
int deserialize_init_process(char* id, void* payload);
void deserialize_mem_alloc(int* pid, int* espacioAReservar, void* payload);
void init_swamp_connection();
void free_memory();

#endif