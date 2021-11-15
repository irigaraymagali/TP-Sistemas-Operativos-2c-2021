#include "memory_utils.h"

void initPaginacion(){
    log_info(logger,"iniciando paginacion");

    pthread_mutex_init(&lru_mutex, NULL);
    pthread_mutex_init(&list_pages_mutex, NULL);
    pthread_mutex_init(&max_hit_tlb_mutex, NULL);
    pthread_mutex_init(&max_miss_tlb_mutex, NULL);

    pthread_mutex_init(&tlb_mutex, NULL);
    pthread_mutex_init(&tlb_lru_mutex, NULL);
    pthread_mutex_init(&entrada_fifo_mutex, NULL);
    pthread_mutex_init(&m_list_mutex, NULL);

    tamanioDePagina = config_get_int_value(config, "TAMANIO_PAGINA");

    if (string_equals_ignore_case(config_get_string_value(config,"TIPO_ASIGNACION"), "FIJA"))
    {
        tipoDeAsignacionDinamica= 0;
    }
    else
    {
        tipoDeAsignacionDinamica= 1;
    }
    

    void* payload = _serialize(sizeof(int), "%d", tipoDeAsignacionDinamica);
    send_message_swamp(TIPO_ASIGNACION, payload, sizeof(int));
    free(payload);
    lRUACTUAL=0;

    tamanioDeMemoria = config_get_int_value(config, "TAMANIO");

    /* TLB */
    max_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    retardo_hit_tlb = config_get_int_value(config, "RETARDO_ACIERTO_TLB");
    retardo_miss_tlb = config_get_int_value(config, "RETARDO_FALLO_TLB");
    max_tlb_hit = 0;
    max_tlb_miss = 0;
    tlb_list = list_create();
    metrics_list = list_create();
    tlb_lru_global = 0;

    memoria = malloc(tamanioDeMemoria);

    cantidadDePaginasPorProceso = config_get_int_value(config, "MARCOS_POR_CARPINCHO");

    todasLasTablasDePaginas = list_create();

    punteroFrameClock =0;
}

int memalloc(int processId, int espacioAReservar){
    int entra = entraEnElEspacioLibre(espacioAReservar, processId);
    uint32_t mayorNroDePagina = 0; // 0 porque indica que no tiene asignado ninguna pagina, las pags siempre arancan en 1
    int ultimoFrame = 0;
    int tempLastHeap = 0;
    int espacioFinalDisponible = 0;
    Pagina* paginaFInalEncontrada = malloc(sizeof(Pagina));

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
                //agragar el bit de presencia para despues pedirle a gonza
                paginaFInalEncontrada = paginaTemporal;
            }
        }
        list_iterator_destroy(iterator2);
        

        //agregar algo que le pida la pagina a gonza si el bit presencia es 0
        
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

            paginaFInalEncontrada->lRU = lRUACTUAL;
            paginaFInalEncontrada->bitUso = 1 ;

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

                Pagina *pagNeeded = getMarcoDe(framenecesitado);
                
                if(pagNeeded->bitPresencia == 0){
                    utilizarAlgritmoDeAsignacion(processId);
                    // pedirle la pag a gonza
                }

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

void* memread(uint32_t pid, int dir_logica, int size){
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
    Pagina *ultimaPagina= malloc(sizeof(Pagina)); //Nose si necesitamos un malloc porque en caso de encontrar una pagina temporal creo estariamos guardando este malloc basura. PROBAR.
    
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
    
       
    if(tipoDeAsignacionDinamica == 1){
        while(cantidadDePaginasAAgregar != 0){
            Pagina *nuevaPagina = malloc(sizeof(Pagina));
            ultimaPagina = getLastPageDe(processId); // que pasaría si no tenes paginas?
            if(ultimaPagina == NULL){
                nuevaPagina->pagina = FIRST_PAGE;
            } else {
                int nroUltimaPagina = ultimaPagina->pagina ;
                nroUltimaPagina++;
                nuevaPagina->pagina= nroUltimaPagina;
            }

            nuevaPagina->frame = getNewEmptyFrame(processId);

            if(nuevaPagina->frame == -1){
                utilizarAlgritmoDeAsignacion(processId);
                nuevaPagina->frame = getNewEmptyFrame(processId);
            }
            nuevaPagina->isfree = BUSY;
            nuevaPagina->bitModificado = 0;
            nuevaPagina->bitPresencia = 1;
            nuevaPagina->bitUso=1;
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

            if(getNewEmptyFrame(processId) == -1){
                utilizarAlgritmoDeAsignacion(processId);

                paginaSiguienteALaUltima->frame = getNewEmptyFrame(processId);
                paginaSiguienteALaUltima->isfree = BUSY;
                lRUACTUAL++;
                paginaSiguienteALaUltima->lRU = lRUACTUAL;
                paginaSiguienteALaUltima->bitUso=1;
                paginaSiguienteALaUltima->bitModificado=1;
                paginaSiguienteALaUltima->bitPresencia=1;

                list_iterator_destroy(iterator);
            
                cantidadDePaginasAAgregar--;
            }else{
                paginaSiguienteALaUltima->isfree = BUSY;
                lRUACTUAL++;
                paginaSiguienteALaUltima->lRU = lRUACTUAL;
                paginaSiguienteALaUltima->bitUso=1;
                paginaSiguienteALaUltima->bitModificado=1;
                paginaSiguienteALaUltima->bitPresencia=1;

                list_iterator_destroy(iterator);
            
            cantidadDePaginasAAgregar--;}
        }
    }      
}

int getNewEmptyFrame(int idProcess){
    int emptyFrame = 0;
    int paginaFinal = tamanioDeMemoria/tamanioDePagina;
    int isfree = 0;

    if (tipoDeAsignacionDinamica)
    {
        while(emptyFrame < paginaFinal){
            isfree= estaOcupadoUn(emptyFrame, idProcess);
            if(isfree!= BUSY){
                return emptyFrame;
            }

            emptyFrame++;
        }
        return -1;
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
        return -1;
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
            list_iterator_destroy(iterator2);
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
        
        if(tempPagina->bitPresencia==0){
            utilizarAlgritmoDeAsignacion(processId);
            tempPagina->frame = getNewEmptyFrame(processId);
            int pay_len = 2*sizeof(int);
            void* payload = _serialize(pay_len, "%d%d", processId, mayorNroDePagina);       
            send_message_swamp(RECV_PAGE, payload, pay_len);

            //pedirselo a gonza
        }
        tempPagina->bitUso = 1;
        
        pthread_mutex_lock(&lru_mutex);
        lRUACTUAL++;
        tempPagina->lRU = lRUACTUAL;
        pthread_mutex_lock(&lru_mutex);

        log_info(logger, "tomo el frame %d con Exito", tempPagina->frame);

        return tempPagina->frame;
    }

    list_iterator_destroy(iterator);

    return -1;
}

int memfree(int idProcess, int direccionLogicaBuscada){
    
    int paginaActual=1;
    int pagAnterior=0;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    int dirAllocFinal = tablaDelProceso->lastHeap;
    int dirAllocActual=0;
    int offsetNextAllocAnterior;
    uint8_t estadoAllocAnterior;


    while((dirAllocActual <= direccionLogicaBuscada) && dirAllocFinal>=direccionLogicaBuscada){
        
        if (dirAllocActual == direccionLogicaBuscada)
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + 2*sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

            int libre= FREE;
            memcpy(memoria + offset,&libre, sizeof(uint8_t));

            int nextAllocActual;
            memcpy(&nextAllocActual ,memoria + offset-sizeof(uint32_t), sizeof(uint32_t));


            if(estadoAllocAnterior == FREE && direccionLogicaBuscada!=0){
                memcpy(memoria + offsetNextAllocAnterior,&nextAllocActual, sizeof(uint32_t));

                int paginaNextAlloc = (nextAllocActual/ tamanioDePagina) + 1;

                int frameNextAlloc = getFrameDeUn(idProcess, paginaNextAlloc);

                int posicionNextAllocNextAllocDentroDelFrame = (nextAllocActual) - ((paginaNextAlloc-1) * tamanioDePagina);

                offset= (frameNextAlloc*tamanioDePagina) + posicionNextAllocNextAllocDentroDelFrame;

                memcpy(memoria + offset ,&dirAllocActual, sizeof(uint32_t));
            }else{
                if((pagAnterior-1) == paginaActual){
                    deletePagina(idProcess, paginaActual);

                    nextAllocActual = 0;
                    
                    memcpy(memoria + offsetNextAllocAnterior,&nextAllocActual, sizeof(uint32_t));

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

            offsetNextAllocAnterior = offset;

            memcpy(&dirAllocActual, memoria + offset, sizeof(uint32_t));

            memcpy(&estadoAllocAnterior, memoria + offset + sizeof(uint32_t), sizeof(uint8_t));

            pagAnterior = paginaActual;
        }
        
    }

    //falta hacer lo de liberar paginas pero deberia preguntar si deberia compactar y si debe llevar algun procesdimiento
    return MATE_FREE_FAULT;
}

void deletePagina(int idProcess,int paginaActual){

    TablaDePaginasxProceso* tablaDePags =get_pages_by(idProcess);

    t_list_iterator* iterator = list_iterator_create(tablaDePags->paginas);
    
    Pagina *tempPagina = list_iterator_next(iterator);

    while (tempPagina->pagina != paginaActual)
    {
       tempPagina = list_iterator_next(iterator);
    }
    
    list_iterator_remove(iterator);
    log_info(logger, "Deleteo la pag %d con Exito", tempPagina->pagina);

    list_iterator_destroy(iterator);
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

int get_nro_page_by_dir_logica(TablaDePaginasxProceso* tabla, int dir_buscada){
    int paginaActual = 1;
    int dirAllocFinal = tabla->lastHeap;
    int dirAllocActual = 0, pid = tabla->id;
    int frameBuscado;

    while((dirAllocActual <= dir_buscada) && dirAllocFinal >= dir_buscada){
        if (dirAllocActual == dir_buscada){
            paginaActual = (dirAllocActual / tamanioDePagina) + 1;
            return paginaActual;
        } else {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            frameBuscado = getFrameDeUn(pid, paginaActual);

            int posAllocDentroDelFrame = (dirAllocActual) - ((paginaActual-1) * tamanioDePagina);
            int offset = (frameBuscado * tamanioDePagina) + posAllocDentroDelFrame;
            
            HeapMetaData* heap = get_heap_metadata(offset);
            dirAllocActual = heap->nextAlloc;
        }
    }
    return -1;
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
        nuevaPagina->bitPresencia =1;
        nuevaPagina->bitModificado = 1;
        nuevaPagina->bitUso=1;

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
                nuevaPagina->bitPresencia =1;
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

int memwrite(int idProcess, int direccionLogicaBuscada, void* loQueQuierasEscribir, int tamanio){
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
                memcpy(memoria + offsetInicioAlloc, loQueQuierasEscribir, tamanio);
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

                memcpy(espacioAuxiliar + offsetInicioAlloc, loQueQuierasEscribir , tamanio);

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
    return -1;
}

void utilizarAlgritmoDeAsignacion(int processID){

    if (string_equals_ignore_case(config_get_string_value(config,"ALGORITMO_REEMPLAZO_MMU"), "LRU"))
    {
       seleccionLRU( processID);
    }
    else
    {
       seleccionClockMejorado();
    }
    


}

void seleccionLRU(int processID){

    uint32_t LRUmenor=9999; //recordar que lo que se busca es el LRU menor
    uint32_t frameVictima=0;

    if (tipoDeAsignacionDinamica)
    {
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        
        while (list_iterator_has_next(iterator)) {
        
            t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
            Pagina *paginatemp = list_iterator_next(iterator2);

            while (list_iterator_has_next(iterator2))
            {
                /* code */
                if(paginatemp->lRU < LRUmenor && paginatemp->bitPresencia==1){
                    LRUmenor= paginatemp->lRU;
                    frameVictima = paginatemp->frame;
                }

                paginatemp = list_iterator_next(iterator2);
            }
        
            list_iterator_destroy(iterator2);
            temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        }
        
        list_iterator_destroy(iterator);
    }
    else
    {
        TablaDePaginasxProceso* temp = get_pages_by(processID);

        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
        Pagina *paginatemp = list_iterator_next(iterator2);

        while (list_iterator_has_next(iterator2))
        {
        
            if(paginatemp->lRU < LRUmenor && paginatemp->bitPresencia==1){
                LRUmenor= paginatemp->lRU;
                frameVictima = paginatemp->frame;
            }

            paginatemp = list_iterator_next(iterator2);
        }
        
        list_iterator_destroy(iterator2);
    }

    //falta una parte que le mande el mendaje a gonza

    /*
        if(gonza tiene espacio)
        liberarFrame(uint32_t nroDeFrame)
     */
    liberarFrame(frameVictima);
}

void seleccionClockMejorado(){

    int frameNoEncontrado =1;

    int frameInicial = punteroFrameClock; 

    int frameFinal = tamanioDeMemoria/tamanioDePagina;

    punteroFrameClock++;

    while(frameNoEncontrado && frameInicial!=punteroFrameClock){

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
        
        if(paginaEncontrada->bitModificado == 0 && paginaEncontrada->bitUso==0){
            frameNoEncontrado =0;
            
            /*
                le pido a gonza que se agarre esta pagina y libero el frame
                liberarFrame(paginaEncontrada->frame)
             */
            liberarFrame(paginaEncontrada->frame);
        }

        punteroFrameClock++;
    }

    while(frameNoEncontrado ){

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
        
        if(paginaEncontrada->bitUso==0){
            frameNoEncontrado =0;
            
            /*
                le pido a gonza que se agarre esta pagina y libero el frame
                liberarFrame(paginaEncontrada->frame)
             */
            liberarFrame(paginaEncontrada->frame);
        }else
        {
            paginaEncontrada->bitUso =0;
        }
        

        punteroFrameClock++;
    }


}

Pagina *getMarcoDe(uint32_t nroDeFrame){

    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    

    Pagina *paginatemp = malloc(sizeof(Pagina));
        
    while (list_iterator_has_next(iterator)) {

        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
        

        while (list_iterator_has_next(iterator2))
        {
            paginatemp = list_iterator_next(iterator2);
            if(paginatemp->frame == nroDeFrame){
                return paginatemp;
            }

        }
        
        list_iterator_destroy(iterator2);
    }
        
    list_iterator_destroy(iterator);

    return paginatemp;
}

void liberarFrame(uint32_t nroDeFrame){
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
    
        
    while (list_iterator_has_next(iterator)) {
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
        

        while (list_iterator_has_next(iterator2))
        {
            Pagina *paginatemp = list_iterator_next(iterator2);
            if(paginatemp->frame == nroDeFrame){
                paginatemp->frame = (tamanioDeMemoria/tamanioDePagina)+1;
                paginatemp->bitPresencia = 0;
            }

            
        }
        
        list_iterator_destroy(iterator2);
        
    }
        
    list_iterator_destroy(iterator);
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

void add_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame){
    TLB* new_instance = new_entrada_tlb(pid, page, frame);
    if (list_size(tlb_list) < max_entradas_tlb) {
       list_add(tlb_list, new_instance); 
    } else {
        replace_entrada(new_instance);
    }

    set_pid_metric_if_missing(pid);
}

TLB* new_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame){
    TLB* new_instance = malloc(sizeof(TLB));
    new_instance->pid = pid;
    new_instance->pagina = page;
    new_instance->frame = frame;
    pthread_mutex_lock(&tlb_lru_mutex);
    new_instance->lru = tlb_lru_global;
    tlb_lru_global++;
    pthread_mutex_unlock(&tlb_lru_mutex);

    return new_instance;
}

void replace_entrada(TLB* new_instance){
    if (string_equals_ignore_case(config_get_string_value(config, "ALGORITMO_REEMPLAZO_TLB"), "FIFO")){
        pthread_mutex_lock(&entrada_fifo_mutex);
        list_replace_and_destroy_element(tlb_list, entrada_fifo, new_instance, (void*)free);
        if (entrada_fifo < max_entradas_tlb){
            entrada_fifo++;
        } else {
            entrada_fifo = 0;
        }
        pthread_mutex_unlock(&entrada_fifo_mutex);
    } else {
        /* TESTEAR. */
        TLB* tlb_to_replace = list_get_minimum(tlb_list, get_minimum_lru_tlb);
        tlb_to_replace->pid = new_instance->pid;
        tlb_to_replace->pagina = new_instance->pagina;
        tlb_to_replace->frame = new_instance->frame;
        tlb_to_replace->lru = new_instance->lru;
        free(new_instance);
    }
}

void* get_minimum_lru_tlb(void* actual, void* next){
    TLB* actual_tlb = (TLB*) actual;
    TLB* next_tlb = (TLB*) next;

    if (actual_tlb->lru < next_tlb->lru){
        return actual;
    }

    return next;
}

TLB* fetch_entrada_tlb(uint32_t pid, int dir_logica){
    TLB* tlb;
    bool has_pid(void* elem){
        TLB* tlb = (TLB*) elem;
        return tlb->pid == pid;
    }

    int count_pid = list_count_satisfying(tlb_list, has_pid);
    if (count_pid > 1){
        pthread_mutex_lock(&list_pages_mutex);
        TablaDePaginasxProceso* tabla = get_pages_by(pid);
        uint32_t nro_page = get_nro_page_by_dir_logica(tabla, dir_logica);
        pthread_mutex_unlock(&list_pages_mutex);
        if (nro_page == -1){
            sum_metric(pid, TLB_MISS);
            return NULL;
        }
        tlb = fetch_entrada_tlb_by_page(nro_page);
    } else {
        tlb = fetch_entrada_tlb_by_pid(pid);
    }
    
    if(tlb != NULL){
        sum_metric(pid, TLB_HIT);
    } else {
        sum_metric(pid, TLB_MISS);
    }

    return tlb;
}

void set_pid_metric_if_missing(uint32_t pid){
    pthread_mutex_lock(&m_list_mutex);
    bool has_pid(void* elem){
        TLB* tlb = (TLB*) elem;
        return tlb->pid == pid;
    }

    if (!list_any_satisfy(metrics_list, has_pid)){
        Metric* m = malloc(sizeof(Metric)); 
        m->pid = pid;
        m->hits = 0;
        m->miss = 0;
        list_add(metrics_list, m);
    }

    pthread_mutex_unlock(&m_list_mutex);
}

void sum_metric(uint32_t pid, int isHit){
    pthread_mutex_lock(&m_list_mutex);

    t_list_iterator* iterator = list_iterator_create(metrics_list);
    while (list_iterator_has_next(iterator)){
        Metric* metric = (Metric*) list_iterator_next(iterator);
        if (metric->pid == pid){
            if(isHit == 1){
                metric->hits++;
                sleep(retardo_hit_tlb);
            } else {
                metric->miss++;
                sleep(retardo_miss_tlb);
            }
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&m_list_mutex);
}

TLB* fetch_entrada_tlb_by_pid(uint32_t pid){
    return get_entrada_tlb_by_condition(C_PID, pid);
}

TLB* fetch_entrada_tlb_by_page(uint32_t page){
    return get_entrada_tlb_by_condition(C_PAGE, page);
}

TLB* fetch_entrada_tlb_by_frame(uint32_t frame){
    return get_entrada_tlb_by_condition(C_FRAME, frame);
}

TLB* get_entrada_tlb_by_condition(int condition, uint32_t value){
    uint32_t v_to_compare;
    for (int i = 0; i < max_entradas_tlb; i++){
        TLB* tlb = (TLB*) list_get(tlb_list, i);
        switch (condition){
            case C_PID:
                v_to_compare = tlb->pid;              
                break;
            case C_PAGE:
                v_to_compare = tlb->pagina;
                break; 
            case C_FRAME:
                v_to_compare = tlb->frame;
                break;            
            default:
                log_error(logger, "Condicion incorrecta.");
                return NULL;
                break;
            }
        if (v_to_compare == value){
            return tlb;
        }
    }

    return NULL;
}

void* send_message_swamp(int command, void* payload, int pay_len){
    if (_send_message(swamp_fd, ID_MEMORIA, command, payload, pay_len, logger) < 0){
        log_error(logger, "Error al enviar mensaje a Swamp");
        return NULL;
    }

    if(command == RECV_PAGE){
        t_mensaje* msg = _receive_message(swamp_fd, logger);
        return msg->payload;
    }

    return NULL;
}

void free_tlb(){
    if (list_is_empty(metrics_list)){
        list_destroy(metrics_list);
    } else {
        list_destroy_and_destroy_elements(metrics_list, free);
    } 

    if (list_is_empty(tlb_list)){
        list_destroy(tlb_list);
    } else {
        list_destroy_and_destroy_elements(metrics_list, free);
    } 
}
