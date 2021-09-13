#ifndef SOCKET_H
#define SOCKET_H

// STANDARD LIBRARIES

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include <commons/log.h>

// DEFAULT VALUES

#define _PORT "4000"       // Puerto por defecto
#define _BACKLOG 3			// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define _PACKAGESIZE 1024	// Define cual va a ser el size maximo del paquete a enviar

// Estructura para recibir los mensajes
typedef struct {
  char *identifier;
  int command;
  void *payload;
  int pay_len;
} t_mensaje;

/**
 * METHOD: _create_socket_listenner
 * 
 * Crea un socket listenner para un puerto dado
 * 
 * @params: 
 *      port -> Puerto donde escuchar las conexiones 
 * 
 * @return: Devuelve un socket listenner
 */

int _create_socket_listenner(char* port, t_log *logger);

/**
 * METHOD: _connect
 * 
 * Crea una conexion para una ip y un puerto especificos
 * 
 * @params: 
 *      ip   -> Ip destino para la conexion
 *      port -> Puerto destino para la conexion
 *      logger -> log para informacion de la conexion
 *  
 * @return: Devuelve el socket de la conexion
 */

int _connect(char *ip, char *port, t_log *logger);

/**
 * METHOD: _listen
 * 
 * Espera una conexion y devuelve un socket con la informacion de la misma
 * 
 * @params: 
 *      socket   -> Listener socket
 *      backlog  -> Cantidad de conexiones simultaneas
 *  
 * @return: Devuelve el socket de la conexion
 */

int _listen(int socket, int backlog, t_log *logger);

// _listen with default backlog
int _dlisten(int socket);

/**
 * METHOD: _send_message
 * 
 * Permite el envio de mensajes con el protocolo <ID><CMD><PAY_LEN><PAYLOAD>
 * 
 * @params: 
 *      socket   -> A quien se le envia el mensaje
 *      identifier  -> Identificador del proceso que envia el mensaje
 *      command  -> Comando a enviar
 *      payload  -> Datos que se envian
 *      pay_len  -> Tamaño de los datos
 *      logger  -> logger del proceso
 *  
 * @return: Devuelve true o false
 * @ejemplo:
 *      _send_message(socket, IDENTIFIER, COMANDO_1, "HOLA", 4, logger);
 */
int _send_message(int socket, char *identifier, int command, void *payload, int pay_len, t_log *logger);



/**
 * METHOD: _receive_message
 * 
 * Permite recibir mensajes con el protocolo <ID><CMD><PAY_LEN><PAYLOAD>
 * 
 * @params: 
 *      buffer   -> Mensaje recibido de la conexion
 * @returns:
 *      Devuelve un objeto del tipo (t_mensaje *) con la informacion del mensaje recibido
 * @ejemplo:

  t_mensaje *mensaje = _receive_message(buffer);

  log_info(logger, "DATA: %s - %d - %d - %s",
    mensaje -> identifier,
    mensaje -> command,
    mensaje -> pay_len,
    mensaje -> payload
  );

 */
t_mensaje *_receive_message(int socket, t_log *logger);

#endif