#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "server.h"

#define CONFIG_PATH "./cfg/memoria.conf"
#define    MEM_INIT  100
#define    MEM_ALLOC 1
#define    MEM_FREE  2
#define    MEM_READ  3
#define    MEM_WRITE 4

#define FREE 1
#define BUSY 0

#define NULL_ALLOC 0

#define FIRST_PAGE 0

#define HEAP_METADATA_SIZE 9

// SWAMP CONST
#define MEM_ID    "MEM"
#define RECV_PAGE 99
int swamp_fd;


t_log* logger;
pthread_mutex_t swamp_mutex;


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
t_config* config;
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
int getNewEmptyFrame(int idProcess);
int estaOcupadoUn(int emptyFrame, int idProcess);
TablaDePaginasxProceso* get_pages_by(int processID);
int getFrameDeUn(int processId, int mayorNroDePagina);
void inicializarUnProceso(int idDelProceso);
void send_message_swamp(int command, void* payload, int pay_len);
void deserealize_payload(void* payload);
int getframeNoAsignadoEnMemoria();
int frameAsignado(int unFrame);

#endif