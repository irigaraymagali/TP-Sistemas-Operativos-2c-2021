#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdbool.h>
#include "tests.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define mate_memalloc 1
#define    memfree 2
#define    memread 3
#define    memwrite 4



typedef struct 
{
    uint32_t prevAlloc;
    uint32_t nextAlloc;
    uint8_t isfree;
} HeapMetaData;

typedef struct 
{
    uint32_t frame;
    uint32_t pagina;
    uint8_t isfree;
    uint32_t bitPresencia;
    uint32_t bitModificado;
    uint32_t lRU;
} TablaDePaginaXProceso;

t_log* logger;



void comandos(int valor);

#endif