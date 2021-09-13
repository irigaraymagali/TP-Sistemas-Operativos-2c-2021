#include "select.h"

// GENERIC SELECT
void _select(char* port, void (*func)(), t_log *logger) {
    int addrlen, new_socket, client_socket[MAX_CLIENTS], activity, i, sd;
	int max_sd;
    struct sockaddr_in address;

    // Buffer con la informacion de cada struct
    void *buffer;

    // Tamanio del buffer
    int b_size;

    // Variable para guardar los mensajes de tipo string
    // char *message;

    // Identificador del proceso que envia el mensaje
    char *id;

    // Codigo de operacion
    int opcode;
     
    // Set of socket descriptors
    fd_set readfds;
 
    // Initialise all client_socket[] to 0 so not checked
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    int lsocket = _create_socket_listenner(port, logger);
	
    // Try to specify maximum of n pending connections for the master socket
    if (listen(lsocket, BACKLOG) < 0) {
        log_error(logger, "[Shared Library]: Ocurrio un error en el listen \n (╯°o°）╯︵ ┻━┻");
        exit(EXIT_FAILURE);
    }
     
    // Accept the incoming connection
    addrlen = sizeof(address);
    log_info(logger, "[Shared Library]: Esperando conexiones en el puerto: %s", port);
    
	while(TRUE) {
        // Clear the socket set
        FD_ZERO(&readfds);
 
        // Add master socket to set
        FD_SET(lsocket, &readfds);
        max_sd = lsocket;
		
        // Add child sockets to set
        for ( i = 0 ; i < MAX_CLIENTS ; i++) {
            // Socket descriptor
			sd = client_socket[i];
            
			// If valid socket descriptor then add to read list
			if(sd > 0) FD_SET( sd, &readfds);
            
            // Highest file descriptor number, need it for the select function
            if(sd > max_sd) max_sd = sd;
        }
 
        // Wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select( max_sd + 1, &readfds, NULL, NULL, NULL);
   
        if ((activity < 0) && (errno != EINTR)) {
            log_error(logger, "[Shared Library]: Ocurrio un error en el select \n (╯°o°）╯︵ ┻━┻");
        }

        // If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(lsocket, &readfds)) {
            if ((new_socket = accept(lsocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                log_error(logger, "[Shared Library]: Ocurrio un error al aceptar una conexion \n (╯°o°）╯︵ ┻━┻");
                exit(EXIT_FAILURE);
            }

            // Inform user of socket number - used in send and receive commands
            log_info(logger, "[Shared Library]: Nueva conexion con id: %d, ip: %s, port: %d", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // Add new socket to array of sockets
            for (i = 0; i < MAX_CLIENTS; i++) {
                // If position is empty
				if( client_socket[i] == 0 ) {
                    client_socket[i] = new_socket;
					break;
                }
            }
        }
         
        // Else its some IO operation on some other socket :)
        for (i = 0; i < MAX_CLIENTS; i++) 
        {
            sd = client_socket[i];

            if (FD_ISSET( sd, &readfds)){

/* -------------------------------- Aca se obtiene el id de proceso como primer mensaje -------------------------------- */

                id = malloc(ID_SIZE + 1);
                if (recv( sd, id, ID_SIZE, 0) <= 0) {
                    
/* -------------------------------- Se perdio la conexion -------------------------------- */
                    free(id);

                    // Call a custom function when a client has disconnected
                    // if (hasCallback) {
                    //   cb(sd, logger);
                    // } else {
                    log_info(logger, "[Shared Library]: El cliente %d se desconecto.", sd);
                    // }
                        
                    // Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                } else {

/* -------------------------------- Recibi el id de proceso -------------------------------- */
                    
                    // Agrego un \0 al final
                    id[ ID_SIZE ] = '\0';

                    // Obtengo el codigo de operacion
                    if (recv( sd, &opcode, sizeof(int), 0) > 0) {
                        
                            // Obtengo el tamanio del buffer que contiene los datos del struct enviado
                            if (recv( sd, &b_size, sizeof(int), 0) > 0) {
                                
                                // SI EL COMANDO ES IGUAL A STRING_COMMAND SIGNIFICA QUE SE ENVIO UN STRING
                                // Creo el buffer para guardar los datos
                                buffer = malloc(opcode == STRING_COMMAND ? b_size + 1 : b_size);
                                // Obtengo el buffer
                                if (recv( sd, buffer, b_size, 0) > 0) {

                                    // If code 999, set \0
                                    if(opcode == STRING_COMMAND) {
                                        memset(buffer + b_size, '\0', 1);
                                    }

                                    // Calling external function with the current file descriptor, process id, operation code and payload
                                    func(sd, id, opcode, buffer, logger);
                                    free(buffer);
                                    free(id);
                                }
                            }
                        // }

                    }
                }
            }
        }
    }
    FD_ZERO(&readfds);
}