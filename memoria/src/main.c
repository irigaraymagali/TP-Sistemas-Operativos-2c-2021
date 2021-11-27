#include "main.h"
int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test") == 0){
        return run_tests();
    }

    logger = log_create(LOG_PATH, PROGRAM, true, LOG_LEVEL_INFO);
    config = config_create(CONFIG_PATH);

    port_fixer();

    pthread_mutex_init(&pid_global_mutex, NULL);
    pid_global = 0;
    init_swamp_connection();

    initPaginacion();

    void* payload = _serialize(sizeof(int), "%d", tipoDeAsignacionDinamica);
    send_message_swamp(TIPO_ASIGNACION, payload, sizeof(int));
    free(payload);

    /* inicializarUnProceso(1);
    inicializarUnProceso(2);

    memalloc(1, 20);
    memwrite(1,0,"carpincho",10);*/

    signal(SIGINT, print_metrics);
    signal(SIGUSR1, print_dump);
    signal(SIGUSR2, clean_tlb);

    _start_server(config_get_string_value(config, PORT_CONFIG), handler, logger);
}

void print_metrics(){
    pthread_mutex_lock(&tlb_mutex);
    log_info(logger, "Generando metricas de TLB");

    pthread_mutex_lock(&max_hit_tlb_mutex);
    log_info(logger, "Cantidad de TLB Hit totales: %d", max_tlb_hit);
    pthread_mutex_unlock(&max_hit_tlb_mutex);

    pthread_mutex_lock(&max_miss_tlb_mutex);
    log_info(logger, "Cantidad de TLB Miss totales: %d", max_tlb_miss);
    pthread_mutex_unlock(&max_miss_tlb_mutex);
  
    print_carpinchos_metrics();

    /*
    Cantidad de TLB Hit totales
    Cantidad de TLB Hit por carpincho indicando carpincho y cantidad.
    Cantidad de TLB Miss totales.
    Cantidad de TLB Miss por carpincho indicando carpincho y cantidad.
    */
    pthread_mutex_unlock(&tlb_mutex);
    free_memory();
}

void print_carpinchos_metrics(){
    t_list_iterator* iterator = list_iterator_create(metrics_list);

    while (list_iterator_has_next(iterator)){
        Metric* actual = list_iterator_next(iterator);
        log_info(logger, "Cantidad de TLB Hit del carpincho %d: %d", actual->pid, actual->hits);    
        log_info(logger, "Cantidad de TLB Miss del carpincho %d: %d", actual->pid, actual->miss);
    }
    list_iterator_destroy(iterator);
}
void print_dump(){
    pthread_mutex_lock(&tlb_mutex);
    char* dump_timestamp;
    char* dump_title;

    dump_timestamp = temporal_get_string_time("%d-%m-%y-%H:%M:%S");

    char *filename = string_new();
    /* char* dump_path = config_get_string_value(config, "PATH_DUMP_TLB");
    create_folder_dump(dump_path);*/

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

        char* record = string_from_format("Entrada: %d\tEstado: %s\tCarpincho: %s\tPagina: %s\tMarco: %s\n", dump->entrada, dump->status, dump->pid, dump->page, dump->frame);
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
    // free(dump_path);
}

void create_folder_dump(char* path){
    char** div_path = string_split(path, "/");
    char* concat_path = string_new();

    while (*div_path != NULL) {
        string_append_with_format(&concat_path, "/%s", div_path);

		DIR* dir = opendir(concat_path);
        if (dir) {
            closedir(dir);
        }

        mkdir(concat_path, 0777);
        div_path++;
	}
}

void write_dump(FILE* file, char* record){
    txt_write_in_file(file, record);
    txt_write_in_stdout(record);
}

void clean_tlb(){
    log_info(logger, "Vaciando las entradas de la TLB...");

    pthread_mutex_lock(&tlb_mutex);
    list_clean_and_destroy_elements(tlb_list, free);
    pthread_mutex_unlock(&tlb_mutex);

    log_info(logger, "Entradas de la TLB vaciadas con exito");
}

void init_swamp_connection(){
    swamp_fd = _connect(config_get_string_value(config, SWAMP_IP), config_get_string_value(config, SWAMP_PORT), logger);
    pthread_mutex_init(&swamp_mutex, NULL);
}

void handler(int fd, char* id, int opcode, void* payload, t_log* logger){
    void* resp;
    int size_msg;
    int pid, size, iresp, espacioAReservar, dir_logica;
    void* to_write = NULL;
    log_info(logger, "Deserializando los parametros recibidos...");

    switch(opcode){
        case MATE_INIT:
            pid = deserialize_init_process(id, payload);
            inicializarUnProceso(pid);  
            iresp = pid;    
            break;
        case MATE_MEMALLOC:
            deserialize_mem_alloc(&pid, &espacioAReservar, payload);
            iresp = memalloc(pid, espacioAReservar);
            break;
        case MATE_MEMFREE:
            deserialize_mem_free(&pid, &dir_logica, payload);
            iresp = memfree(pid, dir_logica);
            break;
        case MATE_MEMREAD:
            deserialize_mem_read(&pid, &dir_logica, &size, payload);
            resp = memread(pid, dir_logica, size);
            size_msg = size;
            break;
        case MATE_MEMWRITE:
            deserialize_mem_write(&pid, &dir_logica, &size, to_write, payload);
            iresp = memwrite(pid, dir_logica, to_write, size);
            break;
        case MATE_CLOSE:
            pid = deserialize_id_process(payload);
            iresp = delete_process(pid);
            break;    
        case SUSPENDER:
            pid = deserialize_id_process(payload);
            iresp = suspend_process(pid);
            break;
        case MATE_SEM_INIT:
        case MATE_SEM_WAIT:
        case MATE_SEM_POST:
        case MATE_SEM_DESTROY:
        case MATE_CALL_IO:
            log_error(logger, "Comando sin validez");
            iresp = -1;
            break;
        default:
            log_error(logger, "Comando incorrecto");
            iresp = -1;
    }
    if(opcode != MATE_MEMREAD){
        resp = _serialize(sizeof(int), "%d", iresp);
        size_msg = sizeof(int);
    }

    _send_message(fd, ID_MEMORIA, opcode, resp, size_msg, logger);
    free(resp);
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

int deserialize_id_process(void* payload){
    int pid;
    memcpy(&pid, payload, sizeof(int));
    return pid;
}

void deserialize_mem_alloc(int* pid, int* espacioAReservar, void* payload){
    int offset = 0;

    memcpy(pid, payload, sizeof(int));
    offset += sizeof(int);

    memcpy(espacioAReservar, payload + offset, sizeof(int));
}

void deserialize_mem_free(int* pid, int* dir_logica, void* payload){
    int offset = 0;

    memcpy(pid, payload, sizeof(int));
    offset += sizeof(int);

    memcpy(dir_logica, payload + offset, sizeof(int));
}

void deserialize_mem_read(int* pid, int* dir_logica, int* size, void* payload){
    int offset = 0;

    memcpy(pid, payload, sizeof(int));
    offset += sizeof(int);

    memcpy(dir_logica, payload + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(size, payload + offset, sizeof(int));
}

void deserialize_mem_write(int* pid, int* dir_logica, int* size, void* info, void* payload){
    int offset = 0;

    memcpy(pid, payload, sizeof(int));
    offset += sizeof(int);

    memcpy(dir_logica, payload + offset, sizeof(int));
    offset += sizeof(int);

    memcpy(size, payload + offset, sizeof(int));
    offset += sizeof(int);

    info = malloc(*size);
    memcpy(info, payload + offset, *size);
}

void free_memory(){
    log_destroy(logger);  
    config_destroy(config);  
    pthread_mutex_destroy(&swamp_mutex);
    pthread_mutex_destroy(&lru_mutex);
    pthread_mutex_destroy(&tlb_mutex);
    pthread_mutex_destroy(&pid_global_mutex);
    pthread_mutex_destroy(&list_pages_mutex);
    pthread_mutex_destroy(&m_list_mutex);

    free_tlb();
    list_destroy_and_destroy_elements(todasLasTablasDePaginas, remove_paginas);

    free(memoria);
    exit(EXIT_SUCCESS);
}

void port_fixer() {

    // FIX PUERTO SWAMP

    if (config_get_int_value(config, "PUERTO_SWAMP") == 5180) {
        char* puerto = string_from_format("%d", 5181);
        config_set_value(config, "PUERTO_SWAMP", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config, "PUERTO_SWAMP") == 5181) {
        char* puerto = string_from_format("%d", 5182);
        config_set_value(config, "PUERTO_SWAMP", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config, "PUERTO_SWAMP") == 5182) {
        char* puerto = string_from_format("%d", 5183);
        config_set_value(config, "PUERTO_SWAMP", puerto);
        free(puerto);
    }

    else {
        char* puerto = string_from_format("%d", 5180);
        config_set_value(config, "PUERTO_SWAMP", puerto);
        free(puerto);
    }

    // FIX PUERTO PROPIO

    if (config_get_int_value(config, "PUERTO") == 5080) {
        char* puerto = string_from_format("%d", 5081);
        config_set_value(config, "PUERTO", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config, "PUERTO") == 5081) {
        char* puerto = string_from_format("%d", 5082);
        config_set_value(config, "PUERTO", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config, "PUERTO") == 5082) {
        char* puerto = string_from_format("%d", 5083);
        config_set_value(config, "PUERTO", puerto);
        free(puerto);
    }

    else {
        char* puerto = string_from_format("%d", 5080);
        config_set_value(config, "PUERTO", puerto);
        free(puerto);
    }
}