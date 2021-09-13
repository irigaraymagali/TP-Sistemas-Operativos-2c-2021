#ifndef SELECT_H
#define SELECT_H

// STANDARD LIBRARIES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "socket.h"

// DEFAULT VALUES

#define TRUE   1
#define FALSE  0
#define MAX_CLIENTS 30
#define BACKLOG 5
#define ID_SIZE 3
#define STRING_COMMAND 999

/**
 * METHOD: _select
 * 
 * Permite monitorear varias conexiones en un thread
 * 
 * @params: 
 *      port -> Puerto en el que se van a escuchar las conexiones
 *      func -> Funcion custom a ejecutar en cada interaccion
 
 *          void myFunction(int fd, char *id, int opcode, void *buffer, t_log *logger);

 *      b_size -> Longitud maxima del string a recibir
 *
 * @example:
 *      Call with _select("9000", myFunction, logger);
 */

void _select(char* port, void (*func)(), t_log *logger);

#endif