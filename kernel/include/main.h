#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdbool.h>
#include "shared_utils.h"
#include "tests.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include "socket.h"
#include "shared_utils.h"
#include "serialization.h"

#endif

typedef struct data_carpincho // la data que le importa tener al backend
{
    int id;
    float *rafaga_anterior; // para despues poder calcular la estimación siguiente
    float *estimacion_anterior; // idem
    float *estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float *llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    float *RR; //para HRRN -> fijarnos si es necesario o no
    bool *prioridad; // 1 si tiene prioridad para pasar a ready -> es para los que vienen de suspended_ready a ready
    char *estado; // => ir cambiandole el estado

    char *semaforo; 
    int *valor_semaforo; 
    char *dispositivo_io; 
    int *size_memoria;
    int *addr_memfree;
    int *origin_memread;
    int *dest_memread;
    int *origin_memwrite;
    int *dest_memwrite;

    void *responder; // para saber a quién responder cuando el carpincho esté listo
} data_carpincho;


typedef struct semaforo
{
    char nombre;
    int valor;
    t_list en_espera; // cambiar a una cola
} semaforo;


t_list* semaforos_carpinchos;
t_list* lista_carpinchos; 


/* Estados:*/
    t_queue* new;
    t_queue* ready;
    t_list*  exec;
    t_list*  exit_list;
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