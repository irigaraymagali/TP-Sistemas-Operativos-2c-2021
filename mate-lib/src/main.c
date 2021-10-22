#include "main.h"
#include <matelib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

// En base al ejemplo de la cátedra, acá fui poniendo lo que modifiqué
//------------------General Functions---------------------/

#define KERNEL_BACKEND = 1 // cuando el kernel responda, va a deolver 1.
#define MEMORIA_BACKEND = 2 // cuando la memoria responda, va a responder 2
#define ERROR_RESPUESTA_BACKEND = -1 // error a devolver cuando la respuesta del backend no sea ni 1 ni 2 como se espera
#define ERROR_FUNCION_NO_VALIDA = -2 // error a devolver cuando se quiera usar una función del kernel pero se conectó con memoria

typedef struct mate_inner_structure
{
    void *memory;
    sem_t *sem_instance; // tendria que ser un array? --- sería poner **sem_instace ?

    float *rafaga_anterior; // para despues poder calcular la estimación siguiente
    float *estimacion_anterior; // idem
    float *estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
    float *llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
    int *prioridad; // 1 si tiene prioridad para pasar a ready -> es para los que vienen de suspended_ready a ready
    char *estado; // no sé cuánto nos va a servir, si no se puede hacer que sea estado_anterior y que nos evite tener otro para prioridad
 
  // datos para poder saber qué está pidiendo el carpincho cuando se conecte con backend
    sem_t *semaforo; 
    int *valor_semaforo; 
    mate_io_resource *dispositivo_io; 
    void mnesaje_io;
    int size_memoria;
    mate_pointer addr_memfree;
    mate_pointer origin_memread;
    void *dest_memread;
    void *origin_memwrite;
    mate_pointer dest_memwrite;

} mate_inner_structure;

// que onda esto y la memoria que usa? quien se encarga de darsela y de borrarla?
t_log* logger = log_create("./cfg/mate-lib.log", "MATE-LIB", true, LOG_LEVEL_INFO);

// idem anterior ?
int *respuesta_backend; // donde vamos a ir guardando la ultima respuesta del backend


int mate_init(mate_instance *lib_ref, char *config)
{
    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)   // creo la estructura interna 

    // falta leer archivo config que recibe la función para tener los datos de conexión, como se hace?
    
    char *ip; // valor del archivo de config recibido
    char *port: // valor del archivo de config recibido

    socket = _connect(ip, port, logger); // crea la conexión con los ip y puerto del config
    
    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_INIT, /* estructura_interna seriailzada*/, sizeof(estructura_interna), logger); // envia la estructura al backend para que inicialice todo
    
    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;  
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
    
    // faltaría agregar algo que chequee si la conexión se pudo hacer y si no que devuelva otro error 
        //de que no pudo. depende de lo que devuelva cuando no logra hacerla

} 


int mate_close(mate_instance *lib_ref)
{
    mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_CLOSE,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}

//-----------------Semaphore Functions---------------------/ 

// se podría hacer una función que abstraiga toda la logica de las funciones de semaforos y que reciba como argumento tambien el nombre de la función

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {
    
    if(respuesta_backend === KERNEL_BACKEND) // si la respuesta del backend fue 1, quiere decir que esta comunicandose con el kernel
    {
        mate_inner_structure estructura_interna = (mate_inner_structure *)lib_ref->group_info)

        estructura_interna->semaforo = sem; // pongo el semaforo en la estructura que se va a mandar al backend:
        estructura_interna->valor_semaforo = value; // pongo el valor del semaforo en la estructura que se va a mandar al backend:

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_INIT ,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
        
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

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_WAIT, /* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
        
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

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_POST, /* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);

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

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_SEM_DESTROY ,/* estructura_interna seriailzada*/, sizeof(estructura_interna) logger);

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

        respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_CALL_IO ,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);

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

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
    
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

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
    
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

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
    
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

    respuesta_backend = _send_message(socket, ID_MATE_LIB, MATE_MEMALLOC,/* estructura_interna seriailzada*/, sizeof(estructura_interna), logger);
    
    // si el mensaje no logra mandarse, qué devuelve _send_message? 
        //para ver si lo sumo al if de abajo y devuelvo otro error

    if(respuesta_backend === KERNEL_BAKEND || respuesta_backend === MEMORIA_BACKEND ){ // para que el carpincho reciba siempre lo mismo. la respuesta del backend va a devolver 1 o 2 según si va con memoria o con kernel
        return 0;
    }
    else{
        return ERROR_RESPUESTA_BACKEND;
    }
}