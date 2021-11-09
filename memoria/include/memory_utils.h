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
#include "serialization.h"

#define CONFIG_PATH "./cfg/memoria.conf"
#define    MEM_INIT  100
#define    MEM_ALLOC 1
#define    MEM_FREE  2
#define    MEM_READ  3
#define    MEM_WRITE 4

#define FREE 1
#define BUSY 0

#define NULL_ALLOC 0

#define FIRST_PAGE 1

#define HEAP_METADATA_SIZE 9


//MATE ERRORS 
#define MATE_FREE_FAULT  -5
#define MATE_READ_FAULT  -6

// SWAMP CONST
#define MEM_ID    "MEM"
#define RECV_PAGE 99
int swamp_fd;

t_log* logger;
pthread_mutex_t swamp_mutex, lru_mutex, tlb_mutex;


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
    uint32_t bitUso;
    uint32_t bitModificado;
    uint32_t lRU;
} Pagina;

typedef struct 
{
    int      pid;
    uint32_t pagina;
    uint32_t frame;
} TLB;

t_list* tlb_list;

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
int punteroFrameClock;
int tamanioDeMemoria;
int cantidadDePaginasPorProceso;

void initPaginacion();
int memalloc(int processId, int espacioAReservar);
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
int memfree(int idProcess, int direccionLogica);
Pagina *getPageDe(int processId,int nroPagina);

void utilizarAlgritmoDeAsignacion(int processID);
void seleccionLRU(int processID);
void seleccionClockMejorado();
void liberarFrame(uint32_t nroDeFrame);
Pagina *getMarcoDe(uint32_t nroDeFrame);
void setAsUsedRecently(int idProcess, int nroDePagina);

int memwrite(int idProcess, int direccionLogica, void* loQueQuierasEscribir);
Pagina* get_page_by_dir_logica(TablaDePaginasxProceso* tabla, int dir_buscada);
HeapMetaData* get_heap_metadata(int offset);
HeapMetaData* set_heap_metadata(HeapMetaData* heap, int offset);
void* memread(uint32_t pid, int dir_logica);

#endif