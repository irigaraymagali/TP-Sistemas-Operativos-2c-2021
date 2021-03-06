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
    //pthread_mutex_init(&mutex_anashe, NULL);
    pthread_mutex_init(&iteration_mutex, NULL);
    pthread_mutex_init(&list_tables_mutex, NULL);
    pthread_mutex_init(&utilizacionDePagina_mutex, NULL);
    
    
    
    //pthread_mutex_init(&m_list_mutex, NULL);    

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
    retardo_hit_tlb = config_get_int_value(config, "RETARDO_ACIERTO_TLB") / 1000;
    retardo_miss_tlb = config_get_int_value(config, "RETARDO_FALLO_TLB") / 1000;
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
    pthread_mutex_lock(&utilizacionDePagina_mutex);
    int entra = entraEnElEspacioLibre(espacioAReservar, processId);
    pthread_mutex_unlock(&utilizacionDePagina_mutex);
    uint32_t mayorNroDePagina = 0; // 0 porque indica que no tiene asignado ninguna pagina, las pags siempre arancan en 1
    int tempLastHeap = 0;
    int espacioFinalDisponible = 0;

    if(entra == -1){
       /* 1-generar un nuevo alloc al final del espacio de direcciones
            2- si no cabe en las p??ginas ya reservadas se deber?? solicitar m??s
         */

        log_info(logger,"se debe generar un nuevo aloc");
        TablaDePaginasxProceso* temp = get_pages_by(processId);

        

        
        pthread_mutex_lock(&list_pages_mutex);
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);

        tempLastHeap = temp->lastHeap;

        while(list_iterator_has_next(iterator2)){
            Pagina* paginaTemporal = (Pagina*)  list_iterator_next(iterator2);

            if(mayorNroDePagina < paginaTemporal->pagina && paginaTemporal->isfree == BUSY){
                mayorNroDePagina = paginaTemporal->pagina;
            }
        }
        list_iterator_destroy(iterator2);
        pthread_mutex_unlock(&list_pages_mutex);
        

        //agregar algo que le pida la pagina a gonza si el bit presencia es 0
        
        espacioFinalDisponible = (mayorNroDePagina* tamanioDePagina) - tempLastHeap - HEAP_METADATA_SIZE; // el 9 es porque hay que agregar el puto heap atras

        HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData));
        nuevoHeap->prevAlloc = tempLastHeap;
        nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
        nuevoHeap->isfree = BUSY; // Nose si esto tiene que significar que es vacio o que est?? ocupado.

        // esto funciona si y solo si la pagina esta en memoria mas adelante hay que agregar los cambios nesesarios para utilizar el swap

        espacioAReservar += HEAP_METADATA_SIZE;

        if(espacioFinalDisponible >= espacioAReservar){

            void* espacioAuxiliar = malloc(sizeof(uint32_t)+sizeof(uint8_t));
            void* espacioAuxiliar2 = malloc(HEAP_METADATA_SIZE);

            int ubicacionNuevoLastHeap = tempLastHeap + espacioAReservar;

            memcpy(espacioAuxiliar,&ubicacionNuevoLastHeap , sizeof(uint32_t));
            memcpy(espacioAuxiliar+sizeof(uint32_t),&nuevoHeap->isfree , sizeof(uint8_t));

            pthread_mutex_lock(&utilizacionDePagina_mutex);
            editarAlgoEnMemoria(processId,tempLastHeap + sizeof(uint32_t),sizeof(uint32_t)+sizeof(uint8_t),espacioAuxiliar);
            pthread_mutex_unlock(&utilizacionDePagina_mutex);


            memcpy(espacioAuxiliar2,&tempLastHeap , sizeof(uint32_t));
            memcpy(espacioAuxiliar2+sizeof(uint32_t),&nuevoHeap->nextAlloc , sizeof(uint32_t));
            memcpy(espacioAuxiliar2+2*sizeof(uint32_t),&nuevoHeap->isfree , sizeof(uint8_t));

            pthread_mutex_lock(&utilizacionDePagina_mutex);
            editarAlgoEnMemoria(processId,ubicacionNuevoLastHeap ,HEAP_METADATA_SIZE,espacioAuxiliar2);
            pthread_mutex_unlock(&utilizacionDePagina_mutex);
            
            free(espacioAuxiliar);
            free(espacioAuxiliar2);
            

            pthread_mutex_lock(&list_pages_mutex);
            temp->lastHeap = tempLastHeap + espacioAReservar;
            pthread_mutex_unlock(&list_pages_mutex);

            free(nuevoHeap);
            return (tempLastHeap );
        } else {
           pthread_mutex_lock(&utilizacionDePagina_mutex);
           hayQueDegenerar = 0;
           agregarXPaginasPara(processId, (espacioAReservar-espacioFinalDisponible));
           

            int ubicacionNuevoLastHeap = tempLastHeap + espacioAReservar;
            // Pagina *ultimaPag = getLastPageDe(processId);

            void* espacioAuxiliar = malloc(sizeof(uint32_t)+sizeof(uint8_t));
            void* espacioAuxiliar2 = malloc(HEAP_METADATA_SIZE);

            memcpy(espacioAuxiliar,&ubicacionNuevoLastHeap , sizeof(uint32_t));
            memcpy(espacioAuxiliar+sizeof(uint32_t),&nuevoHeap->isfree , sizeof(uint8_t));

            editarAlgoEnMemoria(processId,tempLastHeap + sizeof(uint32_t),sizeof(uint32_t)+sizeof(uint8_t),espacioAuxiliar);


            memcpy(espacioAuxiliar2,&tempLastHeap , sizeof(uint32_t));
            memcpy(espacioAuxiliar2+sizeof(uint32_t),&nuevoHeap->nextAlloc , sizeof(uint32_t));
            memcpy(espacioAuxiliar2+2*sizeof(uint32_t),&nuevoHeap->isfree , sizeof(uint8_t));

            editarAlgoEnMemoria(processId,ubicacionNuevoLastHeap ,HEAP_METADATA_SIZE,espacioAuxiliar2);
            pthread_mutex_unlock(&utilizacionDePagina_mutex);

            if(hayQueDegenerar){
                int nroPagAux = mayorNroDePagina+1;
                int pagFin = ((HEAP_METADATA_SIZE + ubicacionNuevoLastHeap)/tamanioDePagina)+1; 
                while(nroPagAux <= pagFin){ 
                    deletePagina(processId, nroPagAux); 
                    log_error(logger,"degenerando pagina %d",nroPagAux); 
                    nroPagAux++; }
            }

            free(espacioAuxiliar);
            free(espacioAuxiliar2);
            memoryDump();
        }
        pthread_mutex_lock(&list_pages_mutex);
        temp->lastHeap = tempLastHeap + espacioAReservar;
        pthread_mutex_unlock(&list_pages_mutex);
        free(nuevoHeap);
        
        return (tempLastHeap);    
    }else
    {
       log_info(logger,"se encontro un alloc intermedio");

        int ubicacionLogicaDelIsfree = (entra+HEAP_METADATA_SIZE-1);
        int paginaDeLaUbicacionLogicaDelIsfree = (ubicacionLogicaDelIsfree/tamanioDePagina) + 1 ;
        int unFrame = getFrameDeUn(processId,paginaDeLaUbicacionLogicaDelIsfree);
        int offset = (unFrame * tamanioDePagina) +  (ubicacionLogicaDelIsfree - ((paginaDeLaUbicacionLogicaDelIsfree-1) * tamanioDePagina));
        int isfree = BUSY;
        
        pthread_mutex_lock(&memory_mutex);
        memcpy(memoria+ offset, &isfree,sizeof(uint8_t));
        pthread_mutex_unlock(&memory_mutex);
        setPaginaAsModificado(processId,paginaDeLaUbicacionLogicaDelIsfree);
        
        int paginaInicialHeapMeta = (entra/tamanioDePagina) +1;
        int paginaFinalHeapMeta = paginaDeLaUbicacionLogicaDelIsfree;
        int nroPagAux = paginaInicialHeapMeta;
        void *espacioAuxiliar = malloc(3*tamanioDePagina);
        int offsetEspacioAux=0;

        while(nroPagAux <= paginaFinalHeapMeta){
           int unFrame = getFrameDeUn(processId,nroPagAux);

            pthread_mutex_lock(&memory_mutex);
            memcpy(espacioAuxiliar+offsetEspacioAux, memoria+ (unFrame*tamanioDePagina),tamanioDePagina);
            pthread_mutex_unlock(&memory_mutex);

            nroPagAux++;
            offsetEspacioAux+=tamanioDePagina;
        }

        offsetEspacioAux= (entra - ((paginaInicialHeapMeta-1) * tamanioDePagina)) + sizeof(uint32_t);

        int nextAlloc;
        int paginaNextAlloc;

        memcpy(&nextAlloc, espacioAuxiliar + offsetEspacioAux, sizeof(uint32_t));

        paginaNextAlloc = (nextAlloc/tamanioDePagina)+1;

        if ((nextAlloc - (entra+HEAP_METADATA_SIZE)) > (espacioAReservar + HEAP_METADATA_SIZE))
        {
            uint32_t nuevoPrevAlloc=entra;
            uint32_t nuevoNextAlloc = nextAlloc;
            uint8_t nuevoisFree = FREE;

            uint32_t ubicacionLogicaDelNextAllocIntermedio= entra + HEAP_METADATA_SIZE + espacioAReservar;
            

            memcpy(espacioAuxiliar+offsetEspacioAux,&ubicacionLogicaDelNextAllocIntermedio ,sizeof(uint32_t));
            offsetEspacioAux+= (HEAP_METADATA_SIZE + espacioAReservar);

            memcpy(espacioAuxiliar+offsetEspacioAux,&nuevoPrevAlloc ,sizeof(uint32_t));
            offsetEspacioAux+=sizeof(uint32_t);

            memcpy(espacioAuxiliar+offsetEspacioAux,&nuevoNextAlloc ,sizeof(uint32_t));
            offsetEspacioAux+=sizeof(uint32_t);

            memcpy(espacioAuxiliar+offsetEspacioAux,&nuevoisFree ,sizeof(uint8_t));

            offsetEspacioAux = (nextAlloc - ((paginaInicialHeapMeta-1) * tamanioDePagina));

            memcpy(espacioAuxiliar+offsetEspacioAux,&ubicacionLogicaDelNextAllocIntermedio ,sizeof(uint32_t));

            nroPagAux=paginaInicialHeapMeta;
            offsetEspacioAux=0;
            pthread_mutex_lock(&memory_mutex);
            while(nroPagAux <= paginaNextAlloc){
                int framenecesitado = getFrameDeUn(processId, nroPagAux);
                
                memcpy(memoria + (framenecesitado*tamanioDePagina), espacioAuxiliar + offsetEspacioAux, tamanioDePagina);
                mandarPaginaAgonza(processId ,framenecesitado, nroPagAux);
                
                setPaginaAsModificado(processId,nroPagAux);
                
                //log_info(logger,"EN memwrite---------dirAllocActual:%d",);
                
                nroPagAux++;
                offsetEspacioAux+=tamanioDePagina;
            }
            pthread_mutex_unlock(&memory_mutex);
        }
        else
        {
            mandarPaginaAgonza(processId, unFrame,paginaDeLaUbicacionLogicaDelIsfree);
        }
        
        free(espacioAuxiliar);
    }
    
    
    return entra;

}

void editarAlgoEnMemoria(int processId,int inicio, int tamanio, void* loQuieroMeter){

    int pagiInicio = (inicio/tamanioDePagina)+1;
    int pagiFin =   ((inicio + tamanio - 1)/tamanioDePagina)+1;
    int frameBuscado;
    int posinicio= inicio - ((pagiInicio-1)*tamanioDePagina);

    if (pagiInicio == pagiFin)
    {
        
        frameBuscado = getFrameDeUn(processId,pagiInicio);
        pthread_mutex_lock(&memory_mutex);
        memcpy(memoria + (frameBuscado*tamanioDePagina)+posinicio,loQuieroMeter,tamanio);
        pthread_mutex_unlock(&memory_mutex);
        setPaginaAsModificado(processId,pagiInicio);
        mandarPaginaAgonza(processId ,frameBuscado, pagiInicio);
    }
    else
    {
        int nropagaux = pagiInicio;
        int tamanioPagInicial=tamanioDePagina-(inicio-((pagiInicio-1)*tamanioDePagina));
        int tamanioPagFinal=(inicio + tamanio) - ((pagiFin-1)*tamanioDePagina);
        int offset=0;

        while (nropagaux <=pagiFin)
        {
            
            if(nropagaux == pagiInicio){
                frameBuscado = getFrameDeUn(processId,pagiInicio);
                pthread_mutex_lock(&memory_mutex);
                memcpy(memoria + (frameBuscado*tamanioDePagina)+posinicio,loQuieroMeter,tamanioPagInicial);
                pthread_mutex_unlock(&memory_mutex);
                setPaginaAsModificado(processId,pagiInicio);
                mandarPaginaAgonza(processId ,frameBuscado, pagiInicio);
                offset+=tamanioPagInicial;
            }else{

                if(nropagaux == pagiFin){

                    frameBuscado = getFrameDeUn(processId,pagiFin);
                    pthread_mutex_lock(&memory_mutex);
                    memcpy(memoria + (frameBuscado*tamanioDePagina),loQuieroMeter+offset,tamanioPagFinal);
                    pthread_mutex_unlock(&memory_mutex);
                    setPaginaAsModificado(processId,pagiFin);
                    mandarPaginaAgonza(processId ,frameBuscado, pagiFin);

                }else
                {
                    
                    frameBuscado = getFrameDeUn(processId,nropagaux);
                    pthread_mutex_lock(&memory_mutex);
                    memcpy(memoria + (frameBuscado*tamanioDePagina),loQuieroMeter+offset,tamanioDePagina);
                    pthread_mutex_unlock(&memory_mutex);
                    setPaginaAsModificado(processId,nropagaux);
                    mandarPaginaAgonza(processId ,frameBuscado, nropagaux);

                    offset+=tamanioDePagina;
                }
            }

            nropagaux++;
        }
        

    }
}

void read_from_memory(int pid, int init_dir, int size, void* read){
    int init_page = (init_dir / tamanioDePagina) + 1;
    int last_page = ((init_dir + size - 1) / tamanioDePagina) + 1;
    int init_pos = init_dir - ((init_page-1) * tamanioDePagina);
    int frame, dir_fisica;

    if (init_page == last_page){
        pthread_mutex_lock(&utilizacionDePagina_mutex);
        frame = getFrameDeUn(pid, init_page);
        dir_fisica = (frame * tamanioDePagina) + init_pos;
        pthread_mutex_lock(&memory_mutex);
        memcpy(read, memoria + dir_fisica, size);
        pthread_mutex_unlock(&memory_mutex);
        pthread_mutex_unlock(&utilizacionDePagina_mutex);
    } else {
        int page_aux = init_page;
        int size_init_page = tamanioDePagina - (init_dir - ((init_page - 1) * tamanioDePagina));
        int size_last_page = (init_dir + size) - ((last_page - 1) * tamanioDePagina);
        int offset = 0;

        while (page_aux <= last_page){
            if(page_aux == init_page) {
                pthread_mutex_lock(&utilizacionDePagina_mutex);
                frame = getFrameDeUn(pid, init_page);
                dir_fisica = (frame * tamanioDePagina) + init_pos;
                pthread_mutex_lock(&memory_mutex);
                memcpy(read, memoria + dir_fisica, size_init_page);
                pthread_mutex_unlock(&memory_mutex);
                pthread_mutex_unlock(&utilizacionDePagina_mutex);
                offset += size_init_page;
            } else {
                pthread_mutex_lock(&utilizacionDePagina_mutex);
                frame = getFrameDeUn(pid, page_aux);
                dir_fisica = (frame * tamanioDePagina);
                if(page_aux == last_page){
                    pthread_mutex_lock(&memory_mutex);
                    memcpy(read + offset, memoria + dir_fisica, size_last_page);
                    pthread_mutex_unlock(&memory_mutex);
                } else {
                    pthread_mutex_lock(&memory_mutex);
                    memcpy(read + offset, memoria + dir_fisica, tamanioDePagina);
                    pthread_mutex_unlock(&memory_mutex);
                    offset += tamanioDePagina;
                }
                pthread_mutex_unlock(&utilizacionDePagina_mutex);
            }
            page_aux++;
        }
    }
}

void setPaginaAsModificado(int processId, int mayorNroDePagina){
    Pagina *tempPagina;

    //pthread_mutex_lock(&list_pages_mutex);
    tempPagina = getPageDe(processId, mayorNroDePagina);


    tempPagina->bitModificado = 1;
    //pthread_mutex_unlock(&list_pages_mutex);
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
        pthread_mutex_lock(&utilizacionDePagina_mutex);
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
            pthread_mutex_unlock(&utilizacionDePagina_mutex);
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
        pthread_mutex_unlock(&utilizacionDePagina_mutex);
    }
    
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_pages_mutex);
    return 1;
}

void* memread(uint32_t pid, int dir_logica, int size){
    log_info(logger,"arranco un memread----------------------------");
    void* read = malloc(size);
    void* err_msg = _serialize(sizeof(int), "%d", MATE_READ_FAULT);

    TablaDePaginasxProceso* pages = get_pages_by(pid);
    if (pages->id != pid){
        log_error(logger, "El proceso ingresado es incorrecto");
        free(read);
        return err_msg;
    }

    if (dir_logica >= pages->lastHeap){
        log_error(logger, "La Longitud de lectura recibida es invalida.");
        free(read);
        return err_msg;
    }

    int dir_content = dir_logica + HEAP_METADATA_SIZE;
    log_info(logger, "Realizando Lectura en Memoria...");
    read_from_memory(pid, dir_content, size, read);
    log_info(logger, "Lectura realizada con exito");
    // int algoint;
    // char* algo = string_new();
    // memcpy(&algoint, read, size);
    // memcpy(algo, read, size);

    // log_info(logger, "ALGO INT: %d", algoint);
    // log_info(logger, "ALGO: %s", algo);

    free(err_msg);
    return read;
}

TablaDePaginasxProceso* get_pages_by(int processID){
    pthread_mutex_lock(&list_tables_mutex);
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
    TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    while (temp->id != processID) {
        temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
    }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_tables_mutex);
    
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
        pthread_mutex_lock(&list_pages_mutex);
        int temp_last_heap = temp->lastHeap;
        pthread_mutex_unlock(&list_pages_mutex);
        
        while(allocActual < temp_last_heap  && espacioEncontrado==0){
            int paginaActual = (allocActual/tamanioDePagina) +1; 

            int frameActual = getFrameDeUn(processId, paginaActual);

            //void* espacioAuxiliar = malloc(2*tamanioDePagina);
           
           if(allocActual == 0){ 
            pthread_mutex_lock(&memory_mutex);
            memcpy(&nextAllocAux, memoria + (frameActual*tamanioDePagina)+sizeof(uint32_t),sizeof(uint32_t));

            memcpy(&isfreeAux, memoria + (frameActual*tamanioDePagina)+2*sizeof(uint32_t),sizeof(uint8_t));
            pthread_mutex_unlock(&memory_mutex);
           }

            if((nextAllocAux-HEAP_METADATA_SIZE)<espacioAReservar || isfreeAux == BUSY){
                while (allocActual < temp->lastHeap  && espacioEncontrado==0)
                {
                    
                    
                    paginaActual = (allocActual/tamanioDePagina) +1; 

                    int paginaFinAlloc = ((allocActual+HEAP_METADATA_SIZE-1)/tamanioDePagina) +1; 

                    int offsetInicioAlloc;

                    void* espacioAuxiliar = malloc(2*tamanioDePagina);

                    int nropagaux= paginaActual;
                    int offsetpagaux =0;
                    while(nropagaux<=paginaFinAlloc){
                        frameActual = getFrameDeUn(processId, nropagaux);
                        
                        pthread_mutex_lock(&memory_mutex);
                        memcpy(espacioAuxiliar+offsetpagaux, memoria + frameActual*tamanioDePagina,tamanioDePagina);
                        pthread_mutex_unlock(&memory_mutex);

                        offsetpagaux =tamanioDePagina;
                        nropagaux++;
                    }

                    offsetInicioAlloc= allocActual - ((paginaActual-1)*tamanioDePagina);

                    pthread_mutex_lock(&memory_mutex);
                    memcpy(&nextAllocAux,espacioAuxiliar + offsetInicioAlloc + sizeof(uint32_t),sizeof(uint32_t));

                    memcpy(&isfreeAux,espacioAuxiliar + offsetInicioAlloc + 2*sizeof(uint32_t),sizeof(uint8_t));
                    pthread_mutex_unlock(&memory_mutex);

                    //log_info(logger,"el nextalloc leido %d",nextAllocAux);

                    if(isfreeAux == FREE && (nextAllocAux - allocActual - HEAP_METADATA_SIZE) >= espacioAReservar){
                    free(espacioAuxiliar);
                    return allocActual;
                    
                    }
                    
                    allocActual = nextAllocAux;
                    free(espacioAuxiliar);
                }
            }else{
                return allocActual;
            }
        }
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
            ultimaPagina = getLastPageDe(processId); // que pasar??a si no tenes paginas?
            if(ultimaPagina == NULL){
                pthread_mutex_lock(&list_pages_mutex);
                nuevaPagina->pagina = FIRST_PAGE;
                pthread_mutex_unlock(&list_pages_mutex);
            } else {
                pthread_mutex_lock(&list_pages_mutex);
                int nroUltimaPagina = ultimaPagina->pagina ;
                nroUltimaPagina++;
                nuevaPagina->pagina= nroUltimaPagina;
                pthread_mutex_unlock(&list_pages_mutex);
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
            log_info(logger,"se ha asignado el frame: %d, nro de pag:%d y pid:%d",nuevaPagina->frame,nuevaPagina->pagina,processId);
            pthread_mutex_lock(&lru_mutex);
            lRUACTUAL++;
            nuevaPagina->lRU = lRUACTUAL;
            pthread_mutex_unlock(&lru_mutex);

            TablaDePaginasxProceso* temp = get_pages_by(processId);

            pthread_mutex_lock(&list_pages_mutex);
            list_add(temp->paginas, nuevaPagina); // ACA puede haber segun helgrind RACE CONDITION -> Array de mutex o un mutex para lista de paginas distinto a la de la tabla.
            pthread_mutex_unlock(&list_pages_mutex);

            cantidadDePaginasAAgregar--;
        }
    }else{
        while(cantidadDePaginasAAgregar != 0){
            TablaDePaginasxProceso* temp = get_pages_by(processId);

            ultimaPagina = getLastPageDe(processId);

            t_list_iterator* iterator = list_iterator_create(temp->paginas);
            Pagina* paginaSiguienteALaUltima = (Pagina*) list_iterator_next(iterator);

            pthread_mutex_lock(&list_pages_mutex);
            while (list_iterator_has_next(iterator) && (ultimaPagina->pagina +1 != paginaSiguienteALaUltima->pagina))
            {
               paginaSiguienteALaUltima = (Pagina*) list_iterator_next(iterator);
            }
            pthread_mutex_unlock(&list_pages_mutex);

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
                    log_info(logger,"Dsp del Algoritmo se ha asignado el frame: %d, nro de pag:%d y pid:%d",nuevaPagina->frame,nuevaPagina->pagina,processId);
                    list_iterator_destroy(iterator);

                    pthread_mutex_lock(&list_pages_mutex);
                    list_add(temp->paginas, nuevaPagina);
                    pthread_mutex_unlock(&list_pages_mutex);

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
                pthread_mutex_lock(&list_pages_mutex);
                paginaSiguienteALaUltima->isfree = BUSY;
                pthread_mutex_lock(&lru_mutex);
                lRUACTUAL++;
                paginaSiguienteALaUltima->lRU = lRUACTUAL;
                pthread_mutex_unlock(&lru_mutex);
                paginaSiguienteALaUltima->bitUso=1;
                paginaSiguienteALaUltima->bitModificado = 0;
                paginaSiguienteALaUltima->bitPresencia = 1;
                pthread_mutex_unlock(&list_pages_mutex);

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
            pthread_mutex_lock(&list_tables_mutex);
            t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);

            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
            

            while (temp->id != idProcess) {

                temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);  

            }
            pthread_mutex_unlock(&list_tables_mutex);

            pthread_mutex_unlock(&list_pages_mutex);
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
    if(todasLasTablasDePaginas != NULL){
        pthread_mutex_lock(&list_tables_mutex);
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            pthread_mutex_lock(&list_pages_mutex);
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
            while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
                if(tempPagina->frame == emptyFrame ){
                    if(tipoDeAsignacionDinamica){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    pthread_mutex_unlock(&list_pages_mutex);
                    pthread_mutex_unlock(&list_tables_mutex);
                    return tempPagina->isfree;
                    }
                    else{
                        int pid = getProcessIdby(emptyFrame);
                        if(pid != -1){
                            list_iterator_destroy(iterator);
                            list_iterator_destroy(iterator2);
                            pthread_mutex_unlock(&list_pages_mutex);
                            pthread_mutex_unlock(&list_tables_mutex);
                            return tempPagina->isfree;
                        }
                    }
                }
            }
            list_iterator_destroy(iterator2);
            pthread_mutex_unlock(&list_pages_mutex);
        }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_tables_mutex);
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
        pthread_mutex_lock(&list_tables_mutex);
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            pthread_mutex_lock(&list_pages_mutex);
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);

            t_list_iterator * iterator2 = list_iterator_create(temp->paginas);
            while(list_iterator_has_next(iterator2)){
                Pagina *tempPagina = (Pagina*) list_iterator_next(iterator2);
    
                if(tempPagina->frame == unFrame){
                    list_iterator_destroy(iterator);
                    list_iterator_destroy(iterator2);
                    pthread_mutex_unlock(&list_pages_mutex);
                    pthread_mutex_unlock(&list_tables_mutex);
                    
                    return 1;
                }
            }
            list_iterator_destroy(iterator2);
            pthread_mutex_unlock(&list_pages_mutex);
        }
    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_tables_mutex);
    }
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
    list_iterator_destroy(iterator);

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

    pthread_mutex_lock(&list_pages_mutex);
    t_list_iterator * iterator = list_iterator_create(tempTabla->paginas);
    
    while(list_iterator_has_next(iterator)){
        Pagina* tempPagina = list_iterator_next(iterator);

        if(tempPagina->bitPresencia){
            contador++;
        }

    }
    pthread_mutex_unlock(&list_pages_mutex);

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
        pthread_mutex_lock(&list_pages_mutex);
        tempPagina = (Pagina *) list_get(temp->paginas, tlb->pagina - 1);
        if(tempPagina->bitPresencia == 1){
            log_info(logger, "Hubo un TLB HIT: Devuelvo el Frame %d con exito", tlb->frame);
            tempPagina->bitUso =1;
            pthread_mutex_lock(&lru_mutex);
            lRUACTUAL++;
            tempPagina->lRU = lRUACTUAL;
            pthread_mutex_unlock(&lru_mutex);
            pthread_mutex_unlock(&list_pages_mutex);

            return tlb->frame;
        }
        pthread_mutex_unlock(&list_pages_mutex);
    } else {
       tempPagina = getPageDe(processId, mayorNroDePagina);
    }


    if(tempPagina->pagina == mayorNroDePagina){
        if(tempPagina->bitPresencia==0){
            utilizarAlgritmoDeAsignacion(processId);
            tempPagina->frame = getNewEmptyFrame(processId);
             pthread_mutex_lock(&list_pages_mutex);
            tempPagina->bitModificado=0;
            log_info(logger,"Dsp del Algoritmo se ha asignado el frame: %d, nro de pag:%d y pid:%d",tempPagina->frame,tempPagina->pagina,processId);
            int pay_len = 2*sizeof(int);
            void* payload = _serialize(pay_len, "%d%d", processId, mayorNroDePagina); 
            log_info(logger, "Pidiendo La Pagina %d del Proceso %d a Swamp", mayorNroDePagina, processId);      
            void* response = send_message_swamp(MEMORY_RECV_SWAP_SEND, payload, pay_len);
            void* swamp_mem = malloc(tamanioDePagina);

            memcpy(swamp_mem, response + sizeof(int), tamanioDePagina);
            
            pthread_mutex_lock(&memory_mutex);
            memcpy(memoria + (tempPagina->frame*tamanioDePagina), swamp_mem, tamanioDePagina);
            pthread_mutex_unlock(&memory_mutex);
            free(payload);
            free(response);
            free(swamp_mem);
            tempPagina->bitPresencia=1;
            pthread_mutex_unlock(&list_pages_mutex);
            //pedirselo a gonza
        }
        pthread_mutex_lock(&list_pages_mutex);
        tempPagina->bitUso = 1;
        

        pthread_mutex_lock(&lru_mutex);
        lRUACTUAL++;
        tempPagina->lRU = lRUACTUAL;
        pthread_mutex_unlock(&lru_mutex);

        log_info(logger, "tomo el frame %d con Exito", tempPagina->frame);

        log_info(logger, "Proceso %d, Numero de pagina %d", processId, tempPagina->pagina);
        if (max_entradas_tlb > 0){
            add_entrada_tlb(processId, tempPagina->pagina, tempPagina->frame);
        }
        pthread_mutex_unlock(&list_pages_mutex);
        return tempPagina->frame;
    }

    return -1;
}

int memfree(int idProcess, int direccionLogicaBuscada){
    log_info(logger,"arranco un memfree----------------------------");
    
    int paginaActual=1;
    int pagAnterior=0;

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    pthread_mutex_lock(&list_pages_mutex);
    int dirAllocFinal = tablaDelProceso->lastHeap;
    pthread_mutex_unlock(&list_pages_mutex);
    int dirAllocActual=direccionLogicaBuscada;
    //int offsetNextAllocAnterior;
    uint8_t estadoAllocAnterior;

    pthread_mutex_lock(&utilizacionDePagina_mutex);
        
        if (direccionLogicaBuscada <= dirAllocFinal)
        {
            paginaActual = (dirAllocActual/ tamanioDePagina) + 1 ;
            
            int frameBuscado = getFrameDeUn(idProcess, paginaActual);

            int nextAllocActual;

            int prevAllocActual;

            int libre= FREE;

            log_info(logger,"liberando alloc..");

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

                setPaginaAsModificado(idProcess,paginaActual);

                mandarPaginaAgonza(idProcess ,frameBuscado, paginaActual);

                memcpy(memoria + frameFinal*tamanioDePagina, paginasAuxiliares+tamanioDePagina, tamanioDePagina);

                setPaginaAsModificado(idProcess,paginaActual+1);

                mandarPaginaAgonza(idProcess ,frameFinal, paginaActual+1);


                free(paginasAuxiliares);
            }

            if(direccionLogicaBuscada!=0){
                pthread_mutex_unlock(&utilizacionDePagina_mutex);
                read_from_memory(idProcess,prevAllocActual+8,1,&estadoAllocAnterior);
                pagAnterior = ((prevAllocActual+HEAP_METADATA_SIZE-1)/tamanioDePagina) +1;
                pthread_mutex_lock(&utilizacionDePagina_mutex);
            }

            if(estadoAllocAnterior == FREE && direccionLogicaBuscada!=0){
                log_info(logger,"libero alloc anterior");

                editarAlgoEnMemoria(idProcess,prevAllocActual+sizeof(uint32_t),sizeof(uint32_t),&nextAllocActual);

                editarAlgoEnMemoria(idProcess,nextAllocActual,sizeof(uint32_t),&prevAllocActual);                


            }else{
                Pagina* page = getLastPageDe(idProcess);
                if((pagAnterior+1) == paginaActual  && direccionLogicaBuscada!=0 && paginaActual == page->pagina){
                    deletePagina(idProcess, paginaActual);

                    tablaDelProceso->lastHeap = prevAllocActual;
                }
            }
            pthread_mutex_unlock(&utilizacionDePagina_mutex);
            return 1;
        }      
     pthread_mutex_unlock(&utilizacionDePagina_mutex);
    return MATE_FREE_FAULT;
}

void deletePagina(int idProcess,int paginaActual){
    TablaDePaginasxProceso* tablaDePags = get_pages_by(idProcess);
    Pagina *tempPagina;
    TLB* tlb = NULL;
    if (max_entradas_tlb > 0){
        tlb = fetch_entrada_tlb(idProcess, paginaActual);
    }
    
    if (tlb != NULL){
        int deleted_page = tlb->pagina;
        pthread_mutex_lock(&list_pages_mutex);
        Pagina* page = getPageDe(idProcess,paginaActual);
        delete_entrada_tlb(idProcess, paginaActual, page->frame);
        t_list_iterator* iterator = list_iterator_create(tablaDePags->paginas);
        
        tempPagina = list_iterator_next(iterator);

        while (tempPagina->pagina != paginaActual)
        {
        tempPagina = list_iterator_next(iterator);
        }
        list_iterator_remove(iterator);
        pthread_mutex_unlock(&list_pages_mutex);
        log_info(logger, "Deleteo la pag %d con Exito", deleted_page);
        return;
    } else {
        pthread_mutex_lock(&list_pages_mutex);
        t_list_iterator* iterator = list_iterator_create(tablaDePags->paginas);
        
        tempPagina = list_iterator_next(iterator);

        while (tempPagina->pagina != paginaActual)
        {
        tempPagina = list_iterator_next(iterator);
        }
        
        delete_entrada_tlb(idProcess, paginaActual, tempPagina->frame);
        list_iterator_remove(iterator);
        pthread_mutex_unlock(&list_pages_mutex);
        log_info(logger, "Deleteo la pag %d con Exito", tempPagina->pagina);
        list_iterator_destroy(iterator);
    }
}

Pagina* get_page_by_dir_logica(TablaDePaginasxProceso* tabla, int dir_buscada){
    int paginaActual = 1;
    pthread_mutex_lock(&list_pages_mutex);
    int dirAllocFinal = tabla->lastHeap;
    pthread_mutex_unlock(&list_pages_mutex);
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

    HeapMetaData* nuevoHeap = malloc(sizeof(HeapMetaData)); // ??Se pierde la data si le hacemos un free?
    nuevoHeap->prevAlloc = 0;
    nuevoHeap->nextAlloc = NULL_ALLOC; //nuevoHeap.nextAlloc = NULL; Tiene que ser un puntero si queremos que sea NULL. Sino -1
    nuevoHeap->isfree = 1;

    pthread_mutex_lock(&list_tables_mutex);
    TablaDePaginasxProceso* nuevaTablaDePaginas = malloc(sizeof(TablaDePaginasxProceso));
    nuevaTablaDePaginas->id = idDelProceso;
    nuevaTablaDePaginas->lastHeap = 0;
    nuevaTablaDePaginas->paginas = list_create();
    
    
    list_add(todasLasTablasDePaginas, nuevaTablaDePaginas);
    pthread_mutex_unlock(&list_tables_mutex);
    
    pthread_mutex_lock(&utilizacionDePagina_mutex);
    if(tipoDeAsignacionDinamica){
        int nuevoFrame = getframeNoAsignadoEnMemoria();
        log_info(logger,"le doy el frame %d",nuevoFrame);
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

        pthread_mutex_lock(&list_pages_mutex);
        list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
        pthread_mutex_unlock(&list_pages_mutex);
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

                pthread_mutex_lock(&list_pages_mutex);
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
                pthread_mutex_unlock(&list_pages_mutex);
            } else {
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

                pthread_mutex_lock(&list_pages_mutex);
                list_add(nuevaTablaDePaginas->paginas, nuevaPagina);
                pthread_mutex_unlock(&list_pages_mutex);
            }
            paginasCargadas++;
        }  
    }
    log_info(logger, "Proceso %d inicializado con Exito", idDelProceso);
    free(nuevoHeap);
    pthread_mutex_unlock(&utilizacionDePagina_mutex);
}

int delete_process(int pid){
    bool exist_table(void* elem){
        if (elem == NULL){
            return false;
        }
        pthread_mutex_lock(&list_tables_mutex);
        TablaDePaginasxProceso* table = (TablaDePaginasxProceso*) elem;
        pthread_mutex_unlock(&list_tables_mutex);
        return table->id == pid;
    }

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
    pthread_mutex_lock(&utilizacionDePagina_mutex);
    pthread_mutex_lock(&list_pages_mutex);
    log_info(logger, "Eliminando la tabla de paginas correspondiente al Proceso %d", pid);
    remove_paginas(table);
    list_remove_by_condition(todasLasTablasDePaginas, exist_table);
    pthread_mutex_unlock(&list_pages_mutex);
    pthread_mutex_unlock(&utilizacionDePagina_mutex);
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
    //free(tabla);
}

void free_tabla_paginas(void* elem){
    if (elem != NULL){
        remove_paginas(elem);
        free((TablaDePaginasxProceso *) elem);
    }
}

int memwrite(int idProcess, int direccionLogicaBuscada, void* loQueQuierasEscribir, int tamanio){
    log_info(logger,"arranco un memwrite----------------------------");

    TablaDePaginasxProceso *tablaDelProceso = get_pages_by(idProcess);

    pthread_mutex_lock(&list_pages_mutex);
    int dirAllocFinal = tablaDelProceso->lastHeap;
    pthread_mutex_unlock(&list_pages_mutex);
    direccionLogicaBuscada+= HEAP_METADATA_SIZE;


        if (direccionLogicaBuscada < dirAllocFinal)
        {
           pthread_mutex_lock(&utilizacionDePagina_mutex);
           editarAlgoEnMemoria(idProcess,direccionLogicaBuscada,tamanio,loQueQuierasEscribir);
           pthread_mutex_unlock(&utilizacionDePagina_mutex);
           
            return 1;
        }

    
    log_info(logger,"No se ha podido realizar la escritura");
    return MATE_WRITE_FAULT;
}

void utilizarAlgritmoDeAsignacion(int processID){
    if (string_equals_ignore_case(config_get_string_value(config,"ALGORITMO_REEMPLAZO_MMU"), "LRU"))
    {
       seleccionLRU(processID);
    }
    else
    {
        
        seleccionClockMejorado(processID);
        

    }
}

void seleccionLRU(int processID){
    uint32_t LRUmenor=99999; //recordar que lo que se busca es el LRU menor
    uint32_t frameVictima=0;
    uint32_t numeroDePagVictima;
    int processVictima;

    if (tipoDeAsignacionDinamica)
    {
        pthread_mutex_lock(&list_pages_mutex);
        t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
        while (list_iterator_has_next(iterator)) {
            TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
            t_list_iterator* iterator2 = list_iterator_create(temp->paginas);

            while (list_iterator_has_next(iterator2))
            {
                Pagina *paginatemp = list_iterator_next(iterator2);
                /* code */
                if(paginatemp->lRU < LRUmenor && paginatemp->bitPresencia==1){
                    LRUmenor= paginatemp->lRU;
                    frameVictima = paginatemp->frame;
                    numeroDePagVictima = paginatemp->pagina;
                    processVictima = temp->id;
                }
            }
            list_iterator_destroy(iterator2);
        }
        list_iterator_destroy(iterator);
        pthread_mutex_unlock(&list_pages_mutex);
    }
    else
    {
        TablaDePaginasxProceso* temp = get_pages_by(processID);
        pthread_mutex_lock(&list_pages_mutex);
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        

        while (list_iterator_has_next(iterator2))
        {
            Pagina *paginatemp = list_iterator_next(iterator2);
        
            if(paginatemp->lRU < LRUmenor && paginatemp->bitPresencia==1){
                LRUmenor= paginatemp->lRU;
                frameVictima = paginatemp->frame;
                numeroDePagVictima = paginatemp->pagina;
                processVictima = processID;
            }

        }
        
        list_iterator_destroy(iterator2);
        pthread_mutex_unlock(&list_pages_mutex);

    }

    //falta una parte que le mande el mendaje a gonza

    int pay_len = 3*sizeof(int)+tamanioDePagina;
    void* paginaAEnviar = malloc(tamanioDePagina);
    for(int j=0; j<tamanioDePagina;j++){
        char valor = '\0';
       
        memcpy(paginaAEnviar + j, &valor, 1);
    }
    pthread_mutex_lock(&memory_mutex);
    memcpy(paginaAEnviar,memoria + (frameVictima*tamanioDePagina),tamanioDePagina);
    pthread_mutex_unlock(&memory_mutex);
    void* payload = _serialize(pay_len, "%d%d%d%v", processVictima, numeroDePagVictima,tamanioDePagina,paginaAEnviar); 
    log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", numeroDePagVictima, processVictima);     
    void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
    
    delete_entrada_tlb(processID, numeroDePagVictima, frameVictima);

    int iresp;

    memcpy(&iresp, resp, sizeof(int));
    if(iresp == 0){
        log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", processVictima);
        hayQueDegenerar =1;
    }
    free(resp);
    free(payload);
    free(paginaAEnviar);

    log_info(logger,"El Algoritmo LRU ha seleccionado el frame: %d, nro de pag:%d y pid:%d",frameVictima,numeroDePagVictima,processVictima);
    liberarFrame(frameVictima);
}

void seleccionClockMejorado(int idProcess){

    int frameNoEncontrado =1;

    int frameInicial = punteroFrameClock; 

    int frameFinal = tamanioDeMemoria/tamanioDePagina;

    //punteroFrameClock++;
    log_warning(logger,"el valor del puntero de clock al entrar es de %d",punteroFrameClock);
    LOOP:do{

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        if(!tipoDeAsignacionDinamica &&  idProcess==getProcessIdby(punteroFrameClock)){
            pthread_mutex_lock(&list_pages_mutex);
            Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
            pthread_mutex_unlock(&list_pages_mutex);

            if(paginaEncontrada->bitModificado == 0 && paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                for(int j=0; j<tamanioDePagina;j++){
                    char valor = '\0';
       
                    memcpy(paginaAEnviar + j, &valor, 1);
                }
                pthread_mutex_lock(&memory_mutex);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                pthread_mutex_unlock(&memory_mutex);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", paginaEncontrada->pagina);
                    hayQueDegenerar=1;
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);
                log_info(logger,"El Algoritmo Clock ha seleccionado el frame: %d, nro de pag:%d y pid:%d",paginaEncontrada->frame,paginaEncontrada->pagina,pid);
                liberarFrame(paginaEncontrada->frame);
            }

            punteroFrameClock++;
        }else{
            if (tipoDeAsignacionDinamica)
            {
                pthread_mutex_lock(&list_pages_mutex);
                Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
                pthread_mutex_unlock(&list_pages_mutex);


            if(paginaEncontrada->bitModificado == 0 && paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                pthread_mutex_lock(&memory_mutex);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                pthread_mutex_unlock(&memory_mutex);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", paginaEncontrada->pagina);
                    hayQueDegenerar=1;
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);
                log_info(logger,"El Algoritmo Clock ha seleccionado el frame: %d, nro de pag:%d y pid:%d",paginaEncontrada->frame,paginaEncontrada->pagina,pid);
                liberarFrame(paginaEncontrada->frame);
            }
            }
            
            punteroFrameClock++;
        }
    }while(frameNoEncontrado && frameInicial!=punteroFrameClock);

    if(frameNoEncontrado)
    {do{

        if(punteroFrameClock>= frameFinal){
            punteroFrameClock =0;
            continue;
        }

        if(!tipoDeAsignacionDinamica &&  idProcess==getProcessIdby(punteroFrameClock)){
                pthread_mutex_lock(&list_pages_mutex);            
            Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
                pthread_mutex_unlock(&list_pages_mutex);


            if(paginaEncontrada->bitUso==0){
                frameNoEncontrado =0;

                int pay_len = 3*sizeof(int)+tamanioDePagina;
                void* paginaAEnviar = malloc(tamanioDePagina);
                pthread_mutex_lock(&memory_mutex);
                memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                pthread_mutex_unlock(&memory_mutex);
                int pid = getProcessIdby(paginaEncontrada->frame);
                void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);

                int iresp;
                memcpy(&iresp, resp, sizeof(int));
                if(iresp == 0){
                    log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", paginaEncontrada->pagina);
                    hayQueDegenerar=1;
                }
                free(resp);
                free(payload);
                free(paginaAEnviar);
                log_info(logger,"El Algoritmo Clock ha seleccionado el frame: %d, nro de pag:%d y pid:%d",paginaEncontrada->frame,paginaEncontrada->pagina,pid);
                liberarFrame(paginaEncontrada->frame);
            }else{
                paginaEncontrada->bitUso =0;
            }


                punteroFrameClock++;
            }else{
                if (tipoDeAsignacionDinamica){
                pthread_mutex_lock(&list_pages_mutex);                    
                    Pagina *paginaEncontrada = getMarcoDe(punteroFrameClock);
                pthread_mutex_unlock(&list_pages_mutex);


                    if(paginaEncontrada->bitUso==0){
                        frameNoEncontrado =0;

                        int pay_len = 3*sizeof(int)+tamanioDePagina;
                        void* paginaAEnviar = malloc(tamanioDePagina);
                        pthread_mutex_lock(&memory_mutex);
                        memcpy(paginaAEnviar,memoria + (paginaEncontrada->frame*tamanioDePagina),tamanioDePagina);
                        pthread_mutex_unlock(&memory_mutex);
                        int pid = getProcessIdby(paginaEncontrada->frame);
                        void* payload = _serialize(pay_len, "%d%d%d%v", pid, paginaEncontrada->pagina,tamanioDePagina,paginaAEnviar);  
                        log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", paginaEncontrada->pagina, pid);      
                        void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);
                        delete_entrada_tlb(pid, paginaEncontrada->pagina, paginaEncontrada->frame);
                        
                        int iresp;
                        memcpy(&iresp, resp, sizeof(int));
                        if(iresp == 0){
                            log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", paginaEncontrada->pagina);
                            hayQueDegenerar=1;
                        }
                        free(resp);
                        free(payload);
                        free(paginaAEnviar);
                        log_info(logger,"El Algoritmo Clock ha seleccionado el frame: %d, nro de pag:%d y pid:%d",paginaEncontrada->frame,paginaEncontrada->pagina,pid);
                        liberarFrame(paginaEncontrada->frame);
                    }else{
                        paginaEncontrada->bitUso =0;
                    }
                }

                punteroFrameClock++;
            }
        }while(frameNoEncontrado && frameInicial!=punteroFrameClock);
        }
    if(frameInicial==punteroFrameClock){
        goto LOOP;
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
                list_iterator_destroy(iterator);
                list_iterator_destroy(iterator2);
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
    
    pthread_mutex_lock(&list_tables_mutex);
    t_list_iterator* iterator = list_iterator_create(todasLasTablasDePaginas);
    
    
        
    while (list_iterator_has_next(iterator)) {
        pthread_mutex_lock(&list_pages_mutex);
        TablaDePaginasxProceso* temp = (TablaDePaginasxProceso*) list_iterator_next(iterator);
        
        t_list_iterator* iterator2 = list_iterator_create(temp->paginas);
        
        

        while (list_iterator_has_next(iterator2))
        {
            Pagina *paginatemp = list_iterator_next(iterator2);
            if(paginatemp->frame == nroDeFrame){
                paginatemp->frame = (tamanioDeMemoria/tamanioDePagina)+1;
                paginatemp->bitPresencia = 0;
                paginatemp->bitModificado=0;
                /* if(!tipoDeAsignacionDinamica){
                Pagina* paginaSiguienteALaUltima = malloc(sizeof(Pagina));
                
                paginaSiguienteALaUltima->frame = paginatemp->frame;
                paginaSiguienteALaUltima->isfree = FREE;
                paginaSiguienteALaUltima->bitPresencia=1; 

                list_add(temp->paginas, paginaSiguienteALaUltima);
                }*/
                 //if(!tipoDeAsignacionDinamica){
                for(int j=0; j<tamanioDePagina;j++){
                    char valor = '\0';
                    pthread_mutex_lock(&memory_mutex);
                    memcpy(memoria + (nroDeFrame*tamanioDePagina) + j, &valor, 1);
                    pthread_mutex_unlock(&memory_mutex);
                }
    //}    
    //memset(memoria + (nroDeFrame*tamanioDePagina), '\0', tamanioDePagina);
            }

            
        }
        
        list_iterator_destroy(iterator2);
        pthread_mutex_unlock(&list_pages_mutex);
    }
   

    list_iterator_destroy(iterator);
    pthread_mutex_unlock(&list_tables_mutex);
}

void memoryDump(){
    int frameFinal = tamanioDeMemoria / tamanioDePagina;
    int frameInicial =0 ;

    while (frameFinal > frameInicial)
    {
        pthread_mutex_lock(&list_pages_mutex);
        Pagina *unaPagina =getMarcoDe(frameInicial);
        pthread_mutex_unlock(&list_pages_mutex);
        
        int pid = getProcessIdby(frameInicial);
        
        log_info(logger,"el memory dump dio id:%d frame:%d Pag:%d",pid,unaPagina->frame,unaPagina->pagina);
        
        frameInicial++;
    }
    

}
void mandarPaginaAgonza(int processID ,uint32_t frameDeMemoria, uint32_t nroDePagina){
    int pay_len = 3*sizeof(int)+tamanioDePagina;
    void* paginaAEnviar = malloc(tamanioDePagina);
    for(int j=0; j<tamanioDePagina;j++){
        char valor = '\0';
        memcpy(paginaAEnviar + j, &valor, 1);
    }
    pthread_mutex_lock(&memory_mutex);
    memcpy(paginaAEnviar,memoria + (frameDeMemoria*tamanioDePagina),tamanioDePagina);
    pthread_mutex_unlock(&memory_mutex);
    int pid = processID;
    void* payload = _serialize(pay_len, "%d%d%d%v", pid, nroDePagina,tamanioDePagina,paginaAEnviar);  
    log_info(logger, "Enviando la Pagina %d del Proceso %d a Swamp", nroDePagina, pid);      
    void* resp = send_message_swamp(MEMORY_SEND_SWAP_RECV, payload, pay_len);

    int iresp;
    memcpy(&iresp, resp, sizeof(int));
    if(iresp == 0){
        log_error(logger, "Error al enviar la pagina %d a Swamp, no posee m??s espacio!", nroDePagina);
        hayQueDegenerar=1;
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
        log_info(logger, "Entrada de TLB insertada en un espacio vac??o con exito");
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
        log_info(logger, "TLB: Reemplazo por FIFO");
        TLB* tlb_to_replace = list_get_minimum(tlb_list, get_minimum_fifo_tlb);
        log_info(logger, "TLB: Entrada Reemplazada: Proceso %d, Pagina %d, Marco %d", tlb_to_replace->pid, tlb_to_replace->pagina, tlb_to_replace->frame);
        log_info(logger, "TLB: Nueva Entrada: Proceso %d, Pagina %d, Marco %d", new_instance->pid, new_instance->pagina, new_instance->frame);

        tlb_to_replace->pid = new_instance->pid;
        tlb_to_replace->pagina = new_instance->pagina;
        tlb_to_replace->frame = new_instance->frame;
        tlb_to_replace->fifo = new_instance->fifo;
        tlb_to_replace->lru = new_instance->lru;

    } else {
        log_info(logger, "TLB: Reemplazo por LRU");
        TLB* tlb_to_replace = list_get_minimum(tlb_list, get_minimum_lru_tlb);
        log_info(logger, "TLB: Entrada Reemplazada: Proceso %d, Pagina %d, Marco %d", tlb_to_replace->pid, tlb_to_replace->pagina, tlb_to_replace->frame);
        log_info(logger, "TLB: Nueva Entrada: Proceso %d, Pagina %d, Marco %d", new_instance->pid, new_instance->pagina, new_instance->frame);

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
        log_info(logger, "Se encontr?? con Exito la entrada de TLB a eliminar");
        tlb->pid = UINT32_MAX;
        tlb->pagina = UINT32_MAX;
        tlb->frame = UINT32_MAX;
        tlb->fifo = UINT32_MAX;
        tlb->lru = UINT32_MAX;
    } else {
        log_warning(logger, "No se encontr?? con Exito la entrada de TLB a eliminar");
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
        pthread_mutex_unlock(&swamp_mutex);
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
