#include "main.h"
#include <matelib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

// En base al ejemplo de la cátedra, acá fui poniendo lo que modifiqué
//------------------General Functions---------------------/


typedef struct mate_inner_structure
{
  void *memory;
  sem_t *sem_instance; // tendria que ser un array? --- sería poner **sem_instace ?
  // se me ocurre que podríamos necesitar agregar
  float *rafaga_anterior; // para despues poder calcular la estimación siguiente
  float *estimacion_anterior; // idem
  float *estimacion_siguiente; // para poder ir guardando acá la estimación cuando se haga
  float *llegada_a_ready; //para guardar cuándo llego a ready para usar en HRRN
  // prioridad para pasar a ready los que vienen de suspended_ready a ready
  int *prioridad;
} mate_inner_structure;


// para mandar la instancia del carpincho a memoria/kernel, mandar: (mate_inner_structure *)lib_ref->group_info)

int mate_init(mate_instance *lib_ref, char *config)
{
  lib_ref->group_info = malloc(sizeof(mate_inner_structure)); 
  
    /* 
        - leer el archivo config para tener la IP y PUERTO para conectarse
        - mandar mensaje a memoria/kernel con la instancia del carpincho para que:
            genere su estructura
            avise que un nuevo carpincho llego a new
            y devuelva 0 cuando el carpincho pase a EXEC
    */
  return 0; // acá retornar lo que devuelve la conexión, así cuando retorna 0, devuelve 0
}


int mate_close(mate_instance *lib_ref)
{
  free(lib_ref->group_info);
    /* se conecta con kernel/memoria para pedir que borre la estructura*/ 
  return 0; //retorna lo que devuelva la conexión
}

//-----------------Semaphore Functions---------------------/

int mate_sem_init(mate_instance *lib_ref, mate_sem_name sem, unsigned int value) {
  if (strncmp(sem, "SEM1", 4))   
  {
    return -1;  // por qué si el semaforo no es SEM1 devuelve -1?
  }
  ((mate_inner_structure *)lib_ref->group_info)->sem_instance = malloc(sizeof(sem_t));
  sem_init(((mate_inner_structure *)lib_ref->group_info)->sem_instance, 0, value);
  return 0;

   /* 
        enviar al kernel/memoria el nombre del semaforo, valor para inicializarlo, y la sem_inner_structure
        devolver lo que devuelva la conexión
    */
}

int mate_sem_wait(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  return sem_wait(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
 
 /*
    enviar al kernel/memoria el nombre del semaforo, valor para inicializarlo, y la sem_inner_structure
    devolver lo que devuelva la conexión
 */


}

int mate_sem_post(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  return sem_post(((mate_inner_structure *)lib_ref->group_info)->sem_instance);

   /*
    verificar si el semaforo está en el array de semaforos
    si no está, devolver -1
    si está, cambiar el valor y devolver 0
    */
}

int mate_sem_destroy(mate_instance *lib_ref, mate_sem_name sem) {
  if (strncmp(sem, "SEM1", 4))
  {
    return -1;
  }
  int res = sem_destroy(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
  free(((mate_inner_structure *)lib_ref->group_info)->sem_instance);
  return res;

  /*
    enviar el semaforo al kernel para que ahí lo elimine todo y retorne la respuesta
  */
}

//--------------------IO Functions------------------------/

int mate_call_io(mate_instance *lib_ref, mate_io_resource io, void *msg)
{
  printf("Doing IO %s...\n", io);
  usleep(10 * 1000);
  if (!strncmp(io, "PRINTER", 7))
  {
    printf("Printing content: %s\n", (char *)msg);
  }
  printf("Done with IO %s\n", io);
  return 0;

  /*
    enviar el io y la instancia al kernel
    que el kernel haga lo necesario para planificación y lo tenga hasta que corresponda
    cuando termina que retorne lo que devuelva.
    en el medio escribir cosas en el log para que se sepa que se esta usando
  */
}

//--------------Memory Module Functions-------------------/

mate_pointer mate_memalloc(mate_instance *lib_ref, int size)
{
  ((mate_inner_structure *)lib_ref->group_info)->memory = malloc(size);
  return 0;
}

int mate_memfree(mate_instance *lib_ref, mate_pointer addr)
{
  if (addr != 0)
  {
    return -1;
  }
  free(((mate_inner_structure *)lib_ref->group_info)->memory);
  return 0;
}

int mate_memread(mate_instance *lib_ref, mate_pointer origin, void *dest, int size)
{
  if (origin != 0)
  {
    return -1;
  }
  memcpy(dest, ((mate_inner_structure *)lib_ref->group_info)->memory, size);
  return 0;
}

int mate_memwrite(mate_instance *lib_ref, void *origin, mate_pointer dest, int size)
{
  if (dest != 0)
  {
    return -1;
  }
  memcpy(((mate_inner_structure *)lib_ref->group_info)->memory, origin, size);
  return 0;
}