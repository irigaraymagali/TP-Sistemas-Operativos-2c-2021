#include "memory_utils.h"

void comandos(int valor){
    switch(valor){
        case MEM_ALLOC:
        break;
        case MEM_FREE:
        break;
        case MEM_READ:
        break;
        case MEM_WRITE:
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
    int mayorNroDePagina = -1;
    int ultimoFrame = 0;
    int tempLastHeap = 0;
    int espacioFinalDisponible = 0;

    if(entra == -1){
       /* 1-generar un nuevo alloc al final del espacio de direcciones
            2- si no cabe en las páginas ya reservadas se deberá solicitar más
         */
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        while (temp->id != processId) {
            
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        } 
        list_iterator_destroy(iterator);

        tempLastHeap = temp->lastHeap;

        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);

        while(list_iterator_has_next(iterator2)){
            Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator2);

            if(mayorNroDePagina < paginaTemporal->pagina){
                mayorNroDePagina = paginaTemporal->pagina;
                ultimoFrame = paginaTemporal->frame;
            }
        }
        list_iterator_destroy(iterator2);
        
        espacioFinalDisponible = (mayorNroDePagina* tamanioDePagina) - tempLastHeap - 9; // el 9 es porque hay que agregar el puto heap atras

        HeapMetaData nuevoHeap;
        nuevoHeap.prevAlloc = tempLastHeap;
        nuevoHeap.nextAlloc = NULL;
        nuevoHeap.isfree = 0;

        // esto funciona si y solo si la pagina esta en memoria mas adelante hay que agregar los cambios nesesarios para utilizar el swap

        int offset;
        if(espacioFinalDisponible >= espacioAReservar){
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - (mayorNroDePagina * tamanioDePagina)) ;

            memcpy(memoria + offset + sizeof(uint32_t) , (tempLastHeap + espacioAReservar), sizeof(uint32_t));

            offset = offset + 9 + espacioAReservar;

            memcpy(memoria + offset, nuevoHeap.prevAlloc, sizeof(uint32_t));

            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, nuevoHeap.nextAlloc, sizeof(uint32_t));

            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, nuevoHeap.isfree, sizeof(uint8_t));
        } else {
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
            Pagina *ultimaPag = getLastPageDe(processId);

            memcpy(espacioAuxiliar,memoria + (ultimoFrame*tamanioDePagina),tamanioDePagina);

            memcpy(espacioAuxiliar + 9 + espacioAReservar,nuevoHeap.prevAlloc, sizeof(uint32_t));

            memcpy(memoria + 9 + espacioAReservar + sizeof(uint32_t) , nuevoHeap.nextAlloc, sizeof(uint32_t));

            memcpy(memoria + 9 + espacioAReservar + 2*sizeof(uint32_t) , nuevoHeap.isfree, sizeof(uint8_t));

            int paginaInicial = mayorNroDePagina;

            while(mayorNroDePagina <= ultimaPag->pagina){
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
        list_iterator_destroy(iterator); 

        if(temp->id == processId){
            
            t_list_iterator* iterator2 = list_iterator_create(temp->paginas);  

            int nextAllocAux = 0;
            int allocActual = 0; 
            int paginaSiguiente = 0;
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
            list_iterator_destroy(iterator2);
        }
    return -1;
}

Pagina *getLastPageDe(int processId){
    int mayorNroDePagina = -1;
    Pagina *ultimaPagina;
    
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    while (temp->id != processId) {
        
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

    }
    list_iterator_destroy(iterator);

    t_list_iterator* iterator2 = list_iterator_create(temp->paginas);

    while(list_iterator_has_next(iterator2)){
        Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator2);

        if((mayorNroDePagina < paginaTemporal->pagina)  && paginaTemporal->isfree==1){
            mayorNroDePagina = paginaTemporal->pagina;
            ultimaPagina = paginaTemporal;
        }
    }
    list_iterator_destroy(iterator2);

    return ultimaPagina;
}

void agregarXPaginasPara(int processId, int espacioRestante){
    int cantidadDePaginasAAgregar = (espacioRestante/tamanioDePagina)+1;
    Pagina *ultimaPagina;
    Pagina *nuevaPagina;
       
    while(cantidadDePaginasAAgregar == 0){
        ultimaPagina = getLastPageDe(processId);

        nuevaPagina->pagina = ultimaPagina->pagina+1;
        nuevaPagina->frame = getNewEmptyFrame();
        nuevaPagina->isfree = 1;
        nuevaPagina->bitModificado =0;
        nuevaPagina->bitPresencia =0;
        lRUACTUAL++;
        nuevaPagina->lRU = lRUACTUAL;

        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        while (temp->id != processId) {
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        }
        list_iterator_destroy(iterator);

        list_add(temp->paginas, nuevaPagina);

        cantidadDePaginasAAgregar--;
    }      
}

int getNewEmptyFrame(){
    int emptyFrame = 0;
    int paginaFinal = tamanioDeMemoria/tamanioDeMemoria;
    int estaLibre = 0;

    while(emptyFrame<paginaFinal){

        estaLibre = estaOcupadoUn(emptyFrame);

        if(estaLibre){
            return emptyFrame;
        }

        emptyFrame++;
    }

    return emptyFrame;
}

int estaOcupadoUn(int emptyFrame){
    int estaOcupado = 0;
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    while (list_iterator_has_next(temp)) {
    
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

        t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
    
        while(list_iterator_has_next(iterator2)){
    
            Pagina *tempPagina = (Pagina*) list_iterator_has_next(iterator2);
    
            if(tempPagina->frame == emptyFrame){
                return tempPagina->isfree;
            }
        }
    }

    return estaOcupado;
}