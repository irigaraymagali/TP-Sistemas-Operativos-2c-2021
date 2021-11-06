#include "memory_utils.h"

void initPaginacion(){
    log_info(logger,"iniciando paginacion");

    pthread_mutex_init(&lru_mutex, NULL);

    config = config_create(CONFIG_PATH);

    tamanioDePagina = config_get_int_value(config, "TAMANIO_PAGINA");

    if (string_equals_ignore_case(config_get_string_value(config,"TIPO_ASIGNACION"), "FIJA"))
    {
        tipoDeAsignacionDinamica= 0;
    }
    else
    {
        tipoDeAsignacionDinamica= 1;
    }
    
    lRUACTUAL=0;

    tamanioDeMemoria= config_get_int_value(config, "TAMANIO");

    memoria = malloc(tamanioDeMemoria);

    cantidadDePaginasPorProceso = config_get_int_value(config, "MARCOS_POR_CARPINCHO");

    todasLasTablasDePaginas = list_create();
}

int memalloc(int processId, int espacioAReservar){
    int entra = entraEnElEspacioLibre(espacioAReservar, processId);
    uint32_t mayorNroDePagina = 0; // 0 porque indica que no tiene asignado ninguna pagina, las pags siempre arancan en 1
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

            if(mayorNroDePagina < paginaTemporal->pagina && paginaTemporal->isfree == BUSY){
                mayorNroDePagina = paginaTemporal->pagina;
                ultimoFrame = paginaTemporal->frame;
            }
        }
        list_iterator_destroy(iterator2);
        
        espacioFinalDisponible = (mayorNroDePagina* tamanioDePagina) - tempLastHeap - HEAP_METADATA_SIZE; // el 9 es porque hay que agregar el puto heap atras

        HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData));
        nuevoHeap->prevAlloc = tempLastHeap;
        nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
        nuevoHeap->isfree = BUSY; // Nose si esto tiene que significar que es vacio o que está ocupado.

        // esto funciona si y solo si la pagina esta en memoria mas adelante hay que agregar los cambios nesesarios para utilizar el swap

        int offset;
        espacioAReservar += HEAP_METADATA_SIZE;

        if(espacioFinalDisponible >= espacioAReservar){
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - ((mayorNroDePagina-1) * tamanioDePagina)) ;

            uint32_t espacioTotal = tempLastHeap + espacioAReservar;
            memcpy(memoria + offset + sizeof(uint32_t), &espacioTotal, sizeof(uint32_t));
            offset = offset + 2*sizeof(uint32_t);
            
            memcpy(memoria + offset, &nuevoHeap->isfree, sizeof(uint8_t));
            offset = offset  + espacioAReservar - 2*sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->prevAlloc, sizeof(uint32_t));            
            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->nextAlloc, sizeof(uint32_t));
            offset = offset + sizeof(uint32_t);

            

            temp->lastHeap = tempLastHeap + espacioAReservar;

            return (tempLastHeap + HEAP_METADATA_SIZE);
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
            offset = (ultimoFrame*tamanioDePagina) + (tempLastHeap - ((mayorNroDePagina-1) * tamanioDePagina));

            int espacioTotal = tempLastHeap + espacioAReservar + HEAP_METADATA_SIZE;
            memcpy(memoria + offset + sizeof(uint32_t), &espacioTotal, sizeof(uint32_t));

            memcpy(memoria + offset + 2*sizeof(uint32_t),&nuevoHeap->isfree, sizeof(uint32_t));


            //obtener ultima pagina
            Pagina *ultimaPag = getLastPageDe(processId); //Que pasa si no tiene paginas?
            int offsetAux = 0;

            memcpy(espacioAuxiliar, memoria + (ultimoFrame*tamanioDePagina), tamanioDePagina);
            offsetAux += (tempLastHeap - ((mayorNroDePagina-1) * tamanioDePagina)) + espacioAReservar; 
            
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
        temp->lastHeap = tempLastHeap + espacioAReservar;
        return (tempLastHeap + HEAP_METADATA_SIZE );    
    }
    
    return entra;

}

void* memread(uint32_t pid, int dir_logica){
    void* read;
    int size_to_read, offset_to_read;

    void* err_msg = _serialize(sizeof(int), "%d", MATE_READ_FAULT);
    HeapMetaData* heap;

    log_info(logger, "Buscando la tabla de paginas del proceso %d", pid);

    TablaDePaginasxProceso* pages = get_pages_by(pid);
    if (pages->id != pid){
        log_error(logger, "El proceso ingresado es incorrecto");
        return err_msg;
    }

    Pagina* page = get_page_by_dir_logica(pages, dir_logica);
    if (page == NULL){
        return err_msg;
    } 
    if(page->bitPresencia == 0){
        log_info(logger, "TODO: buscar en swap");
    }

    heap = get_heap_metadata(dir_logica);
    if(heap->isfree != BUSY && heap->isfree != FREE){
        log_error(logger, "La direccion logica recibida es incorrecta");
        return err_msg;
    }
    if(heap->isfree == FREE){
        log_error(logger, "La direccion logica apunta a un espacio de memoria vacío");
        return err_msg;
    }
    
    offset_to_read = dir_logica + HEAP_METADATA_SIZE; //TODO: AVERIGUAR si apunta por fuera
    size_to_read = heap->nextAlloc - HEAP_METADATA_SIZE;
    read = malloc(size_to_read);
    memcpy(read, memoria + offset_to_read, size_to_read);

    pthread_mutex_lock(&lru_mutex);
    page->lRU = lRUACTUAL;
    lRUACTUAL++;
    pthread_mutex_unlock(&lru_mutex);
    page->bitUso = 1;

    return read;
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
        int dirPaginaSiguiente = 0;
        int dirPaginaActual;
        
        while(list_iterator_has_next(iterator)){
            
            Pagina *tempPag = (Pagina*) list_iterator_next(iterator);

            void *paginaAux = malloc(tamanioDePagina);

            HeapMetaData* unHeap = malloc(sizeof(HeapMetaData));
            
            //traer la pagina a memoria por ahi se deba hacer mas adelante

            memcpy(paginaAux, memoria + (tempPag->frame * tamanioDePagina),tamanioDePagina);

            dirPaginaSiguiente = ((tempPag->pagina-1) * tamanioDePagina) + tamanioDePagina;

            dirPaginaActual = (tempPag->pagina-1) * tamanioDePagina;
            
            while (nextAllocAux <= dirPaginaSiguiente)
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
                   offset = (nextAllocAux - dirPaginaActual);
                    memcpy(&unHeap->prevAlloc, paginaAux + offset, sizeof(uint32_t));
                    allocActual = unHeap->nextAlloc;
                    offset += sizeof(uint32_t); 

                    memcpy(&unHeap->nextAlloc, paginaAux + offset, sizeof(uint32_t));
                    nextAllocAux = unHeap->nextAlloc;
                    offset += sizeof(uint32_t);

                    memcpy(&unHeap->isfree, paginaAux + offset, sizeof(uint8_t));
                }
                
                if (unHeap->nextAlloc == NULL_ALLOC)
                {
                    return -1;
                }
                

                if(unHeap->isfree == FREE && (unHeap->nextAlloc - allocActual - HEAP_METADATA_SIZE) >= espacioAReservar){
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
    uint32_t mayorNroDePagina = 0;
    Pagina *ultimaPagina; //Nose si necesitamos un malloc porque en caso de encontrar una pagina temporal creo estariamos guardando este malloc basura. PROBAR.
    
    TablaDePaginasxProceso* temp = get_pages_by(processId);
    if(list_is_empty(temp->paginas)){
        return NULL;
    }

    t_list_iterator* iterator = list_iterator_create(temp->paginas);
    while(list_iterator_has_next(iterator)){
        Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator);

        if((mayorNroDePagina < paginaTemporal->pagina)  && paginaTemporal->isfree == BUSY){
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
       
    if(tipoDeAsignacionDinamica == 1){
        while(cantidadDePaginasAAgregar != 0){
            ultimaPagina = getLastPageDe(processId); // que pasaría si no tenes paginas?
            if(ultimaPagina == NULL){
                nuevaPagina->pagina = FIRST_PAGE;
            } else {
                nuevaPagina->pagina = ultimaPagina->pagina + 1;
            }

            nuevaPagina->frame = getNewEmptyFrame(processId);
            nuevaPagina->isfree = FREE;
            nuevaPagina->bitModificado = 0;
            nuevaPagina->bitPresencia = 0;
            lRUACTUAL++;
            nuevaPagina->lRU = lRUACTUAL;

            TablaDePaginasxProceso* temp = get_pages_by(processId);

            list_add(temp->paginas, nuevaPagina);

            cantidadDePaginasAAgregar--;
        }
    }else{
        while(cantidadDePaginasAAgregar != 0){
            TablaDePaginasxProceso* temp = get_pages_by(processId);

            ultimaPagina = getLastPageDe(processId);

            t_list_iterator* iterator = list_iterator_create(temp->paginas);
            Pagina* paginaSiguienteALaUltima = (Pagina*) list_iterator_next(iterator);

            while (list_iterator_has_next(iterator) && (ultimaPagina->pagina +1 != paginaSiguienteALaUltima->pagina))
            {
               paginaSiguienteALaUltima = (Pagina*) list_iterator_next(iterator);
            }

            paginaSiguienteALaUltima->isfree = BUSY;

            list_iterator_destroy(iterator);
            
            cantidadDePaginasAAgregar--;
        }
    }      
}

int getNewEmptyFrame(int idProcess){
    int emptyFrame = 0;
    int paginaFinal = tamanioDeMemoria/tamanioDePagina;
    int isfree = 0;

    if (tipoDeAsignacionDinamica)
    {
        while(emptyFrame <= paginaFinal){
            isfree= estaOcupadoUn(emptyFrame, idProcess);
            if(isfree!= BUSY){
                return emptyFrame;
            }

            emptyFrame++;
        }
    }
    else
    {
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

        while (temp->id != idProcess) {

            temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);  

        }

        t_list_iterator * iterator2 = list_iterator_create(temp->paginas);

        while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
    
                if(tempPagina->isfree == FREE ){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    return tempPagina->frame;
                }
        }
    }

    return emptyFrame;
}

int estaOcupadoUn(int emptyFrame, int idProcess){
    int isfree = FREE;
    if(todasLasTablasDePaginas != NULL){
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
            while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
    
                if(tempPagina->frame == emptyFrame ){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    return tempPagina->isfree;
                }
            }
        }
    list_iterator_destroy(iterator);
    }
    return isfree;
}

int getframeNoAsignadoEnMemoria(){
    int emptyFrame = 0;
    int framesTotales = (tamanioDeMemoria/tamanioDePagina) ; //(tamanio de swamp /tamanio de pagina);

    while(emptyFrame<= framesTotales){
        if(frameAsignado(emptyFrame)== 0){
            return emptyFrame;
        }
        emptyFrame++;
    }
    
    return emptyFrame;
}

int frameAsignado(int unFrame){
    

    if(todasLasTablasDePaginas != NULL){
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
            while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
    
                if(tempPagina->frame == unFrame){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    
                    return 1;
                }
            }
            list_iterator_destroy(iterator2);
        }
    list_iterator_destroy(iterator);
    
    }
    return 0;
}

int getFrameDeUn(int processId, int mayorNroDePagina){

    TablaDePaginasxProceso* temp = get_pages_by(processId);

    t_list_iterator* iterator = list_iterator_create(temp->paginas);

    Pagina *tempPagina = list_iterator_next(iterator);
    while (list_iterator_has_next(iterator)  && tempPagina->pagina != mayorNroDePagina) {
        tempPagina = list_iterator_next(iterator);
    }

    if(tempPagina->pagina == mayorNroDePagina){
        list_iterator_destroy(iterator);
        return tempPagina->frame;
    }

    list_iterator_destroy(iterator);

    return -1;
}

int memfree(int idProcess, int direccionLogicaBuscada){
    
    int paginaActual=1;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    int dirAllocFinal = tablaDelProceso->lastHeap;
    int dirAllocActual=0;


    while((dirAllocActual <= direccionLogicaBuscada) && dirAllocFinal>=direccionLogicaBuscada){
        
        if (dirAllocActual == direccionLogicaBuscada)
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + 2*sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

            int libre= FREE;
            memcpy(memoria + offset,&libre, sizeof(uint8_t));

            return 1;
        }
        else
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

            memcpy(&dirAllocActual, memoria + offset, sizeof(uint32_t));
        }
        
    }

    //falta hacer lo de liberar paginas pero deberia preguntar si deberia compactar y si debe llevar algun procesdimiento
    return MATE_FREE_FAULT;
}

Pagina* get_page_by_dir_logica(TablaDePaginasxProceso* tabla, int dir_buscada){
    int paginaActual = 1;
    int dirAllocFinal = tabla->lastHeap;
    int dirAllocActual = 0, pid = tabla->id;
    int frameBuscado;

    while((dirAllocActual <= dir_buscada) && dirAllocFinal >= dir_buscada){
        if (dirAllocActual == dir_buscada){
            paginaActual = (dirAllocActual / tamanioDePagina) + 1;
            return getPageDe(pid, paginaActual);
        } else {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            frameBuscado = getFrameDeUn(pid, paginaActual);

            int posAllocDentroDelFrame = (dirAllocActual) - ((paginaActual-1) * tamanioDePagina);
            int offset = (frameBuscado * tamanioDePagina) + posAllocDentroDelFrame;
            
            HeapMetaData* heap = get_heap_metadata(offset);
            dirAllocActual = heap->nextAlloc;
        }
    }
    return NULL;
}

Pagina *getPageDe(int processId,int nroPagina){

    TablaDePaginasxProceso* temp = get_pages_by(processId);

    t_list_iterator* iterator = list_iterator_create(temp->paginas);
    
   Pagina *tempPagina = list_iterator_next(iterator);
    
    while (tempPagina->pagina != nroPagina)
    {
       tempPagina = list_iterator_next(iterator);
    }
    
    list_iterator_destroy(iterator);
    return tempPagina;
}

void inicializarUnProceso(int idDelProceso){
    log_info(logger, "Inicializando el Proceso %d", idDelProceso);

    HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData));
    nuevoHeap->prevAlloc = 0;
    nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
    nuevoHeap->isfree = 1;

    TablaDePaginasxProceso* nuevaTablaDePaginas = malloc(sizeof(TablaDePaginasxProceso));
    nuevaTablaDePaginas->id = idDelProceso;
    nuevaTablaDePaginas->lastHeap = 0;
    nuevaTablaDePaginas->paginas = list_create();
    list_add(todasLasTablasDePaginas, nuevaTablaDePaginas);
    
    if(tipoDeAsignacionDinamica){
        int nuevoFrame = getframeNoAsignadoEnMemoria();
        int offset = nuevoFrame * tamanioDePagina;

        memcpy(memoria + offset, &nuevoHeap->prevAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset, &nuevoHeap->nextAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset, &nuevoHeap->isfree,sizeof(u_int8_t));

        Pagina* nuevaPagina = malloc(sizeof(Pagina));
        nuevaPagina->pagina=1;
        nuevaPagina->lRU = lRUACTUAL;
        nuevaPagina->isfree= BUSY;
        nuevaPagina->frame = nuevoFrame;
        nuevaPagina->bitPresencia =0;
        nuevaPagina->bitModificado = 1;

        list_add(nuevaTablaDePaginas->paginas, nuevaPagina);

    }else{
        int paginasCargadas = 0;

        while (paginasCargadas != cantidadDePaginasPorProceso)
        {
  
            if (paginasCargadas == 0)
            {
                int nuevoFrame =getframeNoAsignadoEnMemoria();
                int offset = nuevoFrame * tamanioDePagina;

                memcpy(memoria + offset, &nuevoHeap->prevAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset, &nuevoHeap->nextAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset, &nuevoHeap->isfree,sizeof(u_int8_t));

                Pagina* nuevaPagina = malloc(sizeof(Pagina));
                nuevaPagina->pagina=1;
                nuevaPagina->lRU = lRUACTUAL;
                nuevaPagina->isfree= BUSY;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia =0;
                nuevaPagina->bitModificado = 1;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }else
            {
                int nuevoFrame = getframeNoAsignadoEnMemoria();
                //int offset = nuevoFrame * tamanioDePagina;

                Pagina* nuevaPagina = malloc(sizeof(Pagina));
                nuevaPagina->pagina=paginasCargadas+1;
                nuevaPagina->lRU = lRUACTUAL;
                nuevaPagina->isfree= FREE;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia =0;
                nuevaPagina->bitModificado = 1;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }
            paginasCargadas++;
        }  
    }
    log_info(logger, "Proceso %d inicializado con Exito", idDelProceso);
}

int memwrite(int idProcess, int direccionLogicaBuscada, void* loQueQuierasEscribir){
    int paginaActual=1;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    int dirAllocFinal = tablaDelProceso->lastHeap;
    int dirAllocActual=0;


    while((dirAllocActual <= direccionLogicaBuscada) && dirAllocFinal>=direccionLogicaBuscada){
        
        if (dirAllocActual == direccionLogicaBuscada)
        {
            int finDelAlloc = 0;
            
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;

            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

            memcpy(&finDelAlloc, memoria + offset, sizeof(uint32_t));

            int offsetInicioAlloc = (frameBuscado*tamanioDePagina) + (dirAllocActual) - ((paginaActual-1) * tamanioDePagina) + HEAP_METADATA_SIZE;
            
            int paginaFinDelAlloc = (finDelAlloc/tamanioDePagina)+1;

            if (paginaFinDelAlloc == paginaActual)
            {
                memcpy(memoria + offsetInicioAlloc, loQueQuierasEscribir, sizeof(loQueQuierasEscribir));
            }
            else
            {
                int nroPagAux = paginaActual;
                
                void* espacioAuxiliar = malloc(tamanioDePagina*((paginaFinDelAlloc - paginaActual) + 1));
                
                int unOffset =0;
                
                while(nroPagAux<=paginaFinDelAlloc){
                    frameBuscado = getFrameDeUn(idProcess, nroPagAux);
                       
                    memcpy(espacioAuxiliar + unOffset,memoria + (frameBuscado*tamanioDePagina) ,tamanioDePagina);
                    
                    unOffset +=tamanioDePagina;

                    nroPagAux++;    
                }

                offsetInicioAlloc -= (frameBuscado*tamanioDePagina);

                memcpy(espacioAuxiliar + offsetInicioAlloc, loQueQuierasEscribir , sizeof(loQueQuierasEscribir));

                nroPagAux = paginaActual;
                
                unOffset =0;
                
                while(nroPagAux<=paginaFinDelAlloc){
                    frameBuscado = getFrameDeUn(idProcess, nroPagAux);
                       
                    memcpy(memoria + (frameBuscado*tamanioDePagina) ,espacioAuxiliar + unOffset ,tamanioDePagina);
                    
                    unOffset +=tamanioDePagina;

                    nroPagAux++;    
                }
            }
            

            return 1;
        }
        else
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

            memcpy(&dirAllocActual, memoria + offset, sizeof(uint32_t));
        }

}
}

HeapMetaData* get_heap_metadata(int offset){
    HeapMetaData* newHeap = malloc(sizeof(HeapMetaData));

    memcpy(&newHeap->prevAlloc, memoria + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&newHeap->nextAlloc, memoria + offset, sizeof(uint32_t));
    offset = offset + sizeof(uint32_t);

    memcpy(&newHeap->isfree, memoria + offset, sizeof(uint8_t));

    return newHeap;
}

HeapMetaData* set_heap_metadata(HeapMetaData* heap, int offset){
    memcpy(memoria + offset, &heap->prevAlloc, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(memoria + offset, &heap->nextAlloc, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(memoria + offset, &heap->isfree, sizeof(uint8_t));

    return heap;
}

void send_message_swamp(int command, void* payload, int pay_len){
    if (_send_message(swamp_fd, MEM_ID, command, payload, pay_len, logger) < 0){
        log_error(logger, "Error al enviar mensaje a Swamp");
        return;
    }

    if(command == RECV_PAGE){
        t_mensaje* msg = _receive_message(swamp_fd, logger);
        deserealize_payload(msg->payload);
    }
}

void deserealize_payload(void* payload){
    log_info(logger, "Deserealizo el payload");
}