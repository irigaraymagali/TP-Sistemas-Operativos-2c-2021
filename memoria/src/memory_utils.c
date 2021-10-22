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
            log_error(logger,"Comando incorrecto");
            //que hacemos en este caso? nada?
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

        TablaDePaginasxProceso* temp = get_pages_by(processId);

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
        
        espacioFinalDisponible = (mayorNroDePagina* tamanioDePagina) - tempLastHeap - HEAP_METADATA_SIZE; // el 9 es porque hay que agregar el puto heap atras

        HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData));
        nuevoHeap->prevAlloc = tempLastHeap;
        nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
        nuevoHeap->isfree = 1; // Nose si esto tiene que significar que es vacio o que está ocupado.

        // esto funciona si y solo si la pagina esta en memoria mas adelante hay que agregar los cambios nesesarios para utilizar el swap

        int offset;
        if(espacioFinalDisponible >= espacioAReservar){
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - (mayorNroDePagina * tamanioDePagina)) ;

            int espacioTotal = tempLastHeap + espacioAReservar;
            memcpy(memoria + offset + sizeof(uint32_t), &espacioTotal, sizeof(uint32_t));
            offset = offset + HEAP_METADATA_SIZE + espacioAReservar;

            memcpy(memoria + offset, &nuevoHeap->prevAlloc, sizeof(uint32_t));
            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->nextAlloc, sizeof(uint32_t));
            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->isfree, sizeof(uint8_t));
        } else {
            /* 
                    como me da paja de hacerlo ahora esto basicamente es pedir una pagina nueva copiar esas ultimas paginas en un auxiliar
                    hacer lo de arriba en un auxiliar 
                    y pegar las n paginas  de una en la memoria
            */

           //FALTA CAMBIARLE EL ISFREE DE LA PAG
           agregarXPaginasPara(processId, (espacioAReservar-espacioFinalDisponible));

            int espacioDePaginasAux = (((espacioAReservar - espacioFinalDisponible) / tamanioDePagina) + 1)* tamanioDePagina;

            void* espacioAuxiliar = malloc(espacioDePaginasAux + tamanioDePagina);
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - (mayorNroDePagina * tamanioDePagina));

            int espacioTotal = tempLastHeap + espacioAReservar;
            memcpy(memoria + offset + sizeof(uint32_t), &espacioTotal, sizeof(uint32_t));


            //obtener ultima pagina
            Pagina *ultimaPag = getLastPageDe(processId); //Que pasa si no tiene paginas?
            int offsetAux = 0;

            memcpy(espacioAuxiliar, memoria + (ultimoFrame*tamanioDePagina), tamanioDePagina);
            offsetAux += (tempLastHeap - (mayorNroDePagina * tamanioDePagina)) + espacioAReservar; 
            
            memcpy(espacioAuxiliar + offsetAux, &nuevoHeap->prevAlloc, sizeof(uint32_t));
            offsetAux += sizeof(uint32_t);

            memcpy(memoria + offsetAux, &nuevoHeap->nextAlloc, sizeof(uint32_t));
            offsetAux += sizeof(uint32_t);

            memcpy(memoria + offsetAux, &nuevoHeap->isfree, sizeof(uint8_t));

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

TablaDePaginasxProceso* get_pages_by(int processID){
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    while (temp->id != processID) {
        temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    }
    list_iterator_destroy(iterator);
    return temp; 
}

// encuentra si hay un alloc para ubicar el espacio a reservar dentro de las paginas
int entraEnElEspacioLibre(int espacioAReservar, int processId){
    TablaDePaginasxProceso* temp = get_pages_by(processId);

    if(temp->id == processId){
        t_list_iterator* iterator = list_iterator_create(temp->paginas);  

        int nextAllocAux = 0;
        int allocActual = 0; 
        int paginaSiguiente = 0;
        int paginaActual;
        
        while(list_iterator_has_next(iterator)){
            
            Pagina *tempPag = (Pagina*) list_iterator_next(iterator);

            void *paginaAux = malloc(tamanioDePagina);

            HeapMetaData* unHeap = malloc(sizeof(HeapMetaData));
            
            //traer la pagina a memoria por ahi se deba hacer mas adelante

            memcpy(paginaAux, memoria + (tempPag->frame * tamanioDePagina),tamanioDePagina);

            paginaSiguiente = (tempPag->pagina * tamanioDePagina) + tamanioDePagina;

            paginaActual = tempPag->pagina * tamanioDePagina;
            
            while (nextAllocAux <= paginaSiguiente && nextAllocAux != NULL_ALLOC)
            {
                int offset = 0;
                if(nextAllocAux == 0){
                    memcpy(&unHeap->prevAlloc, paginaAux, sizeof(uint32_t));
                    offset += sizeof(uint32_t);
                    memcpy(&unHeap->nextAlloc, paginaAux + offset, sizeof(uint32_t));
                    nextAllocAux = unHeap->nextAlloc;
                    offset += sizeof(uint32_t);

                    memcpy(&unHeap->isfree, paginaAux + offset, sizeof(uint8_t));
                } else {
                   offset = (nextAllocAux - paginaActual);
                    memcpy(&unHeap->prevAlloc, paginaAux + offset, sizeof(uint32_t));
                    allocActual = unHeap->nextAlloc;
                    offset += sizeof(uint32_t); 

                    memcpy(&unHeap->nextAlloc, paginaAux + offset, sizeof(uint32_t));
                    nextAllocAux = unHeap->nextAlloc;
                    offset += sizeof(uint32_t);

                    memcpy(&unHeap->isfree, paginaAux + offset, sizeof(uint8_t));
                }

                if(unHeap->isfree == BUSY && (unHeap->nextAlloc - allocActual) >= espacioAReservar){
                    // falta separarlo y sumar 9 que es el valor de la estructura que iria al final
                    return allocActual;
                }
            }
            free(paginaAux);  
        }
        list_iterator_destroy(iterator);
    }
    return -1;
}

Pagina *getLastPageDe(int processId){
    int mayorNroDePagina = -1;
    Pagina *ultimaPagina; //Nose si necesitamos un malloc porque en caso de encontrar una pagina temporal creo estariamos guardando este malloc basura. PROBAR.
    
    TablaDePaginasxProceso* temp = get_pages_by(processId);
    if(list_is_empty(temp->paginas)){
        return NULL;
    }

    t_list_iterator* iterator = list_iterator_create(temp->paginas);
    while(list_iterator_has_next(iterator)){
        Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator);

        if((mayorNroDePagina < paginaTemporal->pagina)  && paginaTemporal->isfree == 0){
            mayorNroDePagina = paginaTemporal->pagina;
            ultimaPagina = paginaTemporal;
        }
    }
    list_iterator_destroy(iterator);

    return ultimaPagina;
}

void agregarXPaginasPara(int processId, int espacioRestante){
    int cantidadDePaginasAAgregar = (espacioRestante / tamanioDePagina) + 1; //por que +1?
    Pagina *ultimaPagina;
    Pagina *nuevaPagina = malloc(sizeof(Pagina));
       
    while(cantidadDePaginasAAgregar == 0){
        ultimaPagina = getLastPageDe(processId); // que pasaría si no tenes paginas?
        if(ultimaPagina == NULL){
            nuevaPagina->pagina = FIRST_PAGE;
        } else {
            nuevaPagina->pagina = ultimaPagina->pagina + 1;
        }
        nuevaPagina->frame = getNewEmptyFrame();
        nuevaPagina->isfree = 1;
        nuevaPagina->bitModificado = 0;
        nuevaPagina->bitPresencia = 0;
        lRUACTUAL++;
        nuevaPagina->lRU = lRUACTUAL;

        TablaDePaginasxProceso* temp = get_pages_by(processId);

        list_add(temp->paginas, nuevaPagina);

        cantidadDePaginasAAgregar--;
    }      
}

int getNewEmptyFrame(){
    int emptyFrame = 0;
    int paginaFinal = tamanioDeMemoria/tamanioDeMemoria;
    int estaLibre = 0;

    while(emptyFrame < paginaFinal){
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
    while (list_iterator_has_next(iterator)) {
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


int getFrameDeUn(int processId, int mayorNroDePagina){

    TablaDePaginasxProceso* temp = get_pages_by(processId);

    t_list_iterator* iterator = list_iterator_create(temp->paginas);

    Pagina *tempPagina = list_iterator_next(iterator);
    while (list_iterator_has_next(iterator)  || tempPagina->pagina == mayorNroDePagina) {
        Pagina *tempPagina = list_iterator_next(iterator);
    }

    if(tempPagina->pagina == mayorNroDePagina){
        list_iterator_destroy(iterator);
        return tempPagina->frame;
    }

    list_iterator_destroy(iterator);

    return -1;
}

void inicializarUnProceso(int idDelProceso){

    HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData));
    nuevoHeap->prevAlloc = 0;
    nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
    nuevoHeap->isfree = 1;

    TablaDePaginasxProceso* nuevaTablaDePaginas = malloc(sizeof(TablaDePaginasxProceso));
    nuevaTablaDePaginas->id = idDelProceso;
    nuevaTablaDePaginas->lastHeap = 0;
    
    if(tipoDeAsignacionDinamica){
        int nuevoFrame = getNewEmptyFrame();
        int offset = nuevoFrame * tamanioDePagina;

        memcpy(memoria + offset,nuevoHeap->prevAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset,nuevoHeap->nextAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset,nuevoHeap->isfree,sizeof(u_int8_t));

        Pagina* nuevaPagina = malloc(sizeof(nuevaPagina));
        nuevaPagina->pagina=1;
        nuevaPagina->lRU = lRUACTUAL;
        nuevaPagina->isfree= 0;
        nuevaPagina->frame = nuevoFrame;
        nuevaPagina->bitPresencia =0;
        nuevaPagina->bitModificado = 1;

        list_add(nuevaTablaDePaginas->paginas, nuevaPagina);

    }else{
        int paginasCargadas = 1;

        while (paginasCargadas != cantidadDePaginasPorProceso)
        {
  
            if (paginasCargadas == 1)
            {
                int nuevoFrame = getNewEmptyFrame();
                int offset = nuevoFrame * tamanioDePagina;

                memcpy(memoria + offset,nuevoHeap->prevAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset,nuevoHeap->nextAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset,nuevoHeap->isfree,sizeof(u_int8_t));

                Pagina* nuevaPagina = malloc(sizeof(nuevaPagina));
                nuevaPagina->pagina=1;
                nuevaPagina->lRU = lRUACTUAL;
                nuevaPagina->isfree= 0;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia =0;
                nuevaPagina->bitModificado = 1;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }else
            {
                int nuevoFrame = getNewEmptyFrame();
                int offset = nuevoFrame * tamanioDePagina;

                Pagina* nuevaPagina = malloc(sizeof(nuevaPagina));
                nuevaPagina->pagina=paginasCargadas;
                nuevaPagina->lRU = lRUACTUAL;
                nuevaPagina->isfree= 1;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia =0;
                nuevaPagina->bitModificado = 1;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }
            
            
            paginasCargadas++;
        }
        
    }

    free(nuevoHeap);
}