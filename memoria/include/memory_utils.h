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
#include <commons/collections/list.h>

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
} Pagina;


typedef struct 
{
    uint32_t id;
    t_list *paginas;
    int lastHeap;
} TablaDePaginasxProceso;

typedef struct 
{
    uint32_t id;
    t_list paginas;
} ubicacionDeHeap;

t_log* logger;
t_config* configFile;
t_list *todasLasTablasDePaginas;

void* memoria;
int tamanioDePagina;
int tipoDeAsignacionDinamica;
int lRUACTUAL;

void comandos(int valor);
void initPaginacion();
int memalloc(int espacioAReservar, int processId);
int entraEnElEspacioLibre(int espacioAReservar, int processId);
void agregarXPaginasPara(int processId, int espacioRestante);
Pagina *getLastPageDe(int processId);
int getFrameDeUn(int processId, int unaPagina);
int getNewEmptyFrame();

#endif