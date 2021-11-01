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
	//inicializar_configuracion();

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos

    // destruir la lista, esto por si se apaga todo, habria que ponerle que espere a la finalización de todos los hilos
    list_clean_and_destroy_elements(lista_carpinchos,/*void(*element_destroyer)(void*))*/);
    log_destroy(logger);
    free_configuraciones();

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

    ip_memoria = malloc(sizeof(char)); // esta bien ponerle el size asi?
    puerto_memoria = malloc(sizeof(int));
    puerto_escucha = malloc(sizeof(int));
    algoritmo_planificacion = malloc(sizeof(char)); // idem
    estimacion_inicial = malloc(sizeof(int));
    alfa = malloc(sizeof(int));
    dispositivos_io = malloc(sizeof(char)); // es una lista de strings, como le doy el size?
    duraciones_io = malloc(sizeof(int));
    grado_multiprogramacion = malloc(sizeof(int));
    grado_multiprocesamiento = malloc(sizeof(int));
    tiempo_deadlock = malloc(sizeof(int));

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

void free_configuraciones(){
    free(ip_memoria);
	free(puerto_memoria);
	free(puerto_escucha);
    free(algoritmo_planificacion);
    free(estimacion_inicial);
    free(alfa);
    free(dispositivos_io);
    free(duraciones_io);
    free(grado_multiprogramacion);
    free(grado_multiprocesamiento);
    free(tiempo_deadlock);
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
    
    data_carpincho carpincho = malloc(size_of(data_carpincho);
    carpincho->id = mate_inner_structure->id;
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

int mate_close(mate_instance *mate_inner_structure){
    
    int id_carpincho_a_eliminar = mate_inner_structure->id;

    list_remove_and_destroy_element(lista_carpinchos, id_carpincho_a_eliminar, /*void(*element_destroyer)(void*)*/)
    
    //acá estamos eliminando lo que hay en ese index pero medio que dejamos ese index muerto
}

int mate_sem_init(mate_instance *mate_inner_structure){  
}

int mate_sem_wait(mate_instance *mate_inner_structure){
}

int mate_sem_post(mate_instance *mate_inner_structure){
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

//calcular ráfaga siguiente. Esto se debería hacer para todos los carpinchos cuando ingresan a la cola de ready
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

    //fun que de la cola de ready te da el que debe ejecutar ahora ("ejecutar el algoritmo")

    //sacar de la cola de ready al elegido y ponerlo en la la lista de exec

    //asignarle un hilo cpu

    //  o sino que todo eso se haga directamente en ready_a_exec y que ready_a_exec sea directamnete el algoritmo que devuelve el proeso que debe ejecutar?
    
        
}

void ready_a_exec_HRRN(){

       
}

void exec(){
    // logica que mande a ejecutar teniendo en cuenta multiprocesamiento con hilos
    // carpicho_id_listo_para_exec()
}


