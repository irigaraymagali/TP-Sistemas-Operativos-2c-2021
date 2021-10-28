#include "matelib.h"
// #include <matelib.h> --> OJO acá estas importando la lib compilada, para importar el .h de la lib es con ".h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>


// habria que cambiar el manejo de los errores para que lo maneje memoria

//------------------General Functions---------------------/


#define ERROR_RESPUESTA_BACKEND = -1 // error a devolver cuando la respuesta del backend no sea ni 1 ni 2 como se espera
#define ERROR_FUNCION_NO_VALIDA = -2 // error a devolver cuando se quiera usar una función del kernel pero se conectó con memoria




typedef struct mate_inner_structure
{
  // datos para poder saber qué está pidiendo el carpincho cuando se conecte con backend
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
    char *respuesta_a_carpincho;
} mate_inner_structure;


// que onda esto y la memoria que usa? quien se encarga de darsela y de borrarla?
t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

// idem anterior ?
int *respuesta_backend; // donde vamos a ir guardando la ultima respuesta del backend

int id_carpincho;

mate_inner_structure armar_paquete(mate_inner_structure estructura_interna){

    return _serialize(
                        + sizeof(int) 
                        + sizeof(char*) 
                        + sizeof(int) 
                        + sizeof(char*)                         
                        + 6 * sizeof(int)
                        + sizeof(int) 
                       , "%d%s%d%s%d%d%d%d%d%d%s",
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
                        estructura_interna->respuesta_a_carpincho 
                    );

}

int main(){
    id_carpincho = 0;
}

int mate_init(mate_instance *lib_ref, char *config)
{
    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info);   // creo la estructura interna 

    estructura_interna->id = id_carpincho;

    // falta leer archivo config que recibe la función para tener los datos de conexión, como se hace?
    
    char *ip; // valor del archivo de config recibido
    char *port: // valor del archivo de config recibido

    socket = _connect(ip, port, logger); // crea la conexión con los ip y puerto del config

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_INIT, armar_paquete(estructura_interna), sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo
    
    // acá se tendría que quedar esperando con recv()

    if(respuesta_backend < 0 ){ 
        return respuesta_backend;  
    }
    else{
        id_carpincho ++;
        return estructura_interna->respuesta_a_carpincho;
    }    
} 

////////////////////// correcciones
int mate_close(mate_instance *lib_ref)
{
    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_CLOSE,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    // acá se tendría que quedar esperando con recv()    

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}

//-----------------Semaphore Functions---------------------/ 

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {
    
    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->semaforo = sem; // pongo el semaforo en la estructura que se va a mandar al backend:
        estructura_interna->valor_semaforo = value; // pongo el valor del semaforo en la estructura que se va a mandar al backend:

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_INIT ,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
        
        // si el mensaje no logra mandarse, qué devuelve _send_message?    

        if (respuesta_backend === KERNEL_BACKEND){
            return 0;
        }
        else{
            return ERROR_RESPUESTA_BACKEND;
        }
        
    }
    else{
        return ERROR_FUNCION_NO_VALIDA; 
    }

}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {

    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->semaforo = sem; // pongo en la estructura el semaforo que se va a mandar al backend:

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_WAIT, armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
        
        if (respuesta_backend === KERNEL_BACKEND){ 
            return 0;
        }
        else{
            return ERROR_RESPUESTA_BACKEND;
        }
        
    }
    else{
        return ERROR_FUNCION_NO_VALIDA; // error de que no se puede comunicar con el kernel 
    }

}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {

    // podríamos sumar un if que, si en la respuesta_backend tiene 1 (o sea que esta comunicandose con kernel), haga todo esto y que, si no, retorne otra cosa

    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->semaforo = sem;  // pongo en la estructura el semaforo que se va a mandar al backend

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_POST, armar_paquete(estructura_interna) , sizeof(estructura_interna), logger);

        if (respuesta_backend === KERNEL_BACKEND){
            return 0;
        }
        else{
            return ERROR_RESPUESTA_BACKEND;
        }
        
    }
    else{
        return ERROR_FUNCION_NO_VALIDA; // error de que no se puede comunicar con el kernel 
    }

}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {

    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->semaforo = sem;  // pongo en la estructura el semaforo que se va a mandar al backend

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_DESTROY ,armar_paquete(estructura_interna), sizeof(estructura_interna) logger);

        // si el mensaje no logra mandarse, qué devuelve _send_message? para ver si sirve el if de abajo    
        
        if (respuesta_backend === KERNEL_BACKEND){
            return 0;
        }
        else{
            return ERROR_RESPUESTA_BACKEND;
        }
        
    }
    else{
        return ERROR_FUNCION_NO_VALIDA; // error de que no se puede comunicar con el kernel 
    }
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->dispositivo_io = mate_io_resource;  // pongo en la estructura el dispositivo que se va a mandar al backend:
        estructura_interna->mnesaje_io = msg; //  // pongo en la estructura el mensaje que se va a mandar al backend:

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_CALL_IO ,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);

        // si el mensaje no logra mandarse, qué devuelve _send_message? para ver si sirve el if de abajo    
        
        if (respuesta_backend === KERNEL_BACKEND){
            return 0;
        }
        else{
            return ERROR_RESPUESTA_BACKEND;
        }
        
    }
    else{
        return ERROR_FUNCION_NO_VALIDA; // error de que no se puede comunicar con el kernel 
    }


}

//--------------Memory Module Functions-------------------/ 
// ni idea qué tendríamos que hacer acá

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{
    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    estructura_interna->size_memoria = size;  // pongo en la estructura el size que se va a mandar al backend:

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
    /* en el ejemplo estaba esto, tendríamos que hacerlo?
    if (addr != 0)
    {
        return -1;
    }
    */

    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    estructura_interna->addr_memfree = addr;  // pongo en la estructura el address que se va a mandar al backend:

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }

}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
    /* en el ejemplo estaba esto, tendríamos que hacerlo?
    if (addr != 0)
    {
        return -1;
    }
    */

    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    estructura_interna->size_memoria = size;  // pongo en la estructura el size que se va a mandar al backend
    estructura_interna->dest_memread = dest;  // pongo en la estructura el dest que se va a mandar al backend    

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
    /* en el ejemplo estaba esto, tendríamos que hacerlo?
    if (addr != 0)
    {
        return -1;
    }
    */

    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    estructura_interna->origin_memwrite = origin;  // pongo en la estructura el origin que se va a mandar al backend
    estructura_interna->dest_memwrite = dest;  // pongo en la estructura el dest que se va a mandar al backend    
    estructura_interna->size_memoria = size;  // pongo en la estructura el size que se va a mandar al backend

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,armar_paquete(estructura_interna), sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}