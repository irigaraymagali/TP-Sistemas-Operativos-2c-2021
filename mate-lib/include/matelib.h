#ifndef MATELIB_H
#define MATELIB_H
// habría que cambiarlo por MATELIB_H_INCLUDED pero no se si impactará en otro lado


#include <stdio.h>
#include <commons/log.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../shared/include/socket.h"
#include "../../shared/include/shared_utils.h"
#include "../../shared/include/serialization.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>


//-------------------Type Definitions----------------------/
typedef struct mate_instance
{
    void *group_info; // un puntero a algun tipo de estructura que vamos a llenar despues con todas las referencias necesarias para mantener vivas las conexiones y para operar con la misma
} mate_instance;


t_log* logger;
int id_carpincho;

t_config* datos_configuracion;
// TODO: Docstrings

//------------------General Functions---------------------/
int mate_init(mate_instance *lib_ref, char *config); // cuando un carpincho quiera usar la lib, le va a pasar la estructura y el archivo de configuración

int mate_close(mate_instance *lib_ref);

//-----------------Semaphore Functions---------------------/
int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value);

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem);

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem);

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg);

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size);

int mate_memfree(mate_instance *lib_ref, mate_pointer addr);

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size);

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size);

mate_inner_structure* nueva_estructura_interna();

#endif