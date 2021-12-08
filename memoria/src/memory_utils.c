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
    pthread_mutex_init(&memory_mutex, NULL);
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
    entrada_fifo = 0;

    memoria = malloc(tamanioDeMemoria);

    cantidadDePaginasPorProceso = config_get_int_value(config, "MARCOS_POR_CARPINCHO");

    todasLasTablasDePaginas = list_create();

    punteroFrameClock =0;
}

int memalloc(int processId, int espacioAReservar){
    log_info(logger,"arranco un memalloc----------------------------");
    int entra = entraEnElEspacioLibre(espacioAReservar, processId);
    uint32_t mayorNroDePagina = 0; // 0 porque indica que no tiene asignado ninguna pagina, las pags siempre arancan en 1
    int ultimoFrame = 0;
    int tempLastHeap = 0;
    int espacioFinalDisponible = 0;
    Pagina* paginaFInalEncontrada;

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
            pthread_mutex_lock(&memory_mutex);
            uint32_t espacioTotal = tempLastHeap + espacioAReservar;
            memcpy(memoria + offset + sizeof(uint32_t), &espacioTotal, sizeof(uint32_t));
            offset = offset + 2*sizeof(uint32_t);
            
            memcpy(memoria + offset, &nuevoHeap->isfree, sizeof(uint8_t));
            offset = offset  + espacioAReservar - 2*sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->prevAlloc, sizeof(uint32_t));            
            offset = offset + sizeof(uint32_t);

            memcpy(memoria + offset, &nuevoHeap->nextAlloc, sizeof(uint32_t));
            offset = offset + sizeof(uint32_t);
            pthread_mutex_unlock(&memory_mutex);
            pthread_mutex_lock(&lru_mutex);
            lRUACTUAL++;
            paginaFInalEncontrada->lRU = lRUACTUAL;
            pthread_mutex_unlock(&lru_mutex);
            paginaFInalEncontrada->bitUso = 1 ;

            temp->lastHeap = tempLastHeap + espacioAReservar;

            mandarPaginaAgonza(processId ,paginaFInalEncontrada->frame, paginaFInalEncontrada->pagina)        ;

            return (tempLastHeap );
        } else {
            
           agregarXPaginasPara(processId, (espacioAReservar-espacioFinalDisponible));

            int paginaLastHeap = (tempLastHeap/tamanioDePagina)+1;
            int ubicacionNuevoLastHeap = tempLastHeap + espacioAReservar;
            int paginaFinLastHeap= ((ubicacionNuevoLastHeap+HEAP_METADATA_SIZE)/tamanioDePagina)+1;
            // Pagina *ultimaPag = getLastPageDe(processId);
            void* espacioAuxiliar = malloc(((paginaFinLastHeap - paginaLastHeap)+1)*tamanioDePagina);

            int nroPagAux=paginaLastHeap;
            int offsetEspacioAux=0;
            pthread_mutex_lock(&memory_mutex);
            while (nroPagAux <= paginaFinLastHeap)
            {
                int unFrame = getFrameDeUn(processId,nroPagAux);

                memcpy(espacioAuxiliar+offsetEspacioAux, memoria+ (unFrame*tamanioDePagina),tamanioDePagina);

                nroPagAux++;
                offsetEspacioAux+=tamanioDePagina;
            }
            pthread_mutex_unlock(&memory_mutex);

            offsetEspacioAux = tempLastHeap - (tamanioDePagina*(paginaLastHeap-1));
            offsetEspacioAux += sizeof(uint32_t);
            
            
            memcpy(espacioAuxiliar+offsetEspacioAux,&ubicacionNuevoLastHeap , sizeof(uint32_t));
            
            offsetEspacioAux += sizeof(uint32_t);
            memcpy(espacioAuxiliar+offsetEspacioAux,&nuevoHeap->isfree , sizeof(uint8_t));

            offsetEspacioAux += (espacioAReservar + sizeof(uint8_t)-HEAP_METADATA_SIZE) ;
            memcpy(espacioAuxiliar+offsetEspacioAux, &tempLastHeap, sizeof(uint32_t));

            offsetEspacioAux += sizeof(uint32_t);
            memcpy(espacioAuxiliar+offsetEspacioAux, &nuevoHeap->nextAlloc, sizeof(uint32_t));

            offsetEspacioAux += sizeof(uint32_t);
            memcpy(espacioAuxiliar+offsetEspacioAux, &nuevoHeap->isfree, sizeof(uint8_t));
            
            nroPagAux=paginaLastHeap;
            offsetEspacioAux=0;
            pthread_mutex_lock(&memory_mutex);
            while(nroPagAux <= paginaFinLastHeap){
                int framenecesitado = getFrameDeUn(processId, nroPagAux);
                
                memcpy(memoria + (framenecesitado*tamanioDePagina), espacioAuxiliar + offsetEspacioAux, tamanioDePagina);
                mandarPaginaAgonza(processId ,framenecesitado, nroPagAux);
                //log_info(logger,"EN memwrite---------dirAllocActual:%d",);
                
                nroPagAux++;
                offsetEspacioAux+=tamanioDePagina;
            }
            pthread_mutex_unlock(&memory_mutex);
            free(espacioAuxiliar);
        }
        temp->lastHeap = tempLastHeap + espacioAReservar;
        return (tempLastHeap);    
    }
    
    return entra;

}

int suspend_process(int pid) {
    log_info(logger, "Suspendiendo el Proceso %d...", pid);
    int pay_len = 3*sizeof(int) + tamanioDePagina;
    void* free_mem = malloc(tamanioDePagina);

    log_info(logger, "Buscando la tabla de paginas del proceso %d", pid);
    TablaDePaginasxProceso* table = get_pages_by(pid);
    if(table->id != pid){
        log_error(logger, "El proceso %d no se encuentra en Memoria", pid);
        return -1;
    }
    pthread_mutex_lock(&list_pages_mutex);
    t_list_iterator* iterator = list_iterator_create(table->paginas);
    while (list_iterator_has_next(iterator)){
        Pagina* page = (Pagina*) list_iterator_next(iterator);
        int res;
        void* mem_aux = malloc(tamanioDePagina);
        pthread_mutex_lock(&memory_mutex);
        memcpy(mem_aux, memoria + (page->frame * tamanioDePagina), tamanioDePagina);
        pthread_mutex_unlock(&memory_mutex);
        log_info(logger, "Enviando a Swamp la pagina %d del proceso %d", page->pagina, pid);
        void* payload = _serialize(pay_len, "%d%d%d%v", pid, page->pagina, tamanioDePagina, mem_aux);
        log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", page->pagina, pid);
        void* v_res = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);        // SI SWAMP NO TIENE MAS ESPACIO DEBERIAMOS RECIBIR ALGO.
        delete_entrada_tlb(pid, page->pagina, page->frame);

        memcpy(&res, v_res, sizeof(int));
        if(res == 0){
            log_error(logger, "Swamp no pudo setear la pagina %d en disco", page->pagina);
            return -1;
        }

        page->bitPresencia = 0;
        pthread_mutex_lock(&memory_mutex);
        memcpy(memoria + (page->frame * tamanioDePagina), free_mem, tamanioDePagina);
        pthread_mutex_unlock(&memory_mutex);

        free(mem_aux);
        free(v_res);
        free(payload);
        log_info(logger, "Pagina %d suspendida con exito!", page->pagina);
    }
    
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_pages_mutex);
    return 1;
}

void* memread(uint32_t pid, int dir_logica, int size){
    log_info(logger,"arranco un memread----------------------------");
    void* read = malloc(size);
    int size_to_read, offset_to_read;
    int act_page = 1;
    int dirAllocActual = dir_logica;
    HeapMetaData* heap;
    int read_len = 0;

    void* err_msg = _serialize(sizeof(int), "%d", MATE_READ_FAULT);

    log_info(logger, "Buscando la tabla de paginas del proceso %d", pid);

    //IF SI EL SIZE RECIBIDO ES > AL TAMAÑO DEL PROCESO. THEN ERROR.-
    TablaDePaginasxProceso* pages = get_pages_by(pid);
    if (pages->id != pid){
        log_error(logger, "El proceso ingresado es incorrecto");
        return err_msg;
    }
    
    int dirAllocFinal = pages->lastHeap;
    if (size >= dirAllocFinal){
        log_error(logger, "La Longitud de lectura recibida es invalida.");
        return err_msg;
    }

    while(read_len < size){
        int div_heap = 0;
        act_page = (dirAllocActual / tamanioDePagina) + 1;       
        int act_frame = getFrameDeUn(pid, act_page);
        int resto = (dirAllocActual % tamanioDePagina);
        int dir_fisica = (act_frame * tamanioDePagina) + resto;
        int page_len = act_page * tamanioDePagina;

        int size_all_heap = (((dirAllocActual + HEAP_METADATA_SIZE)/ tamanioDePagina) + 1);
        if(act_page != size_all_heap){
            div_heap = 1;
            int first_alloc_act_page = (act_frame * tamanioDePagina);

            void* page_aux = malloc(tamanioDePagina*2);
            pthread_mutex_lock(&memory_mutex);
            memcpy(page_aux, memoria + first_alloc_act_page, tamanioDePagina);
            
            int next_frame = getFrameDeUn(pid, act_page + 1);
            int first_alloc_next_page = (next_frame * tamanioDePagina);

            memcpy(page_aux + tamanioDePagina, memoria + first_alloc_next_page, tamanioDePagina);
            pthread_mutex_unlock(&memory_mutex);
            
            int alloc_on_frame = abs((dirAllocActual) - ((act_page-1) * tamanioDePagina));

            memcpy(&heap->prevAlloc, page_aux + alloc_on_frame, sizeof(uint32_t));
            int offset = alloc_on_frame + sizeof(uint32_t);
           
            memcpy(&heap->nextAlloc, page_aux + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            memcpy(&heap->isfree, page_aux + offset, sizeof(uint8_t));
            
            free(page_aux);
        } else {
            heap = get_heap_metadata(dir_fisica);
            if(heap->isfree != BUSY && heap->isfree != FREE){
                log_error(logger, "La direccion logica recibida es incorrecta");
                return err_msg;
            }
            if(heap->isfree == FREE){
                log_error(logger, "La direccion logica apunta a un espacio de memoria vacío");
                return err_msg;
            }
        }

        int offset_without_alloc = dirAllocActual + HEAP_METADATA_SIZE;
        offset_to_read = (dir_fisica + HEAP_METADATA_SIZE);
        int alloc_len = abs(heap->nextAlloc - offset_without_alloc);
        size_to_read = alloc_len;

        if (size < size_to_read){
            size_to_read = size;
        } else if ((read_len + size_to_read) > size){
            size_to_read = size - read_len;
        }
        int last_dir_page = (act_page * page_len);
        if (heap->nextAlloc > last_dir_page){
            int nxt_long_alloc = abs(last_dir_page - heap->nextAlloc);
            int act_long_alloc = abs(alloc_len - nxt_long_alloc);

            if (size_to_read > act_long_alloc && div_heap == 0){
                size_to_read = act_long_alloc;
                pthread_mutex_lock(&memory_mutex);
                memcpy(read + read_len, memoria + offset_to_read, act_long_alloc);
                read_len += act_long_alloc;

                int next_frame = getFrameDeUn(pid, act_page + 1);
                int next_dir_fisica = (next_frame * tamanioDePagina);

                memcpy(read + read_len, memoria + next_dir_fisica, nxt_long_alloc);
                read_len += nxt_long_alloc;
                dirAllocActual = heap->nextAlloc;
                pthread_mutex_unlock(&memory_mutex);
            } else {
                pthread_mutex_lock(&memory_mutex);
                memcpy(read + read_len, memoria + offset_to_read, size_to_read);
                pthread_mutex_unlock(&memory_mutex);
                read_len += size_to_read;
                dirAllocActual = heap->nextAlloc;
            }
        } else {
            pthread_mutex_lock(&memory_mutex);
            memcpy(read + read_len, memoria + offset_to_read, size_to_read);
            pthread_mutex_unlock(&memory_mutex);
            read_len += size_to_read;
            dirAllocActual = heap->nextAlloc;
        }
    }

    log_info(logger, "Lectura realizada con exito");
    return read;
} 

TablaDePaginasxProceso* get_pages_by(int processID){
    pthread_mutex_lock(&list_pages_mutex);
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    while (temp->id != processID) {
        temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_pages_mutex);
    
    return temp; 
}

// encuentra si hay un alloc para ubicar el espacio a reservar dentro de las paginas
int entraEnElEspacioLibre(int espacioAReservar, int processId){
    TablaDePaginasxProceso* temp = get_pages_by(processId);

    if(temp->id == processId){
        // t_list_iterator* iterator = list_iterator_create(temp->paginas);  

        int nextAllocAux = 0;
        uint8_t isfreeAux;
        int allocActual = 0; 
        // int dirPaginaSiguiente = 0;
        // int dirPaginaActual;6
        int espacioEncontrado=0;
        int temp_last_heap = temp->lastHeap;

        pthread_mutex_lock(&memory_mutex);
        while(allocActual < temp_last_heap  && espacioEncontrado==0){
            int paginaActual = (allocActual/tamanioDePagina) +1; 

            int frameActual = getFrameDeUn(processId, paginaActual);

            //void* espacioAuxiliar = malloc(2*tamanioDePagina);
            
            memcpy(&nextAllocAux, memoria + (frameActual*tamanioDePagina)+sizeof(uint32_t),sizeof(uint32_t));

            memcpy(&isfreeAux, memoria + (frameActual*tamanioDePagina)+2*sizeof(uint32_t),sizeof(uint8_t));


            if((nextAllocAux-HEAP_METADATA_SIZE)<espacioAReservar || isfreeAux == BUSY){
                while (allocActual < temp->lastHeap  && espacioEncontrado==0)
                {
                    
                    
                    paginaActual = (allocActual/tamanioDePagina) +1; 

                    int paginaFinAlloc = ((allocActual+HEAP_METADATA_SIZE)/tamanioDePagina) +1; 

                    int offsetInicioAlloc;

                    void* espacioAuxiliar = malloc(2*tamanioDePagina);

                    int nropagaux= paginaActual;
                    int offsetpagaux =0;
                    while(nropagaux<=paginaFinAlloc){
                        frameActual = getFrameDeUn(processId, nropagaux);
                        
                        memcpy(espacioAuxiliar+offsetpagaux, memoria + frameActual*tamanioDePagina,tamanioDePagina);

                        offsetpagaux =tamanioDePagina;
                        nropagaux++;
                    }

                    offsetInicioAlloc= allocActual - ((paginaActual-1)*tamanioDePagina);

                    memcpy(&nextAllocAux,espacioAuxiliar + offsetInicioAlloc + sizeof(uint32_t),sizeof(uint32_t));

                    memcpy(&isfreeAux,espacioAuxiliar + offsetInicioAlloc + 2*sizeof(uint32_t),sizeof(uint8_t));

                    if(isfreeAux == FREE && (nextAllocAux - allocActual - HEAP_METADATA_SIZE) >= espacioAReservar){
                    pthread_mutex_unlock(&memory_mutex);
                    return allocActual;
                    
                    }
                    
                    allocActual = nextAllocAux;
                    free(espacioAuxiliar);
                }
                
            }else{
                pthread_mutex_unlock(&memory_mutex);
                return allocActual;
            }
        }
        pthread_mutex_unlock(&memory_mutex);
    }
        
    return -1;
}

Pagina *getLastPageDe(int processId){
    uint32_t mayorNroDePagina = 0;
    Pagina *ultimaPagina;
    
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
            pthread_mutex_lock(&lru_mutex);
            lRUACTUAL++;
            nuevaPagina->lRU = lRUACTUAL;
            pthread_mutex_unlock(&lru_mutex);

            TablaDePaginasxProceso* temp = get_pages_by(processId);

            list_add(temp->paginas, nuevaPagina); // ACA puede haber segun helgrind RACE CONDITION -> Array de mutex o un mutex para lista de paginas distinto a la de la tabla.

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

                    Pagina *nuevaPagina = malloc(sizeof(Pagina));
                    nuevaPagina->pagina   = paginaSiguienteALaUltima->pagina + 1;
                    nuevaPagina->frame = getNewEmptyFrame(processId);
                    nuevaPagina->isfree = BUSY;
                    pthread_mutex_lock(&lru_mutex);
                    lRUACTUAL++;
                    nuevaPagina->lRU = lRUACTUAL;
                    pthread_mutex_unlock(&lru_mutex);
                    nuevaPagina->bitUso=1;
                    nuevaPagina->bitModificado=1;
                    nuevaPagina->bitPresencia=1;

                    list_iterator_destroy(iterator);

                    list_add(temp->paginas, nuevaPagina);

                    cantidadDePaginasAAgregar--;
               
                    /*  COMENTO ESTO PORQUE ES LO VIEJO Y NO SE SI EN ALGUN CASO ESPECIFICO ME PUEDA PASAR QUE NO TENGA QUE MALLOCQUEAR
                    paginaSiguienteALaUltima->frame = getNewEmptyFrame(processId);
                    paginaSiguienteALaUltima->isfree = BUSY;
                    pthread_mutex_lock(&lru_mutex);
                    lRUACTUAL++;
                    paginaSiguienteALaUltima->lRU = lRUACTUAL;
                    pthread_mutex_unlock(&lru_mutex);
                    paginaSiguienteALaUltima->bitUso=1;
                    paginaSiguienteALaUltima->bitModificado=1;
                    paginaSiguienteALaUltima->bitPresencia=1;



                    list_iterator_destroy(iterator);

                    cantidadDePaginasAAgregar--;*/
                
            }else{
                paginaSiguienteALaUltima->isfree = BUSY;
                pthread_mutex_lock(&lru_mutex);
                lRUACTUAL++;
                paginaSiguienteALaUltima->lRU = lRUACTUAL;
                pthread_mutex_unlock(&lru_mutex);
                paginaSiguienteALaUltima->bitUso=1;
                paginaSiguienteALaUltima->bitModificado = 0;
                paginaSiguienteALaUltima->bitPresencia = 1;

                list_iterator_destroy(iterator);
            
                cantidadDePaginasAAgregar--;
            }
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
        if( !allFramesUsedForAsignacionFijaPara(idProcess)){
            pthread_mutex_lock(&list_pages_mutex);
            t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            while (temp->id != idProcess) {

                temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);  

            }

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);

            while(list_iterator_has_next(iterator2)){
                    Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);

                    if(tempPagina->isfree == FREE && tempPagina->bitPresencia==1){
                        list_iterator_destroy(iterator);
                        list_iterator_destroy(iterator2);
                        pthread_mutex_unlock(&list_pages_mutex);
                        return tempPagina->frame;
                    }
            }
            pthread_mutex_unlock(&list_pages_mutex);
        }else{
            if(cantidadDeFramesEnMemoriaPor(idProcess)==cantidadDePaginasPorProceso){
                return -1;
            }
            while(emptyFrame < paginaFinal){
                isfree= estaOcupadoUn(emptyFrame, idProcess);
                if(isfree!= BUSY){
                    return emptyFrame;
                }

                emptyFrame++;
            }
            return -1;
        }
        return -1;
    }

    return emptyFrame;
}

int estaOcupadoUn(int emptyFrame, int idProcess){
    int isfree = FREE;
    pthread_mutex_lock(&list_pages_mutex);
    if(todasLasTablasDePaginas != NULL){
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
            while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
    
                if(tempPagina->frame == emptyFrame ){
                    if(tipoDeAsignacionDinamica){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    pthread_mutex_unlock(&list_pages_mutex);
                    return tempPagina->isfree;
                    }
                    else{
                        int pid = getProcessIdby(emptyFrame);
                        if(pid != -1){
                            list_iterator_destroy(iterator);
                            list_iterator_destroy(iterator2);
                            pthread_mutex_unlock(&list_pages_mutex);
                            return tempPagina->isfree;
                        }
                    }
                }
            }
            list_iterator_destroy(iterator2);
        }
    list_iterator_destroy(iterator);
    }
    pthread_mutex_unlock(&list_pages_mutex);
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
    pthread_mutex_lock(&list_pages_mutex);
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
                    pthread_mutex_unlock(&list_pages_mutex);
                    
                    return 1;
                }
            }
            list_iterator_destroy(iterator2);
        }
    list_iterator_destroy(iterator);
    }

    pthread_mutex_unlock(&list_pages_mutex);
    return 0;
}

//chequeo si los primeros frames fijos ya fueron asignados
int allFramesUsedForAsignacionFijaPara(int processID){
    TablaDePaginasxProceso* tempTabla =get_pages_by(processID);

    int contador=0;

    t_list_iterator * iterator = list_iterator_create(tempTabla->paginas);
    
    while(list_iterator_has_next(iterator)){
        Pagina* tempPagina = list_iterator_next(iterator);

        if(tempPagina->isfree == BUSY){
            contador++;
        }

    }

    if(contador >= cantidadDePaginasPorProceso){
        return 1;
    }else
    {
        return 0;
    }
    
}

int cantidadDeFramesEnMemoriaPor(int processID){
    TablaDePaginasxProceso* tempTabla =get_pages_by(processID);

    int contador=0;

    t_list_iterator * iterator = list_iterator_create(tempTabla->paginas);
    
    while(list_iterator_has_next(iterator)){
        Pagina* tempPagina = list_iterator_next(iterator);

        if(tempPagina->bitPresencia){
            contador++;
        }

    }

    return contador;
}

int getFrameDeUn(int processId, int mayorNroDePagina){

    TablaDePaginasxProceso* temp = get_pages_by(processId);
    
    Pagina *tempPagina;
    TLB* tlb = NULL;
    if(max_entradas_tlb > 0){
        tlb = fetch_entrada_tlb(processId, mayorNroDePagina);
    }
    if (tlb != NULL){
        tempPagina = (Pagina *) list_get(temp->paginas, tlb->pagina - 1);
        if(tempPagina->bitPresencia == 1){
            log_info(logger, "Hubo un TLB HIT: Devuelvo el Frame %d con exito", tlb->frame);
            return tlb->frame;
        }
    } else {
       tempPagina = getPageDe(processId, mayorNroDePagina);
    }


    if(tempPagina->pagina == mayorNroDePagina){
        if(tempPagina->bitPresencia==0){
            utilizarAlgritmoDeAsignacion(processId);
            tempPagina->frame = getNewEmptyFrame(processId);
            int pay_len = 2*sizeof(int);
            void* payload = _serialize(pay_len, "%d%d", processId, mayorNroDePagina); 
            log_info(logger, "Pidiendo La Pagina %d del Proceso %d a Swamp", mayorNroDePagina, processId);      
            void* response = send_message_swamp(MEMORY_RECV_SWAP_SEND, payload, pay_len);
            void* swamp_mem = malloc(tamanioDePagina);

            memcpy(swamp_mem, response + sizeof(int), tamanioDePagina);
            
            memcpy(memoria + (tempPagina->frame*tamanioDePagina), swamp_mem, tamanioDePagina);
            free(payload);
            free(response);
            tempPagina->bitPresencia=1;
            //pedirselo a gonza
        }
        pthread_mutex_lock(&list_pages_mutex);
        tempPagina->bitUso = 1;
        pthread_mutex_unlock(&list_pages_mutex);

        pthread_mutex_lock(&lru_mutex);
        lRUACTUAL++;
        tempPagina->lRU = lRUACTUAL;
        pthread_mutex_unlock(&lru_mutex);

        log_info(logger, "tomo el frame %d con Exito", tempPagina->frame);

        log_info(logger, "Proceso %d, Numero de pagina %d", processId, tempPagina->pagina);
        if (max_entradas_tlb > 0){
            add_entrada_tlb(processId, tempPagina->pagina, tempPagina->frame);
        }
        return tempPagina->frame;
    }

    return -1;
}

int memfree(int idProcess, int direccionLogicaBuscada){
    log_info(logger,"arranco un memfree----------------------------");
    
    int paginaActual=1;
    int pagAnterior=0;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    int dirAllocFinal = tablaDelProceso->lastHeap;
    int dirAllocActual=0;
    //int offsetNextAllocAnterior;
    uint8_t estadoAllocAnterior;

    pthread_mutex_lock(&memory_mutex);
    while((dirAllocActual <= direccionLogicaBuscada) && dirAllocFinal>=direccionLogicaBuscada){
        
        if (dirAllocActual == direccionLogicaBuscada)
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int nextAllocActual;

            int prevAllocActual;

            int libre= FREE;

            if(paginaActual == (((dirAllocActual + HEAP_METADATA_SIZE)/ tamanioDePagina) + 1)){
                
                int posicionNextAllocDentroDelFrame = (dirAllocActual + 2*sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

                int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;


                memcpy(memoria + offset,&libre, sizeof(uint8_t));

                memcpy(&nextAllocActual ,memoria + offset-sizeof(uint32_t), sizeof(uint32_t));

                memcpy(&prevAllocActual ,memoria + offset-2*sizeof(uint32_t), sizeof(uint32_t));
                

            }else{
                void* paginasAuxiliares = malloc(tamanioDePagina*2);

                memcpy(paginasAuxiliares, memoria + frameBuscado*tamanioDePagina, tamanioDePagina);

                int frameFinal = getFrameDeUn(idProcess, paginaActual + 1);

                memcpy(paginasAuxiliares + tamanioDePagina, memoria + frameFinal*tamanioDePagina, tamanioDePagina);

                int allocDentroDelFrame = (dirAllocActual ) - ((paginaActual-1) * tamanioDePagina);

                memcpy(&prevAllocActual, paginasAuxiliares + allocDentroDelFrame, sizeof(uint32_t));

                memcpy(&nextAllocActual, paginasAuxiliares + allocDentroDelFrame + sizeof(uint32_t), sizeof(uint32_t));

                memcpy(paginasAuxiliares + allocDentroDelFrame + 2*sizeof(uint32_t), &libre, sizeof(uint8_t));

                memcpy(memoria + frameBuscado*tamanioDePagina, paginasAuxiliares, tamanioDePagina);

                mandarPaginaAgonza(idProcess ,frameBuscado, paginaActual);

                memcpy(memoria + frameFinal*tamanioDePagina, paginasAuxiliares+tamanioDePagina, tamanioDePagina);

                mandarPaginaAgonza(idProcess ,frameFinal, paginaActual+1);


                free(paginasAuxiliares);
            }

            if(estadoAllocAnterior == FREE && direccionLogicaBuscada!=0){
                int paginaInicial = (prevAllocActual/tamanioDePagina) + 1;
                int paginaFinal=  ((nextAllocActual + HEAP_METADATA_SIZE)/tamanioDePagina) + 1;

                void* paginasAuxiliares = malloc((paginaFinal - paginaInicial)+1);

                int contador=paginaInicial;
                int cantDePags=0;
                int offsetAuxiliar =0;
                while(contador<= paginaFinal){
                    int frameIterador = getFrameDeUn(idProcess,contador);

                    memcpy(paginasAuxiliares + offsetAuxiliar, memoria + frameIterador*tamanioDePagina, tamanioDePagina);

                    cantDePags++;
                    contador++;
                    offsetAuxiliar += tamanioDePagina;
                }

                offsetAuxiliar = (prevAllocActual + sizeof(uint32_t)) - ((paginaInicial-1)*tamanioDePagina);

                memcpy(paginasAuxiliares + offsetAuxiliar, &nextAllocActual, tamanioDePagina);

                offsetAuxiliar = (cantDePags* tamanioDePagina) - HEAP_METADATA_SIZE ;

                memcpy(paginasAuxiliares + offsetAuxiliar, &prevAllocActual, tamanioDePagina);

                contador=paginaInicial;
                offsetAuxiliar =0;
                while(contador<= paginaFinal){
                    int frameIterador = getFrameDeUn(idProcess,contador);

                    memcpy(memoria + frameIterador*tamanioDePagina, paginasAuxiliares + offsetAuxiliar, tamanioDePagina);
                    mandarPaginaAgonza(idProcess ,frameIterador, contador);

                    contador++;
                    offsetAuxiliar += tamanioDePagina;
                }


                free(paginasAuxiliares);
            }else{
                Pagina* page = getLastPageDe(idProcess);
                if((pagAnterior+1) == paginaActual  && direccionLogicaBuscada!=0 && paginaActual == page->pagina){
                    deletePagina(idProcess, paginaActual);

                    int paginaInicial = (prevAllocActual/tamanioDePagina) + 1;
                    int paginaFinal = (prevAllocActual+ HEAP_METADATA_SIZE/tamanioDePagina) + 1;
                    void* paginasAuxiliares = malloc((paginaFinal - paginaInicial)+1);

                    int contador = paginaInicial;
                    int offsetAux=0;
                    while (contador<=paginaFinal)
                    {
                        int frameIterador = getFrameDeUn(idProcess, contador);

                        memcpy(paginasAuxiliares+ offsetAux,memoria+frameIterador*tamanioDePagina,tamanioDePagina);

                        offsetAux+=tamanioDePagina;
                        contador++;
                    }
                    
                    
                    offsetAux = (prevAllocActual + sizeof(uint32_t)) - ((paginaInicial-1)*tamanioDePagina);
                    
                    nextAllocActual = 0;
                    
                    memcpy(paginasAuxiliares+ offsetAux,&nextAllocActual, sizeof(uint32_t));
                    
                    contador=paginaInicial;
                    offsetAux =0;
                    while(contador<= paginaFinal){
                        int frameIterador = getFrameDeUn(idProcess,contador);

                        memcpy(memoria + frameIterador*tamanioDePagina, paginasAuxiliares + offsetAux, tamanioDePagina);
                        mandarPaginaAgonza(idProcess ,frameIterador, contador);

                        contador++;
                        offsetAux += tamanioDePagina;
                    }

                    free(paginasAuxiliares);
                }
            }
            pthread_mutex_unlock(&memory_mutex);
            return 1;
        }
        else
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            if(paginaActual == (((dirAllocActual + HEAP_METADATA_SIZE)/ tamanioDePagina) + 1)){
                int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

                int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

                //offsetNextAllocAnterior = offset;

                memcpy(&dirAllocActual, memoria + offset, sizeof(uint32_t));

                memcpy(&estadoAllocAnterior, memoria + offset + sizeof(uint32_t), sizeof(uint8_t));

                pagAnterior = paginaActual;
            }else{
                void* paginasAuxiliares = malloc(tamanioDePagina*2);

                memcpy(paginasAuxiliares, memoria + frameBuscado*tamanioDePagina, tamanioDePagina);

                frameBuscado = getFrameDeUn(idProcess, paginaActual + 1);

                memcpy(paginasAuxiliares + tamanioDePagina, memoria + frameBuscado*tamanioDePagina, tamanioDePagina);

                int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

                memcpy(&dirAllocActual, paginasAuxiliares + posicionNextAllocDentroDelFrame, sizeof(uint32_t));

                memcpy(&estadoAllocAnterior, paginasAuxiliares + posicionNextAllocDentroDelFrame + sizeof(uint32_t), sizeof(uint8_t));

                pagAnterior = paginaActual++;

                free(paginasAuxiliares);
            }
        }
        
    }
    pthread_mutex_unlock(&memory_mutex);
    return MATE_FREE_FAULT;
}

void deletePagina(int idProcess,int paginaActual){

    TablaDePaginasxProceso* tablaDePags =get_pages_by(idProcess);
    Pagina *tempPagina;
    TLB* tlb = NULL;
    if (max_entradas_tlb > 0){
        tlb = fetch_entrada_tlb(idProcess, paginaActual);
    }
    
    if (tlb != NULL){
        list_remove(tablaDePags->paginas, tlb->pagina - 1);
        log_info(logger, "Deleteo la pag %d con Exito", tlb->pagina);
        return;
    } else {
    
    pthread_mutex_lock(&list_pages_mutex);
    t_list_iterator* iterator = list_iterator_create(tablaDePags->paginas);
    
    tempPagina = list_iterator_next(iterator);

    while (tempPagina->pagina != paginaActual)
    {
       tempPagina = list_iterator_next(iterator);
    }
    
    list_iterator_remove(iterator);
    pthread_mutex_unlock(&list_pages_mutex);
    log_info(logger, "Deleteo la pag %d con Exito", tempPagina->pagina);

    list_iterator_destroy(iterator);
    }
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
    Pagina *tempPagina;

    t_list_iterator* iterator = list_iterator_create(temp->paginas);
    tempPagina = list_iterator_next(iterator);
    while (tempPagina->pagina != nroPagina) {
        tempPagina = list_iterator_next(iterator);   
    }
    
    list_iterator_destroy(iterator);
    return tempPagina;
}

void inicializarUnProceso(int idDelProceso){
    log_info(logger, "Inicializando el Proceso %d", idDelProceso);

    HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData)); // ¿Se pierde la data si le hacemos un free?
    nuevoHeap->prevAlloc = 0;
    nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
    nuevoHeap->isfree = 1;

    TablaDePaginasxProceso* nuevaTablaDePaginas = malloc(sizeof(TablaDePaginasxProceso));
    nuevaTablaDePaginas->id = idDelProceso;
    nuevaTablaDePaginas->lastHeap = 0;
    nuevaTablaDePaginas->paginas = list_create();
    pthread_mutex_lock(&list_pages_mutex);
    list_add(todasLasTablasDePaginas, nuevaTablaDePaginas);
    pthread_mutex_unlock(&list_pages_mutex);
    
    if(tipoDeAsignacionDinamica){
        int nuevoFrame = getframeNoAsignadoEnMemoria();
        int offset = nuevoFrame * tamanioDePagina;
        pthread_mutex_lock(&memory_mutex);
        memcpy(memoria + offset, &nuevoHeap->prevAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset, &nuevoHeap->nextAlloc,sizeof(u_int32_t));

        offset+= sizeof(u_int32_t);
        memcpy(memoria + offset, &nuevoHeap->isfree,sizeof(u_int8_t));
        pthread_mutex_unlock(&memory_mutex);
        Pagina* nuevaPagina = malloc(sizeof(Pagina));
        nuevaPagina->pagina=1;
        pthread_mutex_lock(&lru_mutex);
        lRUACTUAL++;
        nuevaPagina->lRU = lRUACTUAL;
        pthread_mutex_unlock(&lru_mutex);
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

                pthread_mutex_lock(&memory_mutex);
                memcpy(memoria + offset, &nuevoHeap->prevAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset, &nuevoHeap->nextAlloc,sizeof(u_int32_t));

                offset+= sizeof(u_int32_t);
                memcpy(memoria + offset, &nuevoHeap->isfree,sizeof(u_int8_t));
                pthread_mutex_unlock(&memory_mutex);

                Pagina* nuevaPagina = malloc(sizeof(Pagina));
                nuevaPagina->pagina=1;
                pthread_mutex_lock(&lru_mutex);
                lRUACTUAL++;
                nuevaPagina->lRU = lRUACTUAL;
                pthread_mutex_unlock(&lru_mutex);
                nuevaPagina->isfree= BUSY;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia =1;
                nuevaPagina->bitModificado = 0;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }else
            {
                int nuevoFrame = getframeNoAsignadoEnMemoria();
                //int offset = nuevoFrame * tamanioDePagina;

                Pagina* nuevaPagina = malloc(sizeof(Pagina));
                nuevaPagina->pagina=paginasCargadas+1;
                pthread_mutex_lock(&lru_mutex);
                lRUACTUAL++;
                nuevaPagina->lRU = lRUACTUAL;
                pthread_mutex_unlock(&lru_mutex);
                nuevaPagina->isfree= FREE;
                nuevaPagina->frame = nuevoFrame;
                nuevaPagina->bitPresencia = 1;
                nuevaPagina->bitModificado = 0;

                
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
            }
            paginasCargadas++;
        }  
    }
    log_info(logger, "Proceso %d inicializado con Exito", idDelProceso);
}

int delete_process(int pid){
    log_info(logger, "Buscando la tabla de paginas del Proceso %d", pid);
    TablaDePaginasxProceso* table = get_pages_by(pid);
    if (table->id != pid) {
        log_error(logger, "El carpincho %d no posee tabla de paginas", pid);
        return -1;
    }
    void* payload = _serialize(sizeof(int), "%d", pid);
    log_info(logger, "Enviando el Carpincho %d a Swamp para eliminarlo de disco", pid);
    send_message_swamp(FINISH_PROCESS, payload, sizeof(int));
   
    free(payload);
    remove_paginas(table);
    free(table);

    return 1;
}

void remove_paginas(void* elem){
    TablaDePaginasxProceso* tabla = (TablaDePaginasxProceso*) elem;
    if (tabla->paginas != NULL){
        if (list_is_empty(tabla->paginas)){
            list_destroy(tabla->paginas);
        } else {
            list_destroy_and_destroy_elements(tabla->paginas, free);
        }
    }
}

int memwrite(int idProcess, int direccionLogicaBuscada, void* loQueQuierasEscribir, int tamanio){
    log_info(logger,"arranco un memwrite----------------------------");
    int paginaActual=1;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    int dirAllocFinal = tablaDelProceso->lastHeap;
    int dirAllocActual=0;

    pthread_mutex_lock(&memory_mutex);
    while((dirAllocActual <= direccionLogicaBuscada) && dirAllocFinal>direccionLogicaBuscada){
        log_info(logger,"EN memwrite---------dirAllocActual:%d",dirAllocActual);
        log_info(logger,"EN memwrite---------dirAllocFinal:%d",dirAllocFinal);
        if (dirAllocActual == direccionLogicaBuscada)
        {
            int finDelAlloc = 0;
            
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;

            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

            int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;
 
            memcpy(&finDelAlloc, memoria + offset, sizeof(uint32_t));

            finDelAlloc = finDelAlloc - 1 ;

            int offsetInicioAlloc = (frameBuscado*tamanioDePagina) + (dirAllocActual) - ((paginaActual-1) * tamanioDePagina) + HEAP_METADATA_SIZE;
            
            int paginaFinDelAlloc = (finDelAlloc/tamanioDePagina)+1;

            if (paginaFinDelAlloc == paginaActual)
            {
                memcpy(memoria + offsetInicioAlloc, loQueQuierasEscribir, tamanio);
                mandarPaginaAgonza(idProcess ,frameBuscado, paginaActual);
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

                offsetInicioAlloc = (dirAllocActual) - ((paginaActual-1) * tamanioDePagina) + HEAP_METADATA_SIZE;

                memcpy(espacioAuxiliar + offsetInicioAlloc, loQueQuierasEscribir , tamanio);

                nroPagAux = paginaActual;
                
                unOffset =0;
                
                while(nroPagAux<=paginaFinDelAlloc){
                    frameBuscado = getFrameDeUn(idProcess, nroPagAux);
                       
                    memcpy(memoria + (frameBuscado*tamanioDePagina) ,espacioAuxiliar + unOffset ,tamanioDePagina);
                    mandarPaginaAgonza(idProcess ,frameBuscado, nroPagAux);
                    
                    unOffset +=tamanioDePagina;

                    nroPagAux++;    
                }
            }
            
            pthread_mutex_unlock(&memory_mutex);
            return 1;
        }
        else
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            if(paginaActual == (((dirAllocActual + HEAP_METADATA_SIZE)/ tamanioDePagina) + 1)){
                int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

                int offset= (frameBuscado*tamanioDePagina) + posicionNextAllocDentroDelFrame;

                //offsetNextAllocAnterior = offset;

                memcpy(&dirAllocActual, memoria + offset, sizeof(uint32_t));

                int variablePrueba = 0;

                memcpy(&variablePrueba, memoria + offset - sizeof(uint32_t), sizeof(uint32_t));

                log_info(logger,"EN memwrite---------dirAllocActual en el if:%d",dirAllocActual);
                log_info(logger,"EN memwrite---------variablePrueba en el if:%d",variablePrueba);

            }else{
                void* paginasAuxiliares = malloc(tamanioDePagina*2);

                memcpy(paginasAuxiliares, memoria + frameBuscado*tamanioDePagina, tamanioDePagina);

                frameBuscado = getFrameDeUn(idProcess, paginaActual + 1);

                memcpy(paginasAuxiliares + tamanioDePagina, memoria + frameBuscado*tamanioDePagina, tamanioDePagina);

                int posicionNextAllocDentroDelFrame = (dirAllocActual + sizeof(uint32_t)) - ((paginaActual-1) * tamanioDePagina);

                memcpy(&dirAllocActual, paginasAuxiliares + posicionNextAllocDentroDelFrame, sizeof(uint32_t));

                log_info(logger,"EN memwrite---------dirAllocActual en el else:%d",dirAllocActual);

                free(paginasAuxiliares);
            }

        }

    }
    pthread_mutex_unlock(&memory_mutex);
    log_info(logger,"No se ha podido realizar la escritura");
    return MATE_WRITE_FAULT;
}

void utilizarAlgritmoDeAsignacion(int processID){

    if (string_equals_ignore_case(config_get_string_value(config,"ALGORITMO_REEMPLAZO_MMU"), "LRU"))
    {
       seleccionLRU( processID);
    }
    else
    {
       seleccionClockMejorado(processID);
    }
    


}

void seleccionLRU(int processID){

    uint32_t LRUmenor=9999; //recordar que lo que se busca es el LRU menor
    uint32_t frameVictima=0;
    uint32_t numeroDePagVictima;

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
                    numeroDePagVictima = paginatemp->pagina;
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
                numeroDePagVictima = paginatemp->pagina;
            }

            paginatemp = list_iterator_next(iterator2);
        }
        
        list_iterator_destroy(iterator2);
    }

    //falta una parte que le mande el mendaje a gonza

    int pay_len = 3*sizeof(int)+tamanioDePagina;
    void* paginaAEnviar = malloc(tamanioDePagina);
    memcpy(paginaAEnviar,memoria + (frameVictima*tamanioDePagina),tamanioDePagina);
    void* payload = _serialize(pay_len, "%d%d%d%v", processID, numeroDePagVictima,tamanioDePagina,paginaAEnviar); 
    log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", numeroDePagVictima, processID);      
    void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
    
    delete_entrada_tlb(processID, numeroDePagVictima, frameVictima);

    int iresp;

    memcpy(&iresp, resp, sizeof(int));
    if(iresp == 0){
        log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", processID);
    }
    free(resp);
    free(payload);
    free(paginaAEnviar);

    liberarFrame(frameVictima);
}

void seleccionClockMejorado(int idProcess){

    int frameNoEncontrado =1;

    int frameInicial = punteroFrameClock; 

    int frameFinal = tamanioDeMemoria/tamanioDePagina;

    punteroFrameClock++;

    while(frameNoEncontrado && frameInicial!=punteroFrameClock){

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        if(!tipoDeAsignacionDinamica &&  idProcess==getProcessIdby(punteroFrameClock)){
            Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);

            if(paginaEncontrada->bitModificado == 0 && paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", paginaEncontrada->pagina);
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);
                liberarFrame(paginaEncontrada->frame);
            }

            punteroFrameClock++;
        }else{
            if (tipoDeAsignacionDinamica)
            {
                Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);

            if(paginaEncontrada->bitModificado == 0 && paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", paginaEncontrada->pagina);
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);
                liberarFrame(paginaEncontrada->frame);
            }
            }
            
            punteroFrameClock++;
        }
    }

    while(frameNoEncontrado ){

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        if(!tipoDeAsignacionDinamica &&  idProcess==getProcessIdby(punteroFrameClock)){
            Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);

            if(paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", paginaEncontrada->pagina);
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);

                liberarFrame(paginaEncontrada->frame);
            }else{
                paginaEncontrada->bitUso =0;
            }


                punteroFrameClock++;
            }else{
                if (tipoDeAsignacionDinamica){
                    Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);

                    if(paginaEncontrada->bitUso==0){
                        frameNoEncontrado =0;

                        int pay_len = 3*sizeof(int)+tamanioDePagina;
                        void* paginaAEnviar = malloc(tamanioDePagina);
                        memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                        int pid = getProcessIdby(paginaEncontrada->frame);
                        void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                        log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                        void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                        delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);
                        
                        int iresp;
                        memcpy(&iresp, resp, sizeof(int));
                        if(iresp == 0){
                            log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", paginaEncontrada->pagina);
                        }
                        free(resp);
                        free(payload);
                        free(paginaAEnviar);

                        liberarFrame(paginaEncontrada->frame);
                    }else{
                        paginaEncontrada->bitUso =0;
                    }
                }

                punteroFrameClock++;
            }
        }

}

Pagina *getMarcoDe(uint32_t nroDeFrame){

    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    

    Pagina *paginatemp;
        
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

uint32_t getProcessIdby(uint32_t nroDeFrame)
{
    uint32_t processEncontrado = -1;

    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    

    Pagina *paginatemp;
        
    while (list_iterator_has_next(iterator)) {
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
        while (list_iterator_has_next(iterator2))
        {
            paginatemp = list_iterator_next(iterator2);
            if(paginatemp->frame == nroDeFrame){
                list_iterator_destroy(iterator);
                list_iterator_destroy(iterator2);
                return temp->id;
            }

        }
        
        list_iterator_destroy(iterator2);
    }
        
    list_iterator_destroy(iterator);

    return processEncontrado;
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

                /* if(!tipoDeAsignacionDinamica){
                Pagina* paginaSiguienteALaUltima = malloc(sizeof(Pagina));
                
                paginaSiguienteALaUltima->frame = paginatemp->frame;
                paginaSiguienteALaUltima->isfree = FREE;
                paginaSiguienteALaUltima->bitPresencia=1; 

                list_add(temp->paginas, paginaSiguienteALaUltima);
                }*/
            }

            
        }
        
        list_iterator_destroy(iterator2);
        
    }
        
    list_iterator_destroy(iterator);
}

void mandarPaginaAgonza(int processID ,uint32_t frameDeMemoria, uint32_t nroDePagina){
    int pay_len = 3*sizeof(int)+tamanioDePagina;
    void* paginaAEnviar = malloc(tamanioDePagina);
    memcpy(paginaAEnviar,memoria + (frameDeMemoria*tamanioDePagina),tamanioDePagina);
    int pid = processID;
    void* payload = _serialize(pay_len, "%d%d%d%v", pid, nroDePagina,tamanioDePagina,paginaAEnviar);  
    log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", nroDePagina, pid);      
    void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);

    int iresp;
    memcpy(&iresp, resp, sizeof(int));
    if(iresp == 0){
        log_error(logger, "Error al enviar la pagina %d a Swamp, no posee más espacio!", nroDePagina);
    }
    free(resp);
    free(payload);
    free(paginaAEnviar);
}

HeapMetaData* get_heap_metadata(int offset){
    HeapMetaData* newHeap = malloc(sizeof(HeapMetaData));

    pthread_mutex_lock(&memory_mutex);
    memcpy(&newHeap->prevAlloc, memoria + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(&newHeap->nextAlloc, memoria + offset, sizeof(uint32_t));
    offset = offset + sizeof(uint32_t);

    memcpy(&newHeap->isfree, memoria + offset, sizeof(uint8_t));
    pthread_mutex_unlock(&memory_mutex);

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

bool has_empty_instance(void* elem){
        if (elem == NULL){
            return false;
        }
        TLB* tlb = (TLB*) elem;
        return (tlb->pid == UINT32_MAX && tlb->pagina == UINT32_MAX && tlb->frame == UINT32_MAX);
}

void add_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame){
    log_info(logger, "Agregando una nueva entrada en la TLB: Proceso %d, Pagina %d, Frame %d", pid, page, frame);
    pthread_mutex_lock(&tlb_mutex);

    bool has_exist_instance(void* elem){
        if (elem == NULL){
            return false;
        }
        TLB* tlb = (TLB*) elem;
        return (tlb->pid == pid && tlb->pagina == page && tlb->frame == frame);
    }

    if (list_any_satisfy(tlb_list, has_exist_instance)){
        log_warning(logger, "Ya existe una Entrada de TLB para el Proceso %d, Pagina %d, Frame %d", pid, page, frame);
        pthread_mutex_unlock(&tlb_mutex);
        return;
    }

    TLB* new_instance = new_entrada_tlb(pid, page, frame);
    TLB* deleted_instance = (TLB *) list_find(tlb_list, has_empty_instance);
    if (deleted_instance != NULL){
        deleted_instance->pid = new_instance->pid;
        deleted_instance->pagina = new_instance->pagina;
        deleted_instance->frame = new_instance->frame;
        deleted_instance->fifo = new_instance->fifo;
        deleted_instance->lru = new_instance->lru;
        free(new_instance);

        set_pid_metric_if_missing(pid);
        pthread_mutex_unlock(&tlb_mutex);
        log_info(logger, "Entrada de TLB insertada en un espacio vacío con exito");
        return;
    }

    if (list_size(tlb_list) < max_entradas_tlb) {
        list_add(tlb_list, new_instance); 
    } else {
        replace_entrada(new_instance);
    }

    set_pid_metric_if_missing(pid);
    pthread_mutex_unlock(&tlb_mutex);
    log_info(logger, "Entrada de TLB insertada con exito");
}

TLB* new_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame){
    TLB* new_instance = malloc(sizeof(TLB));
    new_instance->pid = pid;
    new_instance->pagina = page;
    new_instance->frame = frame;
    pthread_mutex_lock(&entrada_fifo_mutex);
    new_instance->fifo = entrada_fifo;
    entrada_fifo++;
    pthread_mutex_unlock(&entrada_fifo_mutex);
    pthread_mutex_lock(&tlb_lru_mutex);
    new_instance->lru = tlb_lru_global;
    tlb_lru_global++;
    pthread_mutex_unlock(&tlb_lru_mutex);

    return new_instance;
}

void replace_entrada(TLB* new_instance){
    if (string_equals_ignore_case(config_get_string_value(config, "ALGORITMO_REEMPLAZO_TLB"), "FIFO")){    
        log_info(logger, "Reemplazo por FIFO");
        TLB* tlb_to_replace = list_get_minimum(tlb_list, get_minimum_fifo_tlb);
        log_info(logger, "Proceso Reemplazado: Proceso %d, Pagina %d, Marco %d", tlb_to_replace->pid, tlb_to_replace->pagina, tlb_to_replace->frame);
        log_info(logger, "Nueva Entrada: Proceso %d, Pagina %d, Marco %d", new_instance->pid, new_instance->pagina, new_instance->frame);

        tlb_to_replace->pid = new_instance->pid;
        tlb_to_replace->pagina = new_instance->pagina;
        tlb_to_replace->frame = new_instance->frame;
        tlb_to_replace->fifo = new_instance->fifo;
        tlb_to_replace->lru = new_instance->lru;

    } else {
        log_info(logger, "Reemplazo por LRU");
        TLB* tlb_to_replace = list_get_minimum(tlb_list, get_minimum_lru_tlb);
        log_info(logger, "Proceso Reemplazado: Proceso %d, Pagina %d, Marco %d", tlb_to_replace->pid, tlb_to_replace->pagina, tlb_to_replace->frame);
        log_info(logger, "Nueva Entrada: Proceso %d, Pagina %d, Marco %d", new_instance->pid, new_instance->pagina, new_instance->frame);

        tlb_to_replace->pid = new_instance->pid;
        tlb_to_replace->pagina = new_instance->pagina;
        tlb_to_replace->frame = new_instance->frame;
        tlb_to_replace->fifo = new_instance->fifo;
        tlb_to_replace->lru = new_instance->lru;
    }
    free(new_instance);
}

void* get_minimum_lru_tlb(void* actual, void* next){
    TLB* actual_tlb = (TLB*) actual;
    TLB* next_tlb = (TLB*) next;

    if (actual_tlb->lru < next_tlb->lru){
        return actual;
    }
    return next;
}

void* get_minimum_fifo_tlb(void* actual, void* next){
    TLB* actual_tlb = (TLB*) actual;
    TLB* next_tlb = (TLB*) next;

    if (actual_tlb->fifo < next_tlb->fifo){
        return actual;

    }
    return next;
}

void delete_entrada_tlb(uint32_t pid, uint32_t page, uint32_t frame){
    if (max_entradas_tlb == 0){
        return;
    }
    pthread_mutex_lock(&tlb_mutex);
    log_info(logger, "Eliminando Proceso: %d, Pagina: %d, Frame: %d de TLB", pid, page, frame);
    bool has_exist_instance(void* elem){
        if (elem == NULL){
            return false;
        }

        TLB* tlb = (TLB*) elem;
        return (tlb->pid == pid && tlb->pagina == page && tlb->frame == frame);
    }
    TLB* tlb = (TLB *) list_find(tlb_list, has_exist_instance);

    if (tlb != NULL){
        log_info(logger, "Se encontró con Exito la entrada de TLB a eliminar");
        tlb->pid = UINT32_MAX;
        tlb->pagina = UINT32_MAX;
        tlb->frame = UINT32_MAX;
        tlb->fifo = UINT32_MAX;
        tlb->lru = UINT32_MAX;
    } else {
        log_warning(logger, "No se encontró con Exito la entrada de TLB a eliminar");
    }

    log_info(logger, "Entrada eliminada con Exito", pid, page, frame);
    pthread_mutex_unlock(&tlb_mutex);
}

TLB* fetch_entrada_tlb(uint32_t pid, uint32_t page){
    pthread_mutex_lock(&tlb_mutex);
    t_list_iterator* iterator = list_iterator_create(tlb_list);
    while (list_iterator_has_next(iterator)){
        TLB* tlb = (TLB*) list_iterator_next(iterator);
        if (tlb->pid == pid && tlb->pagina == page){
            sum_metric(pid, TLB_HIT);
            log_info(logger, "TLB HIT: Proceso %d Pagina %d y Frame %d", pid, page, tlb->frame);
            pthread_mutex_lock(&max_hit_tlb_mutex);
            max_tlb_hit++;
            pthread_mutex_unlock(&max_hit_tlb_mutex);

            pthread_mutex_lock(&tlb_lru_mutex);
            tlb->lru = tlb_lru_global;
            tlb_lru_global++;
            pthread_mutex_unlock(&tlb_lru_mutex);

            sleep(retardo_hit_tlb);
            pthread_mutex_unlock(&tlb_mutex);
            list_iterator_destroy(iterator);

            return tlb;
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&tlb_mutex);

    log_info(logger, "TLB MISS: Proceso %d Pagina %d", pid, page);
    pthread_mutex_lock(&max_miss_tlb_mutex);
    max_tlb_miss++;
    pthread_mutex_unlock(&max_miss_tlb_mutex);
    sum_metric(pid, TLB_MISS);
    sleep(retardo_miss_tlb);

    return NULL;
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
            } else {
                metric->miss++;
            }
        }
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&m_list_mutex);
}

void* send_message_swamp(int command, void* payload, int pay_len){
    pthread_mutex_lock(&swamp_mutex);
    if (_send_message(swamp_fd, ID_MEMORIA, command, payload, pay_len, logger) < 0){
        log_error(logger, "Error al enviar mensaje a Swamp");
        void* err_msg = _serialize(sizeof(int), "%d", 0);
        return err_msg;
    }

    if(command == MEMORY_RECV_SWAP_SEND || command == MEMORY_SEND_SWAP_RECV){
        t_mensaje* msg = _receive_message(swamp_fd, logger);
        void* payload = msg->payload;
        free(msg->identifier);
        free(msg);
        pthread_mutex_unlock(&swamp_mutex);

        return payload;
    }

    pthread_mutex_unlock(&swamp_mutex);
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
        list_destroy_and_destroy_elements(tlb_list, free);
    } 
}
