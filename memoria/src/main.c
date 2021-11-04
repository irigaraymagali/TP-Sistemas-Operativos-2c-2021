#include "main.h"
int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test") == 0){
        return run_tests();
    }

    logger = log_create(LOG_PATH, PROGRAM, true, LOG_LEVEL_INFO);
    initPaginacion();

    inicializarUnProceso(1);
    inicializarUnProceso(2);
    
    init_swamp_connection();
    _start_server(config_get_string_value(config, PORT_CONFIG), handler, logger);
}

void init_swamp_connection(){
    swamp_fd = _connect(config_get_string_value(config, SWAMP_IP), config_get_string_value(config, SWAMP_PORT), logger);
    pthread_mutex_init(&swamp_mutex, NULL);
}

void handler(int fd, char* id, int opcode, void* payload, t_log* logger){
    log_info(logger, "Recibí un mensaje");

    int pid, espacioAReservar = 10;
    log_info(logger, "Deserializando los parametros recibidos...");
    switch(opcode){
        case MEM_INIT:
            deserialize_init_process(&pid, payload);
            inicializarUnProceso(pid);      
            break;
        case MEM_ALLOC:
            deserialize_mem_alloc(&pid, &espacioAReservar, payload);
            memalloc(espacioAReservar, pid);
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

void deserialize_init_process(int* pid, void* payload){
    memcpy(pid, payload, sizeof(int));
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
}
