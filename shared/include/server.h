#ifndef SERVER_H
#define SERVER_H

// STANDARD LIBRARIES

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include "socket.h"
#include <signal.h>
#include <pthread.h>

#define MAX_CON 5
#define STRING_COMMAND 999

int validator = 1;

typedef struct {

    int socket;
    
    t_log *logger;
    
    void (*func)();    

} t_data;

/**
 * METHOD: _start_server
 * 
 * Crea un servidor multihilo para el manejo de conexiones
 * 
 * @params: 
 *      port -> Puerto donde escuchar las conexiones 
 *      callback -> Funcion para manejar las respuestas de los clientes
 * @example:
 *      void callback(int socket, char *id, int comando, void *buffer, t_log *logger);
 *
 *      logger -> Logger para el print de mensajes
 * 
 * @usage:
 *      _start_server("9000", callback, logger);
 */
void _start_server(char *port, void (*callback)(), t_log *logger);


void _thread_function(t_data *connection);


t_data *_create_metadata(int new_socket, t_log *logger, void (* callback)());


#endif
