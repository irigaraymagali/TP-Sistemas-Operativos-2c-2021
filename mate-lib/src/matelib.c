#include "matelib.h"

t_log* logger;
int id_carpincho;

int armar_socket_desde_archivo(char* config,t_log* logger){
   
    // hacer => leer archivo -> puede ser un .conf y lo hacemos como querramos

    char *ip;
    char *puerto;

    return _connect(ip, puerto, logger); // crea la conexión con backend los ip y puerto del config
    
}

/////////////////////////---------------------------- funciones a parte ------------------------///////////////////////////////////////

void* armar_paquete(mate_inner_structure* estructura_interna){ // serializar estructura interna para mandar al carpincho
    int sem_len =  string_length(estructura_interna->semaforo);
    int sem_dis_io = string_length(estructura_interna->dispositivo_io);
    return _serialize( sizeof(int) * 4 + sem_len + sem_dis_io, "%d%s%d%s", estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo, estructura_interna->dispositivo_io);
}

mate_inner_structure* convertir_a_estructura_interna(mate_instance* lib_ref){ // usarla para, de lo que manda el capincho, poder utilizar la estructura interna
    return (mate_inner_structure *)lib_ref->group_info; 
}


int conexion_con_backend(int id_funcion, mate_inner_structure* estructura_interna){
    
    void* payload = armar_paquete(estructura_interna);
    int sem_len =  string_length(estructura_interna->semaforo);
    int len_dis_io = string_length(estructura_interna->dispositivo_io);
    int size =  sizeof(int) * 4 + sem_len + len_dis_io;
   
    int  conexion_con_backend = _send_message(socket_backend, ID_MATE_LIB, id_funcion, payload, size, logger); // envia la estructura al backend para que inicialice todo
    
    free(payload);

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{
        t_mensaje* buffer = _receive_message(socket_backend, logger);
        return deserializar_numero(buffer);
    }
}

int deserializar_numero(t_mensaje* buffer){

    int numero;
    memcpy(&numero, buffer->payload, sizeof(int));
    free(buffer->identifier);
    free(buffer->payload);
    free(buffer);
    return numero;
}

////////////////////////////////////////                        LIB                          /////////////////////////////////////////////

 // Funciones generales --------------------------------------------------------------

int mate_init(mate_instance *lib_ref, char *config)
{

    logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO); // creo el log para ir guardando todo

    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    int conexion_con_backend;
    int sem_len =  string_length(estructura_interna->semaforo);
    int len_dis_io = string_length(estructura_interna->dispositivo_io);
    int size =  sizeof(int) * 4 + sem_len + len_dis_io;
    void* payload = armar_paquete(estructura_interna);

    // martin -> esta bien que el socket sea siempre este? o habria que abrirlo y cerrarlo cada vez?
    socket_backend = armar_socket_desde_archivo(config,logger);

    printf("socket: %d\n", socket_backend);

    conexion_con_backend = _send_message(socket_backend, ID_MATE_LIB, MATE_INIT, payload, size, logger); // envia la estructura al backend para que inicialice todo

    free(payload);

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{
        int id_recibido;
        t_mensaje* buffer = _receive_message(socket_backend, logger);
        id_recibido = deserializar_numero(buffer);
        estructura_interna->id = id_recibido;


        lib_ref->group_info = (void*)estructura_interna; 
        
        return 0;
    }  
}

int mate_close(mate_instance *lib_ref)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    return conexion_con_backend(MATE_CLOSE, estructura_interna);
}

 // Semáforos --------------------------------------------------------------------

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) 
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);    
    estructura_interna->semaforo = sem;
    estructura_interna->valor_semaforo = value; 
    return conexion_con_backend(MATE_SEM_INIT, estructura_interna);    
}   

int modificar_semaforo(int id_funcion, mate_sem_name sem, mate_instance* lib_ref){
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    estructura_interna->semaforo = sem;
    return conexion_con_backend(id_funcion, estructura_interna);
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) 
{
    return modificar_semaforo(MATE_SEM_WAIT, sem, lib_ref);
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) 
{
    return modificar_semaforo(MATE_SEM_POST, sem, lib_ref);
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) 
{

    return modificar_semaforo(MATE_SEM_DESTROY, sem, lib_ref);
}

 // Funcion Entrada y Salida --------------------------------------------------------------

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    estructura_interna->dispositivo_io = io;  
    return conexion_con_backend(MATE_CALL_IO, estructura_interna);    
}

// Funciones módulo memoria ------------------------------------------------------------------
// hacer => agregar para que devuelva los erroes que meti en las shared
mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{
    mate_pointer conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    void* payload = _serialize(sizeof(int) * 2, "%d%d", estructura_interna->id, size);

    conexion_con_backend = _send_message(socket_backend, ID_MATE_LIB, MATE_MEMALLOC, payload, sizeof(int)*2, logger); 

    free(payload);
    
    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{
        mate_pointer pointer;
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&pointer, buffer->payload, sizeof(int));
        
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
        
        return pointer;  
    } 
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
    int conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    void* paylaod = _serialize(sizeof(int) + sizeof(mate_pointer), "%d%d", estructura_interna->id, addr);
    conexion_con_backend = _send_message( socket_backend, ID_MATE_LIB, MATE_MEMFREE, paylaod, sizeof(int) + sizeof(mate_pointer), logger); 

    free(paylaod);

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{

        int resultado; 
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&resultado, buffer->payload, sizeof(int));
        if (resultado > 0){
            log_info(logger,"Se pudo hacer el memfree");       
        } else {
            log_error(logger,"no se pudo hacer el memfree");
        }

         free(buffer->payload);
         free(buffer->identifier);
         free(buffer);

        return resultado; 
    } 
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
    int conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    void* payload = _serialize(sizeof(int) * 3, "%d%d%d", estructura_interna->id, origin, size);
   
    conexion_con_backend = _send_message(   socket_backend, ID_MATE_LIB, MATE_MEMREAD, payload, sizeof(int) * 3, logger); 

    free(payload);
    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{

        int resultado; 
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&resultado, buffer->payload, sizeof(int));
        if (resultado > 0){
           memcpy(dest, buffer->payload + sizeof(int), resultado); 
        } else {
           log_error(logger, "No se pudo leer el contenido con exito");
        }
       
        free(buffer->payload);
        free(buffer->identifier);
        free(buffer);
        return resultado;  
    }   
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
    int conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    void* payload = _serialize(sizeof(int) * 3 + size, "%d%d%d%v", estructura_interna->id, dest, size, origin);
    conexion_con_backend = _send_message( socket_backend, ID_MATE_LIB, MATE_MEMWRITE, payload, sizeof(sizeof(int) * 2 + sizeof(int) * sizeof(origin) + sizeof(int) +  sizeof(int)), logger); 

    free(payload);
   
    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{
        int resultado; 
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&resultado, buffer->payload, sizeof(int));
        if (resultado > 0){
            log_info(logger,"Se pudo hacer el memwrite");       
        } else {
            log_error(logger,"no se pudo hacer el memwrite");
        }
         free(buffer->payload);
         free(buffer->identifier);
         free(buffer);
        return resultado;
        
    }   
}



