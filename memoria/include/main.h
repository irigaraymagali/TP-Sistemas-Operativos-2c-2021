#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdbool.h>
#include <pthread.h>
#include "server.h"
#include "tests.h"
#include "memory_utils.h"

/* LOG CONST */
#define LOG_PATH "./cfg/memoria.log"
#define PROGRAM  "[MEMORIA]"

/* CONFIG CONST */

#define PORT_CONFIG "PUERTO"
#define SWAMP_IP    "IP_SWAMP"
#define SWAMP_PORT  "PUERTO_SWAMP"





void handler(int fd, char* id, int opcode, void* payload, t_log* logger);
void deserialize_init_process(int* pid, void* payload);
void deserialize_mem_alloc(int* pid, int* espacioAReservar, void* payload);
void init_swamp_connection();
void free_memory();

#endif