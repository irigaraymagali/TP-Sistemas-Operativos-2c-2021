#include "matelib.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


typedef struct mate_inner_structure // datos para poder saber qué está pidiendo el carpincho cuando se conecte con backend
{
    int *id;
    char *semaforo; 
    int *valor_semaforo; 
    char *dispositivo_io; 
    int *size_memoria;
    int *addr_memfree;
    int *origin_memread;
    int **dest_memread;
    int **origin_memwrite;
    int *dest_memwrite;
} mate_inner_structure;



int main(){ 

    t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO); // creo el log para ir guardando todo

    int *id_carpincho = malloc(sizeof(int));
    id_carpincho = 0;

    // está bien guardar esto acá y así?
    int *respuesta_backend = malloc(sizeof(int));
    int *respuesta_para_carpincho = malloc(sizeof(int)); //sizeofchar*100 va bien?
    int *conexion_con_backend = malloc(sizeof(int));

    int *socket;

    log_destroy(logger); 
    destroy(id_carpincho);
    destroy(respuesta_backend);
    destroy(respuesta_para_carpincho);
    destroy(conexion_con_backend);
    destroy(socket);
    
    
}

/////////////////////////---------------------------- funciones a parte ------------------------///////////////////////////////////////

mate_inner_structure armar_paquete(mate_inner_structure estructura_interna){ // serializar estructura interna para mandar al carpincho

    return _serialize(
                        + sizeof(int) 
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
                        estructura_interna->dest_memwrite,
                        string_length(estructura_interna->respuesta_a_carpincho),
                    );

}

mate_inner_structure convertir_a_estructura_interna(lib_ref estructura_carpincho){ // usarla para, de lo que manda el capincho, poder utilizar la estructura interna
    return (mate_inner_structure *)lib_ref->group_info); 
}

void armar_socket_desde_binario(char *config){
   
    // leer archivo binario

    char *ip;
    char *puerto;

    socket = _connect(ip, port, logger); // crea la conexión con backend los ip y puerto del config
    
}

int conexion_con_backend(int id_funcion){

    conexion_con_backend = mandar_mensaje_a_backend(id_funcion)
    if(conexion_con_backend < 0 ){ 
        return conexion_con_backend;  
    }
    else{
        // está bien así?
        return _receive_message(socket, logger);
    }
}


int mandar_mensaje_a_backend(int id_funcion){
     
     return _send_message(socket, ID_MATE_LIB, id_funcion, armar_paquete(estructura_interna), sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo
    
}

////////////////////////////////////////                        LIB                          /////////////////////////////////////////////

 // Funciones generales --------------------------------------------------------------

int mate_init(mate_instance *lib_ref, char *config)
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->id = id_carpincho;

    armar_socket_desde_binario(config);

    conexion_con_backend = _send_message(socket, ID_MATE_LIB, MATE_INIT, armar_paquete(estructura_interna), sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo
    
    if(conexion_con_backend < 0 ){ // no uso la función que armé porque acá voy a necesitar también incrementar el id
        return conexion_con_backend;  
    }
    else{
        // hacer que se quede esperando con un recv a la respuesta del backend y que lo guarde para responder al carpincho
        respuesta_para_carpincho = recv();
        id_carpincho ++; //incremento id para que el proximo tenga el siguiente
        return respuesta_para_carpincho;
    }   
}

int mate_close(mate_instance *lib_ref)
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    return conexion_con_backend(MATE_CLOSE);
    
}

 // Semáforos --------------------------------------------------------------------

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) 
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->semaforo = sem;
    estructura_interna->valor_semaforo = value; 

    return conexion_con_backend(MATE_SEM_INIT);
    
}   

int modificar_semaforo(int id_funcion, mate_sem_name sem){

    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->semaforo = sem;

    return conexion_con_backend(id_funcion);

}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) 
{
    return modificar_semaforo(MATE_SEM_WAIT, sem);
}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) 
{
    return modificar_semaforo(MATE_SEM_POST, sem);
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) 
{

    return modificar_semaforo(MATE_SEM_DESTROY, sem);
}


 // Funcion Entrada y Salida --------------------------------------------------------------

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->dispositivo_io = mate_io_resource;  
    estructura_interna->mnesaje_io = msg; 
    
    return conexion_con_backend(MATE_CALL_IO);    

}

// Funciones módulo memoria ------------------------------------------------------------------

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{

    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->size_memoria = size; 

    return conexion_con_backend(MATE_MEMALLOC);    

}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{

    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->addr_memfree = addr; 

    return conexion_con_backend(MATE_MEMFREE);    

}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->size_memoria = size;  
    estructura_interna->dest_memread = dest; 

    return conexion_con_backend(MATE_MEMREAD);    

}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
    mate_inner_structure estructura_interna = convertir_a_estructura_interna(lib_ref);

    estructura_interna->origin_memwrite = origin; 
    estructura_interna->dest_memwrite = dest;  
    estructura_interna->size_memoria = size; 

    return conexion_con_backend(MATE_MEMWRITE);    

}




    



