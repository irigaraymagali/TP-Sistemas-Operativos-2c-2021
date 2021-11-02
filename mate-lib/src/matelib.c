#include "matelib.h"

t_log* logger;
int id_carpincho;

int armar_socket_desde_binario(char* config,t_log* logger){
   
    // leer archivo binario

    char *ip;
    char *puerto;


    return _connect(ip, puerto, logger); // crea la conexión con backend los ip y puerto del config
    
}

/////////////////////////---------------------------- funciones a parte ------------------------///////////////////////////////////////

void* armar_paquete(mate_inner_structure* estructura_interna){ // serializar estructura interna para mandar al carpincho

    return _serialize(
                        sizeof(int) 
                        + sizeof(char*) 
                        + sizeof(int) 
                        + sizeof(char*)                         
                        + 6 * sizeof(int)
                       , "%d%s%d%s%d%d%d%d%d%d",
                        estructura_interna->id,
                        string_length(estructura_interna->semaforo),
                        estructura_interna->semaforo,
                        estructura_interna->valor_semaforo, 
                        string_length(estructura_interna->dispositivo_io),
                        estructura_interna->dispositivo_io, 
                        estructura_interna->size_memoria, 
                        estructura_interna->addr_memfree, 
                        estructura_interna->origin_memread, 
                        estructura_interna->dest_memread, 
                        estructura_interna->origin_memwrite, 
                        estructura_interna->dest_memwrite
                    );

}

mate_inner_structure* convertir_a_estructura_interna(mate_instance* lib_ref){ // usarla para, de lo que manda el capincho, poder utilizar la estructura interna
    return (mate_inner_structure *)lib_ref->group_info; 
}


int conexion_con_backend(int id_funcion, mate_inner_structure* estructura_interna){

    int  conexion_con_backend = _send_message(socket, ID_MATE_LIB, id_funcion, armar_paquete(estructura_interna), sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo
    
    if(conexion_con_backend < 0 ){ 
        log_info(logger, "no se pudo crear la conexión");
        return conexion_con_backend;  
    }
    else{
        // está bien así?
        return _receive_message(socket, logger);
    }
}


////////////////////////////////////////                        LIB                          /////////////////////////////////////////////

 // Funciones generales --------------------------------------------------------------

int mate_init(mate_instance *lib_ref, char *config)
{

    logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO); // creo el log para ir guardando todo
    

    printf("hola\n");

    int conexion_con_backend;

    int socket;

    // para pruebas
    socket =  _connect("127.0.0.1", "5001", logger);

    printf("socket: %d\n", socket);
    
    // socket = armar_socket_desde_binario(config,logger);

    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    conexion_con_backend = _send_message(socket, ID_MATE_LIB, MATE_INIT, armar_paquete(estructura_interna), sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo

    if(conexion_con_backend < 0 ){ 
        printf("no se pudo conectar con backend \n");
        return conexion_con_backend;  
    }
    else{

        int id_recibido;

        _receive_message(socket, logger);
        
        // deserializar mensaje

        estructura_interna->id = id_recibido;

        printf("el id es: %d\n", estructura_interna->id);

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
    //estructura_interna->mnesaje_io = msg; no hago esto porque no lo vamos a usar, se aclara en un issue
    
    return conexion_con_backend(MATE_CALL_IO, estructura_interna);    

}

// Funciones módulo memoria ------------------------------------------------------------------

// CAMBIAR TODAS LAS FUNCIONES DE MEMORIA PARA QUE LE PASEN SOLO LOS DATOS IMPORTANTES Y QUE SEA POLIMORFICO CON KERNEL Y MEMORIA

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{

    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->size_memoria = size; 

    return conexion_con_backend(MATE_MEMALLOC, estructura_interna);     

}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{

    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->addr_memfree = addr; 

    return conexion_con_backend(MATE_MEMFREE, estructura_interna);    

}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->size_memoria = size;  
    estructura_interna->dest_memread = dest; 

    return conexion_con_backend(MATE_MEMREAD, estructura_interna);    

}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
    mate_inner_structure* estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->origin_memwrite = origin; 
    estructura_interna->dest_memwrite = dest;  
    estructura_interna->size_memoria = size; 

    return conexion_con_backend(MATE_MEMWRITE, estructura_interna);    

}




    



