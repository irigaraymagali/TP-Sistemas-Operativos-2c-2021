#ifndef MAIN_H
#define MAIN_H

#include "server.h"
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
#include <assert.h>

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

typedef struct tiempo
{
    int minutos;
    int segundos;
    int milisegundos;
} tiempo;

typedef struct data_carpincho // la data que le importa tener al backend, hacer todo punteros
{
    int id;
    float rafaga_anterior; // para despues poder calcular la estimación siguiente --> inicializar en 0
    float estimacion_anterior; // idem --> inicializar segun config
    float estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    float RR; //para HRRN 
    char estado; 
    int CPU_en_uso;
    int tiempo_entrada_a_exec; // para calcular milisegundos en exec
    int fd; // para saber a quien le tiene que responder
    char *semaforo; // guarda el char porque es lo que nos manda el carpincho
    int valor_semaforo; // guarda int porque es lo que nos guarda el carpincho
    char *dispositivo_io; 

    char *nombre_semaforo_por_el_que_se_bloqueo; //nombre del semaforo por el que se bloqueo --> sacarlo cuando se desbloquee
    t_list *semaforos_retenidos; //nombre de los semaforos a los que paso su wait --> lista de semaforos
    // si le hace el post sacarlo
    int tiene_su_espera; //id del carpincho que tiene retenido al semaforo que el esta esperando

} data_carpincho;

// Estados
t_queue* new;
t_list* ready;
t_list*  exec;
t_list*  exit_list;
t_list* blocked;
t_list* suspended_blocked;
t_queue* suspended_ready;

t_queue* CPU_libres;
    
t_list* hilos_CPU;
t_list* semaforos_carpinchos;
t_list* lista_carpinchos;   
t_list* lista_dispositivos_io;

//t_list* semaforos_retenidos; 
t_list* lista_posibles;
t_list* lista_conectados;
t_list* ciclo_deadlock;

// Mutex para modificar las colas:
pthread_mutex_t sem_cola_new;
pthread_mutex_t sem_cola_ready;
pthread_mutex_t sem_cola_exec;
pthread_mutex_t sem_cola_blocked;
pthread_mutex_t sem_cola_exit;
pthread_mutex_t sem_cola_suspended_blocked;
pthread_mutex_t sem_cola_suspended_ready;
pthread_mutex_t sem_CPU_libres;
pthread_mutex_t mutex_para_CPU;

// log
t_log *logger;

t_config* config;

t_list* ptr_dispositivos_io;
t_list* ptr_duraciones_io;

// id carpincho
int id_carpincho;

// Semáforos
sem_t sem_grado_multiprogramacion_libre;
sem_t sem_grado_multiprocesamiento_libre;
sem_t hay_estructura_creada;
sem_t cola_ready_con_elementos;
sem_t cola_exec_con_elementos;
sem_t cola_blocked_con_elementos;
sem_t cola_suspended_blocked_con_elementos;
sem_t cola_suspended_ready_con_elementos;
sem_t sem_programacion_lleno;
sem_t sem_procesamiento_lleno;
sem_t sem_hay_bloqueados;
sem_t hay_bloqueados_para_deadlock;
    sem_t liberar_CPU[1000];
    sem_t CPU_libre[1000];
    sem_t usar_CPU[1000];

    pthread_t hilo_CPU[1000]; // gonza -> nos va a pasar algo parecido que con los semaforos


void handler(int fd, char* id, int opcode, void* payload, t_log* logger);

void inicializar_colas();
void inicializar_semaforos();
void crear_hilos_CPU();
void free_memory();
data_carpincho* encontrar_estructura_segun_id(int id);
data_carpincho* deserializar(void* buffer);

void mate_init(int fd);
void mate_close(int id_carpincho, int fd);
bool esIgualASemaforo(char* nombre_semaforo, void *semaforo_igual);
void mate_sem_init(int id_carpincho, char* nombre_semaforo, int valor_semaforo, int fd);
bool esIgualA(void *semaforo_igual);
void mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo, int fd);
void mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo, int fd);
void mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo, int fd);
bool es_igual_dispositivo(mate_io_resource nombre_dispositivo, void *dispositivo);
void mate_call_io(int id_carpincho, mate_io_resource nombre_io, int fd);
bool igual_a(void *dispositivo);

void crear_estructura_dispositivo();
int contar_elementos(char** elementos);

void mate_memalloc(int id_carpincho, int size, int fd);
void mate_memfree(int id_carpincho, mate_pointer addr, int fd);
void mate_memread(int id_carpincho, mate_pointer origin, int size, int fd);
void mate_memwrite(int id_carpincho, void* origin, mate_pointer dest, int size, int fd);

void entrantes_a_ready();
void ready_a_exec();
void exec_a_block(int id_carpincho);
void exec_a_exit(int id_carpincho, int fd);
void crear_hilos_planificadores();
void block_a_ready(data_carpincho *carpincho);
void suspended_blocked_a_suspended_ready(data_carpincho *carpincho);
void suspender();
bool estan_las_condiciones_para_suspender();
data_carpincho* ready_a_exec_SJF();
data_carpincho* ready_a_exec_HRRN();
void calculo_estimacion_siguiente(data_carpincho *carpincho);
void calculo_rafaga_anterior(data_carpincho *carpincho);
void calculo_RR(data_carpincho *carpincho);
int calcular_milisegundos();
void asignar_hilo_CPU(data_carpincho *carpincho);
void ejecuta(void *id_cpu);

void detectar_deadlock();
bool formar_ciclo();
void solucionar_deadlock();
void liberar_carpincho(void *carpincho);

void port_fixer();

void exec_a_block_io(int id_carpincho,  mate_io_resource nombre_io);