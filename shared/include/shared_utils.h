#ifndef SHARED_UTILS_H
#define SHARED_UTILS_H

#include <stdio.h>
#include <commons/log.h>
#include <stdbool.h>

// constantes para saber qué función está usando el carpincho cuando se comunica con backend
#define MATE_INIT 1
#define MATE_CLOSE 2
#define MATE_SEM_INIT 3
#define MATE_SEM_WAIT 4
#define MATE_SEM_POST 5
#define MATE_SEM_DESTROY 6
#define MATE_CALL_IO 7
#define MATE_MEMALLOC 8
#define MATE_MEM_FREE 9
#define MATE_MEM_READ 10
#define MATE_MEM_WRITE 11

// constante para saber quién envia mensaje a memoria
#define ID_MATE_LIB 111


char* mi_funcion_compartida();

#endif