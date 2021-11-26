#include "main.h"

// cuando quiera lleanr la estructura con palabras, tengo que hacer el malloc.

int main(int argc, char ** argv){

    config = config_create("./cfg/kernel.conf");

    id_carpincho = 1; 

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos
    semaforos_carpinchos = list_create(); // crear lista para ir guardando los semaforos
    hilos_CPU = list_create(); // crear lista para ir guardando los hilos cpus
    lista_dispositivos_io = list_create(); // crear lista para ir guardando los dispositivios io

    logger = log_create("./cfg/kernel.log", "[Kernel]", true, LOG_LEVEL_INFO);
	char *puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");        
    
    crear_estructura_dispositivo();
    inicializar_semaforos();
	inicializar_colas();
    crear_hilos_CPU();

    signal(SIGINT, free_memory);

    pthread_t planficador_largo_plazo;
    pthread_create(&planficador_largo_plazo, NULL, (void*) entrantes_a_ready, NULL);
    pthread_t planficador_corto_plazo;
    pthread_create(&planficador_corto_plazo, NULL, (void*) ready_a_exec, NULL);
    pthread_t planficador_mediano_plazo;
    pthread_create(&planficador_mediano_plazo, NULL, (void*) suspender, NULL); 
    pthread_t deteccion_deadlock;
    pthread_create(&deteccion_deadlock, NULL, (void*) detectar_deadlock, NULL);
    
    _start_server(puerto_escucha, handler, logger);

    pthread_join ( planficador_largo_plazo , NULL ) ;
    pthread_join ( planficador_corto_plazo , NULL ) ;
    pthread_join ( planficador_mediano_plazo , NULL ) ;
    pthread_join ( deteccion_deadlock , NULL ) ;    

}

///////////////////////////////////////////// INICIALIZACIONES ////////////////////////////////

void inicializar_colas(){
    
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

    CPU_libres = queue_create();
    pthread_mutex_init(&sem_CPU_libres, NULL);

    pthread_mutex_init(&mutex_para_CPU, NULL);

}

void inicializar_semaforos(){ 

    int grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    int grado_multiprocesamiento = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");

    sem_init(&sem_grado_multiprogramacion_libre,0,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento_libre, 0,grado_multiprocesamiento); 
    sem_init(&hay_estructura_creada,0,0);
    sem_init(&cola_ready_con_elementos,0,0);
    sem_init(&cola_exec_con_elementos,0,0);
    sem_init(&cola_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_blocked_con_elementos,0,0);
    sem_init(&cola_suspended_ready_con_elementos,0,0); 
    sem_init(&sem_programacion_lleno,0,0);
    sem_init(&sem_procesamiento_lleno,0,0);
    sem_init(&sem_hay_bloqueados,0,0);
    sem_init(&hay_bloqueados_para_deadlock,0,0);
}


void crear_hilos_CPU(){ 

    int grado_multiprocesamiento = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");

	for(int i = 0; i< grado_multiprocesamiento; i++){
        
        void* id_CPU;
        id_CPU = malloc(sizeof(int));
        memcpy(id_CPU, &i, sizeof(int));
        sem_init(&(liberar_CPU[i]), 0, 0);
        sem_init(&(CPU_libre[i]), 0, 1); // post pruebas => ver si es 0 o 1 en el segundo argumento
        sem_init(&(usar_CPU[i]), 0, 0);       

        pthread_create(&(hilo_CPU[i]), NULL, (void*) ejecuta, id_CPU); 
        
        queue_push(CPU_libres, id_CPU);

        // hay que ver como se hace el free de todos los id_CPU que se crean aca y se ponen en la queue
	}
}


void free_memory(){
    /*
    void remove_semaforos_carpinchos(void* elem){
        semaforo *semaforo_borrar = (semaforo *) elem;
        queue_destroy_and_destroy_elements(semaforo_borrar->en_espera, free);
    }
    
    config_destroy(config);     
    log_destroy(logger);

	list_destroy_and_destroy_elements(lista_carpinchos, free); 

    list_destroy_and_destroy_elements(hilos_CPU, free);

    list_destroy_and_destroy_elements(semaforos_carpinchos, remove_semaforos_carpinchos);
    list_destroy_and_destroy_elements(semaforos_carpinchos, free);

    sem_destroy(&sem_grado_multiprogramacion_libre);  
	sem_destroy(&sem_grado_multiprocesamiento_libre); 

    sem_destroy(&hay_estructura_creada);
    sem_destroy(&cola_ready_con_elementos);
    sem_destroy(&cola_exec_con_elementos);
    sem_destroy(&cola_blocked_con_elementos);
    sem_destroy(&hay_bloqueados_para_deadlock);
    sem_destroy(&cola_suspended_blocked_con_elementos);
    sem_destroy(&cola_suspended_ready_con_elementos); 

    pthread_mutex_destroy(&sem_cola_new);
    pthread_mutex_destroy(&sem_cola_ready);
    pthread_mutex_destroy(&sem_cola_exec);
    pthread_mutex_destroy(&sem_cola_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_ready);
    pthread_mutex_destroy(&mutex_para_CPU);

    // hacer => free a todo
    // martin => liberar memoria en todos lados

    exit(EXIT_SUCCESS);
    */ //ver despues para liberar memoria
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

data_carpincho* deserializar(void* buffer){

    int str_len;
    int offset = 0;
    data_carpincho* estructura_interna;
    estructura_interna = malloc(sizeof(estructura_interna));

    memcpy(&estructura_interna->id, buffer, sizeof(int));
    offset += sizeof(int); 

    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&estructura_interna->semaforo, buffer + offset, str_len);
    offset += str_len;

    memcpy(&estructura_interna->valor_semaforo, buffer + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&str_len, buffer + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&estructura_interna->dispositivo_io, buffer + offset, str_len);

    return estructura_interna;
} 


//////////////// FUNCIONES GENERALES ///////////////////

void mate_init(int fd){

    data_carpincho *carpincho;
    carpincho = malloc(sizeof(data_carpincho));

    carpincho->id = id_carpincho;
    carpincho->rafaga_anterior = 0;
    carpincho->estimacion_anterior = 0;
    carpincho->estimacion_siguiente = config_get_int_value(config, "ESTIMACION_INICIAL");
    carpincho->estado = NEW;
    carpincho->fd = fd; 
    carpincho->semaforos_retenidos = list_create();

    void *payload;
    payload = _serialize(sizeof(int), "%d", carpincho->id);

    int socket_memoria;
    char* ip_memoria = string_from_format("%s", config_get_string_value(config, "IP_MEMORIA"));
    char* puerto_memoria = string_from_format("%s", config_get_string_value(config, "PUERTO_MEMORIA"));
    socket_memoria = _connect(ip_memoria, puerto_memoria, logger);
    free(ip_memoria);
    free(puerto_memoria);
    
    int respuesta_memoria;

    if(socket_memoria >= 0){
        _send_message(socket_memoria, ID_KERNEL, MATE_INIT, payload , sizeof(int), logger); 
        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));
        
        close(socket_memoria); 
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }
    else{
        log_error(logger, "no se pudo conectar con el módulo memoria");
    }

    if(respuesta_memoria >= 0){
        log_info(logger, "La estructura del carpincho %d se creó correctamente en memoria", id_carpincho);
        list_add(lista_carpinchos, (void *) carpincho);
        // sem_post(&hay_estructura_creada);
        _send_message(fd, ID_KERNEL, MATE_INIT, payload, sizeof(int), logger);
    }
    else{
        log_info(logger, "El módulo memoria no pudo crear la estructura");
    }
    id_carpincho += 2; 
    free(payload);

}

void mate_close(int id_carpincho, int fd){
    
    data_carpincho *carpincho_a_cerrar;
    carpincho_a_cerrar = encontrar_estructura_segun_id(id_carpincho);

    carpincho_a_cerrar->fd = fd;

    bool es_el_carpincho(void* carpincho){
        return (data_carpincho *) carpincho == carpincho_a_cerrar;
    }

    list_remove_by_condition(lista_carpinchos, es_el_carpincho);
    
    if(carpincho_a_cerrar->estado == EXEC){
        exec_a_exit(id_carpincho, fd); 
    }
    else if(carpincho_a_cerrar->estado == BLOCKED) 
    {      
         bool es_el_mismo(void* carpincho){
            return ((data_carpincho *) carpincho)->id == carpincho_a_cerrar->id;
        }

        pthread_mutex_lock(&sem_cola_blocked);
        list_remove_by_condition(blocked, es_el_mismo); // salir de la cola de bloqueado
        pthread_mutex_unlock(&sem_cola_blocked);
   
        sem_post(&sem_grado_multiprogramacion_libre);

    // hacer
        // borrar su estructura
        // avisar a memoria que lo eliminamos 
    }
    else{ 
        bool son_el_mismo(void* carpincho){
            return ((data_carpincho *) carpincho)->id == carpincho_a_cerrar->id;
        }

        pthread_mutex_lock(&sem_cola_suspended_blocked);
        list_remove_by_condition(suspended_blocked, son_el_mismo); // salir de la cola de suspended_blocked
        pthread_mutex_unlock(&sem_cola_suspended_blocked);

    // hacer
        // borrar su estructura
        // avisar a memoria que lo eliminamos 
    }

    // post pruebas => chequear si es necesario que se haga post a todos los semaforos que tenia retenido el carpincho
    log_info(logger, "La estructura del carpincho %d se eliminó correctamente", id_carpincho);
    // eliminar file descriptor
}

//////////////// FUNCIONES SEMAFOROS ///////////////////

bool esIgualASemaforo(char* nombre_semaforo, void *semaforo_igual){
    
    return strcmp(((semaforo *) semaforo_igual)->nombre, nombre_semaforo) == 0;
}

void mate_sem_init(int id_carpincho, char * nombre_semaforo, int valor_semaforo, int fd){  

    bool esIgualA(void *semaforo_igual){
        return esIgualASemaforo(nombre_semaforo, semaforo_igual);
    }

    if(list_any_satisfy(semaforos_carpinchos, (void *)esIgualA)){  
        void  *payload;
        payload = _serialize(sizeof(int), "%d", -1);
        log_info(logger, "El semaforo %s ya estaba incializado", *nombre_semaforo);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else {
        semaforo semaforo_nuevo;
        void *ptr_semaforo;    
        ptr_semaforo = (semaforo *) malloc(sizeof(semaforo));     
        ptr_semaforo = &semaforo_nuevo;  
        
        void *payload = _serialize(sizeof(int), "%d", 0);  

        semaforo_nuevo.nombre = nombre_semaforo;
        semaforo_nuevo.valor = valor_semaforo;
        semaforo_nuevo.en_espera = queue_create();

        list_add(semaforos_carpinchos, ptr_semaforo);

        log_info(logger, "Se inicializó el semáforo %s", *nombre_semaforo );        
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger);         
    }
}

void mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, (void *)esIgualA)){ 

        semaforo *semaforo_wait;
        semaforo_wait = (semaforo *)list_find(semaforos_carpinchos, (void *)esIgualA);
        semaforo_wait->valor --; 

        data_carpincho *carpincho;
        carpincho = encontrar_estructura_segun_id(id_carpincho);
        carpincho->fd = fd;

        if(semaforo_wait->valor<1){
            log_info(logger, "Se hizo un wait de un semaforo menor a 1, se bloquea el carpincho");
            carpincho->nombre_semaforo_por_el_que_se_bloqueo = nombre_semaforo;
            exec_a_block(id_carpincho); 
            queue_push(semaforo_wait->en_espera, carpincho);
        }
        else
        {
            list_add(carpincho->semaforos_retenidos, nombre_semaforo); 
            void* payload;
            payload = _serialize(sizeof(int), "%d", 0);
            log_info(logger, "Se hizo un wait de un semaforo mayor a 1, carpincho puede seguir");        
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        }
    }
    else
    {
        void* payload;
        payload = _serialize(sizeof(int), "%d", -1);
        log_info(logger, "Se intento hacer wait de un semaforo no inicializado");
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }

}

void mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo, int fd){
    
    void *payload;

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  

        semaforo *semaforo_post;
        semaforo_post = list_find(semaforos_carpinchos, (void *) esIgualA);
        semaforo_post->valor ++; 

        payload = _serialize(sizeof(int), "%d", 0);
        
        if(!queue_is_empty(semaforo_post->en_espera)){
            data_carpincho *carpincho_a_desbloquear;
            carpincho_a_desbloquear = queue_peek(semaforo_post->en_espera);
            queue_pop(semaforo_post->en_espera);
            carpincho_a_desbloquear->nombre_semaforo_por_el_que_se_bloqueo = NULL;
            data_carpincho *carpincho_que_hizo_post = encontrar_estructura_segun_id(id_carpincho);
            if(list_any_satisfy(carpincho_que_hizo_post->semaforos_retenidos,(void *)esIgualA)){
                   list_remove_by_condition(carpincho_que_hizo_post->semaforos_retenidos,(void *)esIgualA);
            }
            // post pruebas => ver caso en que mas de carpincho este siendo bloqueado por el que hace un post

            if(carpincho_a_desbloquear->estado == BLOCKED){ 
                carpincho_a_desbloquear->estado = READY;
                block_a_ready(carpincho_a_desbloquear);
            }
            else{
                carpincho_a_desbloquear->estado = SUSPENDED_READY;
               suspended_blocked_a_suspended_ready(carpincho_a_desbloquear);
            }
        }
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else
    {
        payload = _serialize(sizeof(int), "%d", -1);
        log_info(logger, "Se intento hacer post de un semaforo no inicializado");
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
}

void mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo, int fd) {
    void* payload; 

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(semaforo, nombre_semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  
        
        semaforo *semaforo_destroy = list_find(semaforos_carpinchos,esIgualA);
       
       if(!queue_is_empty(semaforo_destroy->en_espera)){ 
            list_remove_by_condition(semaforos_carpinchos,esIgualA);
            payload = _serialize(sizeof(int), "%d", 0);
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
       }
       else{
            log_info(logger, "No se puede destruir un semáforo que tenga carpinchos en wait");
            payload = _serialize(sizeof(int), "%d", -2);
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
       }
    }
    else
    {
        log_info(logger, "Se intento borrar un semaforo no inicializado");
        payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
}

//////////////// FUNCIONES IO ///////////////////

bool es_igual_dispositivo(mate_io_resource nombre_dispositivo, void *dispositivo){
    return  strcmp(((dispositivo_io *)dispositivo)->nombre, nombre_dispositivo) == 0 ;    
} 

void mate_call_io(int id_carpincho, mate_io_resource nombre_io, int fd){
    
    void * payload;
    
    bool igual_a(void *dispositivo){
        return es_igual_dispositivo(nombre_io, dispositivo);
    }

    if(list_any_satisfy(lista_dispositivos_io, igual_a)){  
        
        dispositivo_io *dispositivo_pedido; 
        exec_a_block(id_carpincho);
        data_carpincho *carpincho;
        carpincho = encontrar_estructura_segun_id(id_carpincho);
        carpincho->estado = BLOCKED;
        dispositivo_pedido = (dispositivo_io *)list_find(lista_dispositivos_io, igual_a); 

        if(dispositivo_pedido->en_uso){
           queue_push(dispositivo_pedido->en_espera, carpincho);  
           log_info(logger, "El dispositivo IO esta en uso");     
        }
        else{ 
            dispositivo_pedido->en_uso = true;
            usleep(dispositivo_pedido->duracion);
            block_a_ready(carpincho);
            while(!queue_is_empty(dispositivo_pedido->en_espera)){
                data_carpincho *carpincho_siguiente;
                carpincho_siguiente = (data_carpincho*)queue_peek(dispositivo_pedido->en_espera);
                queue_pop(dispositivo_pedido->en_espera);
                usleep(dispositivo_pedido->duracion);
                block_a_ready(carpincho_siguiente);
            }
            log_info(logger, "Se le dio el dispositivo IO");
        }
    }
    else
    {
        log_info(logger, "Se pidio un dispositivo IO que no existe");
         payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
}

void crear_estructura_dispositivo(){ 
    
    char** dispositivos_io = config_get_array_value(config, "DISPOSITIVOS_IO");
    char** duraciones_io = config_get_array_value(config, "DURACIONES_IO");

    for(int i= 0; i < contar_elementos(dispositivos_io); i++){

            dispositivo_io *dispositivo;
            dispositivo = (dispositivo_io *)malloc(sizeof(dispositivo_io)); 
            dispositivo->nombre = string_from_format("%s",dispositivos_io[i]);
            dispositivo->duracion = atoi(duraciones_io[i]);
            dispositivo->en_uso = false;
            dispositivo->en_espera = queue_create(); 

            list_add(lista_dispositivos_io, dispositivo);
        }
}

int contar_elementos(char** elementos) {
    int i = 0;
    while (elementos[i] != NULL) {
        i++;
    }
    return i;
}

//////////////// FUNCIONES MEMORIA ///////////////////

void mate_memalloc(int id_carpincho, int size, int fd){  // martin => hay que revisar lo que le mandamos a la lib, porque espera que le devolvamos al carpincho un mate_pointer
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;
    void *payload = _serialize(sizeof(int) * 2, "%d%d", carpincho->id, size );
    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);

    int respuesta_memoria;
    
    if(socket_memoria >= 0){
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMALLOC, payload,sizeof(int)*2,logger);   

        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        close(socket_memoria);
    }
    else{
        log_error(logger, "No se pudo conectar con el módulo memoria");
    }

    if(respuesta_memoria >= 0){
        log_info(logger,"El memalloc se realizó correctamente");
        void *payload = _serialize(sizeof(int), "%d", 0);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else{
        log_info(logger,"No se pudo realizar el memalloc");
        void *payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }   
}

void mate_memfree(int id_carpincho, mate_pointer addr, int fd){
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    void *payload = _serialize(    sizeof(int) + sizeof(mate_pointer), 
                                    "%d%d", 
                                    carpincho->id, 
                                    addr 
                                );
    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
    
    int respuesta_memoria;
    
    if(socket_memoria >= 0){
        _send_message(socket_memoria,  ID_KERNEL, MATE_MEMFREE,payload,sizeof(int) + sizeof(mate_pointer),logger);   

        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));
        
        close(socket_memoria);
    }
    else{
        log_error(logger, "No se pudo conectar con el módulo memoria");
    }

    if(respuesta_memoria >= 0){
        log_info(logger,"El memalloc se realizó correctamente");
        void *payload = _serialize(sizeof(int), "%d", 0);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else{
        log_info(logger,"No se pudo realizar el memalloc");
        void *payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }   
}

void mate_memread(int id_carpincho, mate_pointer origin, int size, int fd){ // martin => está bien lo que estamos retornando al carpincho?
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    void *payload = _serialize(     sizeof(int) * 3 + sizeof(int), 
                                    "%d%d%d%v%d",
                                    carpincho->id, 
                                    origin,
                                    size
                                );
    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);

    int respuesta_memoria;

    if(socket_memoria >= 0){
        
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMREAD,payload,sizeof(sizeof(int) * 3 + sizeof(int)),logger);   
          t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        close(socket_memoria);
    }
    else{
        log_error(logger, "No se pudo conectar con el módulo memoria");
    }


    if(respuesta_memoria >= 0){
        log_info(logger,"El memread se realizó correctamente");
        void *payload = _serialize(sizeof(int), "%d", 0);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else{
        log_info(logger,"No se pudo realizar el memread");
        void *payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
}

void mate_memwrite(int id_carpincho, void* origin, mate_pointer dest, int size, int fd){
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    void *payload = _serialize(         sizeof(int) * 2 + sizeof(int) * sizeof(origin) + sizeof(int) +  sizeof(int), 
                                        "%d%d%v%d%d",
                                        carpincho->id, 
                                        sizeof(origin),
                                        origin,
                                        dest,
                                        size
                                );

    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);

    int respuesta_memoria;
    
    if(socket_memoria >= 0){
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMWRITE,payload,sizeof(sizeof(int) * 2 + sizeof(int) * sizeof(origin) + sizeof(int) +  sizeof(int)),logger);   

        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        close(socket_memoria);
    }
    else{
        log_error(logger, "No se pudo conectar con el módulo memoria");
    }

    if(respuesta_memoria >= 0){

        log_info(logger,"El memread se realizó correctamente");
        void *payload = _serialize(sizeof(int), "%d", 0);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    else{
        log_info(logger,"No se pudo realizar el memread");
        void *payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 

    }
}

void entrantes_a_ready(){

   data_carpincho *carpincho_a_mover;
   int valor = 1; 

    while(1){
        sem_wait(&hay_estructura_creada);
        sem_wait(&sem_grado_multiprogramacion_libre); 
		
        if(!queue_is_empty(suspended_ready)) 
        {
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_suspended_ready); 

            carpincho_a_mover = queue_peek(suspended_ready);

            list_add(ready, carpincho_a_mover);
            queue_pop(suspended_ready);

            pthread_mutex_unlock(&sem_cola_ready); 
            pthread_mutex_unlock(&sem_cola_suspended_ready); 

            log_info(logger, "El carpincho %d paso a READY", carpincho_a_mover->id);
            
        }
        else{ 
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_new);

            carpincho_a_mover = queue_peek(new);

            list_add(ready, carpincho_a_mover); 
            queue_pop(new);

            pthread_mutex_unlock(&sem_cola_new);
            pthread_mutex_unlock(&sem_cola_ready);

            log_info(logger, "El carpincho %d paso a READY", carpincho_a_mover->id);
        }
        
        carpincho_a_mover->estado = READY;
        sem_post(&cola_ready_con_elementos); 

        valor = sem_getvalue(&sem_grado_multiprogramacion_libre, &valor);
        if(valor == 0){ 
            sem_post(&sem_programacion_lleno);
        }
    }
}

void ready_a_exec(){  

    int valor = 1;
    void *payload;

    data_carpincho *carpincho_a_mover;

    while(1){ 

        sem_wait(&cola_ready_con_elementos);   
        sem_wait(&sem_grado_multiprocesamiento_libre);

        if(string_equals_ignore_case(config_get_string_value(config, "ALGORITMO_PLANIFICACION"), "SJF")){
            carpincho_a_mover = ready_a_exec_SJF(); 
        }
        else{
            carpincho_a_mover = ready_a_exec_HRRN();
        }

        bool es_el_mismo(void* carpincho){
            data_carpincho* aux = (data_carpincho*)carpincho;
            return aux->id == carpincho_a_mover->id;
        }

        asignar_hilo_CPU(carpincho_a_mover); 

        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_exec);

        list_add(exec, carpincho_a_mover);
        list_remove_by_condition(ready, (void *)es_el_mismo);
    
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_ready);

        log_info(logger, "El carpincho %d paso a EXEC", carpincho_a_mover->id);

        carpincho_a_mover->estado = EXEC;

        carpincho_a_mover->tiempo_entrada_a_exec = calcular_milisegundos(); 

        payload =  _serialize(sizeof(int), "%d", 0);
        _send_message(carpincho_a_mover->fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        
        valor = sem_getvalue(&sem_grado_multiprocesamiento_libre, &valor);
        if( valor == 0){ 
            sem_post(&sem_procesamiento_lleno);
        }
    }
}

void exec_a_block(int id_carpincho){

    data_carpincho *carpincho_a_bloquear;
    carpincho_a_bloquear = encontrar_estructura_segun_id(id_carpincho);

    bool es_el_mismo(void* carpincho){
            return ((data_carpincho *) carpincho)->id == carpincho_a_bloquear->id;
        }


    calculo_rafaga_anterior(carpincho_a_bloquear); 

    pthread_mutex_lock(&sem_cola_exec); 
	pthread_mutex_lock(&sem_cola_blocked);

    list_add(blocked, carpincho_a_bloquear); 
    list_remove_by_condition(exec, es_el_mismo);
    
	pthread_mutex_unlock(&sem_cola_blocked);
	pthread_mutex_unlock(&sem_cola_exec);

    carpincho_a_bloquear->estado = BLOCKED; 

    sem_post(&(liberar_CPU[carpincho_a_bloquear->CPU_en_uso])); 

    sem_post(&sem_hay_bloqueados);
    sem_post(&hay_bloqueados_para_deadlock);

    log_info(logger, "El carpincho %d paso a BLOCKED", carpincho_a_bloquear->id);
}

void exec_a_exit(int id_carpincho, int fd){
    
    void *payload;

    data_carpincho *carpincho_que_termino;
    carpincho_que_termino = encontrar_estructura_segun_id(id_carpincho);

    bool es_el_mismo_carpincho(data_carpincho* carpincho, data_carpincho* carpincho_que_termino){
        return carpincho->id == carpincho_que_termino->id;
    }

    bool es_el_mismo(void* carpincho){
        data_carpincho* aux = (data_carpincho*)carpincho; 
        return es_el_mismo_carpincho(aux,carpincho_que_termino);
    }

    payload = _serialize(sizeof(int), "%d", id_carpincho);

    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
    _send_message(socket_memoria, ID_KERNEL, MATE_CLOSE, payload, sizeof(int), logger); 

    t_mensaje *buffer;
    int respuesta_memoria;
    buffer = _receive_message(socket_memoria, logger);
    memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

    close(socket_memoria);

    pthread_mutex_lock(&sem_cola_exec); 
    list_remove_by_condition(exec, es_el_mismo);
	pthread_mutex_unlock(&sem_cola_exec);

    log_info(logger, "El carpincho %d paso a EXIT", carpincho_que_termino->id);

    sem_post(&(liberar_CPU[carpincho_que_termino->CPU_en_uso])); 
    sem_post(&sem_grado_multiprogramacion_libre);

    _send_message(fd, ID_KERNEL, 1, 0, sizeof(int), logger); 
}

bool sonElMismoC(data_carpincho *carpincho_lista, data_carpincho * carpincho_a_ready){
    return carpincho_lista->id == carpincho_a_ready->id;
}

void block_a_ready(data_carpincho *carpincho_a_ready){ 

    bool es_el_mismo(void* carpincho){
            return ((data_carpincho *) carpincho)->id == carpincho_a_ready->id;
        }

    pthread_mutex_lock(&sem_cola_ready); 
    pthread_mutex_lock(&sem_cola_blocked);

    list_add(ready, carpincho_a_ready);
    list_remove_by_condition(blocked, es_el_mismo);

    pthread_mutex_unlock(&sem_cola_blocked);
    pthread_mutex_unlock(&sem_cola_ready);

    log_info(logger, "El carpincho %d paso a READY", carpincho_a_ready->id);

    carpincho_a_ready->estado = READY;
    sem_post(&cola_ready_con_elementos);

    sem_wait(&sem_hay_bloqueados);

    // no tiene que evaluar ni grado de multiprogramacion ni de multiproc porque ya estaba considerado.
    // no responde nada a la lib porque todavia no esta en exec, solo paso a ready
}

void suspended_blocked_a_suspended_ready(data_carpincho *carpincho){

    bool esIgualACarpincho (void* carpincho_lista){
       return ((data_carpincho *) carpincho_lista)->id == carpincho->id;
    }   

    pthread_mutex_lock(&sem_cola_suspended_ready);  
    pthread_mutex_lock(&sem_cola_suspended_blocked);

    queue_push(suspended_ready, (void*)carpincho);
    list_remove_by_condition(suspended_blocked, esIgualACarpincho);

    pthread_mutex_unlock(&sem_cola_suspended_blocked);
    pthread_mutex_unlock(&sem_cola_suspended_ready);

    log_info(logger, "El carpincho %d paso a SUSPENDED READY", carpincho->id);

    carpincho->estado = SUSPENDED_READY; 
    sem_post(&hay_estructura_creada);

}

void suspender(){
    //void *payload;
    while(1){

        sem_wait(&sem_procesamiento_lleno);
        sem_wait(&sem_programacion_lleno);
        sem_wait(&sem_hay_bloqueados);

        data_carpincho *carpincho_a_suspender; 

        if(estan_las_condiciones_para_suspender()){

            int longitud = list_size(blocked);

            pthread_mutex_lock(&sem_cola_blocked);
            pthread_mutex_lock(&sem_cola_suspended_blocked);

            carpincho_a_suspender = list_remove(blocked, longitud); //retorna y remueve el ultimo de bloqueados
            list_add(suspended_blocked, carpincho_a_suspender);
            
            pthread_mutex_unlock(&sem_cola_blocked);
            pthread_mutex_unlock(&sem_cola_suspended_blocked);

            carpincho_a_suspender->estado = SUSPENDED_BLOCKED;
            log_info(logger, "El carpincho %d paso a SUSPENDED BLOCKED", carpincho_a_suspender->id);
            sem_post(&liberar_CPU[carpincho_a_suspender->CPU_en_uso]); //--> esta bien que sea el de liberar CPU?
        
            /* 
            payload = _serialize(sizeof(int), "%d", carpincho_a_suspender->id);
            // hacer: corregir lo de socket_memoria
            _send_message(socket_memoria, ID_KERNEL, SUSPENDER, payload, sizeof(int), logger);  //avisar a mem para que libere
            */

            sem_post(&sem_grado_multiprocesamiento_libre);
            sem_post(&sem_grado_multiprogramacion_libre);  
        }
    }
     
}

bool estan_las_condiciones_para_suspender(){
    int valor1 = sem_getvalue(&sem_grado_multiprogramacion_libre, &valor1);
    int valor2 = sem_getvalue(&hay_estructura_creada, &valor2);
    
    return valor1 == 0 && !list_is_empty(blocked) && valor2 > 0;
}

////////////////////////// ALGORITMOS ////////////////////////

data_carpincho* ready_a_exec_SJF(){
  
    float min_hasta_el_momento = 0;
    
    t_list_iterator* list_iterator = list_iterator_create(ready);
    data_carpincho* carpincho_menor;
    while (list_iterator_has_next(list_iterator)) {
        data_carpincho* carpincho_actual = list_iterator_next(list_iterator);
        calculo_estimacion_siguiente(carpincho_actual);
        float estimacion_actual = carpincho_actual->estimacion_siguiente; 
            if(min_hasta_el_momento == 0){
                carpincho_menor = carpincho_actual;
                min_hasta_el_momento = estimacion_actual;
            }
            else{
                if(estimacion_actual < min_hasta_el_momento){   
                    min_hasta_el_momento = estimacion_actual;
                    carpincho_menor = carpincho_actual;
                }
            }           
    }
    list_iterator_destroy(list_iterator);

    log_info(logger, "SJF elige al carpincho %d ", carpincho_menor->id);
    return  carpincho_menor;        
}

data_carpincho* ready_a_exec_HRRN(){ 

   float max_hasta_el_momento = 0;
   t_list_iterator* list_iterator = list_iterator_create(ready);
    data_carpincho* carpincho_mayor;
    while (list_iterator_has_next(list_iterator)) {
        data_carpincho* carpincho_actual = list_iterator_next(list_iterator);
        calculo_RR(carpincho_actual);
        float RR_actual = carpincho_actual->RR;
        if(max_hasta_el_momento == 0){
                carpincho_mayor = carpincho_actual;
                max_hasta_el_momento = RR_actual;
            }
            else{
                if(RR_actual > max_hasta_el_momento){   
                    max_hasta_el_momento = RR_actual;
                    carpincho_mayor = carpincho_actual;
                }
            }          
    }
    list_iterator_destroy(list_iterator);
    log_info(logger, "HRRN elige al carpincho %d ", carpincho_mayor->id);
    return carpincho_mayor;     
}


void calculo_estimacion_siguiente(data_carpincho *carpincho){ 
    calculo_rafaga_anterior(carpincho);
    float alfa = (float) config_get_double_value(config, "ALFA"); // hacer: ver que alfa no es un valor entero entonces la funcion get_int_value no lo va a entender
    carpincho->estimacion_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * (1 - alfa); // gonza -> invalid operands to binary * (have ‘float *’ and ‘int’) (le agregue *)
}

void calculo_rafaga_anterior(data_carpincho *carpincho){ // para SJF

    int tiempo_salida = calcular_milisegundos();

    carpincho->rafaga_anterior = tiempo_salida - carpincho->tiempo_entrada_a_exec; 
}

void calculo_RR(data_carpincho *carpincho){ // para HRRN

    char ahora = calcular_milisegundos();

    float espera = ahora - carpincho->llegada_a_ready;
    calculo_estimacion_siguiente(carpincho); 
    float prox_rafaga =  carpincho->estimacion_siguiente;

    carpincho->RR = 1 + espera/prox_rafaga;
}

int calcular_milisegundos(){

    char* tiempo_sacado = temporal_get_string_time("%M:%S:%MS");
    
    char** tiempo_formateado = string_split(tiempo_sacado,":");

    tiempo tiempo_calculado; 

    tiempo_calculado.minutos = atoi(tiempo_formateado[0]);
    tiempo_calculado.segundos = atoi(tiempo_formateado[1]);
    tiempo_calculado.milisegundos = atoi(tiempo_formateado[2]);

    return tiempo_calculado.minutos * 60000 + tiempo_calculado.segundos * 60 + tiempo_calculado.milisegundos;
}


//////////////////////////// Funciones para exec ///////////////////////////////////

void asignar_hilo_CPU(data_carpincho *carpincho){

    int *id_CPU_disponible;

    pthread_mutex_lock(&sem_CPU_libres);
    id_CPU_disponible = (int *) queue_peek(CPU_libres);
    queue_pop(CPU_libres);
    pthread_mutex_unlock(&sem_CPU_libres);

    sem_post(&(usar_CPU[*id_CPU_disponible]));

    carpincho->CPU_en_uso = *id_CPU_disponible;

}

void ejecuta(void *id_cpu){ 

    while(1){

        int *id = (int *) id_cpu;
    
        pthread_mutex_lock(&mutex_para_CPU); 

        sem_wait(&usar_CPU[*id]); // espera hasta que algun carpincho haga post para usar ese cpu
        sem_wait(&CPU_libre[*id]); // ya no está más libre ese cpu

        pthread_mutex_unlock(&mutex_para_CPU); 
        
        sem_wait(&liberar_CPU[*id]); // espera a que algun carpincho indique que quiere liberar el cpu
        queue_push(CPU_libres, id);
        
        pthread_mutex_lock(&mutex_para_CPU); 
        sem_post(&CPU_libre[*id]); 
        sem_post(&sem_grado_multiprocesamiento_libre); //hay algun cpu libre
        pthread_mutex_lock(&mutex_para_CPU); 
    }
}

void handler( int fd, char* id, int opcode, void* payload, t_log* logger){
    
    log_info(logger, "Kernel, mensaje recibido");
    data_carpincho* estructura_interna = deserializar(payload);
    int id_carpincho;
    int size_memoria;
    int addr_memfree;
    int origin_memread;
    void *origin_memwrite;
    int dest_memwrite;
    int offset = 0;
    int ptr_len = 0;

    if(strcmp(id, ID_MATE_LIB) == 0){ 
        switch(opcode){
            case MATE_INIT:
                mate_init(fd);
            break;
            case MATE_CLOSE: 
                mate_close(estructura_interna->id,fd); 
            break;
            case MATE_SEM_INIT: 
                mate_sem_init(estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo, fd);            
            break;
            case MATE_SEM_WAIT: 
                mate_sem_wait(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
            case MATE_SEM_POST: 
                mate_sem_post(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
            case MATE_SEM_DESTROY:
                mate_sem_destroy(estructura_interna->id, estructura_interna->semaforo, fd);            
            break;
            case MATE_CALL_IO:
                mate_call_io(estructura_interna->id, estructura_interna->dispositivo_io, fd);  
            break;       
            case MATE_MEMALLOC: 
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload, sizeof(int));
                offset += sizeof(int);

                mate_memalloc(id_carpincho, size_memoria, fd);      
            break;      
            case MATE_MEMFREE:
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // addr_memfree
                memcpy(&addr_memfree, payload, sizeof(int));
                offset += sizeof(int);

                mate_memfree(id_carpincho, addr_memfree, fd);      
            break;      
            case MATE_MEMREAD: 
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // origin_memread
                memcpy(&origin_memread, payload, sizeof(int));
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload, sizeof(int));
                offset += sizeof(int);

                mate_memread(id_carpincho, origin_memread, size_memoria, fd);            
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
                memcpy(&dest_memwrite, payload, sizeof(int));
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload, sizeof(int));
                offset += sizeof(int);

                mate_memwrite(id_carpincho, origin_memwrite, dest_memwrite, size_memoria, fd);     
            break;  
        }
    }
}


////////////////////////////// DEADLOCK ////////////////////////////////


void agregando_a_lista_posible_deadlock(){

    lista_posibles = list_create();
    lista_conectados = list_create();
            
    for(int i= 0; i< list_size(lista_carpinchos); i++){ 
        data_carpincho *carpincho = list_get(lista_carpinchos,i);

        bool cumple_estado = carpincho->estado==BLOCKED || carpincho->estado==SUSPENDED_BLOCKED; // los carpinchos pueden estar en bloq suspended tamb para ser un posible?
        bool cumple_retencion = !list_is_empty(carpincho->semaforos_retenidos);
        bool cumple_bloqueo = carpincho->nombre_semaforo_por_el_que_se_bloqueo !=NULL; //cuando se desbloquee cambiarlo a null
        if(cumple_estado && cumple_retencion && cumple_bloqueo){
            list_add(lista_posibles, carpincho);
        }
    }

}


void detectar_deadlock(){

    int tiempo_deadlock = config_get_int_value(config, "TIEMPO_DEADLOCK");
    data_carpincho *c1;
    data_carpincho *c2;
    while(1){
        sem_wait(&hay_bloqueados_para_deadlock); 
        sleep(tiempo_deadlock);

        agregando_a_lista_posible_deadlock();

        for(int i= 0; i< list_size(lista_posibles); i++){
            c1 = list_get(lista_posibles, i);
            for(int k= 0; k< list_size(lista_posibles); k++){
                
                c2 = list_get(lista_posibles, k);

                bool mismo_que_c2_espera(void* nombre_sem_retenido_c1){
                    return strcmp(nombre_sem_retenido_c1, c2->nombre_semaforo_por_el_que_se_bloqueo) ==0;
                }

                if(i!=k){
                    if(list_any_satisfy(c1->semaforos_retenidos, mismo_que_c2_espera)){
                        c2->tiene_su_espera = c1->id;
                        list_add(lista_conectados,c2);
                    }
                }       
            }    
        }
    }

    if(formar_ciclo()){
        log_info(logger, "Hay DEADLOCK");
        solucionar_deadlock();
    }

    list_clean(lista_posibles);
    list_clean(lista_conectados);
        
}

bool formar_ciclo(){

        int id_base = 0;
        data_carpincho *carpincho_base;
        int carpincho_C;
        data_carpincho *carpincho_B;
        data_carpincho *carpincho_A;


    while( id_base != (list_size(lista_conectados)-1)){  

        carpincho_C = id_base;

        bool esIgualACarpincho_A(void *id){
            return (int)id == carpincho_A->id;
        }
        bool esIgualACarpincho_B(void *id){
            return (int)id == carpincho_B->id;
        }
        bool esIgualACarpincho_C(void *id){
            return (int)id == carpincho_C;
        }

        for(int i = 0; i < list_size(lista_conectados); i++){

            carpincho_base = list_get(lista_conectados, id_base); 
            carpincho_A = list_get(lista_conectados, carpincho_C);  
            carpincho_B = encontrar_estructura_segun_id(carpincho_A->tiene_su_espera); 

            carpincho_C = carpincho_B->tiene_su_espera;  

            if(!list_any_satisfy(ciclo_deadlock, (void *)esIgualACarpincho_A)){
                list_add(ciclo_deadlock,(void*)carpincho_A->id);   
            }
            if(!list_any_satisfy(ciclo_deadlock, (void *)esIgualACarpincho_B)){
                list_add(ciclo_deadlock,(void*)carpincho_B->id);
            }     
            if(!list_any_satisfy(ciclo_deadlock, (void *)esIgualACarpincho_C)){
                list_add(ciclo_deadlock,(void*)carpincho_C);
            }
                        
            if(carpincho_base->id == carpincho_C || carpincho_base->tiene_su_espera == carpincho_C){
                return true;               
            }
        }

        list_clean(ciclo_deadlock);
        id_base ++;

    }

    return false;
}

void solucionar_deadlock(){

    int mayor_id_hasta_ahora = 0;
    int id_actual;

    for(int i= 0; i< list_size(ciclo_deadlock); i++){
        
        data_carpincho *carpincho = list_get(ciclo_deadlock, i);
        id_actual = carpincho->id;
        if(id_actual > mayor_id_hasta_ahora){    
            mayor_id_hasta_ahora = id_actual;
        }
    }

    data_carpincho *carpincho_a_eliminar = encontrar_estructura_segun_id(mayor_id_hasta_ahora);

    log_info(logger, "Se elimina al carpincho %d para solucionar deadlock", carpincho_a_eliminar->id);
    mate_close(mayor_id_hasta_ahora, carpincho_a_eliminar->fd);

    for(int i=0; i<list_size(carpincho_a_eliminar->semaforos_retenidos); i++){ 
        sem_t *semaforo = (sem_t *) list_get((carpincho_a_eliminar->semaforos_retenidos), i);
        sem_post(&(*semaforo));
    }

}
