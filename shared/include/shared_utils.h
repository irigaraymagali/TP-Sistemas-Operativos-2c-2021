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
#define MATE_MEMFREE 9
#define MATE_MEMREAD 10
#define MATE_MEMWRITE 11

// constante para saber quién envia mensaje a memoria
#define ID_MATE_LIB '1'

// valores que devuelve el backend
#define KERNEL_BACKEND = 1 // cuando el kernel responda, va a deolver 1.
#define MEMORIA_BACKEND = 2 // cuando la memoria responda, va a responder 2

typedef struct mate_inner_structure // datos para poder saber qué está pidiendo el carpincho cuando se conecte con backend
{
    int *id;
    char *semaforo; 
    int *valor_semaforo; 
    char *dispositivo_io; 
    int *size_memoria;
    int *addr_memfree;
    int *origin_memread;
    int *dest_memread;
    int *origin_memwrite;
    int *dest_memwrite;
} mate_inner_structure;



char* mi_funcion_compartida();


#endif