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

int memalloc(int espacioAReservar, int processId){

    int entra = entraEnElEspacioLibre(espacioAReservar, processId);
    if(entra == -1){
       /* 1-generar un nuevo alloc al final del espacio de direcciones
            2- si no cabe en las páginas ya reservadas se deberá solicitar más
         */
        return 0;    
    }
    
    return entra;

}

// encuentra si hay un alloc para ubicar el espacio a reservar dentro de las paginas
int entraEnElEspacioLibre(int espacioAReservar, int processId){
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        while (temp->id != processId) {
            
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

        } 

        if(temp->id == processId){
            
            t_list_iterator* iterator2 = list_iterator_create(temp->paginas);  

            int nextAllocAux=0;
            int allocActual=0; 
            int paginaSiguiente =0;
            int paginaActual;
            
            while(list_iterator_has_next(iterator2)){
                
                Pagina *tempPag = (Pagina*) list_iterator_next(iterator2);

                void *paginaAux = malloc(tamanioDePagina);

                HeapMetaData unHeap;
                
                //traer la pagina a memoria por ahi se deba hacer mas adelante

                memcpy(paginaAux, memoria + (tempPag->frame * tamanioDePagina),tamanioDePagina);

                paginaSiguiente = (tempPag->pagina * tamanioDePagina) + tamanioDePagina;

                paginaActual = tempPag->pagina * tamanioDePagina;
                
                while (nextAllocAux <= paginaSiguiente && nextAllocAux != NULL)
                {
                    if(nextAllocAux==0){
                    
                    memcpy(unHeap.prevAlloc, paginaAux,sizeof(uint32_t));

                    memcpy(unHeap.nextAlloc, paginaAux + sizeof(uint32_t),sizeof(uint32_t));
                    nextAllocAux = unHeap.nextAlloc;

                    memcpy(unHeap.isfree, paginaAux + sizeof(uint32_t),sizeof(uint8_t));
                    }else{
                        memcpy(unHeap.prevAlloc, paginaAux + (nextAllocAux - paginaActual),sizeof(uint32_t));
                        
                        allocActual = unHeap.nextAlloc;
                        memcpy(unHeap.nextAlloc, paginaAux + sizeof(uint32_t),sizeof(uint32_t));
                        nextAllocAux = unHeap.nextAlloc;

                        memcpy(unHeap.isfree, paginaAux + sizeof(uint32_t),sizeof(uint8_t));
                    }

                    if(unHeap.isfree== 1 && (unHeap.nextAlloc - allocActual)>= espacioAReservar){
                        
                        // falta separarlo pero hay que preguntar si que pasa cuando el espacio a separar es menor a 10??
                        return allocActual;
                    }
                }
                  free(paginaAux);  
            }
        }
    return -1;
}