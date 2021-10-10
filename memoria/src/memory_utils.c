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
    int mayorNroDePagina =-1;
    int ultimoFrame =0;
    int tempLastHeap=0;
    int espacioFinalDisponible=0;

    if(entra == -1){
       /* 1-generar un nuevo alloc al final del espacio de direcciones
            2- si no cabe en las páginas ya reservadas se deberá solicitar más
         */
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
         while (temp->id != processId) {
            
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
 
        } 

        tempLastHeap= temp->lastHeap;

        //NOTA MENTAL BUSCAR LA FUNCION DEL TP PASADO PARA BUSCAR LA ULTIMA PAGINA DE UN PROCESO Y SACAR ESTA PIJA

        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);

            while(list_iterator_has_next(iterator2)){
                Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator2);

                if(mayorNroDePagina < paginaTemporal->pagina){
                    mayorNroDePagina = paginaTemporal->pagina;
                    ultimoFrame = paginaTemporal->frame;
                }
            }
        espacioFinalDisponible = (mayorNroDePagina* tamanioDePagina) - tempLastHeap - 9 ; // el 9 es porque hay que agregar el puto heap atras

        HeapMetaData nuevoHeap ;
        nuevoHeap.prevAlloc = tempLastHeap;
        nuevoHeap.nextAlloc = NULL;
        nuevoHeap.isfree = 0;

        // esto funciona si y solo si la pagina esta en memoria mas adelante hay que agregar los cambios nesesarios para utilizar el swap

        int offset;

        if(espacioFinalDisponible >= espacioAReservar){
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - (mayorNroDePagina * tamanioDePagina)) ;

            memcpy(memoria + offset + sizeof(uint32_t) , (tempLastHeap + espacioAReservar), sizeof(uint32_t));

            offset = offset + 9 + espacioAReservar;

            memcpy(memoria + offset , nuevoHeap.prevAlloc, sizeof(uint32_t));

            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset , nuevoHeap.nextAlloc, sizeof(uint32_t));

            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset , nuevoHeap.isfree, sizeof(uint8_t));
        }else{
            /* 
                    como me da paja de hacerlo ahora esto basicamente es pedir una pagina nueva copiar esas ultimas paginas en un auxiliar
                    hacer lo de arriba en un auxiliar 
                    y pegar las n paginas  de una en la memoria
            */
           agregarXPaginasPara(processId, (espacioAReservar-espacioFinalDisponible));

            int espacioDePaginasAux = (((espacioAReservar - espacioFinalDisponible) /tamanioDePagina) + 1)* tamanioDePagina;

           void* espacioAuxiliar = malloc( espacioDePaginasAux + tamanioDePagina );

           offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - (mayorNroDePagina * tamanioDePagina)) ;

           memcpy(memoria + offset + sizeof(uint32_t) , (tempLastHeap + espacioAReservar), sizeof(uint32_t));


            //obtener ultima pagina
            Pagina ultimaPag = getLastPageDe(processId);

            memcpy(espacioAuxiliar,memoria + (ultimoFrame*tamanioDePagina),tamanioDePagina);

            memcpy(espacioAuxiliar + 9 + espacioAReservar,nuevoHeap.prevAlloc, sizeof(uint32_t));

            memcpy(memoria + 9 + espacioAReservar + sizeof(uint32_t) , nuevoHeap.nextAlloc, sizeof(uint32_t));

            memcpy(memoria + 9 + espacioAReservar + 2*sizeof(uint32_t) , nuevoHeap.isfree, sizeof(uint8_t));

            int paginaInicial = mayorNroDePagina;

            while(mayorNroDePagina <= ultimaPag.pagina){
                int framenecesitado = mayorNroDePagina;
                
                framenecesitado = getFrameDeUn(processId, mayorNroDePagina);

                memcpy(memoria + (framenecesitado*tamanioDePagina), espacioAuxiliar + (tamanioDePagina*(mayorNroDePagina-paginaInicial)), tamanioDePagina);

                mayorNroDePagina++;
            }

            free(espacioAuxiliar);
        }

        return (tempLastHeap + espacioAReservar);    
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
                        
                        // falta separarlo y sumar 9 que es el valor de la estructura que iria al final
                        return allocActual;
                    }
                }
                  free(paginaAux);  
            }
        }
    return -1;
}