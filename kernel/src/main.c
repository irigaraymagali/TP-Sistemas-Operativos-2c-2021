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

//////////////// FUNCIONES GENERALES ///////////////////

int crear_estructura(mate_instance *mate_inner_structure){
    /*
    - completar con id
    - completar la estructura con por ej los valores del config de la rafaga y estimacion
    - reservar espacio en memoria 
    - avisar que llegó un nuevo carpincho a new => post a new_con_elementos
    - una vez que está en EXEC retornar 0
    */
}

int borrar_estructura(mate_instance *mate_inner_structure){
    /* conectar con memoria para borrar todo*/
}

int iniciar_semaforo(mate_instance *mate_inner_structure, mate_sem_name sem, unsigned int value){
    mate_instance->sem_instance = malloc(sizeof(sem_t));

        /* 
           guardar el semaforo en la estructura del carpincho
        */
        
    return sem_init(mate_instance->sem_instance, 0, value); // qué pasa si quiero inicializar más de un hilo?

}

int wait_semaforo(mate_instance *mate_inner_structure, mate_sem_name sem, unsigned int value){
   /* 
        cuando le hagan post le retorne 0 por la conexión así ahí puede seguir
        meter en lista de blocked
    */
    sem_wait(sem); //
    return pasar_a_ready_o_bloqueado_ready();
}

int post_semaforo(mate_instance *mate_inner_structure, mate_sem_name sem, unsigned int value){
   /* 
        
    */
    sem_post(sem); //
}




///////////////// PLANIFICACIÓN ////////////////////////


void new_a_ready(){ //hilo

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

//calcular ráfaga siguiente. Esto se debería hacer para todos los carpinchos cuando ingresan a la cola de ready
//float calculo_rafaga_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * alfa


// quién llamaría a la función
void ready_a_exec(){  
    sem_wait(&cola_ready_con_elementos); //espera que le avisen que hay uno en ready    
    // depende del algoritmo que tenga en el config (algoritmo_planificacion)
    if(algoritmo_planificacion === "SJF"){
        ready_a_exec_SJF();
    }
    else{
        ready_a_exec_HRRN();
    }
}


void ready_a_exec_SJF(){
    // cómo sé cuáles carpinchos están 

}

void ready_a_exec_HRRN(){
    
}

void exec(){
    // logica que mande a ejecutar teniendo en cuenta multiprocesamiento con hilos
    // carpicho_id_listo_para_exec()
}


