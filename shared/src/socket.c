#include "socket.h"

int _create_socket_listenner(char *port, t_log *logger) {
	struct addrinfo infoConexion;
	struct addrinfo *serverInfo;

	/*MEMSET:
		str − This is a pointer to the block of memory to fill.
		c − This is the value to be set. The value is passed as an int, but the function fills 
			the block of memory using the unsigned char conversion of this value.
		n − This is the number of bytes to be set to the value.*/


	memset(&infoConexion, 0, sizeof(infoConexion));
	infoConexion.ai_family = AF_UNSPEC; // No importa si uso IPv4 o IPv6
	infoConexion.ai_flags = AI_PASSIVE;
	infoConexion.ai_socktype = SOCK_STREAM;
	
	int opt = 1; 

	/* GETADDRINFO:
		(const char *node, const char *service,const struct addrinfo *hints,struct addrinfo **res)
		(node+ service --> identify an internet host and a service)*/

	if (getaddrinfo(NULL, port, &infoConexion, &serverInfo) < 0)
	{
		log_error(logger, "[Shared Library]: Error al obtener info de la conexion");
		freeaddrinfo(serverInfo);
		return -1;
	}

	/* SOCKET:
	socket() creates an endpoint for communication and returns a file descriptor that refers to that endpoint.  The file descriptor
    returned by a successful call will be the lowest-numbered file descriptor not currently open for the process.

	RETURN VALUE         
       On success, a file descriptor for the new socket is returned.  On error, -1 is returned, and errno is set appropriately. */

	int list_sock = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if(list_sock == -1){
		log_error(logger, "[Shared Library]: Error al crear socket listenner");
		freeaddrinfo(serverInfo);
		return -1;
	}

	/* BIND: When a socket is created with socket(2), it exists in a name space (address family) but has no address assigned to it.  
	bind() assigns the address specified by addr to the socket referred to by the file descriptor sockfd.  addrlen specifies the size, 
	in bytes, of the address structure pointed to by addr.  Traditionally, this operation is called “assigning a name to a socket”.*/
	
	bind(list_sock, serverInfo->ai_addr, serverInfo->ai_addrlen);
	freeaddrinfo(serverInfo);

	// set socket to allow multiple connections
	if(setsockopt(list_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		log_error(logger, "[Shared Library]: Error al setear multiples conexiones -setsockopt");
		return -1;
	}

	log_info(logger, "[Shared Library]: Se creo un socket listenner en el puerto: %s ", port);

	return list_sock;
}


int _connect(char *ip, char *port, t_log *logger){

	struct addrinfo infoConexion;
	struct addrinfo *serverInfo;

	memset(&infoConexion, 0, sizeof(infoConexion));
	infoConexion.ai_family = AF_UNSPEC;
	infoConexion.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, port, &infoConexion, &serverInfo);

	int socketServer = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if(socketServer < 0) {
		log_error(logger, "[Shared Library]: Error al crear socket de servidor. /n (╯°□°）╯︵ ┻━┻");
		freeaddrinfo(serverInfo);
		return -1;
	}

	log_info(logger, "Intentando conexion a IP: %s y PORT: %s ..", ip, port);
	int conexion = connect(socketServer, serverInfo->ai_addr, serverInfo->ai_addrlen);

	if(conexion < 0) {
		log_error(logger, "[Shared Library]: Error al conectarse con servidor /n (╯°□°）╯︵ ┻━┻");
		freeaddrinfo(serverInfo);
		return -1;
	}

	log_info(logger, "[Shared Library]: Conexion exitosa. (╯°o°)ᕗ");

	freeaddrinfo(serverInfo);

	return socketServer;
}

//BACKLOG: cantidad maxima de conexiones(accept) por socket 

int _listen(int socket, int backlog, t_log *logger) { //solo 1 conexion
    
  	listen(socket, backlog);

	struct sockaddr_in addr;           // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	log_info(logger, "Esperando conexiones..");

	int socketCliente = accept(socket, (struct sockaddr *) &addr, &addrlen);

  	return socketCliente;
}

int _dlisten(int socket) {
    
	listen(socket, _BACKLOG);

	struct sockaddr_in addr;           // Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(socket, (struct sockaddr *) &addr, &addrlen);

	return socketCliente;
}

int _send_message(int socket, char *identifier, int command, void *payload, int pay_len, t_log *logger) {

	// Envio el id de proceso
	if (send(socket, identifier, strlen(identifier), 0) < 0) {
		log_error(logger, "[Shared Library]: Error al enviar mensaje. (╯°□°）╯︵ ┻━┻");
		return 0;
	}

	// Envio el opcode
	if (send(socket, &command, sizeof(int), 0) < 0) {
		log_error(logger, "[Shared Library]: Error al enviar mensaje. (╯°□°）╯︵ ┻━┻");
		return 0;
	}

	// Envio el tamanio del buffer
	if (send(socket, &pay_len, sizeof(int), 0) < 0) {
		log_error(logger, "[Shared Library]: Error al enviar mensaje. (╯°□°）╯︵ ┻━┻");
		return 0;
	}

	// Envio el buffer
	if (send(socket, payload, pay_len, 0) < 0) {
		log_error(logger, "[Shared Library]: Error al enviar mensaje. (╯°□°）╯︵ ┻━┻");
		return 0;
	}
	
	log_info(logger, "[Shared Library]: ╰( ⁰ ਊ ⁰ )━☆ﾟ.*･｡ﾟ Mensaje enviado correctamente.");
	
	return 1;
}

t_mensaje *_receive_message(int socket, t_log *logger) {

	t_mensaje *temp = malloc(sizeof(t_mensaje));
  
	temp -> identifier = malloc(4);
	recv(socket, temp -> identifier, 3, 0);
	temp -> identifier[3] = '\0';

	//log_info(logger, "Proceso: %s", temp -> identifier);

	recv(socket, &(temp -> command), sizeof(int), 0);

	//log_info(logger, "Comando: %d", temp -> command);

	recv(socket, &(temp -> pay_len), sizeof(int), 0);

	//log_info(logger, "Tamanio: %d", temp -> pay_len);

	temp -> payload = malloc(temp -> command == 999 ? temp -> pay_len + 1 : temp -> pay_len);
	recv(socket, temp -> payload, temp -> pay_len, 0);
	if (temp -> command == 999) {
		memset(temp -> payload + temp -> pay_len, '\0', 1);
		log_info(logger, "Recibi: String: %s", temp -> payload);
	}

	log_info(logger, "Recibi un mensaje..");

	return temp;
}