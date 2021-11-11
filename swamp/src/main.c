#include "main.h"

int main(int argc, char ** argv) {
    if (argc > 1 && strcmp(argv[1], "-test") == 0)
    {
        return run_tests();
    }

    signal(SIGINT, cerrar_swamp);

    config_file = config_create(CONFIG_PATH);
    log_file = log_create(LOG_PATH, "[Swamp ᶘ◕ᴥ◕ᶅ]", 1, LOG_LEVEL_INFO);
    swap_file_size = config_get_int_value(config_file, "TAMANIO_SWAP");
    swap_page_size = config_get_int_value(config_file, "TAMANIO_PAGINA");
    marcos_por_carpincho = config_get_int_value(config_file, "MARCOS_POR_CARPINCHO");

    inicializar_directorios();
    inicializar_swap_files();

    consola("TIPO_ASIGNACION ASIGNACION_FIJA", 0);
    consola("GUARDAR_PAGINA 1 2 1111111111111111111111111111111111111111111111111111111111111111", 0);
    consola("GUARDAR_PAGINA 2 3 2222222222222222222222222222222222222222222222222222222222222222", 0);
    consola("GUARDAR_PAGINA 3 4 3333333333333333333333333333333333333333333333333333333333333333", 0);
    consola("GUARDAR_PAGINA 3 5 ASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDA", 0);
    consola("GUARDAR_PAGINA 1 5 JEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJE", 0);
    consola("FINALIZAR_PROCESO 3", 0);

    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, "1");
    t_list_iterator* list_iterator2 = list_iterator_create(tabla_paginas);
    while (list_iterator_has_next(list_iterator2)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator2);
        if (nodo_actual->proceso == 9999) {
            log_error(log_file, "nodo_actual->proceso: %d", nodo_actual->proceso);
            log_warning(log_file, "nodo_actual->pagina: %d", nodo_actual->pagina);
        }
    }
    log_info(log_file, "size: %d", list_size(tabla_paginas));
    list_iterator_destroy(list_iterator2);

    consola("GUARDAR_PAGINA 7 4 NINININININININININININININININININININININININININININININININI", 0);

    t_list_iterator* list_iterator3 = list_iterator_create(tabla_paginas);
    while (list_iterator_has_next(list_iterator3)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator3);
        if (nodo_actual->proceso == 7) {
            log_error(log_file, "nodo_actual->proceso: %d", nodo_actual->proceso);
            log_warning(log_file, "nodo_actual->pagina: %d", nodo_actual->pagina);
        }
    }
    log_info(log_file, "size: %d", list_size(tabla_paginas));
    list_iterator_destroy(list_iterator3);
    
    consola("GUARDAR_PAGINA 7 5 UWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUW", 0);

    t_list_iterator* list_iterator4 = list_iterator_create(tabla_paginas);
    while (list_iterator_has_next(list_iterator4)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator4);
        if (nodo_actual->proceso == 7) {
            log_error(log_file, "nodo_actual->proceso: %d", nodo_actual->proceso);
            log_warning(log_file, "nodo_actual->pagina: %d", nodo_actual->pagina);
        }
    }
    log_info(log_file, "size: %d", list_size(tabla_paginas));
    list_iterator_destroy(list_iterator4);

    recibir_mensajes();
    cerrar_swamp();
}

void cerrar_swamp() {
    log_info(log_file, "Cerrando Swamp (∪︿∪)...");
    close(server_socket);
    dictionary_destroy(swap_dict);
    list_destroy_and_destroy_elements(swap_list, nodo_swap_list_destroy);
    config_destroy(config_file);
    log_destroy(log_file);
    exit(EXIT_SUCCESS);
}