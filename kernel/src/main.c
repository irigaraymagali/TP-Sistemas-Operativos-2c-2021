#include "main.h"

int main(int argc, char ** argv){

    char* config_path;
    if (argc > 1 && !string_is_empty(argv[1])){
        config_path = argv[1];
    } else {
        config_path = CONFIG_PATH;
    }

    logger = log_create("./cfg/kernel.log", "[Kernel]", true, LOG_LEVEL_INFO);
    config = config_create(config_path);
    if (config == NULL){
        log_error(logger, "Error Al intentar abrir el archivo de Config: Revisar PATH %s", config_path);
        log_destroy(logger);
        return EXIT_FAILURE;
    }

    id_carpincho_global = 1; 
    pthread_mutex_init(&id_carpincho_mutex, NULL);

    lista_carpinchos = list_create(); // crear lista para ir guardando los carpinchos
    semaforos_carpinchos = list_create(); // crear lista para ir guardando los semaforos
    lista_dispositivos_io = list_create(); // crear lista para ir guardando los dispositivios io

    el_ciclo = list_create();

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
    pthread_create(&deteccion_deadlock, NULL, (void*) detectate_deadlock, NULL);

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

    lista_posibles = list_create();
    lista_conectados = list_create();

    pthread_mutex_init(&mutex_para_CPU, NULL);
    pthread_mutex_init(&sem_io_uso, NULL);
    pthread_mutex_init(&sem_cola_io, NULL);

    pthread_mutex_init(&mutex_para_posibles_deadlock, NULL);

}

void inicializar_semaforos(){ 
    int grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    int grado_multiprocesamiento = config_get_int_value(config, "GRADO_MULTIPROCESAMIENTO");

    sem_init(&sem_grado_multiprogramacion_libre,1,grado_multiprogramacion);  
	sem_init(&sem_grado_multiprocesamiento_libre, 1,grado_multiprocesamiento); 
    sem_init(&hay_estructura_creada,1,0);
    sem_init(&cola_ready_con_elementos,1,0);
    sem_init(&cola_exec_con_elementos,1,0);
    sem_init(&cola_blocked_con_elementos,1,0);
    sem_init(&cola_suspended_blocked_con_elementos,1,0);
    sem_init(&cola_suspended_ready_con_elementos,1,0); 
    sem_init(&sem_programacion_lleno,1,0);
    sem_init(&sem_procesamiento_lleno,1,0);
    sem_init(&sem_hay_bloqueados,1,0);
    sem_init(&segui_chequeando_deadlock,1,0);
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

	}
}


void free_memory(){

    config_destroy(config);     
    log_destroy(logger);

    list_destroy_and_destroy_elements(lista_carpinchos, liberar_carpincho); 
    list_destroy_and_destroy_elements(semaforos_carpinchos, liberar_semaforo);
    list_destroy(ready);
    list_destroy(exec);
    list_destroy(blocked);
    list_destroy(suspended_blocked);
    list_destroy_and_destroy_elements(lista_dispositivos_io,liberar_dispositivo);
    list_destroy(lista_posibles);
    list_destroy(lista_conectados);
    list_destroy(el_ciclo);

    queue_destroy(new);
    queue_destroy(suspended_ready);
    queue_destroy(CPU_libres);

    sem_destroy(&sem_grado_multiprogramacion_libre);  
    sem_destroy(&sem_grado_multiprocesamiento_libre); 
    sem_destroy(&hay_estructura_creada);
    sem_destroy(&cola_ready_con_elementos);
    sem_destroy(&cola_exec_con_elementos);
    sem_destroy(&cola_blocked_con_elementos);
    sem_destroy(&cola_suspended_blocked_con_elementos);
    sem_destroy(&cola_suspended_ready_con_elementos); 
    sem_destroy(&sem_programacion_lleno); 
    sem_destroy(&sem_procesamiento_lleno); 
    sem_destroy(&sem_hay_bloqueados); 
    sem_destroy(&hay_carpinchos_pidiendo_io); 
    sem_destroy(&segui_chequeando_deadlock);

    pthread_mutex_destroy(&sem_cola_new);
    pthread_mutex_destroy(&sem_cola_ready);
    pthread_mutex_destroy(&sem_cola_exec);
    pthread_mutex_destroy(&sem_cola_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_blocked);
    pthread_mutex_destroy(&sem_cola_suspended_ready);
    pthread_mutex_destroy(&mutex_para_CPU);
    pthread_mutex_destroy(&sem_cola_exit);
    pthread_mutex_destroy(&sem_cola_io);
    pthread_mutex_destroy(&sem_io_uso);
    pthread_mutex_destroy(&id_carpincho_mutex);

    pthread_mutex_destroy(&mutex_para_posibles_deadlock);

    exit(EXIT_SUCCESS);
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
    carpincho_encontrado = (data_carpincho *) list_find(lista_carpinchos, buscar_id);

    return carpincho_encontrado;
}

semaforo* encontrar_estructura_semaforo(int id){
    
    bool id_pertenece_al_semaforo(int id, semaforo *sem){
        return sem->id == id; 
    }

    bool buscar_id(void * sem){
        return id_pertenece_al_semaforo(id, (semaforo *) sem);
    }

    semaforo *semaforo_encontrado;
    semaforo_encontrado = (semaforo *) list_find(semaforos_carpinchos, buscar_id);

    return semaforo_encontrado;
}

///////////////// RECIBIR MENSAJES //////////////////////// 

data_carpincho* deserializar(void* buffer){

    int sem_len, io_len;
    int offset = 0;
    int id;
    memcpy(&id, buffer, sizeof(int));
    offset += sizeof(int); 

if(validar_existencia_carpincho(id)){
    data_carpincho* estructura_interna = encontrar_estructura_segun_id(id);

    memcpy(&sem_len, buffer + offset, sizeof(int));
    offset += sizeof(int);

    free(estructura_interna->semaforo);
    
    estructura_interna->semaforo = malloc(sem_len);
    memcpy(estructura_interna->semaforo, buffer + offset, sem_len);
    offset += sem_len;
    estructura_interna->semaforo[sem_len] = '\0';

    memcpy(&estructura_interna->valor_semaforo, buffer + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(&io_len, buffer + offset, sizeof(int));
    offset += sizeof(int);

    free(estructura_interna->dispositivo_io);
    estructura_interna->dispositivo_io = malloc(io_len);
    memcpy(estructura_interna->dispositivo_io, buffer + offset, io_len);
    estructura_interna->dispositivo_io[io_len] = '\0';

    return estructura_interna;
   }else{
       return NULL;
   }
} 


//////////////// FUNCIONES GENERALES ///////////////////

void mate_init(int fd){

    data_carpincho *carpincho;
    carpincho = malloc(sizeof(data_carpincho));

    pthread_mutex_lock(&id_carpincho_mutex);
    carpincho->id = id_carpincho_global;
    id_carpincho_global ++;  
    pthread_mutex_unlock(&id_carpincho_mutex);

    carpincho->rafaga_anterior = 0;
    carpincho->estimacion_anterior = 0;
    carpincho->estimacion_siguiente = config_get_int_value(config, "ESTIMACION_INICIAL");
    carpincho->llegada_a_ready = 0;
    carpincho->RR = 0;
    carpincho->estado = NEW;
    carpincho->CPU_en_uso = 9999;
    carpincho->tiempo_entrada_a_exec = 0;
    carpincho->tiempo_salida_a_exec = 0;
    carpincho->fd = fd; 
    carpincho->semaforo = string_new(); 
    carpincho->valor_semaforo = 0;
    carpincho->dispositivo_io = string_new(); 
    carpincho->nombre_semaforo_por_el_que_se_bloqueo = string_new();
    carpincho->tiene_su_espera = 0;
    carpincho->sem_retenido = 0;

    log_info(logger, "CARPINCHO %d - hizo un INIT", carpincho->id);

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
        memcpy(&respuesta_memoria, buffer->payload, sizeof(int));
        
        close(socket_memoria); 
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }
    else{
        log_error(logger, "No se ha podido conectar con el módulo memoria");
        close(socket_memoria);
    }

    if(respuesta_memoria >= 0){
        log_info(logger, "CARPINCHO %d - ha sido creado correctamente en memoria", carpincho->id);
        list_add(lista_carpinchos, (void *) carpincho);
        queue_push(new, (void *)carpincho);
        sem_post(&hay_estructura_creada);
    }
    else{
        log_error(logger, "CARPINCHO %d - Memoria no pudo crear la estructura", carpincho->id);
        free(carpincho);
    }
    free(payload);
}

void mate_close(int id_carpincho, int fd){

    log_info(logger, "CARPINCHO %d - hizo un CLOSE", id_carpincho);
    
    data_carpincho *carpincho_a_cerrar;
    carpincho_a_cerrar = encontrar_estructura_segun_id(id_carpincho);

    carpincho_a_cerrar->fd = fd;

    bool es_el_carpincho(void* carpincho){
        return ((data_carpincho *)carpincho)->id == carpincho_a_cerrar->id;
    }

    if(carpincho_a_cerrar->estado == EXEC){
        exec_a_exit(id_carpincho, fd); 
    }
    else if(carpincho_a_cerrar->estado == BLOCKED)  // porque hay deadlock
    {      
        void *payload;
        payload = _serialize(sizeof(int), "%d", id_carpincho);

        int socket_memoria;
        socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
        _send_message(socket_memoria, ID_KERNEL, MATE_CLOSE, payload, sizeof(int), logger); 

        t_mensaje *buffer;
        int respuesta_memoria;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        close(socket_memoria);

        pthread_mutex_lock(&sem_cola_blocked);
        list_remove_by_condition(blocked, es_el_carpincho);       
        pthread_mutex_unlock(&sem_cola_blocked);

        list_remove_and_destroy_by_condition(lista_carpinchos, es_el_carpincho, liberar_carpincho);

        sem_post(&sem_grado_multiprogramacion_libre);

        void* payload2 = _serialize(sizeof(int), "%d", CERRADO_POR_DEADLOCK);

        _send_message(fd, ID_KERNEL, MATE_CLOSE, payload2, sizeof(int), logger);
        free(payload);
        free(payload2);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);

    }
    else{ 

        void *payload;
        payload = _serialize(sizeof(int), "%d", id_carpincho);

        int socket_memoria;
        socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
        _send_message(socket_memoria, ID_KERNEL, MATE_CLOSE, payload, sizeof(int), logger); 

        t_mensaje *buffer;
        int respuesta_memoria;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        close(socket_memoria);
        
        pthread_mutex_lock(&sem_cola_suspended_blocked);     
        list_remove_by_condition(suspended_blocked, es_el_carpincho); 
        pthread_mutex_unlock(&sem_cola_suspended_blocked);

        list_remove_and_destroy_by_condition(lista_carpinchos, es_el_carpincho, liberar_carpincho);

        sem_post(&sem_grado_multiprogramacion_libre);

        payload = _serialize(sizeof(int), "%d", CERRADO_POR_DEADLOCK);

        _send_message(fd, ID_KERNEL, MATE_CLOSE, payload, sizeof(int), logger);
        free(payload);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }

    log_info(logger, "CARPINCHO %d - La estructura ha sido eliminada correctamente", id_carpincho);

}

void liberar_carpincho(void *carpincho){
    
    data_carpincho * aux = (data_carpincho *) carpincho;
    if(aux != NULL){
        
        if(aux->semaforo != NULL){
            free(aux->semaforo);
        }
        if(aux->nombre_semaforo_por_el_que_se_bloqueo != NULL){
            free(aux->nombre_semaforo_por_el_que_se_bloqueo);
        }
        if(aux->dispositivo_io != NULL){
            free(aux->dispositivo_io);
        }
        free(aux);
    }
}

void liberar_dispositivo(void *dispositivo){
    
    dispositivo_io * aux = (dispositivo_io *) dispositivo;
    if(aux != NULL){
        free(aux->nombre);
        queue_destroy(aux->en_espera);
        free(aux);
    }
}

void liberar_semaforo(void *semaforo_a_borrar){
   
    semaforo* otro;
    otro = (semaforo *) semaforo_a_borrar;
    if(otro != NULL){
        free(otro->nombre);
        queue_destroy(otro->en_espera);
        free(otro);
    }
}

//////////////// FUNCIONES SEMAFOROS ///////////////////

bool esIgualASemaforo(char* nombre_semaforo, void *semaforo_igual){
    semaforo* sem = (semaforo *) semaforo_igual;
    return string_equals_ignore_case(sem->nombre, nombre_semaforo);
}

void mate_sem_init(int id_carpincho, char * nombre_semaforo, int valor_semaforo, int fd){  

    log_info(logger, "CARPINCHO %d - hizo un SEM INIT", id_carpincho);

    bool esIgualA(void *semaforo_igual){
        return esIgualASemaforo(nombre_semaforo, semaforo_igual);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  
        void  *payload;
        payload = _serialize(sizeof(int), "%d", -1);
        log_error(logger, "CARPINCHO %d - El semaforo %s que intentó inicializar ya estaba incializado", id_carpincho, nombre_semaforo);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        free(payload);
    }
    else {
        semaforo* semaforo_nuevo = malloc(sizeof(semaforo));     
        
        void *payload = _serialize(sizeof(int), "%d", 0);  
        log_info(logger, "CARPINCHO %d - pidió inicializar el semáforo %s", id_carpincho, nombre_semaforo);   

        semaforo_nuevo->nombre = string_from_format("%s",nombre_semaforo);
        semaforo_nuevo->valor = valor_semaforo;
        semaforo_nuevo->en_espera = queue_create();
        semaforo_nuevo->id = id_semaforos;

        id_semaforos++;

        list_add(semaforos_carpinchos, (void *) semaforo_nuevo);

        log_info(logger, "CARPINCHO %d - el semáforo %s se inicializó correctamente", id_carpincho, nombre_semaforo);   
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger);     
        free(payload);    
    }
}

void mate_sem_wait(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(nombre_semaforo, semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){ 

        semaforo *semaforo_wait;
        semaforo_wait = (semaforo *)list_find(semaforos_carpinchos, esIgualA);
        semaforo_wait->valor --; 

        data_carpincho *carpincho;
        carpincho = encontrar_estructura_segun_id(id_carpincho);
        carpincho->fd = fd;

        if(semaforo_wait->valor < 0){
            log_info(logger, "CARPINCHO %d - hizo un WAIT del semaforo %s que tenía valor menor a 0, se bloqueará el carpincho", id_carpincho, (char *) nombre_semaforo);
            free(carpincho->nombre_semaforo_por_el_que_se_bloqueo);
            carpincho->nombre_semaforo_por_el_que_se_bloqueo = string_from_format("%s", (char *)nombre_semaforo);
            carpincho->sem_por_el_que_se_bloqueo = semaforo_wait->id;
            queue_push(semaforo_wait->en_espera, carpincho);
            printf("CARPINCHO %d - ha sido bloqueado por pedir el semáforo %s", id_carpincho, (char *)nombre_semaforo);
            exec_a_block(id_carpincho); 
        }
        else
        {
            carpincho->sem_retenido = semaforo_wait->id;
            void* payload;
            payload = _serialize(sizeof(int), "%d", 0);
            log_info(logger, "CARPINCHO %d - hizo un wait del semaforo %s que tiene valor mayor o igual a 1, puede seguir", id_carpincho, (char *) nombre_semaforo);        
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
            free(payload);
        }
    }
    else
    {
        void* payload;
        payload = _serialize(sizeof(int), "%d", -1);
        log_error(logger, "CARPINCHO %d - intentó hacer wait de un semaforo no inicializado", id_carpincho);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        free(payload);
    }

}

bool validar_existencia_carpincho(int otro_id){


    bool id_pertenece_al_carpincho(int id, data_carpincho *carpincho){
        return carpincho->id == id; 
    }

    bool buscar_id(void * carpincho){
        return id_pertenece_al_carpincho(otro_id, (data_carpincho * ) carpincho);
    }

    return list_any_satisfy(lista_carpinchos, buscar_id);
}




void mate_sem_post(int id_carpincho, mate_sem_name nombre_semaforo, int fd){

    if(fd!=-1){ 

        log_info(logger, "CARPINCHO %d - hizo un POST", id_carpincho);
        
        void *payload;
        int valor_previo_semaforo;

        bool esIgualA(void *semaforo){
            return esIgualASemaforo(nombre_semaforo, semaforo);
        }

        if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  

            semaforo *semaforo_post;
            semaforo_post = list_find(semaforos_carpinchos, esIgualA);
            valor_previo_semaforo = semaforo_post->valor;
            semaforo_post->valor ++; 

            log_info(logger, "CARPINCHO %d - se incrementó el valor del semáforo %s", id_carpincho, (char *) nombre_semaforo);

            payload = _serialize(sizeof(int), "%d", 0);

            int log_en_espera = queue_size(semaforo_post->en_espera);

            if(log_en_espera>0){
                if(semaforo_post->valor > -1){ 
                
                data_carpincho *carpincho_a_desbloquear;
                carpincho_a_desbloquear = (data_carpincho *) queue_peek(semaforo_post->en_espera);
                if(validar_existencia_carpincho(carpincho_a_desbloquear->id)){
                    log_info(logger,"INFO SEMÁFOROS - había carpinchos (como el %d) esperando el semaforo %s", carpincho_a_desbloquear->id, (char *)nombre_semaforo);
                    queue_pop(semaforo_post->en_espera);
                    carpincho_a_desbloquear->nombre_semaforo_por_el_que_se_bloqueo = NULL;
                    carpincho_a_desbloquear->sem_por_el_que_se_bloqueo = 0;
                    data_carpincho *carpincho_que_hizo_post = encontrar_estructura_segun_id(id_carpincho);

                    carpincho_que_hizo_post->sem_retenido=0;
			
                      if(carpincho_a_desbloquear->estado == BLOCKED){ 
                        block_a_ready(carpincho_a_desbloquear);
                    }
                    else{
                        carpincho_a_desbloquear->estado = SUSPENDED_READY;
                        suspended_blocked_a_suspended_ready(carpincho_a_desbloquear);
                    }
                }else{

                    queue_pop(semaforo_post->en_espera);

                    carpincho_a_desbloquear = (data_carpincho *) queue_peek(semaforo_post->en_espera);
                    if(validar_existencia_carpincho(carpincho_a_desbloquear->id)){
                        log_info(logger,"INFO SEMÁFOROS - había carpinchos (como el %d) esperando el semaforo %s", carpincho_a_desbloquear->id, (char *)nombre_semaforo);
                        queue_pop(semaforo_post->en_espera);
                        carpincho_a_desbloquear->nombre_semaforo_por_el_que_se_bloqueo = NULL;
                        carpincho_a_desbloquear->sem_por_el_que_se_bloqueo = 0;
                        data_carpincho *carpincho_que_hizo_post = encontrar_estructura_segun_id(id_carpincho);

                        carpincho_que_hizo_post->sem_retenido=0;

                        if(carpincho_a_desbloquear->estado == BLOCKED){ 
                            block_a_ready(carpincho_a_desbloquear);
                        }
                        else{
                            carpincho_a_desbloquear->estado = SUSPENDED_READY;
                            suspended_blocked_a_suspended_ready(carpincho_a_desbloquear);
                        }
                    }
                }
            }
            }
            log_info(logger, "CARPINCHO %d - el semáforo %s que tenía valor %d y cambió a %d", id_carpincho, nombre_semaforo, valor_previo_semaforo, semaforo_post->valor);
            
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger);
        }
        else
        {
            payload = _serialize(sizeof(int), "%d", -1);
            log_error(logger, "CARPINCHO %d - intentó hacer post de un semaforo no inicializado", id_carpincho);
            _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        }
        free(payload);
    }
    else{
                log_info(logger, "DEADLOCK - se hizo un POST");
            
                int valor_previo_semaforo;

                bool esIgualA(void *semaforo){
                    return esIgualASemaforo(nombre_semaforo, semaforo);
                }

                semaforo *semaforo_post;
                semaforo_post = list_find(semaforos_carpinchos, esIgualA);
                valor_previo_semaforo = semaforo_post->valor;
                semaforo_post->valor ++; 

                log_info(logger, "DEADLOCK - se incrementó el valor del semáforo %s ya que carpincho %d lo tenia retenido",(char *) nombre_semaforo, id_carpincho);

                if(!queue_is_empty(semaforo_post->en_espera)&& semaforo_post->valor >= 0){
                    data_carpincho *carpincho_a_desbloquear;
                    carpincho_a_desbloquear = (data_carpincho *) queue_peek(semaforo_post->en_espera);

                    if(validar_existencia_carpincho(carpincho_a_desbloquear->id)){
                        log_info(logger,"INFO SEMÁFOROS - había carpinchos (como el %d) esperando el semaforo %s", carpincho_a_desbloquear->id, (char *)nombre_semaforo);
                        queue_pop(semaforo_post->en_espera);
                        carpincho_a_desbloquear->nombre_semaforo_por_el_que_se_bloqueo = NULL;
                        carpincho_a_desbloquear->sem_por_el_que_se_bloqueo = 0;

                        if(carpincho_a_desbloquear->estado == BLOCKED){ 
                            block_a_ready(carpincho_a_desbloquear);
                        }
                        else{
                            carpincho_a_desbloquear->estado = SUSPENDED_READY;
                            suspended_blocked_a_suspended_ready(carpincho_a_desbloquear);
                        }
                    }else{

                        queue_pop(semaforo_post->en_espera);
                    }
                }
                log_info(logger, "DEADLOCK - el semáforo %s que tenía valor %d y cambió a %d",nombre_semaforo, valor_previo_semaforo, semaforo_post->valor);    
        
    }

}

void mate_sem_destroy(int id_carpincho, mate_sem_name nombre_semaforo, int fd) {

    log_info(logger, "CARPINCHO %d - hizo un SEM DESTROY", id_carpincho);
    void* payload; 

    bool esIgualA(void *semaforo){
        return esIgualASemaforo(nombre_semaforo, semaforo);
    }

    if(list_any_satisfy(semaforos_carpinchos, esIgualA)){  
        
        semaforo *semaforo_destroy = list_find(semaforos_carpinchos,esIgualA);

        queue_clean(semaforo_destroy->en_espera);
       
        list_remove_and_destroy_by_condition(semaforos_carpinchos,esIgualA,liberar_semaforo);
        payload = _serialize(sizeof(int), "%d", 0);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        log_info(logger, "CARPINCHO %d - destruyó el semáforo %s correctamente", id_carpincho, nombre_semaforo);
    } 
    else
    {
        log_error(logger, "CARPINCHO %d - intentó borrar un semaforo no inicializado", id_carpincho);
        payload = _serialize(sizeof(int), "%d", -1);
        _send_message(fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
    }
    free(payload);
}

//////////////// FUNCIONES IO ///////////////////

bool es_igual_dispositivo(mate_io_resource nombre_dispositivo, void *dispositivo) {
    dispositivo_io* aux = (dispositivo_io*) dispositivo; 
    return string_equals_ignore_case(aux->nombre, (char*) nombre_dispositivo);
} 

void mate_call_io(int id_carpincho, mate_io_resource nombre_io, int fd){

    char *nombre_dispositivo = (char *) nombre_io;

    log_info(logger, "CARPINCHO %d - hizo MATE CALL IO", id_carpincho);
    
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);

    carpincho->fd = fd;

    exec_a_block(id_carpincho);
	
    bool es_igual_a_dispositivo(void* dispositivo_recibido) {
        dispositivo_io* dispositivo;
        dispositivo = (dispositivo_io *) dispositivo_recibido; 
        return string_equals_ignore_case(dispositivo->nombre, nombre_dispositivo);
    }
    dispositivo_io* encontrar_dispositivo(char* nombre_dispositivo) {
            dispositivo_io* dispositivo_encontrado;
            dispositivo_encontrado = (dispositivo_io *) list_find(lista_dispositivos_io, es_igual_a_dispositivo);
            return dispositivo_encontrado;
    }

    dispositivo_io *dispositivo_pedido;
    dispositivo_pedido = encontrar_dispositivo(nombre_dispositivo);


    if(dispositivo_pedido->en_uso == 1){
        log_info(logger, "CARPINCHO %d - el dispositivo %s pedido está en uso, queda a la espera de que lo liberen", carpincho->id, (char *)nombre_io);     
        pthread_mutex_lock(&sem_cola_io);
        queue_push(dispositivo_pedido->en_espera, (void *)carpincho);  
        pthread_mutex_unlock(&sem_cola_io);
    }

    sem_wait(&dispositivo_sem[dispositivo_pedido->id]);

        dispositivo_pedido->en_uso = true;
        log_info(logger, "CARPINCHO %d - se le da el dispositivo %s",carpincho->id, (char*)nombre_io);
        sleep((dispositivo_pedido->duracion)/1000);
        log_info(logger, "CARPINCHO %d - terminó de usar el dispositivo %s",carpincho->id, (char*)nombre_io);

    sem_post(&dispositivo_sem[dispositivo_pedido->id]);
    
    dispositivo_pedido->en_uso = false;
        
    if(carpincho->estado == BLOCKED){
        block_a_ready(carpincho);
    }
    else{
        suspended_blocked_a_suspended_ready(carpincho);
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
            dispositivo->id=i;

            list_add(lista_dispositivos_io, dispositivo);

            sem_init(&(dispositivo_sem[i]), 0, 1);    
        }

   for (int i = 0; dispositivos_io[i] != NULL; i++)
            {
                free(dispositivos_io[i]);
            }
            free(dispositivos_io);

     for (int i = 0; duraciones_io[i] != NULL; i++)
            {
                free(duraciones_io[i]);
            }
            free(duraciones_io);
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
    log_info(logger, "CARPINCHO %d - hizo MATE MEMALLOC", id_carpincho);
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;
    void *payload = _serialize(sizeof(int) * 2, "%d%d", carpincho->id, size);
    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
    int respuesta_memoria;
    
    if(socket_memoria >= 0){
        t_mensaje *buffer;
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMALLOC, payload,sizeof(int)*2,logger);   
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria, buffer->payload, sizeof(int));

        log_info(logger, "CARPINCHO %d - el MEMALLOC fue realizado correctamente", id_carpincho);

        close(socket_memoria);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }
    else{
        respuesta_memoria = -1;
        log_error(logger, "No se ha podido conectar con el módulo memoria");
    }

    void* respuesta_memoria_serializada = _serialize(sizeof(int), "%d", respuesta_memoria);
    _send_message(fd, ID_KERNEL, 1, respuesta_memoria_serializada, sizeof(int), logger); 
    free(respuesta_memoria_serializada);
    free(payload);
}

void mate_memfree(int id_carpincho, mate_pointer addr, int fd){
    log_info(logger, "CARPINCHO %d - hizo MATE MEMFREE", id_carpincho);
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;
    void* payload = _serialize(sizeof(int) + sizeof(mate_pointer), "%d%d", carpincho->id, addr);

    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
    
    int respuesta_memoria;
    
    if(socket_memoria >= 0){
        _send_message(socket_memoria,  ID_KERNEL, MATE_MEMFREE,payload,sizeof(int) + sizeof(mate_pointer),logger);   
        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));
        
        log_info(logger, "CARPINCHO %d - el MEMFREE fue realizado correctamente", id_carpincho);

        close(socket_memoria);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }
    else{
        log_error(logger, "No se ha podido conectar con el módulo memoria");
        respuesta_memoria = -1;
    }

    void* respuesta_memoria_serializada = _serialize(sizeof(int), "%d", respuesta_memoria);
    _send_message(fd, ID_KERNEL, 1, respuesta_memoria_serializada, sizeof(int), logger); 
    free(respuesta_memoria_serializada);
    free(payload);
}

void mate_memread(int id_carpincho, mate_pointer origin, int size, int fd){ // martin => está bien lo que estamos retornando al carpincho?
    log_info(logger, "CARPINCHO %d - hizo MATE MEMREAD", id_carpincho);
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;

    void* payload = _serialize(sizeof(int) * 3, "%d%d%d", carpincho->id, origin, size);  
    t_mensaje *buffer;
    
    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);

    if(socket_memoria >= 0){
        int respuesta_memoria;
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMREAD,payload,sizeof(int) * 3,logger);   
        buffer = _receive_message(socket_memoria, logger);
        log_info(logger, "CARPINCHO %d - el MEMREAD fue realizado correctamente", id_carpincho);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));
        if(respuesta_memoria == -6){
            _send_message(fd, ID_KERNEL, MATE_MEMREAD, buffer->payload, sizeof(int), logger); 
        }else
        {
            _send_message(fd, ID_KERNEL, MATE_MEMREAD, buffer->payload, size, logger);
        }
        
        
        close(socket_memoria);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);

    }
    else{
        log_error(logger, "No se ha podido conectar con el módulo memoria");
        int respuesta_memoria = -6;
        void* respuesta_memoria_serializada = _serialize(sizeof(int), "%d", respuesta_memoria);
        _send_message(fd, ID_KERNEL, MATE_MEMREAD, respuesta_memoria_serializada, sizeof(int), logger); 
        free(respuesta_memoria_serializada);
    }
    free(payload);
}

void mate_memwrite(int id_carpincho, void* origin, mate_pointer dest, int size, int fd){
    log_info(logger, "CARPINCHO %d -  hizo MATE MEMWRITE", id_carpincho);
    data_carpincho *carpincho;
    carpincho = encontrar_estructura_segun_id(id_carpincho);
    carpincho->fd = fd;
    void* payload = _serialize(sizeof(int) * 3 + size, "%d%d%d%v", carpincho->id, dest, size, origin);

    int socket_memoria;
    socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);

    int respuesta_memoria;
    
    
    if(socket_memoria >= 0){
        _send_message(socket_memoria, ID_KERNEL, MATE_MEMWRITE, payload, sizeof(int) * 3 + size, logger); 

        t_mensaje *buffer;
        buffer = _receive_message(socket_memoria, logger);
        memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

        log_info(logger, "CARPINCHO %d - el MEMWRITE fue realizado correctamente", id_carpincho);

        close(socket_memoria);
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    }
    else{
        log_error(logger, "No se ha podido conectar con el módulo memoria");
        respuesta_memoria = -1;
    }

    void* respuesta_memoria_serializada = _serialize(sizeof(int), "%d", respuesta_memoria);
    _send_message(fd, ID_KERNEL, 1, respuesta_memoria_serializada, sizeof(int), logger); 
    free(respuesta_memoria_serializada);
    free(payload);
}

void entrantes_a_ready(){

   data_carpincho *carpincho_a_mover;
   int valor; 
    int gradoMultiprogramacion;
        sem_getvalue(&sem_grado_multiprogramacion_libre, &gradoMultiprogramacion);

    while(1){

        sem_wait(&hay_estructura_creada);
        sem_wait(&sem_grado_multiprogramacion_libre);

        if(!queue_is_empty(suspended_ready)) 
        {
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_suspended_ready); 
                carpincho_a_mover = (data_carpincho *) queue_peek(suspended_ready);
                list_add(ready, carpincho_a_mover);
                queue_pop(suspended_ready);
            pthread_mutex_unlock(&sem_cola_ready); 
            pthread_mutex_unlock(&sem_cola_suspended_ready); 

            log_info(logger, "CARPINCHO %d - pasó de SUSPENDED-READY a READY", carpincho_a_mover->id);
            
        }
        else{ 
            pthread_mutex_lock(&sem_cola_ready); 
            pthread_mutex_lock(&sem_cola_new);

            carpincho_a_mover = (data_carpincho *) queue_peek(new);

            list_add(ready, carpincho_a_mover); 
            queue_pop(new);

            pthread_mutex_unlock(&sem_cola_new);
            pthread_mutex_unlock(&sem_cola_ready);

            log_info(logger, "CARPINCHO %d - pasó de NEW a READY", carpincho_a_mover->id);
        }
        
        carpincho_a_mover->estado = READY;
        sem_post(&cola_ready_con_elementos); 

        sem_getvalue(&sem_grado_multiprogramacion_libre, &valor);
        if(valor == 0){ 
            sem_post(&sem_programacion_lleno);
        }
        int gradoMultiprogramacion2;        
        sem_getvalue(&sem_grado_multiprogramacion_libre, &gradoMultiprogramacion2);
        log_info(logger,"GRADO MULTIPROGRAMACIÓN LIBRE: %d", gradoMultiprogramacion2);
    }
}

void ready_a_exec(){  

    data_carpincho *carpincho_a_mover;
    int valor2;

    while(1){ 
        void *payload;

        sem_wait(&cola_ready_con_elementos);
        sem_wait(&sem_grado_multiprocesamiento_libre);

        sem_getvalue(&sem_grado_multiprocesamiento_libre, &valor2);
        log_info(logger,"GRADO MULTIPROCESAMIENTO LIBRE: %d", valor2);

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

        log_info(logger, "CARPINCHO %d - se le asignó el hilo CPU: %d", carpincho_a_mover->id, carpincho_a_mover->CPU_en_uso);

        pthread_mutex_lock(&sem_cola_ready); 
		pthread_mutex_lock(&sem_cola_exec);
            list_add(exec, carpincho_a_mover);
            list_remove_by_condition(ready, (void *)es_el_mismo);
		pthread_mutex_unlock(&sem_cola_exec);
		pthread_mutex_unlock(&sem_cola_ready);

        log_info(logger, "CARPINCHO %d - pasó de READY a EXEC", carpincho_a_mover->id);

        carpincho_a_mover->estado = EXEC;

        carpincho_a_mover->tiempo_entrada_a_exec = calcular_milisegundos(); 

        int prog;
        sem_getvalue(&sem_grado_multiprogramacion_libre, &prog);
        log_info(logger,"GRADO MULTIPROGRAMACION LIBRE: %d", prog);

        if(prog == 0){ 
            sem_post(&sem_procesamiento_lleno);
        }

        payload = _serialize(sizeof(int), "%d", carpincho_a_mover->id);
        _send_message(carpincho_a_mover->fd, ID_KERNEL, 1, payload, sizeof(int), logger); 
        
        
        free(payload);
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
    carpincho_a_bloquear->tiempo_salida_a_exec = calcular_milisegundos(); 

    sem_post(&(liberar_CPU[carpincho_a_bloquear->CPU_en_uso])); 
    
    log_info(logger, "CARPINCHO %d - liberó el CPU: %d ", carpincho_a_bloquear->id, carpincho_a_bloquear->CPU_en_uso);
    
    sem_post(&sem_hay_bloqueados);

    log_info(logger, "CARPINCHO %d - paso de EXEC a BLOCKED", carpincho_a_bloquear->id);
}


void exec_a_exit(int id_carpincho, int fd){
    
    void *payload;
    int valor;
	
    log_info(logger,"CARPINCHO %d - ha llegado el momento de eliminarlo", id_carpincho);

    data_carpincho *carpincho_que_termino;
    carpincho_que_termino = encontrar_estructura_segun_id(id_carpincho);

    bool es_el_mismo(void* carpincho){
        return carpincho_que_termino->id == ((data_carpincho *)carpincho)->id;
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

    int id_carpincho_eliminado;
    id_carpincho_eliminado = carpincho_que_termino->id;
    int cpu_carpincho_eliminado;
    cpu_carpincho_eliminado = carpincho_que_termino->CPU_en_uso;
    pthread_mutex_lock(&sem_cola_exec); 
    list_remove_by_condition(exec, es_el_mismo);
    
    list_remove_and_destroy_by_condition(lista_carpinchos, es_el_mismo, liberar_carpincho); 
	pthread_mutex_unlock(&sem_cola_exec);

    sem_post(&(liberar_CPU[cpu_carpincho_eliminado]));
    sem_post(&sem_grado_multiprogramacion_libre);

    log_info(logger, "CARPRINCHO %d - liberó el CPU %d", id_carpincho_eliminado,  cpu_carpincho_eliminado);
    log_info(logger, "CARPRINCHO %d - paso de EXEC a EXIT", id_carpincho_eliminado);

    sem_getvalue(&sem_grado_multiprocesamiento_libre, &valor);

    void* payload2 = _serialize(sizeof(int), "%d", 0);
    _send_message(fd, ID_KERNEL, 2, payload2, sizeof(int), logger);
    
    free(payload);
    free(payload2);
    free(buffer->identifier);
    free(buffer->payload);
    free(buffer);
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

    carpincho_a_ready->estado = READY;
    log_info(logger, "CARPINCHO %d - paso de BLOCK a READY", carpincho_a_ready->id);
    sem_post(&cola_ready_con_elementos);
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

    carpincho->estado = SUSPENDED_READY; 
    log_info(logger, "CARPINCHO %d - paso de SUSPENDED BLOCKED a SUSPENDED READY", carpincho->id);
    sem_post(&hay_estructura_creada);

}

void suspender(){
    while(1){

        sem_wait(&sem_procesamiento_lleno);

        log_info(logger, "SUSPENSIÓN - chequeando condiciones");

        data_carpincho *carpincho_a_suspender; 

        int long_blocked;
        int long_entrantes;
        int long_ready;

        long_entrantes = queue_size(new) + queue_size(suspended_ready);
        long_ready = list_size(ready);
        long_blocked = list_size(blocked);
        
        log_info(logger,"SUSPENSIÓN - cantidad de carpinchos bloqueados: %d", long_blocked);
        log_info(logger,"SUSPENSIÓN - cantidad de carpinchos ready: %d", long_ready);
        log_info(logger,"SUSPENSIÓN - cantidad de carpinchos entrantes: %d", long_entrantes);


        if(long_entrantes>0 && long_ready==0 && long_blocked>0){
            	log_warning(logger, "SUSPENSIÓN - se cumplen las condiciones necesarias");

		pthread_mutex_lock(&sem_cola_blocked);
		pthread_mutex_lock(&sem_cola_suspended_blocked);
		carpincho_a_suspender = list_remove(blocked, long_blocked-1); //retorna y remueve el ultimo de bloqueados
		log_info(logger, "SUSPENSIÓN - el carpincho a suspender es: %d", carpincho_a_suspender->id);
		list_add(suspended_blocked, (void *)carpincho_a_suspender);
		pthread_mutex_unlock(&sem_cola_blocked);
		pthread_mutex_unlock(&sem_cola_suspended_blocked);

		carpincho_a_suspender->estado = SUSPENDED_BLOCKED;
		log_info(logger, "SUSPENSIÓN - El carpincho %d pasó a SUSPENDED BLOCKED", carpincho_a_suspender->id);
		sem_post(&liberar_CPU[carpincho_a_suspender->CPU_en_uso]); //--> esta bien que sea el de liberar CPU?

		void *payload;
		payload =_serialize(sizeof(int), "%d", carpincho_a_suspender->id);

		int socket_memoria;
		socket_memoria = _connect(config_get_string_value(config, "IP_MEMORIA"), config_get_string_value(config, "PUERTO_MEMORIA"), logger);
		_send_message(socket_memoria, ID_KERNEL, SUSPENDER, payload, sizeof(int), logger); 

		t_mensaje *buffer;
		int respuesta_memoria;
		buffer = _receive_message(socket_memoria, logger);
		memcpy(&respuesta_memoria,  buffer->payload, sizeof(int));

		close(socket_memoria);

		sem_post(&sem_grado_multiprogramacion_libre);

		free(payload); 
        } 
        else{
                log_info(logger, "SUSPENSIÓN - no se dieron las condiciones para suspender ... por ahora");  
        }
    }     
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
            carpincho_menor->estimacion_anterior = carpincho_menor->estimacion_siguiente;
    }
    list_iterator_destroy(list_iterator);

    log_info(logger, "PLANIFICACIÓN - el carpincho %d ha sido el elegido por SJF", carpincho_menor->id);
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
    log_info(logger, "PLANIFICACIÓN - el carpincho %d ha sido el elegido por HRRN", carpincho_mayor->id);
    return carpincho_mayor;     
}

void calculo_estimacion_siguiente(data_carpincho *carpincho){ 
    calculo_rafaga_anterior(carpincho);
    float alfa = (float) config_get_double_value(config, "ALFA");
    carpincho->estimacion_siguiente = carpincho->rafaga_anterior * alfa + carpincho->estimacion_anterior * (1 - alfa); 
}

void calculo_rafaga_anterior(data_carpincho *carpincho){
    carpincho->rafaga_anterior = carpincho->tiempo_salida_a_exec - carpincho->tiempo_entrada_a_exec; 
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

    free(tiempo_sacado);

    for (int i = 0; tiempo_formateado[i] != NULL; i++)
            {
                free(tiempo_formateado[i]);
            }
            free(tiempo_formateado);
    return tiempo_calculado.minutos * 60000 + tiempo_calculado.segundos * 60 + tiempo_calculado.milisegundos;
}


//////////////////////////// Funciones para exec ///////////////////////////////////

void asignar_hilo_CPU(data_carpincho *carpincho){

    int *id_CPU_disponible;

    pthread_mutex_lock(&sem_CPU_libres);
    id_CPU_disponible = (int *) queue_peek(CPU_libres);
    queue_pop(CPU_libres);
    pthread_mutex_unlock(&sem_CPU_libres);

    carpincho->CPU_en_uso = *id_CPU_disponible;

    sem_post(&(usar_CPU[*id_CPU_disponible]));   
}

void ejecuta(void *id_cpu){ 

    int gradoMultiprocesamiento;

    while(1){

        int *id = (int *) id_cpu;
    
        sem_wait(&usar_CPU[*id]); // espera hasta que algun carpincho haga post para usar ese cpu
        sem_wait(&CPU_libre[*id]); // ya no está más libre ese cpu
        
        sem_wait(&liberar_CPU[*id]); // espera a que algun carpincho indique que quiere liberar el cpu
        
        pthread_mutex_unlock(&mutex_para_CPU); 
        queue_push(CPU_libres, id);
        pthread_mutex_lock(&mutex_para_CPU);

        sem_post(&CPU_libre[*id]); 

        sem_post(&sem_grado_multiprocesamiento_libre); //hay algun cpu libre
        
        sem_getvalue(&sem_grado_multiprocesamiento_libre, &gradoMultiprocesamiento);
        log_info(logger,"GRADO MULTIPROCESAMIENTO LIBRE: %d", gradoMultiprocesamiento);
    }
}

void handler( int fd, char* id, int opcode, void* payload, t_log* logger){
    
    data_carpincho* estructura_interna;
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
                //log_info(logger, "se pidió un MATE INIT");
                mate_init(fd);
            break;
            case MATE_CLOSE: 
                //log_info(logger, "se pidió un MATE CLOSE");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                  mate_close(estructura_interna->id,fd);   
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }
                
            break;
            case MATE_SEM_INIT: 
                //log_info(logger, "se pidió un MATE SEM INIT");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                mate_sem_init(estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo, fd);  
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }       
            break;
            case MATE_SEM_WAIT: 
                //log_info(logger, "se pidió un MATE SEM WAIT");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                mate_sem_wait(estructura_interna->id, estructura_interna->semaforo, fd); 
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }           
            break;
            case MATE_SEM_POST: 
                //log_info(logger, "se pidió un MATE SEM POST");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                mate_sem_post(estructura_interna->id, estructura_interna->semaforo, fd);     
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }       
            break;
            case MATE_SEM_DESTROY:
                //log_info(logger, "se pidió un MATE SEM DESTROY");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                mate_sem_destroy(estructura_interna->id, estructura_interna->semaforo, fd);       
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }    
            break;
            case MATE_CALL_IO:
                //log_info(logger, "se pidió un MATE CALL IO");
                estructura_interna = deserializar(payload);
                if(estructura_interna != NULL){
                mate_call_io(estructura_interna->id, estructura_interna->dispositivo_io, fd);  
                }else{
                  void* payload = _serialize(sizeof(int), "%d", -6);
                 _send_message(fd, ID_KERNEL, 99, payload, sizeof(int), logger);
                 free(payload);
                }
            break;       
            case MATE_MEMALLOC: 
                // id_carpincho
                //log_info(logger, "se pidió un MATE MEMALLOC");
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload + offset, sizeof(int));
                offset += sizeof(int);

                mate_memalloc(id_carpincho, size_memoria, fd);      
            break;      
            case MATE_MEMFREE:
                //log_info(logger, "se pidió un MATE MEMFREE");
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // addr_memfree
                memcpy(&addr_memfree, payload + offset, sizeof(int));
                offset += sizeof(int);

                mate_memfree(id_carpincho, addr_memfree, fd);      
            break;      
            case MATE_MEMREAD: 
                //log_info(logger, "se pidió un MATE MEMREAD");
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);
                // origin_memread
                memcpy(&origin_memread, payload + offset, sizeof(int));
                offset += sizeof(int);
                // size_memoria
                memcpy(&size_memoria, payload + offset, sizeof(int));
                offset += sizeof(int);

                mate_memread(id_carpincho, origin_memread, size_memoria, fd);            
            break;
            case MATE_MEMWRITE:  
                //log_info(logger, "se pidió un MATE MEMWRITE");
                // id_carpincho
                memcpy(&id_carpincho, payload, sizeof(int));
                offset += sizeof(int);

                // dest_memwrite
                memcpy(&dest_memwrite, payload + offset, sizeof(int));
                offset += sizeof(int);

                memcpy(&ptr_len, payload + offset, sizeof(int));
                offset += sizeof(int);
                size_memoria = ptr_len;
                origin_memwrite = malloc(ptr_len);
                // origin_memwrite
                memcpy(origin_memwrite, payload + offset, ptr_len);

                mate_memwrite(id_carpincho, origin_memwrite, dest_memwrite, size_memoria, fd);    
                free(origin_memwrite); 
            break;  
            default:
                log_error(logger, "comando incorrecto");
        }
    }
}



////////////////////////////// DEADLOCK ////////////////////////////////

void agregando_a_lista_posible_deadlock(){

    bool no_es_nulo(void* carpincho){
        return carpincho != NULL;
    }

      t_list* lista_carpinchos_filtrada;

      lista_carpinchos_filtrada = list_filter(lista_carpinchos,no_es_nulo);
            
    for(int i= 0; i< list_size(lista_carpinchos_filtrada); i++){ 
        data_carpincho *carpi_que_espera = list_get(lista_carpinchos_filtrada,i);

        bool cumple_estado = carpi_que_espera->estado==BLOCKED || carpi_que_espera->estado==SUSPENDED_BLOCKED;
        bool cumple_retencion = carpi_que_espera->sem_retenido !=0;
        bool cumple_bloqueo = carpi_que_espera->nombre_semaforo_por_el_que_se_bloqueo !=NULL; //falta --> cuando se desbloquee cambiarlo a null
        
        if(cumple_estado && cumple_retencion && cumple_bloqueo){
            list_add(lista_posibles, carpi_que_espera);
            buscar_quien_tiene_su_espera(carpi_que_espera);     
        }else{
             sem_post(&segui_chequeando_deadlock);
	}
    }
    list_destroy(lista_carpinchos_filtrada);
}
 
void buscar_quien_tiene_su_espera(data_carpincho* carpi_que_espera){

    bool no_es_nulo(void* carpincho){
        return carpincho != NULL;
    }

    int id_sem_que_espera= carpi_que_espera->sem_por_el_que_se_bloqueo;

      t_list* lista_carpinchos_filtrada;

      lista_carpinchos_filtrada = list_filter(lista_carpinchos,no_es_nulo);
    
    for(int i=0; i<list_size(lista_carpinchos_filtrada); i++){

        data_carpincho* carpi_que_retiene;
        carpi_que_retiene = list_get(lista_carpinchos_filtrada,i);

        if(carpi_que_retiene->id != carpi_que_espera->id){

            if(carpi_que_retiene->sem_retenido == id_sem_que_espera){
                log_info(logger, "Carpincho %d está esperando al %d",carpi_que_espera->id, carpi_que_retiene->id);
                carpi_que_espera->tiene_su_espera = carpi_que_retiene->id;
                list_add(lista_conectados,(void *) carpi_que_espera);
            }
        }
    }
    sem_post(&segui_chequeando_deadlock);
    list_destroy(lista_carpinchos_filtrada);
}


void detectate_deadlock(){

    int tiempo_deadlock = config_get_int_value(config, "TIEMPO_DEADLOCK");

    while(1){
        
        usleep(tiempo_deadlock*1000);
        log_warning(logger, "DEADLOCK - luego de %d milisegundos, llegó el momento de buscar sospechosos", tiempo_deadlock);
   
        agregando_a_lista_posible_deadlock();

        sem_wait(&segui_chequeando_deadlock);

        int cantidad_posibles = list_size(lista_posibles);

        if(cantidad_posibles >= 2){
            if(hay_ciclo()){
                solucionatelo(el_ciclo);
            }
        } else {
		sem_post(&segui_chequeando_deadlock);	
	}
        list_clean(lista_posibles);  
        list_clean(lista_conectados);
        list_clean(el_ciclo); 
    }
}

bool hay_ciclo(){

    t_list* aux_conectados = list_create();
    list_add_all(aux_conectados, lista_conectados);
    int cant_conectados = list_size(lista_conectados);

    for(int i=0; i<cant_conectados; i++){
        data_carpincho* primer_carpi;
        primer_carpi = list_get(aux_conectados,i);
        if(chequear_ciclo(primer_carpi)){
            list_destroy(aux_conectados);
            return true;
        }else if(i==cant_conectados){
            list_destroy(aux_conectados);
            return false;
        }
    }

    list_clean(aux_conectados);
    list_destroy(aux_conectados);
    return false;
}


bool chequear_ciclo(data_carpincho *primer_carpi){

    int id_inicial = primer_carpi->id;
    int id_carpi_2 = primer_carpi->tiene_su_espera;

    data_carpincho* carpi_2;
    
    if(validar_existencia_carpincho(id_carpi_2)){
    	carpi_2 = encontrar_estructura_segun_id(id_carpi_2); 
    }else{
        return false;
    }

    int id_carpi_3 = carpi_2->tiene_su_espera;
    data_carpincho* carpi_3;

    if(validar_existencia_carpincho(id_carpi_3)){
    carpi_3 = encontrar_estructura_segun_id(id_carpi_3); 
    }else{
        log_warning(logger,"DEADLOCK - No se han encontrado carpinchos en deadlock");
        return false;
    }
  
    if( id_carpi_3 == id_inicial){
        list_add(el_ciclo,(void *)id_inicial);
        list_add(el_ciclo,(void *)id_carpi_2);
        return true;
        }
    else{
        int id_carpi_4 = carpi_3->tiene_su_espera;
        
        data_carpincho* carpi_4;

        if(validar_existencia_carpincho(id_carpi_4)){
            carpi_4 = encontrar_estructura_segun_id(id_carpi_4);
            }else{
                log_warning(logger,"DEADLOCK - No se han encontrado carpinchos en deadlock");
                return false;
            }
        
        if(id_carpi_4 == id_inicial){
            list_add(el_ciclo,(void *)id_inicial);
            list_add(el_ciclo,(void *)id_carpi_2);
            list_add(el_ciclo,(void *)id_carpi_3);
            return true;
        }
        else{
            int id_carpi_5 = carpi_4->tiene_su_espera;

            data_carpincho* carpi_5;

            if(validar_existencia_carpincho(id_carpi_5)){
                carpi_5 = encontrar_estructura_segun_id(id_carpi_5);
            }else{
                log_warning(logger,"DEADLOCK - No se han encontrado carpinchos en deadlock");
                return false;
            }

            if(id_carpi_5 == id_inicial){
                list_add(el_ciclo,(void *)id_inicial);
                list_add(el_ciclo,(void *)id_carpi_2);
                list_add(el_ciclo,(void *)id_carpi_3);
                list_add(el_ciclo,(void *)id_carpi_4);
                return true;
            }
            else{   
                int id_carpi_6 = carpi_5->tiene_su_espera;
                
                    if(id_carpi_6 == id_inicial){
                        list_add(el_ciclo,(void *)id_inicial);
                        list_add(el_ciclo,(void *)id_carpi_2);
                        list_add(el_ciclo,(void *)id_carpi_3);
                        list_add(el_ciclo,(void *)id_carpi_4);
                        list_add(el_ciclo,(void *)id_carpi_5);
                        return true;
                    }
                    else{
			log_warning(logger,"DEADLOCK - No se han encontrado carpinchos en deadlock");
			return false;
                    }
            }
        }
    }
}

void solucionatelo(t_list* el_ciclo){

    log_warning(logger, "DEADLOCK - se encontraron %d carpinchos en deadlock. Procedo a solucionarlo", list_size(el_ciclo));

    int mayor_id_hasta_ahora = 0;
    int id_actual;
 
        for(int i= 0; i< list_size(el_ciclo); i++){
        
            id_actual = (int) list_get(el_ciclo,i);
            if(id_actual > mayor_id_hasta_ahora){    
                mayor_id_hasta_ahora = id_actual;
            }
        }
    
    data_carpincho *carpincho_a_eliminar = encontrar_estructura_segun_id(mayor_id_hasta_ahora);

    log_info(logger, "DEADLOCK - para solucionarlo, se elimina al carpincho %d porque tenía el mayor ID", carpincho_a_eliminar->id);
  
    pthread_mutex_unlock(&sem_cola_blocked);
    int id_sem= carpincho_a_eliminar->sem_retenido;
    int id_sem_culpable = carpincho_a_eliminar->sem_por_el_que_se_bloqueo;;

    semaforo* sem = encontrar_estructura_semaforo(id_sem);
    semaforo* sem_culpable = encontrar_estructura_semaforo(id_sem_culpable);

    mate_close(mayor_id_hasta_ahora, carpincho_a_eliminar->fd);

    log_info(logger, "DEADLOCK - se le hace POST a los semáforos que tenia retenidos el carpincho eliminado");
    
    mate_sem_post(mayor_id_hasta_ahora, (mate_sem_name)sem_culpable->nombre, -1);
    mate_sem_post(mayor_id_hasta_ahora, (mate_sem_name)sem->nombre, -1);


}
