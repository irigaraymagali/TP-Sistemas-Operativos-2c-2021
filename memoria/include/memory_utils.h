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
#include "shared_utils.h"

#define CONFIG_PATH "./cfg/default.conf"

#define FREE 1
#define BUSY 0

#define NULL_ALLOC 0

#define FIRST_PAGE 1

#define HEAP_METADATA_SIZE 9


//MATE ERRORS 
#define MATE_FREE_FAULT  -5
#define MATE_READ_FAULT  -6
#define MATE_WRITE_FAULT -7

#define TLB_MISS 0
#define TLB_HIT  1

int swamp_fd;

typedef enum {
    C_PID,
    C_PAGE,
    C_FRAME,
} TLB_Condition;

pthread_mutex_t swamp_mutex, list_pages_mutex, lru_mutex, tlb_mutex, tlb_lru_mutex, entrada_fifo_mutex, max_hit_tlb_mutex, max_miss_tlb_mutex, memory_mutex;

pthread_mutex_t m_list_mutex;

int max_tlb_hit, max_tlb_miss, max_entradas_tlb, retardo_hit_tlb, retardo_miss_tlb, entrada_fifo, tlb_lru_global;

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

typedef struct {
    uint32_t pid;
    uint32_t pagina;
    uint32_t frame;
    uint32_t fifo;
    uint32_t lru;
} TLB;

typedef struct {
    int pid;
    int hits;
    int miss;
} Metric;

t_list* tlb_list;

t_list* metrics_list;

typedef struct 
{
    uint32_t id;
    t_list *paginas;
    int lastHeap;
} TablaDePaginasxProceso;


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
void* send_message_swamp(int command, void* payload, int pay_len);
void deserealize_payload(void* payload);
int getframeNoAsignadoEnMemoria();
int frameAsignado(int unFrame);
int memfree(int idProcess, int direccionLogica);
Pagina *getPageDe(int processId,int nroPagina);
void deletePagina(int idProcess,int paginaActual);
void setPaginaAsModificado(int processId, int mayorNroDePagina);

void utilizarAlgritmoDeAsignacion(int processID);
void seleccionLRU(int processID);
void seleccionClockMejorado(int idProcess);
void liberarFrame(uint32_t nroDeFrame);
void mandarPaginaAgonza(int processID ,uint32_t frameDeMemoria, uint32_t nroDePagina);
Pagina *getMarcoDe(uint32_t nroDeFrame);
void setAsUsedRecently(int idProcess, int nroDePagina);
uint32_t getProcessIdby(uint32_t nroDeFrame);

int memwrite(int idProcess, int direccionLogica, void* loQueQuierasEscribir, int tamanio);
Pagina* get_page_by_dir_logica(TablaDePaginasxProceso* tabla, int dir_buscada);
HeapMetaData* get_heap_metadata(int offset);
HeapMetaData* set_heap_metadata(HeapMetaData* heap, int offset);
void* memread(uint32_t pid, int dir_logica, int size);

void add_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame);
TLB* new_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame);
void* get_minimum_lru_tlb(void* actual, void* next);
void replace_entrada(TLB* new_instance);
TLB* fetch_entrada_tlb(uint32_t pid, uint32_t page);
void free_tlb();
int delete_process(int pid);
void remove_paginas(void* elem);
void set_pid_metric_if_missing(uint32_t pid); 
void sum_metric(uint32_t pid, int isHit);
int allFramesUsedForAsignacionFijaPara(int processID);

int cantidadDeFramesEnMemoriaPor(int processID);

int suspend_process(int pid);

void memoryDump();
void editarAlgoEnMemoria(int processId,int inicio, int tamanio, void* loQuieroMeter);

void* get_minimum_fifo_tlb(void* actual, void* next);

void delete_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame);

bool has_empty_instance(void* elem);

#endif