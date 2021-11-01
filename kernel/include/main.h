#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <commons/log.h>
#include <stdbool.h>
#include "shared_utils.h"
#include "tests.h"

#endif

t_list lista_carpinchos; // crear lista carpinchos con el tipo de dato de las commons


typedef struct data_carpincho // la data que le importa tener al backend
{
    int id;
    float *rafaga_anterior; // para despues poder calcular la estimación siguiente
    float *estimacion_anterior; // idem
    float *estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float *llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    bool *prioridad; // 1 si tiene prioridad para pasar a ready -> es para los que vienen de suspended_ready a ready
    char *estado; 
    char *semaforo; 
    int *valor_semaforo; 
    char *dispositivo_io; 
    int *size_memoria;
    int *addr_memfree;
    int *origin_memread;
    int *dest_memread;
    int *origin_memwrite;
    int *dest_memwrite;
} data_carpincho;

typedef struct mate_inner_structure // para deserializar lo que va a mandarle la lib
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

/* Estados:*/
    t_queue* new;
    t_queue* ready;
    t_list*  exec;
    t_list*  exit;
    t_queue* blocked;
    t_queue* suspended_blocked;
    t_queue* suspended_ready;

/* Mutex para modificar las colas:*/
    pthread_mutex_t sem_cola_new;
    pthread_mutex_t sem_cola_ready;
    pthread_mutex_t sem_cola_exec;
    pthread_mutex_t sem_cola_blocked;
    pthread_mutex_t sem_cola_exit;
    pthread_mutex_t sem_cola_suspended_blocked;
    pthread_mutex_t sem_cola_suspended_ready;

/* configuraciones*/
    t_config* config;

	char *ip_memoria; 
	int *puerto_memoria;
	int *puerto_escucha;
    char *algoritmo_planificacion;
    int *estimacion_inicial;
    int *alfa;
    char *dispositivos_io; 
    int *duraciones_io; 
    int *grado_multiprogramacion;
    int *grado_multiprocesamiento;
    int *tiempo_deadlock;