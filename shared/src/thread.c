#include "thread.h"

void _thread_pool_create(void *( *func )(), pthread_t *_thread_pool, int _thread_pool_size, t_log *logger) {

    for (int i = 0; i < _thread_pool_size; i++) {
        pthread_create(&_thread_pool[i], NULL, func, logger);
        pthread_detach(_thread_pool[i]);
    }
}


// No usar
void _thread_pool_destroy(pthread_t *_thread_pool, int _thread_pool_size) {

    for (int i = 0; i < _thread_pool_size; i++) {
        pthread_join(_thread_pool[i], NULL);
    }
}

/* ---------------------------- SERVER WITH THREAD POOL ---------------------------- */

/*
#include "server.h"

// GENERIC MULTITHREADED SERVER WITH THREAD POOL
void _start_server(char* port, t_log *logger) {
    int addrlen, new_socket;
    struct sockaddr_in address;

    int lsocket = _create_socket_listenner(port, logger);
	
    // Try to specify maximum of n pending connections for the master socket
    if (listen(lsocket, BACKLOG) < 0) {
        log_error(logger, "[Shared Library]: Ocurrio un error en el listen \n (╯°o°）╯︵ ┻━┻");
        exit(EXIT_FAILURE);
    }

    while (validator) {
        if ((new_socket = accept(lsocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            log_error(logger, "[Shared Library]: Ocurrio un error al aceptar una conexion \n (╯°o°）╯︵ ┻━┻");
            exit(EXIT_FAILURE);
        }

        int *client = malloc (sizeof(int));
        *client = new_socket;
        pthread_mutex_lock(&_socket_queue_mutex);
        queue_push(_socket_queue, client);
        pthread_cond_signal(&_socket_queue_cond);
        pthread_mutex_unlock(&_socket_queue_mutex);
        free(client);
    }

}

void *_thread_function(t_log *logger) {
    int *client;
    log_info(logger, "Starting new thread for connection handling..");
    while (validator) {
        if (!queue_is_empty(_socket_queue)){
            pthread_mutex_lock(&_socket_queue_mutex);
            // if ((client = queue_pop(_socket_queue)) == NULL) {
                pthread_cond_wait(&_socket_queue_cond, &_socket_queue_mutex);
                client = queue_pop(_socket_queue);
            // };
            pthread_mutex_unlock(&_socket_queue_mutex);
            if (client != NULL) {
                _handle_connection(*client, logger);
                free(client);
            }
        }
    }

    return NULL;
}

void _handle_connection (int socket, t_log* logger) {
    log_info(logger, "Handling new connection for ID: %d", socket);

    // t_mensaje *mensaje = _receive_message(socket, logger);

    // free(mensaje -> identifier);
    // free(mensaje -> payload);
    // free(mensaje);

    close(socket);
}
*/
