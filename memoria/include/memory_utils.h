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

#define    MEM_ALLOC 1
#define    MEM_FREE  2
#define    MEM_READ  3
#define    MEM_WRITE 4

#define FREE 0
#define BUSY 1

#define NULL_ALLOC 0

#define FIRST_PAGE 0

#define HEAP_METADATA_SIZE 9

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
int tamanioDeMemoria;
int cantidadDePaginasPorProceso;


void comandos(int valor);
void initPaginacion();
int memalloc(int espacioAReservar, int processId);
int entraEnElEspacioLibre(int espacioAReservar, int processId);
void agregarXPaginasPara(int processId, int espacioRestante);
Pagina *getLastPageDe(int processId);
int getFrameDeUn(int processId, int unaPagina);
int getNewEmptyFrame();
int estaOcupadoUn(int emptyFrame);
TablaDePaginasxProceso* get_pages_by(int processID);
int getFrameDeUn(int processId, int mayorNroDePagina);
void inicializarUnProceso(int idDelProceso)

#endif