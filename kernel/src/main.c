#include "main.h"



int main(int argc, char ** argv){

    int* socket;
    socket = malloc(sizeof(int));
    // leer archivo config

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

    //

    int id_carpincho = 1;

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos
    semaforos_carpinchos = list_create(); // crear lista para ir guardando los semaforos
    hilos_CPU = list_create(); // crear lista para ir guardando los hilos cpus

    t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

    leer_archivo_config();
	inicializar_semaforos();
	inicializar_colas();
    crear_hilos_CPU();
    crear_hilos_planificadores();
    
    _start_server(puerto_escucha, handler, logger);
    _connect(ip_memoria, puerto_memoria, logger);

    // borrar todo, habria que ponerle que espere a la finalización de todos los hilos
    free_memory();

} 


///////////////////////////////////////////// INICIALIZACIONES ////////////////////////////////

void inicializar_colas(){ // Colas estados y sus mutex:
    
    new = queue_create();
	pthread_mutex_init(&sem_cola_new, NULL);

	ready = list_create();
	pthread_mutex_init(&sem_cola_ready, NULL);

	exec = list_create();
	pthread_mutex_init(&sem_cola_exec, NULL);

    exit_list = list_create();
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

    sem_init(&sem_grado_multiprogramacion_libre,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento_libre, 0,grado_multiprocesamiento); 

    sem_init(&hay_estructura_creada,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 

    // hacer sem_destroy al final
}

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
	for(int i= 0; i< grado_multiprocesamiento; i++){

        sem_init(&liberar_CPU[i], 0, 0);
        sem_init(&CPU_libre[i], 0, 1; // ver si es 0 o 1 en el segundo argumento
        sem_init(&usar_CPU[i], 0, 0);        
        
        (pthread_create(&hilo_cpu[i], NULL, (void*)ejecuta, i); 
        
        hilo_cpu *nuevo_cpu;
        nuevo_cpu = malloc(sizeof(hilo_cpu)); //es necsario? esta bien?
        nuevo_cpu->id = i;
        nuevo_cpu->semaforo = &CPU_libre[i]; //esto funciona asi?
        
        list_add(hilos_CPU, nuevo_cpu);

	}
}

void free_memory(){

    config_destroy(config);
    list_clean_and_destroy_elements(lista_carpinchos,/*void(*element_destroyer)(void*))*/);
    list_clean_and_destroy_elements(semaforos_carpinchos,/*void(*element_destroyer)(void*))*/);   
    list_clean_and_destroy_elements(hilos_CPU,/*void(*element_destroyer)(void*))*/);    
     
    log_destroy(logger);

    // pthread_mutex_destroy

    sem_destroy(&sem_grado_multiprogramacion_libre);  
	sem_destroy(&sem_grado_multiprocesamiento_libre); 

    sem_destroy(&cola_ready_con_elementos);
    sem_destroy(&cola_exec_con_elementos);
    sem_destroy(&cola_blocked_con_elementos);
    sem_destroy(&cola_suspended_blocked_con_elementos);
    sem_destroy(&cola_suspended_ready_con_elementos); 

	for(int i= 0; i< grado_multiprocesamiento; i++){
        pthread_destroy(&hilo_cpu[i]);
        sem_destroy(&liberar_CPU[i]);
        sem_destroy(&CPU_libre[i]);
        sem_destroy(&usar_CPU[i]);   
	}

    free(socket);

}


///////////////// RECIBIR MENSAJES //////////////////////// 

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

}


void handler( int fd, char* id, int opcode, void* payload, t_log* logger){
    
    log_info(logger, "Recibí un mensaje");
    data_carpincho* estructura_interna;
    estructura_interna = malloc(sizeof(estructura_interna));
    int id_carpincho;
    int size_memoria;
    int addr_memfree;
    int origin_memread;
    void *dest_memread;
    void *origin_memwrite;
    int dest_memwrite;
    int offset = 0;
    int ptr_len = 0;
    

    // fijarme cómo acá tengo que mandarle tambien a las funciones el socket para que despues puedan responderle

    switch(opcode){
        case MATE_INIT:
            estructura_interna = deserializar(payload);
            mate_init(estructura_interna->id, fd);
            break;
        case MATE_CLOSE: 
            estructura_interna = deserializar(payload);
            mate_close(estructura_interna->id,fd);
            break;
        case MATE_SEM_INIT: 
            estructura_interna = deserializar(payload);
            mate_sem_init(estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo, fd);            
            break;
        case MATE_SEM_WAIT: 
            estructura_interna = deserializar(payload);
            mate_sem_wait(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
        case MATE_SEM_POST: 
            estructura_interna = deserializar(payload);
            mate_sem_post(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
        case MATE_SEM_DESTROY:
            estructura_interna = deserializar(payload);
            mate_sem_destroy(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
        case MATE_CALL_IO:
            estructura_interna = deserializar(payload);
            mate_call_io(estructura_interna->id, estructura_interna->dispositivo_io, fd);  
            break;
        // ver qué tengo que pasar acá          
        case MATE_MEMALLOC: 
            // id_carpincho
            memcpy(&id_carpincho, payload, sizeof(int));
            offset += sizeof(int);
            // size_memoria
            memcpy(&size_memoria, payload, sizeof(int);
            offset += sizeof(int);

            mate_memalloc(id_carpincho, size_memoria, fd);      
            break;      
        case MATE_MEMFREE:
            // id_carpincho
            memcpy(&id_carpincho, payload, sizeof(int));
            offset += sizeof(int);
            // addr_memfree
            memcpy(&addr_memfree, payload, sizeof(int);
            offset += sizeof(int);

            mate_memfree(id_carpincho, addr_memfree, fd);      
            break;      
        case MATE_MEMREAD:
            // id_carpincho
            memcpy(&id_carpincho, payload, sizeof(int));
            offset += sizeof(int);
            // origin_memread
            memcpy(&origin_memread, payload, sizeof(int);
            offset += sizeof(int);
            // dest_memread
            memcpy(&ptr_len, payload + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&dest_memread, payload + offset, sizeof(int)* ptr_len);
            // size_memoria
            memcpy(&size_memoria, payload, sizeof(int);
            offset += sizeof(int);

            mate_memread(id_carpincho, origin_memread, dest_memread, size_memoria), fd;            
            break;
        case MATE_MEMWRITE: 
            // id_carpincho
            memcpy(&id_carpincho, payload, sizeof(int));
            offset += sizeof(int);
            // origin_memwrite
            memcpy(&ptr_len, payload + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&origin_memwrite, payload + offset, sizeof(int)*ptr_len);
            // dest_memwrite
            memcpy(&dest_memwrite, payload, sizeof(int);
            offset += sizeof(int);
            // size_memoria
            memcpy(&size_memoria, payload, sizeof(int);
            offset += sizeof(int);

            mate_memwrite(id_carpincho, origin_memwrite, dest_memwrite, size_memoria, fd);     
            break; 
        break;  

    }
}


//////////////// FUNCIONES GENERALES ///////////////////

int mate_init(int id_carpincho, int fd){
    
    data_carpincho carpincho = malloc(size_of(data_carpincho);
    carpincho->id = id_carpincho;
    carpincho->rafaga_anterior = 0;
    carpincho->estimacion_anterior = 0;
    carpincho->estimacion_siguiente = estimacion_inicial; // viene por config
    // carpincho->llegada_a_ready no le pongo valor porque todavia no llegó
    // carpincho->RR no le pongo nada todavia
    carpincho->prioridad = false;
    carpincho->estado = NEW;

    list_add(lista_carpinchos, carpincho);
    sem_post(&hay_estructura_creada);

    id_carpincho += 2; // incrementa carpinchos impares

    pthread_t esperando_estar_en_exec;   //sacar el hilo o lo dejamos así?
    pthread_create(&esperando_estar_en_exec, NULL, (void*) esperando_estar_en_exec, (carpincho,fd)); // estan bien pasados los dos argumentos?
    
}

void esperando_estar_en_exec(carpincho,fd){
while(true){
    // ver si es necesario que haya un wait
    if(carpincho->estado === EXEC){
        _send_message(fd, ID_MATE_LIB, 1, carpincho->id, sizeof(int), logger); // va así?
        return 0;
    }
}
}


int mate_close(int id_carpincho, int fd){

    list_remove_by_condition(lista_carpinchos, encontrar_estructura_segun_id);

    exec_a_exit(id_carpincho);

    _send_message(fd, ID_MATE_LIB, 1, 0, sizeof(int), logger); 
}


//////////////// FUNCIONES SEMAFOROS ///////////////////

int mate_sem_init(int id_carpincho, mate_sem_name nombre_semaforo, int valor_semaforo, int fd){  
    
    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  

        bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
        }

        log_info(logger, "El semaforo ya estaba incializado");
        _send_message(fd, ID_MATE_LIB, 1, -1, sizeof(int), logger); // le manda que esta mal, va así? 

    }
    else 
    {
        semaforo semaforo = malloc(size_of(semaforo));
        semaforo->nombre = nombre_semaforo;
        semaforo->valor = valor_semaforo;

        list_add(semaforos_carpinchos,semaforo);

        log_info(logger, "Se inicializó el semáforo");        
        _send_message(fd, ID_MATE_LIB, 1, 0, sizeof(int), logger); // va así? 
    
    }
}

bool esIgualASemaforo(mate_sem_name nombre_semaforo, void *semaforo){
    return semaforo->nombre === nombre_semaforo;
}

semaforo semaforoIgualANombreSemaforo(mate_sem_name nombre_semaforo, void *semaforo){
    return semaforo->nombre === nombre_semaforo;
}


int mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    semaforo semaforoIgualA(void *semaforo){
        return semaforoIgualANombreSemaforo(nombre_semaforo, semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        semaforo semaforo = list_find(semaforos_carpinchos, semaforoIgualA);
        semaforo->valor_semaforo --; 

        if(semaforo->valor_semaforo<1){
            exec_a_block(id_carpincho); 
            queue_push(semaforo->en_espera, encontrar_estructura_segun_id(id_carpincho); //queda esperando para que lo desbloqueen, es el primero. conviene usar una que
        }
        else
        {
            // retornar mensaje a la lib
        }
    }
    else
    {
        log_info(logger, "se intento hacer wait de un semaforo no inicializado");
    }

}



int mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    semaforo semaforoIgualA(void *semaforo){
        return semaforoIgualANombreSemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        (list_find(semaforos_carpinchos, semaforoIgualA))->valor_semaforo ++; 

        carpincho_a_desbloquear = queue_pop((list_find(semaforos_carpinchos, semaforoIgualA))->en_espera);

        if(carpincho_a_desbloquear->estado === BLOCKED){
            carpincho_a_desbloquear->estado = READY;
            block_a_ready(carpincho_a_desbloquear);
        }
        else{ // si no esta en blocked es porque estaba en suspended blocked, ahora lo cambio a suspended_ready
            carpincho_a_desbloquear->estado = SUSPENDED_READY;
            suspended_a_ready(carpincho_a_desbloquear);
        }
    }
    else
    {
        log_info(logger, "se intento hacer post de un semaforo no inicializado");
    }


}

int mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo, int fd) {

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    semaforo semaforoIgualA(void *semaforo){
        return semaforoIgualANombreSemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        // if => qué pasa si tienen algun carpincho en wait? se puede eliminar el semaforo? que pasa con los que estan en espera?
            list_remove_by_condition(semaforos_carpinchos,esIgualA);
    }
    else
    {
        log_info(logger, "se intento borrar un semaforo no inicializado");
    }




}


//////////////// FUNCIONES IO ///////////////////


    //para el find:
        bool igual_a(void *dispoitivo){
            return es_igual_dispositivo(dispositivo, nombre_dispositivo);
        }

        bool es_igual_dispositivo(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }

    //para el any satisfy:
        dispositivo_io dispositivo_igual_a(void *dispositivo){
                return dispositivo_igual_a_nombre(nombre_dispositivo, dispositivo);
            }

        dispositivo_io dispositivo_igual_a_nombre(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }

        bool esIgualDispositivo(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }


int mate_call_io(int id_carpincho, mate_io_resource nombre_io, int fd){

    // si nombre_io esta disponible => bloquear al carpincho por io
    // si no => bloquearlo? a la espera de que se desocupe

    // lista dispositivos_io y duraciones_io por config

    if(list_any_satisfy(dispositivos_io, igual_a)){  

        dispositivo_pedido = list_find(dispositivos_io, dispositivo_igual_a); 

        if(dispositivo_pedido->en_uso === false){
            exec_a_block_io(id,dispositivo); //ver
            dispositivo_pedido->en_uso = true;
        }
        else{ // si esta en uso => ?
            
        }
    }
    else
    {
        log_info(logger, "Se pidio un dispositivo IO que no existe");
    }




}


//////////////// FUNCIONES MEMORIA ///////////////////

mate_pointer mate_memalloc(int id_carpincho, int size, int fd){
}

int mate_memfree(int id_carpincho, mate_pointer addr, int fd){
}

int mate_memread(int id_carpincho, mate_pointer origin, void *dest, int size, int fd){
}

int mate_memwrite(int id_carpincho, void origin, mate_pointer dest, int size, int fd){
}
    


///////////////////// PLANIFICACIÓN ////////////////////////


void new_a_ready(){

    data_carpincho carpincho_a_mover;
    int respuesta_memoria;
    t_mensaje buffer;

    while(1){
        sem_wait(&hay_estructura_creada);
        sem_wait(&sem_grado_multiprogramacion_libre); //grado multiprogramacion --> HACER POST CUANDO SALE DE EXEC!
		
        // tendria que chequear que se mande bien la conexión?
        _send_message(socket, ID_KERNEL, MATE_INIT, _serialize(sizeof(int), "%d", estructura_interna->id), sizeof(int), logger); // envia la estructura al backend para que inicialice todo

        buffer = _receive_message(socket, logger);
        memcpy(&respuesta_memoria, buffer, sizeof(int));
    
        if(respuesta_memoria === estructura_interna->id){  // si la memoria crea la estructura, le devuelve el id
            
            log_info(logger, "La estructura del carpincho se creó correctamente en memoria");

            // saco de cola new y pongo en cola ready al primero (FIFO):
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_new);

            carpincho_a_mover = queue_peek(new);
            
            carpincho_a_mover->estado = READY;

            //queue_push(ready, *queue_peek(new)); seria poner ese el la lista de ready
            queue_pop(new);

            pthread_mutex_unlock(&sem_cola_new);
            pthread_mutex_unlock(&sem_cola_ready);

            sem_post(&cola_ready_con_elementos); //avisa: hay procesos en ready 
        }
        else{
            log_info(logger, "El módulo memoria no pudo crear la estructura")
        }
    }
}

void suspended_a_ready(){
    sem_wait(&sem_grado_multiprogramacion_libre);
    //avisarle a mem que recupere las pags
}


void ready_a_exec(){  

    data_carpincho carpincho_a_mover;

    while(1){ 

        sem_wait(&cola_ready_con_elementos);   
        sem_wait(&sem_grado_multiprocesamiento_libre);

    // Depende del algoritmo en el config:
        if(algoritmo_planificacion == "SJF"){
            carpincho_a_mover = ready_a_exec_SJF();
        }
        else{
            carpincho_a_mover = ready_a_exec_HRRN();
        }

    // Sacar de la cola de ready al elegido (por el algoritmo) y ponerlo en la la lista de exec
        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_exec);

        carpincho_a_mover->estado = EXEC;

        list_add(lista_exec, *carpincho_a_mover);
        //queue_pop(ready, *carpincho_a_mover);  sacar ese de la lista de ready
    
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_ready);

        asignarle_hilo_CPU(carpincho_a_mover);

        carpincho_a_mover->tiempo_entrada_a_exec = temporal_get_string_time("%H:%M:%S:%MS");
    }
}


void exec_a_block(int id_carpincho){
    // le pasan el id del carpincho y lo saca de la lista de exec, lo pone en block y le hace signal al cpu
    
    data_carpincho carpincho_a_bloquear = encontrar_estructura_segun_id(id_carpincho);
    calculo_rafaga_anterior(carpincho_a_bloquear); 

    // Sacar de la lista de exec y ponerlo en la la cola de blocked
        pthread_mutex_lock(&sem_cola_exec); 
		pthread_mutex_lock(&sem_cola_blocked);

        carpincho_a_bloquear->estado = BLOCKED;

        queue_push(blocked, *carpincho_a_bloquear); 
        //list_(lista_exec, *carpincho_a_bloquear);
    
		pthread_mutex_unlock(&sem_cola_blocked);
		pthread_mutex_unlock(&sem_cola_exec);

        // "libera" el hilo cpu en el que estaba:
        sem_post(liberarCPU[carpincho_a_bloquear->hilo_CPU_usado->id]);

}


void exec_a_block_io(int id_carpincho, char dispositivo){
    
    exec_a_block(id_carpincho);

    // ver lo de las duraciones


}


void exec_a_exit(int id_carpincho){
    
    carpincho_que_termino = encontrar_estructura_segun_id(id_carpincho);

    // Sacar de la lista de exec --> hace falta ponerlo en la lista de exit?
        pthread_mutex_lock(&sem_cola_exec); 
		
        //carpincho_que_termino->estado = EXIT;

        //list_(lista_exec, *carpincho_que_termino); 
    
		pthread_mutex_unlock(&sem_cola_exec);

        // "libera" el hilo cpu en el que estaba:
        sem_post(liberarCPU[carpincho_a_bloquear->hilo_CPU_usado->id]);

        //avisar a mem para que libere?
}

//FALTA
void block_a_ready(data_carpincho *carpincho){

    // cuando se desbloquea un semaforo o termina IO pase a ready
    //

}

//FALTA
void suspended_a_ready(){

    // logica para que cuando se desbloquee un semaforo o se termina IO de un carpincho que se habia suspendido y ahora esta en suspendido ready, vea si lo quiere pasar a ready segun grado de multiprogramacion y eso
    //carpincho->prioridad = true;
}




////////////////////////// ALGORITMOS ////////////////////////

data_carpincho ready_a_exec_SJF(){

        for(int i= 0; i<list_size(ready); i++){

            float min_hasta_el_momento = 0;
            data_carpincho carpincho_sig;
            data_carpincho carpincho_listo = list_get(ready, i); // agarro un carpincho
            calculo_estimacion_siguiente(carpincho_listo);       // le calculo su estimacion
            float estimacion_actual = carpincho_listo->estimacion_siguiente; //agarro su estimacion
                if(estimacion_actual < min_hasta_el_momento){    // si esta es menor => pasa a ser la minima hasta el momento
                    carpincho_sig = carpincho_listo;
                    min_hasta_el_momento = estimacion_actual;
                }
        }
        
    return carpincho_sig;        
}

data_carpincho ready_a_exec_HRRN(){ 
    
        for(int i= 0; i<list_size(ready); i++){

            float max_hasta_el_momento = 0;
            data_carpincho carpincho_sig;
            data_carpincho carpincho_listo = list_get(ready, i); 
            calculo_RR(carpincho_listo);                         
            float RR_actual = carpincho_listo->RR;               
                if(RR_actual > max_hasta_el_momento){            
                    max_hasta_el_momento = RR_actual;
                    carpincho_sig = carpincho_listo;
                }
        }

    return carpincho_sig;     
}


void calculo_estimacion_siguiente(data_carpincho *carpincho){

    calculo_rafaga_anterior(carpincho);

    carpincho->estimacion_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * (1 - alfa);

}

// para SJF
void calculo_rafaga_anterior(data_carpincho *carpincho){

    //cuánto tiempo en milisegundos estuvo el carpincho en exec tomando la diferencia entre el timestamp al ingresar y al salir de exec.
    char tiempo_al_salir = temporal_get_string_time("%M %S %MS"); // ver si funciona asi, sino ("%H:%M:%S:%MS") => cambiar pasaje a int
    int tiempo_entrada = atoi(*carpincho->tiempo_entrada_a_exec);
    int tiempo_salida = atoi(*tiempo_al_salir);

    //pasar a milisegundos
    
    int diferencia_milisegundos = tiempo_salida - tiempo_entrada;

    carpincho->rafaga_anterior = diferencia_milisegundos;

}


// para HRRN
void calculo_RR(data_carpincho *carpincho){

    char ahora = temporal_get_string_time("%M %S %MS"); // ver si funciona así, sino ("%H:%M:%S:%MS") => cambiar pasaje a int
    int tiempo_ahora= atoi(*ahora);

    float w = tiempo_ahora - carpincho->llegada_a_ready;
    calculo_estimacion_siguiente(carpincho); //duda: prox rafaga = estimacion siguinete de SJF??
    float s =  carpincho->estimacion_siguiente;

    carpincho->RR = 1 + w/s;
}


//////////////////////////// Funciones para exec ///////////////////////////////////

void asignar_hilo_CPU(data_carpincho carpincho){

    hilo_cpu hilo_cpu_disponible;

    bool buscar_disponible(void* hilo_cpu){
        int *valor;
        sem_getvalue(cpu->semaforo, valor);
        return valor === 1;
    }

    hilo_CPU_disponible = list_find(hilos_CPU, buscar_disponible);

    sem_post(&usar_CPU[hilo_CPU_disponible->id]);

    carpicho->hilo_CPU_usado = hilo_CPU_disponible;

}



void ejecuta(int id){ 

    while(1){

        //mutex
        sem_wait(&usar_CPU[id]); // espera hasta que algun carpincho haga post para usar ese cpu
        sem_wait(&CPU_libre[id]); // indica que ya no está más libre ese cpu
        // mutex
        
        sem_wait(&liberar_CPU[id]); // espera a que algun carpincho indique que quiere liberar el cpu
        
        //
        sem_post(&CPU_libre[id]); // indica que ya esta el cpu libre de nuevo
        sem_post(&grado_multiprocesamiento_libre); // indica que ya hay algun cpu libre
        //
    }
}







/////////////////////////////// Busquedas estructura carpincho ////////////////////////////

// segun su id:
bool id_pertenece_al_carpincho(int ID, data_carpincho *carpincho){
        return carpincho->id === ID;
    }

data_carpincho encontrar_estructura_segun_id(int ID){

    bool buscar_id(void *ID){
        return id_pertenece_al_carpincho(ID, carpincho);
    }

    carpincho_encontrado = list_find(lista_carpinchos,buscar_id);

    return carpincho_encontrado;
}


