#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>


// CONECTARSE CON MATE LIB, llega un proceso => agregarlo a cola new

// Estados:
    t_queue* new;
    t_queue* ready;
    t_list*  exec;
    t_list*  exit;
    t_queue* blocked;
    t_queue* suspended_blocked;
    t_queue* suspended_ready;

// Mutex para modificar las colas:
    pthread_mutex_t sem_cola_new;
    pthread_mutex_t sem_cola_ready;
    pthread_mutex_t sem_cola_exec;
    pthread_mutex_t sem_cola_blocked;
    pthread_mutex_t sem_cola_exit;
    pthread_mutex_t sem_cola_suspended_blocked;
    pthread_mutex_t sem_cola_suspended_ready;

    t_config* config;

int main(int argc, char ** argv){

    if(argc > 1 && strcmp(argv[1],"-test")==0)
        return run_tests();
    else{  
        t_log* logger = log_create("./cfg/kernel.log", "KERNEL", true, LOG_LEVEL_INFO);
        log_info(logger, "Soy el Kernel! %s", mi_funcion_compartida());
        log_destroy(logger);
    } 

// Config:  (falta)
	ip_memoria= config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    dispositivos_io= config_get_string_value(config, "DISPOSITIVOS_IO");
    duraciones_io = config_get_int_value(config, "DURACIONES_IO");
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    grado_multiprocesamiento= config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");
    tiempo_deadlock = config_get_int_value(config, "TIEMPO_DEADLOCK");
    estimacion_inicial = config_get_int_value(config, "ESTIMACION_INICIAL");
    alfa = config_get_int_value(config, "ALFA");

// Inicializacion de semaforos:
    sem_init(&sem_grado_multiprogramacion,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento, 0,grado_multiprocesamiento); 

	sem_init(&cola_new_con_elementos,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_exit_con_elementos,0,0);
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 

// Colas estados:
	new = queue_create();
	pthread_mutex_init(&sem_cola_new, NULL);

	ready = queue_create();
	pthread_mutex_init(&sem_cola_ready, NULL);

	exec = list_create();
	pthread_mutex_init(&sem_cola_exec, NULL);

    exit = list_create();
    pthread_mutex_init(&sem_cola_exit, NULL);

	blocked = queue_create();
	pthread_mutex_init(&sem_cola_blocked, NULL);

    suspended_blocked = queue_create();
	pthread_mutex_init(&sem_cola_suspended_blocked, NULL);

    suspended_ready = queue_create();
	pthread_mutex_init(&sem_cola_suspended_ready, NULL);
	
    pthread_mutex_init(&socket_memoria, NULL);

}


void new_a_ready(carpincho* carpincho){ //hilo

    while(1){
        sem_wait(&cola_new_con_elementos); //si hay procesos en new
        sem_wait(sem_grado_multiprogramacion); //grado multiprogramacion --> post cuando sale de exec
		
       // saco de cola new y pongo en cola ready al primero:
        pthread_mutex_lock(&sem_cola_ready; 
		pthread_mutex_lock(&sem_cola_new);

        queue_push(ready, *queue_peek(new);
        queue_pop(new);

		pthread_mutex_unlock(&sem_cola_new);
		pthread_mutex_unlock(&sem_cola_ready);

		sem_post(&cola_ready_con_elementos); //avisa que hay procesos en ready
    }
    
}

void ready_a_exec(carpincho* carpincho){  
    //saco de cola ready y pongo en cola exec según algoritmo
}


