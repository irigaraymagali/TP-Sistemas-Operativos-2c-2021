#include "memory_utils.h"

void comandos(int valor){
    switch(valor){
        case mate_memalloc:
        break;
        case memfree:
        break;
        case memread:
        break;
        case memwrite:
        break;
        
        default:
         log_error(logger,"que comando metiste wachin????");
         
        
    }
   
}


void initPaginacion(){
    
    log_info(logger,"iniciando paginacion");

    configFile = config_create(""../cfg/memoria.conf");

}