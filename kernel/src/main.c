#include "main.h"



int main(int argc, char ** argv){
   /* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);
    socket_memoria = (int *)malloc(sizeof(int));
    
    inicializar_colas();

    id_carpincho = malloc(sizeof(int));
    *id_carpincho = 1;*/ para pruebas
    
    int* socket;
    socket_memoria = malloc(sizeof(int));

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

    int id_carpincho = 1;

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos
    semaforos_carpinchos = list_create(); // crear lista para ir guardando los semaforos
    hilos_CPU = list_create(); // crear lista para ir guardando los hilos cpus
    lista_dispositivos_io = list_create(); // crear lista para ir guardando los dispositivios io

    t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

    leer_archivo_config();
	inicializar_semaforos();
	inicializar_colas();
    crear_hilos_CPU();
    crear_hilos_planificadores();
    
    _start_server(puerto_escucha, handler, logger);
    socket_memoria = _connect(ip_memoria, puerto_memoria, logger);

    // borrar todo, habria que ponerle que espere a la finalización de todos los hilos
    free_memory();
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
    mate_pointer pointer;
    
    if(id === ID_MATE_LIB){
        switch(opcode){
            case MATE_INIT:
                estructura_interna = deserializar(payload);
                mate_init(fd);
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
                offset += sizeof(int)* ptr_len;
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
                offset += sizeof(int)* ptr_len;                
                // dest_memwrite
                memcpy(&dest_memwrite, payload, sizeof(int);
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload, sizeof(int);
                offset += sizeof(int);

                mate_memwrite(id_carpincho, origin_memwrite, dest_memwrite, size_memoria, fd);     
            break;  
        }
    }

}




////////////////////////////////////////////////////////////// pasar a las funciones de memoria 

    else{
        switch(opcode){
            case MATE_MEMALLOC:
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);

                responder_a_lib(id_carpincho);
                
            break;
            case MATE_MEMFREE:
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);

                responder_a_lib(id_carpincho);
            break;
            case MATE_MEMREAD:

                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);

                responder_a_lib(id_carpincho);
            break;  
            case MATE_MEMWRITE:
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);

                responder_a_lib(id_carpincho);
            break;
    }

void responder_a_lib(int id_carpincho){
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho); 
    int fd = carpincho->fd;
    
    if(id_carpincho < 0){
        log_info(logger, "no se pudo realizar la operación de memoria");
        _send_message(fd, ID_KERNEL, 1, -1, sizeof(int), logger);
    }
    else{
        _send_message(fd, ID_KERNEL, 1, 0, sizeof(int), logger);
    }
}

////////////////////////////////////////////////////////////// pasar a las funciones de memoria 



///////////////////////////////////////////// INICIALIZACIONES ////////////////////////////////

void inicializar_colas(){ // Colas estados y sus mutex:
    
    new = queue_create();
	pthread_mutex_init(&sem_cola_new, NULL);

	ready = list_create();
	pthread_mutex_init(&sem_cola_ready, NULL);

	exec = list_create();
	pthread_mutex_init(&sem_cola_exec, NULL);

	blocked = list_create();
	pthread_mutex_init(&sem_cola_blocked, NULL);

    suspended_blocked = list_create();
	pthread_mutex_init(&sem_cola_suspended_blocked, NULL);

    suspended_ready = queue_create();
	pthread_mutex_init(&sem_cola_suspended_ready, NULL);

    hilos_CPU = list_create();   

    semaforos_carpinchos = list_create(); 
	
    //pthread_mutex_init(&socket_memoria, NULL); //falta declarar socket_memoria => para que este mutex?
}

void inicializar_semaforos(){ // Inicializacion de semaforos:
    sem_init(&sem_grado_multiprogramacion_libre,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento_libre, 0,grado_multiprocesamiento); 
    sem_init(&hay_estructura_creada,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 
}
// para compilar
void entrantes_a_ready(){}
void ready_a_exec(){}
void suspender(){}
void ejecuta(){}


void crear_hilos_planificadores(){
    pthread_t planficador_largo_plazo;
    pthread_create(&planficador_largo_plazo, NULL, (void*) entrantes_a_ready, NULL);

    pthread_t planficador_corto_plazo;
    pthread_create(&planficador_corto_plazo, NULL, (void*) ready_a_exec, NULL);
    
    pthread_t planficador_mediano_plazo;
    pthread_create(&planficador_mediano_plazo, NULL, (void*) suspender, NULL); //FALTA función "x"
}

void crear_hilos_CPU(){ // creación de los hilos CPU

   
	for(int i= 0; i< grado_multiprocesamiento; i++){
        
        sem_t liberar_CPU[i];
        sem_t CPU_libre[i];
        sem_t usar_CPU[i];

        sem_init(&liberar_CPU[i], 0, 0);
        sem_init(&CPU_libre[i], 0, 1); // ver si es 0 o 1 en el segundo argumento
        sem_init(&usar_CPU[i], 0, 0);        
        
        pthread_t hilo_CPU[i];
        pthread_create(&hilo_CPU[i], NULL, (void*) ejecuta, &i); 
        
        CPU *nuevo_CPU;
        nuevo_CPU = malloc(sizeof(hilo_CPU)); //es necsario? esta bien?
        nuevo_CPU->id = i;
        nuevo_CPU->semaforo = CPU_libre[i]; //esto funciona asi?
        
        list_add(hilos_CPU, nuevo_CPU);
	}
}

void free_memory(){
    
    void remove_semaforos_carpinchos(void* elem){
        semaforo *semaforo_borrar = (semaforo *) elem;
        queue_destroy_and_destroy_elements(semaforo_borrar->en_espera, free);
    }
    
    config_destroy(config);     
    log_destroy(logger);

	list_destroy_and_destroy_elements(lista_carpinchos, free); // podría poner free en vez de esta funcion?

    list_destroy_and_destroy_elements(hilos_CPU, free);

    list_destroy_and_destroy_elements(semaforos_carpinchos, remove_semaforos_carpinchos);
    list_destroy_and_destroy_elements(semaforos_carpinchos, free);
    

    sem_destroy(&sem_grado_multiprogramacion_libre);  
	sem_destroy(&sem_grado_multiprocesamiento_libre); 

    sem_destroy(&hay_estructura_creada);
    sem_destroy(&cola_ready_con_elementos);
    sem_destroy(&cola_exec_con_elementos);
    sem_destroy(&cola_blocked_con_elementos);
    sem_destroy(&cola_suspended_blocked_con_elementos);
    sem_destroy(&cola_suspended_ready_con_elementos); 

    //mutex
    pthread_mutex_destroy(&sem_cola_new);
    pthread_mutex_destroy(&sem_cola_ready);
    pthread_mutex_destroy(&sem_cola_exec);
    pthread_mutex_destroy(&sem_cola_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_ready);

	for(int i= 0; i< grado_multiprocesamiento; i++){
        sem_t liberar_CPU[i];
        sem_t CPU_libre[i];
        sem_t usar_CPU[i];        
        sem_destroy(&liberar_CPU[i]);
        sem_destroy(&CPU_libre[i]);
        sem_destroy(&usar_CPU[i]);   
	}

    free(socket_memoria);

}

///////////////// FUNCIONES ÚTILES ////////////////////////

data_carpincho* encontrar_estructura_segun_id(int id){
    
    bool id_pertenece_al_carpincho(int id, data_carpincho *carpincho){
        return carpincho->id == id;
    }

    bool buscar_id(void * carpincho){
        return id_pertenece_al_carpincho(id, (data_carpincho * ) carpincho);
    }

    data_carpincho *carpincho_encontrado;
    carpincho_encontrado = (data_carpincho *) list_find(lista_carpinchos,buscar_id);

    return carpincho_encontrado;
}

///////////////// RECIBIR MENSAJES //////////////////////// 

void deserializar(void* buffer){

    int str_len;
    int offset = 0;
    data_carpincho* estructura_interna;
    estructura_interna = malloc(sizeof(estructura_interna));

    // int id
    memcpy(&estructura_interna->id, buffer, sizeof(int));
    offset += sizeof(int); 

    // char semaforo
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&estructura_interna->semaforo, buffer + offset, str_len);

    // int valor_semaforo
    memcpy(&estructura_interna->valor_semaforo, buffer, sizeof(int));
    offset += sizeof(int);

    // char dispositivo_io
    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&estructura_interna->dispositivo_io, buffer + offset, str_len);

}

//////////////// FUNCIONES GENERALES ///////////////////

void mate_init(int fd){
    
    data_carpincho carpincho;
    data_carpincho *ptr_carpincho;
    ptr_carpincho = (data_carpincho *) malloc(sizeof(data_carpincho));
    ptr_carpincho = &carpincho;
    carpincho.id = (int)id_carpincho;
    carpincho.rafaga_anterior = 0;
    carpincho.estimacion_anterior = 0;
    carpincho.estimacion_siguiente = estimacion_inicial; // viene por config
    // carpincho->llegada_a_ready no le pongo valor porque todavia no llegó
    // carpincho->RR no le pongo nada todavia
    carpincho.estado = NEW;
    carpincho.fd = fd;

    // tendria que chequear que se cree bien la conexión?
    _send_message(*socket_memoria, ID_KERNEL, MATE_INIT, _serialize(sizeof(int), "%d", ptr_carpincho->id), sizeof(int), logger); // envia la estructura al backend para que inicialice todo

    void *buffer;
    int respuesta_memoria;
    respuesta_memoria = -1;
    buffer = _receive_message(*socket_memoria, logger);
    memcpy((void*)respuesta_memoria, buffer, sizeof(int));
    
    if(respuesta_memoria >= 0){  // si la memoria crea la estructura, le devuelve el id
            
        log_info(logger, "La estructura del carpincho %d se creó correctamente en memoria", *id_carpincho);
        list_add(lista_carpinchos, ptr_carpincho);
        sem_post(&hay_estructura_creada);
        _send_message(fd, ID_KERNEL, MATE_INIT, _serialize(sizeof(int), "%d", ptr_carpincho->id), sizeof(int), logger);
    }
    else{
        log_info(logger, "El módulo memoria no pudo crear la estructura");
    }
    *id_carpincho += 2; // incrementa carpinchos impares
}

void mate_close(int id_carpincho, int fd){
    
    data_carpincho *carpincho_a_cerrar;
    carpincho_a_cerrar = encontrar_estructura_segun_id(id_carpincho);

    carpincho_a_cerrar->fd = fd;

    bool es_el_carpincho(void* carpincho){
        return (data_carpincho *) carpincho == carpincho_a_cerrar;
    }

    list_remove_by_condition(lista_carpinchos, es_el_carpincho);

   // exec_a_exit(id_carpincho, fd); // acá se encarga de avisar a memoria y responder al fd

}

//////////////// FUNCIONES SEMAFOROS ///////////////////

// para las funciones de orden superior

bool esIgualASemaforo(char* nombre_semaforo, void *semaforo_igual){
    return ((semaforo *) semaforo_igual)->nombre == nombre_semaforo;
}

void mate_sem_init(int id_carpincho, char* nombre_semaforo, int valor_semaforo, int fd){  

    bool esIgualA(void *semaforo_igual){
        return esIgualASemaforo(nombre_semaforo, semaforo_igual);
    }

    if(list_any_satisfy(semaforos_carpinchos, (void *)esIgualA)){  
        log_info(logger, "El semaforo ya estaba incializado");
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", -1), sizeof(int), logger); 
    }
    else 
    {
        semaforo semaforo_nuevo;
        void *ptr_semaforo;    
        ptr_semaforo = (semaforo *) malloc(sizeof(semaforo));     
        ptr_semaforo = &semaforo_nuevo;  

        semaforo_nuevo.nombre = nombre_semaforo;
        semaforo_nuevo.valor = valor_semaforo;
        semaforo_nuevo.en_espera = queue_create();

        list_add(semaforos_carpinchos, ptr_semaforo);

        // esto está quedando mal no se por que
        log_info(logger, "Se inicializó el semáforo %d   ", *semaforo_nuevo.nombre  );        
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", 0), sizeof(int), logger);         
    }
}


void mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, (void *)esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        semaforo *semaforo_wait;
        semaforo_wait = (semaforo *)list_find(semaforos_carpinchos, (void *)esIgualA);
        semaforo_wait->valor --; 

        data_carpincho *carpincho;
        carpincho = encontrar_estructura_segun_id(id_carpincho);
        carpincho->fd = fd;

        if(semaforo_wait->valor<1){
            log_info(logger, "Se hizo un wait de un semaforo menor a 1, se bloquea el carpincho");
            //exec_a_block(id_carpincho); 
            queue_push(semaforo_wait->en_espera, carpincho); //queda esperando para que lo desbloqueen, es el primero. 
        }
        else
        {
            log_info(logger, "Se hizo un wait de un semaforo mayor a 1, carpincho puede seguir");        
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", 0), sizeof(int), logger); 
        }
    }
    else
    {
        log_info(logger, "se intento hacer wait de un semaforo no inicializado");
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", -1), sizeof(int), logger); 
    }

}


void mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  // para ver cómo pasar la función: https://www.youtube.com/watch?v=1kYyxZXGjp0

        semaforo *semaforo_post;
        semaforo_post = list_find(semaforos_carpinchos, (void *) esIgualA);
        semaforo_post->valor ++; 
        
        if(!queue_is_empty(semaforo_post->en_espera)){
            data_carpincho *carpincho_a_desbloquear;
            carpincho_a_desbloquear = queue_peek(semaforo_post->en_espera);
            queue_pop(semaforo_post->en_espera);

            if(carpincho_a_desbloquear->estado == BLOCKED){
                carpincho_a_desbloquear->estado = READY;
                //block_a_ready(carpincho_a_desbloquear);
            }
            else{ // si no esta en blocked es porque estaba en suspended blocked, ahora lo cambio a suspended_ready
                carpincho_a_desbloquear->estado = SUSPENDED_READY;
               // suspended_blocked_a_suspended_ready(carpincho_a_desbloquear);
            }
        }
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", 0), sizeof(int), logger); 
    }
    else
    {
        log_info(logger, "se intento hacer post de un semaforo no inicializado");
        _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", -1), sizeof(int), logger); 
    }
}


void mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo, int fd) {
    
    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  
        
        semaforo *semaforo_destroy = list_find(semaforos_carpinchos,esIgualA);
       
       if(!queue_is_empty(semaforo_destroy->en_espera)){ // solo puede destruirlo si está inicializado y no tiene carpinchos en la lista de espera
            list_remove_by_condition(semaforos_carpinchos,esIgualA);
            _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", 0), sizeof(int), logger); 
       }
       else{
            log_info(logger, "no se puede destruir un semáforo que tenga carpinchos en wait");
            _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", -2), sizeof(int), logger); 
       }
    }
    else
    {
        log_info(logger, "se intento borrar un semaforo no inicializado");
            _send_message(fd, ID_KERNEL, 1, _serialize(sizeof(int), "%d", -1), sizeof(int), logger); 
    }
}


/////////////////////// hasta aca llegue compilando

//////////////// FUNCIONES IO ///////////////////

    //para el find:

        bool es_igual_dispositivo(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }

    //para el any satisfy:
        dispositivo_io dispositivo_igual_a_nombre(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }

        bool esIgualDispositivo(mate_io_resource nombre_dispositivo, void *dispositivo){
                return dispositivo->nombre === nombre_dispositivo;
            }


int mate_call_io(int id_carpincho, mate_io_resource nombre_io, int fd){

    dispositivo_io dispositivo_igual_a(void *dispositivo){
        return dispositivo_igual_a_nombre(nombre_io, dispositivo);
    }

    bool igual_a(void *dispoitivo){
        return es_igual_dispositivo(dispositivo, nombre_dispositivo);
    }

    if(list_any_satisfy(dispositivos_io, igual_a)){  
        
        exec_a_block(id_carpincho);
        data_carpincho carpincho = encontrar_estructura_segun_id(id_carpincho);
        carpincho->estado = BLOCKED;
        dispositivo_pedido = list_find(dispositivos_io, dispositivo_igual_a); 

        if(dispositivo_pedido->en_uso){
           queue_push(dispositivo_pedido->en_espera, carpincho);       
        }
        else{ 
            dispositivo_pedido->en_uso = true;
            usleep(dispositivo_pedido->duracion);
            block_a_ready(carpincho);
            while(!queue_is_empty(dispositivo_pedido->en_espera)){
                data_carpincho carpincho_siguiente;
                carpincho_siguiente = queue_peek(dispositivo_pedido->en_espera);
                queue_pop(dispositivo_pedido->en_espera);
                usleep(dispositivo_pedido->duracion);
                block_a_ready(carpincho_siguiente);
            }
        }
    }
    else
    {
        log_info(logger, "Se pidio un dispositivo IO que no existe");
        _send_message(fd, ID_KERNEL, 1, -1, sizeof(int), logger);
    }

}

// lista dispositivos_io y duraciones_io por config
void crear_estructura_dispositivo(){ //deberia crearse al principio, no cuando lo piden

    for(int i= 0; i<list_size(dispositivos_io); i++){

            dispositivo_io dispositivo = malloc(size_of(dispositivo_io); //free del malloc al final
            dispositivo->nombre = list_get(dispositivos_io, i);
            dispositivo->duracion = list_get(duraciones_io, i);
            dispositivo->en_uso = false;
            // dispositivo->en_espera = queue_create();  para crear la cola de espera y ahi ir guardando los carpinchos

            list_add(lista_dispositivos_io, *dispositivo);

        }

}


//////////////// FUNCIONES MEMORIA ///////////////////

mate_pointer mate_memalloc(int id_carpincho, int size, int fd){
    data_carpincho carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    _send_message(socket_memoria, 
                    ID_KERNEL, 
                    MATE_MEMALLOC,
                     _serialize(    sizeof(int) * 2, 
                                    "%d%d", 
                                    estructura_interna->id, 
                                    size 
                                ),
                    sizeof(int)*2,
                    logger);   

}

int mate_memfree(int id_carpincho, mate_pointer addr, int fd){
    data_carpincho carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    _send_message(socket_memoria, 
                    ID_KERNEL, 
                    MATE_MEMFREE,
                     _serialize(    sizeof(int) + sizeof(mate_pointer), 
                                    "%d%d", 
                                    estructura_interna->id, 
                                    addr 
                                ), 
                    sizeof(int) + sizeof(mate_pointer),
                    logger); 
}

int mate_memread(int id_carpincho, mate_pointer origin, void *dest, int size, int fd){
    data_carpincho carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    _send_message(socket_memoria, 
                    ID_KERNEL, 
                    MATE_MEMREAD, 
                    _serialize(     sizeof(int) * 3 + sizeof(int) * sizeof(dest) + sizeof(int), 
                                    "%d%d%d%v%d",
                                    estructura_interna->id, 
                                    origin,
                                    sizeof(dest),
                                    dest,
                                    size
                                ), 
                    sizeof(sizeof(int) * 3 + sizeof(int) * sizeof(dest) + sizeof(int)), 
                    logger); 

}

int mate_memwrite(int id_carpincho, void origin, mate_pointer dest, int size, int fd){
    data_carpincho carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    _send_message(socket_memoria, 
                    ID_KERNEL, 
                    MATE_MEMWRITE, 
                    _serialize(         sizeof(int) * 2 + sizeof(int) * sizeof(origin) + sizeof(int) +  sizeof(int), 
                                        "%d%d%v%d%d",
                                        estructura_interna->id, 
                                        sizeof(origin),
                                        origin,
                                        dest,
                                        size
                                ), 
                    sizeof(sizeof(int) * 2 + sizeof(int) * sizeof(origin) + sizeof(int) +  sizeof(int)), 
                    logger
                );

}
    

///////////////////// PLANIFICACIÓN ////////////////////////

void entrantes_a_ready(){

    data_carpincho *carpincho_a_mover;
    int respuesta_memoria;
    t_mensaje buffer;

    while(1){
        sem_wait(&hay_estructura_creada); // hacerle post en mate_init y cuando uno pasa a estar en suspended_ready
        sem_wait(&sem_grado_multiprogramacion_libre); //grado multiprogramacion --> HACER POST CUANDO SALE DE EXEC! o en suspender()
		
        if(!queue_is_empty(suspended_ready)) // se fija si hay elementos que hayan venido de suspended, y si hay lo saca de ahi
        {
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_suspended_ready); 

            carpincho_a_mover = queue_peek(suspended_ready);

            queue_push(ready, carpincho_a_mover);
            queue_pop(suspended_ready)

            pthread_mutex_unlock(&sem_cola_ready); 
            pthread_mutex_unlock(&sem_cola_suspended_ready); 

            // avisarle a memoria que recupere las paginas
        }
        else{ // si no habia venido ninguno de suspended, saco de cola new y pongo en cola ready al primero (FIFO):
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_new);

            carpincho_a_mover = queue_peek(new);

            list_add(ready, carpincho_a_mover); 
            queue_pop(new);

            pthread_mutex_unlock(&sem_cola_new);
            pthread_mutex_unlock(&sem_cola_ready);
        }
        
        carpincho_a_mover->estado = READY;
        sem_post(&cola_ready_con_elementos); //avisa: hay procesos en ready 

        if(sem_getvalue(&sem_grado_multiprogramacion_libre) === 0){ // si con este se llena el grado de multiprocesamiento, podria necesitarse suspender 
            sem_post(&sem_programacion_lleno);
        }
    }
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

        list_add(exec, *carpincho_a_mover);
        list_remove_by_condition(ready, es_el_mismo);
    
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_ready);

        carpincho_a_mover->estado = EXEC;

        bool es_el_mismo(void* carpincho){
            return es_el_mismo_carpincho(carpincho,carpincho_a_mover);
            }

        bool es_el_mismo_carpincho(data_carpincho carpincho, data_carpincho carpincho_a_mover){
            return carpincho->id === carpincho_a_mover->id;
            }

        asignarle_hilo_CPU(carpincho_a_mover);

        carpincho_a_mover->tiempo_entrada_a_exec = calcular_milisegundos(); //dejarlo aca o cuando lo agregas a la lista de exec?

        //mandar mensaje a la lib, con el fd que tiene en la estructura el carpincho
        _send_message(carpincho->fd, ID_KERNEL, 1, carpincho->id, sizeof(int), logger); // va así?

        if(sem_getvalue(&sem_grado_multiprocesamiento_libre) === 0){ // si con este se llena el grado de multiprocesamiento, podria necesitarse suspender 
            sem_post(&sem_procesamiento_lleno);
        }

    }
}



void exec_a_block(int id_carpincho){
     bool es_el_mismo(void* carpincho){
        return es_el_mismo_carpincho(carpincho,carpincho_a_bloquear);
    }

    bool es_el_mismo_carpincho(data_carpincho carpincho, data_carpincho carpincho_a_bloquear){
        return carpincho->id === carpincho_a_bloquear->id;
    }

    // le pasan el id del carpincho y lo saca de la lista de exec, lo pone en block y le hace signal al cpu

    data_carpincho carpincho_a_bloquear = encontrar_estructura_segun_id(id_carpincho);
    calculo_rafaga_anterior(carpincho_a_bloquear); 

    // Sacar de la lista de exec y ponerlo en la la cola de blocked
    pthread_mutex_lock(&sem_cola_exec); 
	pthread_mutex_lock(&sem_cola_blocked);

    list_add(blocked, *carpincho_a_bloquear); 
    list_remove_by_condition(exec, es_el_mismo);
    
	pthread_mutex_unlock(&sem_cola_blocked);
	pthread_mutex_unlock(&sem_cola_exec);

    carpincho_a_bloquear->estado = BLOCKED; 

    sem_post(&sem_hay_bloqueados);

    sem_post(liberar_CPU[carpincho_a_bloquear->hilo_CPU_usado->id]);

}


void exec_a_block_io(int id_carpincho, char dispositivo){ //hace falta que sea una distinta?
    
    // ver lo de las duraciones
    exec_a_block(id_carpincho);

}


void exec_a_exit(int id_carpincho, int fd){
    
    data_carpincho carpincho_que_termino;
    carpincho_que_termino = encontrar_estructura_segun_id(id_carpincho);

    bool es_el_mismo(void* carpincho){
        return es_el_mismo_carpincho(carpincho,carpincho_que_termino);
    }

    bool es_el_mismo_carpincho(data_carpincho carpincho, data_carpincho carpincho_que_termino){
        return carpincho->id === carpincho_que_termino->id;
    }

    _send_message(socket_memoria, ID_KERNEL, MATE_CLOSE, _serialize(sizeof(int), "%d", id_carpincho), sizeof(int), logger);  //avisar a mem para que libere

    pthread_mutex_lock(&sem_cola_exec); 
    list_remove_by_condition(exec, es_el_mismo);
	pthread_mutex_unlock(&sem_cola_exec);

    sem_post(liberar_CPU[carpincho_a_bloquear->hilo_CPU_usado->id]); // "libera" el hilo cpu en el que estaba:
    sem_post(&sem_grado_multiprogramacion_libre);

    _send_message(fd, ID_KERNEL, 1, 0, sizeof(int), logger); 
}

void block_a_ready(data_carpincho *carpincho){ //la llaman cuando se hace post o cuando se termina IO
   
   void* esIgualACarpincho (void* carpincho_lista){
       return (data_carpincho *) carpincho_lista === carpincho;
   }

    pthread_mutex_lock(&sem_cola_ready); 
    pthread_mutex_lock(&sem_cola_blocked;

    list_add(ready, carpincho);
    list_remove_by_condition(blocked, esIgualACarpincho);

    pthread_mutex_unlock(&sem_cola_blocked);
    pthread_mutex_unlock(&sem_cola_ready);

    carpincho->estado = READY;
    sem_post(&cola_ready_con_elementos);

    sem_wait(&sem_hay_bloqueados);

    // no tiene que evaluar ni grado de multiprogramacion ni de multiproc porque ya estaba considerado.
    // no responde nada a la lib porque todavia no esta en exec, solo paso a ready
}

void suspended_blocked_a_suspended_ready(data_carpincho *carpincho){

    void* esIgualACarpincho (void* carpincho_lista){
       return (data_carpincho *) carpincho_lista === carpincho;
    }    

    pthread_mutex_lock(&sem_cola_ready); 
    pthread_mutex_lock(&sem_cola_blocked);

    queue_pop(suspended_ready, carpincho);
    list_remove_by_condition(suspended_blocked, esIgualACarpincho);

    pthread_mutex_unlock(&sem_cola_blocked);
    pthread_mutex_unlock(&sem_cola_ready);

    carpincho->estado = SUSPENDED_READY; 
    sem_post(&hay_estructura_creada);

}

void suspender(){

while(1){

    sem_wait(&sem_procesamiento_lleno);
    sem_wait(&sem_programacion_lleno);
    sem_wait(&sem_hay_bloqueados);

    if(estan_las_condiciones_para_suspender()){

        int longitud;
        longitud = list_size(blocked);

        pthread_mutex_lock(&sem_cola_blocked);
        pthread_mutex_lock(&sem_cola_suspended_blocked);

        carpincho_a_suspender = list_remove(blocked, longitud);
        list_add(suspended_blocked, carpincho_a_suspender);

        pthread_mutex_unlock(&sem_cola_blocked);
        pthread_mutex_unlock(&sem_cola_suspended_blocked);

        carpincho_a_suspender->estado = SUSPENDED_BLOCKED;
        sem_post(liberar_CPU[carpincho_a_bloquear->hilo_CPU_usado->id]);

        _send_message(socket_memoria, ID_KERNEL, SUSPENDER, _serialize(sizeof(int), "%d", carpincho_a_suspender->id), sizeof(int), logger);  //avisar a mem para que libere
        
    }
}
}

bool estan_las_condiciones_para_suspender(){
    return sem_getvalue(&sem_grado_multiprogramacion_libre) === 0 && !list_is_empty(blocked) && sem_getvalue(&hay_estructura_creada) > 0;
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

    int tiempo_salida = calcular_milisegundos();

    carpincho->rafaga_anterior = tiempo_salida - carpincho->tiempo_entrada_a_exec;
}

// para HRRN
void calculo_RR(data_carpincho *carpincho){

    char ahora = calcular_milisegundos();

    float espera = ahora - carpincho->llegada_a_ready;
    calculo_estimacion_siguiente(carpincho); 
    float prox_rafaga =  carpincho->estimacion_siguiente;

    carpincho->RR = 1 + espera/prox_rafaga;
}


int calcular_milisegundos(){

    char tiempo_sacado = temporal_get_string_time("%M:%S:%MS");
    
    char** tiempo_formateado = str_split(tiempo_sacado,':');

    tiempo tiempo_calculado; 

    tiempo_calculado->minutos = atoi(tiempo_formateado[0])
    tiempo_calculado->segundos = atoi(tiempo_formateado[1])
    tiempo_calculado->milisegundos = atoi(tiempo_formateado[2])

        // paso todo a milisegundos para que sea mas facil despues sacar la diferencia
    return tiempo_calculado->minutos * 60000 + tiempo_calculado->segundos * 60 + tiempo_calculado->milisegundos
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











