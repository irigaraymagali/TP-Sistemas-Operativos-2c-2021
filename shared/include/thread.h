#ifndef THREAD_H
#define THREAD_H

// STANDARD LIBRARIES

#include <pthread.h>
#include <commons/log.h>

// FUNCTIONS

/**
 * METHOD: _thread_pool_create
 * 
 * Permite crear una thread pool del tamaño especificado en THREAD_POOL_SIZE
 * 
 * @params: 
 *      func -> Funcion custom a ejecutar en cada iteracion [use func()]
 *      _thread_pool -> Array de worker threads
 *      _thread_pool_size -> tamaño de la threadpool (tamaño del array de threads)
 *
 * @example:
 *      Call with _thread_pool_create(myFunction, threads, pool_size);
 */
void _thread_pool_create(void *( *func )(), pthread_t *_thread_pool, int _thread_pool_size, t_log *logger);

/**
 * METHOD: _thread_pool_destroy
 * 
 * Permite eliminar la thread pool una vez hayan terminado de trabajar los threads (Thread Join)
 *
 * @params: 
 *      _thread_pool -> Array de worker threads
 *      _thread_pool_size -> tamaño de la threadpool (tamaño del array de threads)
 *
 *
 * @example:
 *      Call with _thread_pool_destroy(threads, pool_size);
 */

// NO USAR! se utiliza pthread_detach( ) en el create
void _thread_pool_destroy(pthread_t *_thread_pool, int _thread_pool_size);

/* ---------------------------- SERVER WITH THREAD POOL ---------------------------- */

/*
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
#include "thread.h"
#include <commons/collections/queue.h>

// DEFAULT VALUES

#define TRUE   1
#define FALSE  0
#define MAX_CLIENTS 30
#define BACKLOG 5
#define ID_SIZE 3
#define STRING_COMMAND 999

int validator = 1;

t_queue *_socket_queue;

pthread_mutex_t _socket_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t _socket_queue_cond = PTHREAD_COND_INITIALIZER;

void _start_server(char* port, t_log *logger);
void *_thread_function(t_log *logger);
void _handle_connection (int socket, t_log* logger);

#endif

*/

#endif
