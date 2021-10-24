#include "main.h"
int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test") == 0){
        return run_tests();
    }

    logger = log_create(LOG_PATH, PROGRAM, true, LOG_LEVEL_INFO);

    config = config_create(CONFIG_PATH);
    
    init_swamp_connection();
    _start_server(config_get_string_value(config, PORT_CONFIG), handler, logger);
}

void init_swamp_connection(){
    swamp_fd = _connect(config_get_string_value(config, SWAMP_IP), config_get_string_value(config, SWAMP_PORT), logger);
    pthread_mutex_init(&swamp_mutex, NULL);
}

void handler(int fd, char* id, int opcode, void* payload, t_log* logger){
    log_info(logger, "Recibí un mensaje");
}

void free_memory(){
    log_destroy(logger);  
    config_destroy(config);  
    pthread_mutex_destroy(&swamp_mutex);
}
