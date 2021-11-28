#include "matelib.h"

int deserializar_numero(t_mensaje* buffer){

    int numero;
    memcpy(&numero, buffer->payload, sizeof(int));
    free(buffer->identifier);
    free(buffer->payload);
    free(buffer);
    return numero;
}


/////////////////////////---------------------------- funciones a parte ------------------------///////////////////////////////////////

void* armar_paquete(mate_inner_structure* estructura_interna){ // serializar estructura interna para mandar al carpincho
    int sem_len =  string_length(estructura_interna->semaforo);
    int sem_dis_io = string_length(estructura_interna->dispositivo_io);
    log_info(logger, "Serializando la estructura del carpincho %d", estructura_interna->id);
    return _serialize( sizeof(int) * 4 + sem_len + sem_dis_io, "%d%s%d%s", estructura_interna->id, estructura_interna->semaforo, estructura_interna->valor_semaforo, estructura_interna->dispositivo_io);
}

mate_inner_structure* convertir_a_estructura_interna(mate_instance* lib_ref){ // usarla para, de lo que manda el capincho, poder utilizar la estructura interna
    return (mate_inner_structure *)lib_ref->group_info; 
}

 mate_inner_structure* nueva_estructura_interna(){
    mate_inner_structure* nueva_estructura = malloc(sizeof(mate_inner_structure));
    nueva_estructura->semaforo = string_new();
    nueva_estructura->dispositivo_io = string_new();

    return nueva_estructura;
 }

int conexion_con_backend(int id_funcion, mate_inner_structure* estructura_interna){
    
    void* payload = armar_paquete(estructura_interna);
    int sem_len =  string_length(estructura_interna->semaforo);
    int len_dis_io = string_length(estructura_interna->dispositivo_io);
    int size =  sizeof(int) * 4 + sem_len + len_dis_io;
   
    int  conexion_con_backend = _send_message(socket_backend, ID_MATE_LIB, id_funcion, payload, size, logger); // envia la estructura al backend para que inicialice todo
    
    free(payload);

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión con backend para el carpincho %d", estructura_interna->id);
        return conexion_con_backend;  
    }
    else{
        t_mensaje* buffer = _receive_message(socket_backend, logger);
        return  deserializar_numero(buffer);
    }
}

////////////////////////////////////////                        LIB                          /////////////////////////////////////////////

 // Funciones generales --------------------------------------------------------------

int mate_init(mate_instance *lib_ref, char *config)
{
    
    // post pruebas => ver si trae problemas tener un solo log
    logger = log_create("./cfg/mate-lib.log", "[Mate-Lib]", true, LOG_LEVEL_INFO); // creo el log para ir guardando todo

    mate_inner_structure* estructura_interna = nueva_estructura_interna();

    int sem_len =  string_length(estructura_interna->semaforo);
    int len_dis_io = string_length(estructura_interna->dispositivo_io);
    int size =  sizeof(int) * 4 + sem_len + len_dis_io;
    void* payload = armar_paquete(estructura_interna);
    t_config* datos_configuracion = config_create(config);
    socket_backend = _connect(config_get_string_value(datos_configuracion, "IP_BACKEND"), config_get_string_value(datos_configuracion, "PUERTO_BACKEND"), logger);
    
    printf("socket: %d\n", socket_backend);

    if(socket_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión con backend para que cree la estructura del carpincho");
        printf("\nestoy aca\n");
        free(payload);
        return socket_backend;  
    }
    else{
        log_info(logger, "enviando mensaje a backend para que cree la estructura del carpincho");        
        _send_message(socket_backend, ID_MATE_LIB, MATE_INIT, payload, size, logger); // envia la estructura al backend para que inicialice todo
        int id_recibido;
        t_mensaje* buffer = _receive_message(socket_backend, logger);
        id_recibido = deserializar_numero(buffer);
        estructura_interna->id = id_recibido;
        log_info(logger, "el backend ya creó la estructura del carpincho y su id es %d", estructura_interna->id);

    //post pruebas => podríamos revisar aca que onda que solo esta vez les pasamos la referencia

        lib_ref->group_info = malloc(sizeof(mate_inner_structure));
        lib_ref->group_info = (void*)estructura_interna; 
        free(payload);
        
        return 0;
    } 
}

int mate_close(mate_instance *lib_ref)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    //free(lib_ref->group_info);
    log_info(logger, "pidiendo al backend que elimine al carpincho %d", estructura_interna->id);
    return conexion_con_backend(MATE_CLOSE, estructura_interna);
}

 // Semáforos --------------------------------------------------------------------

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) 
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    log_info(logger, "el carpincho con id %d pidió inicializar al semaforo %s con valor %d", estructura_interna->id, sem, value);    
    string_append(&estructura_interna->semaforo, (char*) sem);
    // free(estructura_interna->semaforo);
    // estructura_interna->semaforo = string_from_format("%s", sem);
    estructura_interna->valor_semaforo = value; 
    return conexion_con_backend(MATE_SEM_INIT, estructura_interna);    
}   

int modificar_semaforo(int id_funcion, mate_sem_name sem, mate_instance* lib_ref){
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    log_info(logger, "el carpincho con id %d pidio el semaforo %s", estructura_interna->id,(char *) sem);             
    // estructura_interna->semaforo = sem;
    free(estructura_interna->semaforo);
    estructura_interna->semaforo = string_from_format("%s", sem);
    return conexion_con_backend(id_funcion, estructura_interna);
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) 
{   
    log_info(logger, "el carpincho pidió hacer wait");         
    return modificar_semaforo(MATE_SEM_WAIT, sem, lib_ref);
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) 
{
    log_info(logger, "el carpincho con id %d pidió hacer post");         
    return modificar_semaforo(MATE_SEM_POST, sem, lib_ref);
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) 
{
    log_info(logger, "el carpincho con id %d pidió hacer destroy");         
    return modificar_semaforo(MATE_SEM_DESTROY, sem, lib_ref);
}

 // Funcion Entrada y Salida --------------------------------------------------------------

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    free(estructura_interna->dispositivo_io);
    estructura_interna->dispositivo_io = string_from_format("%s", (char*) io);
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

    printf("\ngonza nadie te quiere \n");
    if(conexion_con_backend < 0 ){ 
        free(payload);
        log_info(logger, "no se pudo crear la conexión con backend para hacer memalloc pedida por el carpincho %d", estructura_interna->id);
        return conexion_con_backend;  
    }
    else{
        int pointer;
        t_mensaje* buffer;
        log_info(logger, "pidiendo a backend el memalloc pedida por el carpincho %d", estructura_interna->id);
        buffer = _receive_message(socket_backend, logger);
        memcpy(&pointer, buffer->payload, sizeof(int));
        log_info(logger, "el resultado del memalloc pedida por el carpincho %d fue %d", estructura_interna->id, pointer );        
        free(buffer->identifier);
        free(buffer->payload);
        free(buffer);
    
        free(payload);
        
        printf("\nesto viene antes de return (mate_pointer) pointer; %d \n", pointer);
        return (mate_pointer) pointer;  
    } 
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
    int conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    void* payload = _serialize(sizeof(int) + sizeof(mate_pointer), "%d%d", estructura_interna->id, addr);
    conexion_con_backend = _send_message( socket_backend, ID_MATE_LIB, MATE_MEMFREE, payload, sizeof(int) + sizeof(mate_pointer), logger); 

    free(payload);

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión con backend para hacer memfree pedida por el carpincho %d", estructura_interna->id);
        return conexion_con_backend;  
    }
    else{

        int resultado; 
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&resultado, buffer->payload, sizeof(int));
        if (resultado > 0){
            log_info(logger,"Se pudo hacer el memfree pedido por el carpincho %d", estructura_interna->id);       
        } else {
            log_error(logger,"no se pudo hacer el memfree pedido por el carpincho %d", estructura_interna->id);
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

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión con backend para hacer memread pedida por el carpincho %d", estructura_interna->id);
        free(payload);
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
        free(payload);
        return resultado;  
    }   
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
    int conexion_con_backend;
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);
    void* payload = _serialize(sizeof(int) * 3 + size, "%d%d%d%v", estructura_interna->id, dest, size, origin);
    conexion_con_backend = _send_message(socket_backend, ID_MATE_LIB, MATE_MEMWRITE, payload, sizeof(int) * 3 + size, logger); 

    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión con backend para hacer memwrite pedida por el carpincho %d", estructura_interna->id);
        free(payload);
        return conexion_con_backend;  
    }
    else{
        int resultado; 
        t_mensaje* buffer;

        buffer = _receive_message(socket_backend, logger);

        memcpy(&resultado, buffer->payload, sizeof(int));
        if (resultado > 0){
            log_info(logger,"Se pudo hacer el memwrite pedido por el carpincho %d", estructura_interna->id);           
        } else {
            log_error(logger,"no se pudo hacer el memwrite pedido por el carpincho %d", estructura_interna->id);
        }
         free(buffer->payload);
         free(buffer->identifier);
         free(buffer);
        free(payload);
        return resultado;
        
    }   
}