#ifndef MAIN_H
#define MAIN_H

#include "shared_utils.h"
#include "tests.h"
#include "socket.h"
#include "consola.h"
#include "swap.h"
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#define CONFIG_PATH "./cfg/swamp.cfg"
#define LOG_PATH "./cfg/swamp.log"
#define BUFFER_SIZE 1024

t_config* config_file;
t_log* log_file;
char buffer[BUFFER_SIZE];
int server_socket;

void recibir_mensajes();
void cerrar_swamp();

#endif