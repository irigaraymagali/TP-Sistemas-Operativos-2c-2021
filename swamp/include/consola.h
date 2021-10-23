#ifndef CONSOLA_H
#define CONSOLA_H

#include "main.h"
#include "swap.h"
#include <sys/mman.h>  // Biblioteca mmap()
#include <sys/stat.h>  // Biblioteca open(), mkdir()
#include <sys/types.h> // Biblioteca ftruncate(), opendir()
#include <fcntl.h>     // Biblioteca open()
#include <unistd.h>    // Biblioteca pwrite(), close()
#include <dirent.h>    // Biblioteca opendir()
#include <errno.h>

void recibir_mensajes();
void consola(char* buffer, int socket_conexion);
int contar_parametros(char** parametros);

#endif
