#include "main.h"
int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test") == 0){
        return run_tests();
    }

    logger = log_create(LOG_PATH, PROGRAM, true, LOG_LEVEL_INFO);

    pthread_mutex_init(&pid_global_mutex, NULL);
    pid_global = 0;
    initPaginacion();

    inicializarUnProceso(1);
    inicializarUnProceso(2);

    init_swamp_connection();

    signal(SIGINT, print_metrics);
    signal(SIGUSR1, print_dump);
    signal(SIGUSR2, clean_tlb);

    _start_server(config_get_string_value(config, PORT_CONFIG), handler, logger);
}

void print_metrics(){
    log_info(logger, "Generando metricas de TLB");

    log_info(logger, "Cantidad de TLB Hit totales: %d", max_tlb_hit);
    log_info(logger, "Cantidad de TLB Hit del carpincho %d: %d", 1, max_tlb_hit);

    log_info(logger, "Cantidad de TLB Miss totales: %d", max_tlb_hit);
    log_info(logger, "Cantidad de TLB Miss del carpincho %d: %d", 1, max_tlb_hit);


    /*
    Cantidad de TLB Hit totales
    Cantidad de TLB Hit por carpincho indicando carpincho y cantidad.
    Cantidad de TLB Miss totales.
    Cantidad de TLB Miss por carpincho indicando carpincho y cantidad.
    */

   free_memory();
}

void print_dump(){
    pthread_mutex_lock(&tlb_mutex);
    char* dump_timestamp;
    char* dump_title;

    dump_timestamp = temporal_get_string_time("%d-%m-%y-%H:%M:%S");

    char *filename = string_new();
    string_append(&filename, "./dump_");
    string_append(&filename, dump_timestamp);
    string_append(&filename, ".dmp");
    dump_title = string_from_format("DUMP: %s\n", dump_timestamp);

    // FILE* dump_file = fopen(filename, "w");

    FILE* dump_file = txt_open_for_append(filename);
    if(dump_file == NULL){
        pthread_mutex_unlock(&tlb_mutex);
        perror("Error al abrir archivo dump");
        log_error(logger, "Error al abrir archivo dump");
        return;
    }

    write_dump(dump_file, "\n---------------------------------------------------------------------------------------------------\n");
    write_dump(dump_file, dump_title);
    
    t_list_iterator* iterator = list_iterator_create(tlb_list);
    int total_entradas = 3;

    for (int i = 0; i < total_entradas; i++){
        TLB* tlb;
        Dump* dump = malloc(sizeof(Dump));
        dump->entrada = i;
        dump->status = string_new();

        if (list_iterator_has_next(iterator)){
            tlb = list_iterator_next(iterator);

            string_append(&dump->status, "OCUPADO");
            dump->pid = string_itoa(tlb->pid);
            dump->page = string_itoa(tlb->pagina);
            dump->frame = string_itoa(tlb->frame);
        } else {
            string_append(&dump->status, "LIBRE");
            dump->pid = string_new();
            string_append(&dump->pid, "-");
            dump->page = string_new();
            string_append(&dump->page, "-");
            dump->frame = string_new();
            string_append(&dump->frame, "-");
        }

        char* record = string_from_format("Entrada: %d\tEstado: %s\tCarpincho: %s\tPagina: %s\tMarco: %s\n", dump->entrada, dump->status, tlb->pid, tlb->pagina, tlb->frame);
        write_dump(dump_file, record);
        write_dump(dump_file, "---------------------------------------------------------------------------------------------------\n");

        free(record);   
        free(dump->frame);
        free(dump->page);
        free(dump->pid);
        free(dump->status);
        free(dump);
    }

    pthread_mutex_unlock(&tlb_mutex);
    free(dump_title);
    txt_close_file(dump_file);
    free(filename);
    free(dump_timestamp);
}

void write_dump(FILE* file, char* record){
    txt_write_in_file(file, record);
    txt_write_in_stdout(record);
}

void clean_tlb(){
    log_info(logger, "Vaciando las entradas de la TLB...");

    pthread_mutex_lock(&tlb_mutex);
    list_clean_and_destroy_elements(tlb_list, (void*)free);
    pthread_mutex_unlock(&tlb_mutex);

    log_info(logger, "Entradas de la TLB vaciadas con exito");
}

void init_swamp_connection(){
    swamp_fd = _connect(config_get_string_value(config, SWAMP_IP), config_get_string_value(config, SWAMP_PORT), logger);
    pthread_mutex_init(&swamp_mutex, NULL);
}

void handler(int fd, char* id, int opcode, void* payload, t_log* logger){
    void* resp;
    int pid, iresp, espacioAReservar = 10, dir_logica = 0;
    log_info(logger, "Deserializando los parametros recibidos...");

    switch(opcode){
        case MATE_INIT:
            pid = deserialize_init_process(id, payload);
            inicializarUnProceso(pid);      
            break;
        case MATE_MEMALLOC:
            deserialize_mem_alloc(&pid, &espacioAReservar, payload);
            iresp = memalloc(pid, espacioAReservar);
            break;
        case MATE_MEMFREE:
            iresp = memfree(pid, dir_logica);
            break;
        case MATE_MEMREAD:
            resp = memread(pid, dir_logica);
            break;
        case MATE_MEMWRITE:
            iresp = memwrite(pid, dir_logica, payload); // PAYLOAD NO VA
            break;
        case MATE_CLOSE:
        case MATE_SEM_INIT:
        case MATE_SEM_WAIT:
        case MATE_SEM_POST:
        case MATE_SEM_DESTROY:
        case MATE_CALL_IO:
            log_error(logger, "Comando sin validez");
            break;
        default:
            log_error(logger,"Comando incorrecto");
            //que hacemos en este caso? nada?
    }
    if(opcode != MATE_MEMREAD){
        resp = _serialize(sizeof(int), "%d", iresp);
    }

    _send_message(fd, MEM_ID, opcode, resp, sizeof(resp), logger);
}

int deserialize_init_process(char* id, void* payload){
    int pid;
    if (string_equals_ignore_case(id, ID_MATE_LIB)){
        pthread_mutex_lock(&pid_global_mutex);
        pid = pid_global;
        pid_global += 2;
        pthread_mutex_unlock(&pid_global_mutex);
    } else {
        memcpy(&pid, payload, sizeof(int));
    }

    return pid;
}

void deserialize_mem_alloc(int* pid, int* espacioAReservar, void* payload){
    int offset = 0;

    memcpy(pid, payload, sizeof(int));
    offset += sizeof(int);

    memcpy(espacioAReservar, payload + offset, sizeof(int));
}

void free_memory(){
    log_destroy(logger);  
    config_destroy(config);  
    pthread_mutex_destroy(&swamp_mutex);
    pthread_mutex_destroy(&lru_mutex);
    pthread_mutex_destroy(&tlb_mutex);
    pthread_mutex_destroy(&pid_global_mutex);
    pthread_mutex_destroy(&list_pages_mutex);
    free_tlb();

    exit(EXIT_SUCCESS);
}
