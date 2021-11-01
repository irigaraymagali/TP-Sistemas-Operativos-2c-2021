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


int main(int argc, char ** argv){

    t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

    crear_hilos_CPU();
    crear_hilos_planificadores();
	inicializar_semaforos();
	inicializar_colas();
    leer_archivo_config();

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos

    _start_server(puerto_escucha, handler, logger);

    // borrar todo, habria que ponerle que espere a la finalización de todos los hilos
    free_memory();

} 

///////////////////////////////////////////// INICIALIZACIONES ////////////////////////////////
// Colas estados y sus mutex:
void inicializar_colas(){
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

// Inicializacion de semaforos:
void inicializar_semaforos(){
    sem_init(&sem_grado_multiprogramacion,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento, 0,grado_multiprocesamiento); 

	sem_init(&cola_new_con_elementos,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_exit_con_elementos,0,0); //hace falta tenerla? 
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 
}

void leer_archivo_config(){

    config = config_create("../cfg/kernel.conf");

	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_int_value(config, "PUERTO_MEMORIA");
	puerto_escucha = config_get_int_value(config, "PUERTO_ESCUCHA");    
    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    estimacion_inicial = config_get_int_value(config, "ESTIMACION_INICIAL");
    alfa = config_get_int_value(config, "ALFA");
    dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
    duraciones_io = config_get_array_value(config, "DURACIONES_IO"); 
    grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    grado_multiprocesamiento = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");
    tiempo_deadlock = config_get_int_value(config, "TIEMPO_DEADLOCK");

}

void free_memory(){

    config_destroy(config);
    list_clean_and_destroy_elements(lista_carpinchos,/*void(*element_destroyer)(void*))*/);
    log_destroy(logger);

    // pthread_mutex_destroy

}
 
///////////////// CREACION HILOS //////////////////////// 

void crear_hilos_planificadores(){

    pthread_t planficador_largo_plazo;
    pthread_create(&planficador_largo_plazo, NULL, (void*) new_a_ready, NULL);

    pthread_t planficador_corto_plazo;
    pthread_create(&planficador_corto_plazo, NULL, (void*) ready_a_exec, NULL);

    pthread_t planficador_mediano_plazo;
    pthread_create(&planficador_mediano_plazo, NULL, (void*) x, NULL); //FALTA función "x"
}

void crear_hilos_CPU(){ // creación de los hilos CPU

    pthread_t hilo_cpu[grado_multiprocesamiento];
	lista_ejecutando = list_create(); 			// lista que tiene a los que estan ejecutando
	for(int i= 0; i< grado_multiprocesamiento; i++){
         (pthread_create(&hilo_cpu[i], NULL, (void*)hiloCPU, NULL)
	}
}

void hiloCPU(){ // lo que necesitemos que el CPU esté haciendo

}


int crear_socket_listener(){

    //leer archivo config para tener puerto_ escucha

    int puerto_escucha;

    return _create_socket_listenner(puerto_escucha, logger);

}

void handler(int fd, char* id, int opcode, void* payload, t_log* logger){
    log_info(logger, "Recibí un mensaje");
   
   mate_inner_structure mensaje;

    switch(opcode){
        case MATE_INIT:
            mensaje = /*deserializar*/
            mate_init(mensaje->id);
        case MATE_CLOSE: 
            mensaje = /*deserializar*/
            mate_close(mensaje->id);
        case MATE_SEM_INIT: 
            mensaje = /*deserializar*/
            mate_sem_init(mensaje->id, mensaje->semaforo, mensaje->valor_semaforo);            
        case MATE_SEM_WAIT: 
            mensaje = /*deserializar*/
            mate_sem_wait(mensaje->id, mensaje->semaforo);            
        case MATE_SEM_POST: 
            mensaje = /*deserializar*/
            mate_sem_post(mensaje->id, mensaje->semaforo);            
        case MATE_SEM_DESTROY:
            mensaje = /*deserializar*/
            mate_sem_destroy(mensaje->id, mensaje->semaforo);            
        case MATE_CALL_IO:
            mensaje = /*deserializar*/
            mate_call_io(mensaje->id, mensaje->dispositivo_io);            
        case MATE_MEMALLOC: 
            mensaje = /*deserializar*/
            mate_memalloc(mensaje->id, mensaje->size_memoria);            
        case MATE_MEMFREE:
            mensaje = /*deserializar*/
            mate_memfree(mensaje->id, mensaje->addr_memfree);            
        case MATE_MEMREAD:
            mensaje = /*deserializar*/
            mate_memread(mensaje->id, mensaje->origin_memread, mensaje->dest_memread, mensaje->size_memoria);            
        case MATE_MEMWRITE: 
            mensaje = /*deserializar*/
            mate_memwrite(mensaje->id, mensaje->origin_memwrite, mensaje->dest_memwrite, mensaje->size_memoria);      
        break;  

    }
}



//////////////// FUNCIONES GENERALES ///////////////////

int mate_init(int id_carpincho){
    
    data_carpincho carpincho = malloc(size_of(data_carpincho);
    carpincho->id = id_carpincho;
    carpincho->rafaga_anterior = 0;
    carpincho->estimacion_anterior = 0;
    carpincho->estimacion_siguiente = /*calculo para estimación que va a ser la misma para todos*/
    // carpincho->llegada_a_ready no le pongo valor porque todavia no llegó
    carpincho->prioridad = false;
    carpincho->estado = 'N';

    list_add_in_index(lista_carpinchos, id_carpincho, carpincho);

    /*
    - reservar espacio en memoria 
    - avisar que llegó un nuevo carpincho a new => post a new_con_elementos
    - una vez que está en EXEC retornar 0
    */
}

int mate_close(int id_carpincho){
    
    int id_carpincho_a_eliminar = mate_inner_structure->id;

    list_remove_and_destroy_element(lista_carpinchos, id_carpincho_a_eliminar, /*void(*element_destroyer)(void*)*/)
    
    // acá estamos eliminando lo que hay en ese index pero medio que dejamos ese index muerto
}


//////////////// FUNCIONES SEMAFOROS ///////////////////

int mate_sem_init(int id_carpincho, mate_sem_name nombre_semaforo, int valor_semaforo){  
}

int mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo){
}

int mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo){
}

int mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo) {
}


//////////////// FUNCIONES IO ///////////////////

int mate_call_io(int id_carpincho, mate_io_resource nombre_io){
}


//////////////// FUNCIONES MEMORIA ///////////////////

mate_pointer mate_memalloc(int id_carpincho, int size){
}

int mate_memfree(int id_carpincho, mate_pointer addr){
}

int mate_memread(int id_carpincho, mate_pointer origin, void *dest, int size){
}

int mate_memwrite(int id_carpincho, void origin, mate_pointer dest, int size){
}
    


///////////////// PLANIFICACIÓN ////////////////////////

void new_a_ready(){

    while(1){
        //sem_wait(&cola_new_con_elementos); //si hay procesos en new  --> quien hace el post?, cuando se inicializa?
        sem_wait(sem_grado_multiprogramacion); //grado multiprogramacion --> HACER POST CUANDO SALE DE EXEC!
		
       // saco de cola new y pongo en cola ready al primero (FIFO):
        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_new);

        queue_push(ready, *queue_peek(new);
        queue_pop(new);

		pthread_mutex_unlock(&sem_cola_new);
		pthread_mutex_unlock(&sem_cola_ready);

		sem_post(&cola_ready_con_elementos); //avisa: hay procesos en ready 
    }
    
}

// calcular ráfaga siguiente. Esto se debería hacer para todos los carpinchos cuando ingresan a la cola de ready
//      float calculo_rafaga_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * alfa

void ready_a_exec(){  
    sem_wait(&cola_ready_con_elementos); //espera aviso que hay en ready    

    // depende del algoritmo en el config (algoritmo_planificacion)
    if(algoritmo_planificacion == "SJF"){
        ready_a_exec_SJF();
    }
    else{
        ready_a_exec_HRRN();
    }
}

///////////////// ALGORITMOS ////////////////////////

void ready_a_exec_SJF(){

    //chequear grado multiprocesamiento
    sem_wait(&sem_grado_multiprocesamiento); // --> falta el post

    // fun que de la cola de ready te da el que debe ejecutar ahora ("ejecutar el algoritmo")

    // sacar de la cola de ready al elegido y ponerlo en la la lista de exec

    // asignarle un hilo cpu

    //  o sino que todo eso se haga directamente en ready_a_exec y que ready_a_exec sea directamnete el algoritmo que devuelve el proeso que debe ejecutar?
    
        
}

void ready_a_exec_HRRN(){

       
}

void exec(){
    // logica que mande a ejecutar teniendo en cuenta multiprocesamiento con hilos
    // carpicho_id_listo_para_exec()
}


