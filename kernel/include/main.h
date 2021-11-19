#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <pthread.h>
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
#include <commons/temporal.h>
#include "socket.h"
#include "shared_utils.h"
#include "serialization.h"
#include <commons/string.h>

#endif

typedef struct semaforo
{
    char* nombre;
    int valor;
    t_queue *en_espera; 
} semaforo;

typedef struct dispositivo_io
{
    char *nombre;
    int duracion;
    bool en_uso;
    t_queue *en_espera; 
} dispositivo_io;

typedef struct CPU
{
    int id;
    sem_t semaforo;
} CPU;

typedef struct tiempo
{
    int minutos;
    int segundos;
    int milisegundos;
} tiempo;

typedef struct data_carpincho // la data que le importa tener al backend
{
    int id;
    float rafaga_anterior; // para despues poder calcular la estimación siguiente --> inicializar en 0
    float estimacion_anterior; // idem --> inicializar segun config
    float estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    float RR; //para HRRN -> fijarnos si es necesario o no
    char estado; // => ir cambiandole el estado
    CPU hilo_CPU_usado; // para saber en qué hilo cpu se esta ejecutando
    char tiempo_entrada_a_exec; // para calcular milisegundos en exec
    int fd; // para saber a quien le tiene que responder
    char semaforo; // guarda el char porque es lo que nos manda el carpincho
    int valor_semaforo; // guarda int porque es lo que nos guarda el carpincho
    char dispositivo_io; 

} data_carpincho;

// Estados
t_queue* new;
t_list* ready;
t_list*  exec;
t_list*  exit_list;
t_list* blocked;
t_list* suspended_blocked;
t_queue* suspended_ready;
    
t_list* hilos_CPU;
t_list* semaforos_carpinchos;
t_list* lista_carpinchos;   
t_list* lista_dispositivos_io;


// Mutex para modificar las colas:
pthread_mutex_t sem_cola_new;
pthread_mutex_t sem_cola_ready;
pthread_mutex_t sem_cola_exec;
pthread_mutex_t sem_cola_blocked;
pthread_mutex_t sem_cola_exit;
pthread_mutex_t sem_cola_suspended_blocked;
pthread_mutex_t sem_cola_suspended_ready;

// log
t_log *logger;

// socket memoria;
int *socket_memoria;

// configuración
t_config* config;
char *ip_memoria; 
int puerto_memoria;
int puerto_escucha;
char *algoritmo_planificacion;
float estimacion_inicial;
int alfa;
char *dispositivos_io; 
int *duraciones_io; 
int grado_multiprogramacion;
int grado_multiprocesamiento;
int tiempo_deadlock;

t_list* ptr_dispositivos_io;
t_list* ptr_duraciones_io;


// id carpincho
int *id_carpincho;

// Semáforos
sem_t sem_grado_multiprogramacion_libre;
sem_t sem_grado_multiprocesamiento_libre;
sem_t hay_estructura_creada;
sem_t cola_ready_con_elementos;
sem_t cola_exec_con_elementos;
sem_t cola_blocked_con_elementos;
sem_t cola_suspended_blocked_con_elementos;
sem_t cola_suspended_ready_con_elementos;
sem_t liberar_CPU;
sem_t CPU_libre;
sem_t usar_CPU;  
sem_t sem_programacion_lleno;
sem_t sem_procesamiento_lleno;
sem_t sem_hay_bloqueados;
