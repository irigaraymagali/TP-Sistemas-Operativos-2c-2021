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

// tendría que conocer la estructura interna acá también
typedef struct mate_inner_structure
{
    //void *memory;
    float *rafaga_anterior; // para despues poder calcular la estimación siguiente
    float *estimacion_anterior; // idem
    float *estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float *llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    int *prioridad; // 1 si tiene prioridad para pasar a ready -> es para los que vienen de suspended_ready a ready
    char *estado; // no sé cuánto nos va a servir, si no se puede hacer que sea estado_anterior y que nos evite tener otro para prioridad
 
  // datos para poder saber qué está pidiendo el carpincho cuando se conecte con backend
    char *semaforo; 
    int *valor_semaforo; 
    char *dispositivo_io; 
    char *mnesaje_io;
    int *size_memoria;
    int *addr_memfree;
    int *origin_memread;
    int **dest_memread;
    int **origin_memwrite;
    int *dest_memwrite;
    int *respuesta_a_carpincho;

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

    t_config* config;

// que onda esto y la memoria que usa? quien se encarga de darsela y de borrarla?
t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

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

int main(int argc, char ** argv){


} 


int crear_socket_listener(){

    //leer archivo config para tener puerto_ escucha

    int puerto_escucha;

    return _create_socket_listenner(puerto_escucha, logger);

}


int recibir_mensaje(){

    int str_len;
    char* string;
    int offset = 0;
    mate_inner_structure* estructura_interna = malloc(sizeof(mate_inner_structure));

    void* buffer = _recive_message(buffer, logger);
    memcpy(&(estructura_interna)->rafaga_anterior, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->estimacion_anterior, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->estimacion_siguiente, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->llegada_a_ready, buffer, sizeof(float));
    offset += sizeof(float); 
    memcpy(&estructura_interna)->prioridad, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->estado = malloc(str_len + 1);
    memcpy(&estructura_interna)->estado, buffer + offset, str_len);
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->semaforo = malloc(str_len + 1);
    memcpy(&estructura_interna)->semaforo, buffer + offset, str_len);
    memcpy(&estructura_interna)->valor_semaforo, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->dispositivo_io = malloc(str_len + 1);
    memcpy(&estructura_interna)->dispositivo_io, buffer + offset, str_len);
    memcpy(&estructura_interna)->size_memoria, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->addr_memfree, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->origin_memread, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->dest_memread, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->origin_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->dest_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 
    memcpy(&estructura_interna)->respuesta_a_carpincho, buffer, sizeof(int));
    offset += sizeof(int);


    ejecutar_funcion_switch(buffer->codigo_operacion);

}

ejecutar_funcion_switch(void * buffer){
    switch(codigo){
        case MATE_INIT:
            mate_init(mensaje);
        case MATE_CLOSE: 
            mate_close(mensaje);
        case MATE_SEM_INIT: 
            mate_sem_init(mensaje);            
        case MATE_SEM_WAIT: 
            mate_sem_wait(mensaje);            
        case MATE_SEM_POST: 
            mate_sem_post(mensaje);            
        case MATE_SEM_DESTROY:
            mate_sem_destroy(mensaje);            
        case MATE_CALL_IO:
            mate_call_io(mensaje);            
        case MATE_MEMALLOC: 
            mate_memalloc(mensaje);            
        case MATE_MEMFREE:
            mate_memfree(mensaje);            
        case MATE_MEMREAD:
            mate_memread(mensaje);            
        case MATE_MEMWRITE: 
            mate_memwrite(mensaje);      
        break;      
    }
}



//////////////// FUNCIONES GENERALES ///////////////////

int mate_init(mate_instance *mate_inner_structure){
    /*
    - completar con id
    - completar la estructura con por ej los valores del config de la rafaga y estimacion
    - reservar espacio en memoria 
    - avisar que llegó un nuevo carpincho a new => post a new_con_elementos
    - una vez que está en EXEC retornar 0
    */
}

int mate_close(mate_instance *mate_inner_structure){
    /* conectar con memoria para borrar todo*/
}

int mate_sem_init(mate_instance *mate_inner_structure){
    mate_instance->sem_instance = malloc(sizeof(sem_t));

        /* 
           guardar el semaforo en la estructura del carpincho
        */
        
    return sem_init(mate_instance->sem_instance, 0, value); // qué pasa si quiero inicializar más de un hilo?

}

int mate_sem_wait(mate_instance *mate_inner_structure){
   /* 
        cuando le hagan post le retorne 0 por la conexión así ahí puede seguir
        meter en lista de blocked
    */
    sem_wait(sem); //
    return pasar_a_ready_o_bloqueado_ready();
}

int mate_sem_post(mate_instance *mate_inner_structure){
   /* 
        
    */
    sem_post(sem); //
}

int mate_sem_destroy(mate_instance *mate_inner_structure) {

}

int mate_call_io(mate_instance *mate_inner_structure){

}

mate_pointer mate_memalloc(mate_instance *mate_inner_structure){

}

int mate_memfree(mate_instance *mate_inner_structure){

}

int mate_memread(mate_instance *mate_inner_structure){

}

int mate_memwrite(mate_instance *mate_inner_structure){

}

///////////////// CREACION HILOS //////////////////////// => habría que ponerlos dentro de alguna función

pthread_t planficador_largo_plazo;
pthread_create(&planficador_largo_plazo, NULL, (void*) new_a_ready, NULL);

pthread_t planficador_corto_plazo;
pthread_create(&planficador_corto_plazo, NULL, (void*) ready_a_exec, NULL);

pthread_t planficador_mediano_plazo;
pthread_create(&planficador_mediano_plazo, NULL, (void*) x, NULL); //FALTA función


///////////////// PLANIFICACIÓN ////////////////////////

void new_a_ready(){

    while(1){
        //sem_wait(&cola_new_con_elementos); //si hay procesos en new  --> quien hace el post?, cuando se inicializa?
        sem_wait(sem_grado_multiprogramacion); //grado multiprogramacion --> HACER POST CUANDO SALE DE EXEC!
		
       // saco de cola new y pongo en cola ready al primero (FIFO):
        pthread_mutex_lock(&sem_cola_ready; 
		pthread_mutex_lock(&sem_cola_new);

        queue_push(ready, *queue_peek(new);
        queue_pop(new);

		pthread_mutex_unlock(&sem_cola_new);
		pthread_mutex_unlock(&sem_cola_ready);

		sem_post(&cola_ready_con_elementos); //avisa: hay procesos en ready 
    }
    
}

//calcular ráfaga siguiente. Esto se debería hacer para todos los carpinchos cuando ingresan a la cola de ready
//      float calculo_rafaga_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * alfa

void ready_a_exec(){  
    sem_wait(&cola_ready_con_elementos); //espera aviso que hay en ready    

    // depende del algoritmo en el config (algoritmo_planificacion)
    if(algoritmo_planificacion === "SJF"){
        ready_a_exec_SJF();
    }
    else{
        ready_a_exec_HRRN();
    }
}

///////////////// ALGORITMOS ////////////////////////

void ready_a_exec_SJF(){

        
}

void ready_a_exec_HRRN(){
    

       
}

void exec(){
    // logica que mande a ejecutar teniendo en cuenta multiprocesamiento con hilos
    // carpicho_id_listo_para_exec()
}


