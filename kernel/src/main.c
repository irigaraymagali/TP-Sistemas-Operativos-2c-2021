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
    semaforos_carpinchos = list_create(); // crear lista para ir guardando los semaforos

    void* buffer = _recive_message(buffer, logger); // recibir mensajes de la lib
    deserializar(buffer);

    _start_server(puerto_escucha, handler, logger);

    // borrar todo, habria que ponerle que espere a la finalización de todos los hilos
    free_memory();

} 

///////////////////////////////////////////// INICIALIZACIONES ////////////////////////////////

void inicializar_colas(){ // Colas estados y sus mutex:
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
	
    pthread_mutex_init(&socket_memoria, NULL); //falta declarar socket_memoria
}
void inicializar_semaforos(){ // Inicializacion de semaforos:

    // (tener en cuenta: el segundo parámetro deberia ser 1 si es compartido entre carpinchos)

    sem_init(&sem_grado_multiprogramacion,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento, 0,grado_multiprocesamiento); 

    //sem_init(&hilo_CPU, 0,(1,1,1)); //long vector = grado multiprocesamiento 

	sem_init(&cola_new_con_elementos,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_exit_con_elementos,0,0); //hace falta tenerla? 
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 

    //hacer sem_destroy al final
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
    list_clean_and_destroy_elements(semaforos_carpinchos,/*void(*element_destroyer)(void*))*/);    
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
         (pthread_create(&hilo_cpu[i], NULL, (void*)ejecuta, NULL)
	}
}

void crear_semaforos_CPU(){
	lista_semaforos_CPU = list_create();
	for(int i= 0; i< grado_multiprocesamiento; i++){
         sem_init(&semaforo_CPU[i],0,1)
	}
}

int crear_socket_listener(){

    //leer archivo config para tener puerto_escucha

    return _create_socket_listenner(puerto_escucha, logger);

}


int deserializar(buffer){

    int str_len;
    char* string;
    int offset = 0;
    data_carpincho* estructura_interna = malloc(sizeof(data_carpincho));

    // int id
    memcpy(&(estructura_interna)->id, buffer, sizeof(int));
    offset += sizeof(int); 

    // char semaforo
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->semaforo = malloc(str_len + 1);
    memcpy(&estructura_interna)->semaforo, buffer + offset, str_len);

    // int valor_semaforo
    memcpy(&(estructura_interna)->valor_semaforo, buffer, sizeof(int));
    offset += sizeof(int);

    // char dispositivo_io
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    estructura_interna->dispositivo_io = malloc(str_len + 1);
    memcpy(&estructura_interna)->dispositivo_io, buffer + offset, str_len);
 
    // int size_memoria
    memcpy(&(estructura_interna)->size_memoria, buffer, sizeof(int));
    offset += sizeof(int); 

    // int addr_memfree
    memcpy(&(estructura_interna)->addr_memfree, buffer, sizeof(int));
    offset += sizeof(int); 

    // int origin_memread
    memcpy(&(estructura_interna)->origin_memread, buffer, sizeof(int));
    offset += sizeof(int); 

    // int dest_memread
    memcpy(&(estructura_interna)->dest_memread, buffer, sizeof(int));
    offset += sizeof(int); 

    // int origin_memwrite
    memcpy(&(estructura_interna)->origin_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 

    // int dest_memwrite
    memcpy(&(estructura_interna)->dest_memwrite, buffer, sizeof(int));
    offset += sizeof(int); 

    handler(buffer->opcode, estructura_interna);
    free(estructura_interna);
}


void handler( int opcode, data_carpincho estructura_interna){
    
    log_info(logger, "Recibí un mensaje");

    switch(opcode){
        case MATE_INIT:
            mate_init(estructura_interna->id);
        case MATE_CLOSE: 
            mate_close(estructura_interna->id);
        case MATE_SEM_INIT: 
            mate_sem_init(estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo);            
        case MATE_SEM_WAIT: 
            mate_sem_wait(estructura_interna->id, estructura_interna->semaforo);            
        case MATE_SEM_POST: 
            mate_sem_post(estructura_interna->id, estructura_interna->semaforo);            
        case MATE_SEM_DESTROY:
            mate_sem_destroy(estructura_interna->id, estructura_interna->semaforo);            
        case MATE_CALL_IO:
            mate_call_io(estructura_interna->id, estructura_interna->dispositivo_io);            
        case MATE_MEMALLOC: 
            mate_memalloc(estructura_interna->id, estructura_interna->size_memoria);            
        case MATE_MEMFREE:
            mate_memfree(estructura_interna->id, estructura_interna->addr_memfree);            
        case MATE_MEMREAD:
            mate_memread(estructura_interna->id, estructura_interna->origin_memread, estructura_interna->dest_memread, estructura_interna->size_memoria);            
        case MATE_MEMWRITE: 
            mate_memwrite(estructura_interna->id, estructura_interna->origin_memwrite, estructura_interna->dest_memwrite, estructura_interna->size_memoria);      
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

    // avisar que hay uno en ready e invocar función

}

int mate_close(int id_carpincho){

    list_remove_and_destroy_element(lista_carpinchos, id_carpincho, /*void(*element_destroyer)(void*)*/)
    
    // acá estamos eliminando lo que hay en ese index pero medio que dejamos ese index muerto

    // responder al carpincho que todo ok
}


//////////////// FUNCIONES SEMAFOROS ///////////////////

int mate_sem_init(int id_carpincdho, mate_sem_name nombre_semaforo, int valor_semaforo){  

    semaforo semaforo = malloc(size_of(semaforo));
    semaforo->nombre = nombre_semaforo;
    semaforo->valor = valor_semaforo;

    list_add(semaforos_carpinchos,semaforo);

    // responder al carpincho que todo ok 

}


bool esIgualASemaforo(mate_sem_name nombre_semaforo, void *semaforo){
    return semaforo->nombre === nombre_semaforo;
}

semaforo semaforoIgualANombreSemaforo(mate_sem_name nombre_semaforo, void *semaforo){
    return semaforo->nombre === nombre_semaforo;
}


int mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    semaforo semaforoIgualA(void *semaforo){
        return semaforoIgualANombreSemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        // si el sem esta en 0 le puedo hacer wait?

        (list_find(semaforos_carpinchos, esIgualA))->valor_semaforo --; // hacer las funciones que devuelvan el semaforo

        if(semaforo->valor_semaforo<1){
            
            // logica para que el carpincho se quede esperando el post si es que tiene que hacerlo
        
        }
        else
        {
           list_add(semaforo->en_espera, id_carpincho);
           // cual es la dif entre hacerlo como queue o como list?
        }
    }
    else
    {
        log_info(logger, "se intento hacer wait de un semaforo no inicializado");
    }

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
        //sem_wait(&cola_new_con_elementos); //si hay procesos en new  --> post cuando se inicializa?
        sem_wait(sem_grado_multiprogramacion); //grado multiprogramacion --> HACER POST CUANDO SALE DE EXEC!
		
       // saco de cola new y pongo en cola ready al primero (FIFO):
        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_new);

        queue_push(ready, *queue_peek(new));
        queue_pop(new);

		pthread_mutex_unlock(&sem_cola_new);
		pthread_mutex_unlock(&sem_cola_ready);

		sem_post(&cola_ready_con_elementos); //avisa: hay procesos en ready 
    }
    
}

// calcular ráfaga siguiente. Esto se debería hacer para todos los carpinchos cuando ingresan a la cola de ready
    //    float calculo_rafaga_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * alfa

void ready_a_exec(){  
    sem_wait(&cola_ready_con_elementos); //espera aviso que hay en ready    

    sem_wait(&sem_grado_multiprocesamiento); // --> post cuando sale de exec? 

    // Depende del algoritmo en el config (algoritmo_planificacion)
    if(algoritmo_planificacion == "SJF"){
        ready_a_exec_SJF();
    }
    else{
        ready_a_exec_HRRN();
    }

    // Sacar de la cola de ready al elegido (por el algoritmo) y ponerlo en la la lista de exec
        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_exec);

        list_add(lista_exec, *elegido);//elegido: proceso que va a ejecutar
        queue_pop(ready, *elegido); 
    
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_ready);


    // Asignar hilo CPU:
    void asignar_hilo_CPU(lista_semaforos_CPU){ //al primer semaforo CPU que valga 1 (que este disponible) le hace un wait (marcandolo como ocupado)
    
    t_sem hilo_CPU_disponible;
    
    /*
        funcion(semaforo){
            if (sem_gevalue(semaforo) === 1)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }

        funcion(semaforo){

            return sem_gevalue(semaforo) === 1; 
        
        }
    */

     hilo_CPU_disponible = list_find(*lista_semaforos_CPU, funcion);  //ver
        sem_wait(&hilo_CPU_disponible); // --> post cuando deja el hilo?
    } 

    
}




void ejecuta(){ 

// recibimos todo lo que pide el carpincho (semaforos y dispositivos io) hasta que termine o se bloquee --> seria avisarle al capincho (desbloquearlo) para que pueda seguir despues del init

while(1){

    // ...

    wait(LIBERAR_HILO_CPU);
}


}

///////////////// ALGORITMOS ////////////////////////

void ready_a_exec_SJF(){ // De la cola de ready te da el que debe ejecutar ahora según SJF

    
    
}

void ready_a_exec_HRRN(){ // De la cola de ready te da el que debe ejecutar ahora según HRRN

    
       
}

void exec_a_block(data_carpincho *carpincho){

    // le pasan el carpincho y aca lo saca de la lista de exec, lo pone en block y le hace signal al cpu

}

void exec(){
    // logica que mande a ejecutar teniendo en cuenta multiprocesamiento con hilos
    // carpicho_id_listo_para_exec()
}

// para SJF
float calculo_rafaga_siguiente(data_carpincho *carpincho){

    carpincho->estimacion_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * (1 - alfa) 

}

// para HRRN
float calculo_RR(data_carpincho *carpincho){

   //float w = ahora - carpincho->llegada_a_ready // ahora = momento en el que se esta caulculando el RR
   //float s =  prox rafaga

   //carpincho->RR = 1 + w/s 

}