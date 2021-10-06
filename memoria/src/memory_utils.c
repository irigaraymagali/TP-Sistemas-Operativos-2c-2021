#include "memory_utils.h"

void comandos(int valor){
    switch(valor){
        case mate_memalloc:
        break;
        case memfree:
        break;
        case memread:
        break;
        case memwrite:
        break;
        
        default:
         log_error(logger,"que comando metiste wachin????");
         
        
    }
   
}


void initPaginacion(){
    
    log_info(logger,"iniciando paginacion");

    configFile = config_create("../cfg/memoria.conf");

}

int memalloc(int espacioAReservar){

    int entra = entraEnElEspacioLibre(espacioAReservar);
    if(entra){
            //traer el espacio donde entra a memoria o a un espacio auxiliar
            //buscar el alloc
            //si es mas grande separarlo
        return 0;
    }else
    {
        /* 1-generar un nuevo alloc al final del espacio de direcciones
            2- si no cabe en las páginas ya reservadas se deberá solicitar más
         */
        return 0;
    }
    

}

int entraEnElEspacioLibre(int espacioAReservar, int processId){
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        while (temp->id != processId && ) {
            
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

        } 
    t_list_iterator* iterator2 = list_iterator_create(temp->paginas);    

    return 0;
}