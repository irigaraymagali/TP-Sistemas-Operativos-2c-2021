#include "main.h"

int main(int argc, char ** argv){
    if(argc > 1 && strcmp(argv[1],"-test")==0)
        return run_tests();
    else{  
        t_log* logger = log_create("./cfg/mate_lib.log", "MATE_LIB", true, LOG_LEVEL_INFO);
        log_info(logger, "Soy el Mate Lib! %s", mi_funcion_compartida());
        log_destroy(logger);
    } 
}