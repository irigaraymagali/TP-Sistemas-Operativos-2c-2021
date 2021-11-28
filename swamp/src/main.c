#include "main.h"

int main(int argc, char ** argv) {
    if (argc > 1 && strcmp(argv[1], "-test") == 0)
    {
        return run_tests();
    }

    signal(SIGINT, cerrar_swamp);

    config_file = config_create(CONFIG_PATH);
    port_fixer();
    log_file = log_create(LOG_PATH, "[Swamp ᶘ◕ᴥ◕ᶅ]", 1, LOG_LEVEL_INFO);
    swap_file_size = config_get_int_value(config_file, "TAMANIO_SWAMP");
    swap_page_size = config_get_int_value(config_file, "TAMANIO_PAGINA");
    marcos_por_carpincho = config_get_int_value(config_file, "MARCOS_POR_CARPINCHO");
    retardo_swamp = config_get_int_value(config_file, "RETARDO_SWAMP");

    inicializar_directorios();
    inicializar_swap_files();

    // consola("TIPO_ASIGNACION ASIGNACION_FIJA", 0);
    // consola("GUARDAR_PAGINA 1 1 1111111111111111111111111111111111111111111111111111111111111111", 0);
    // consola("GUARDAR_PAGINA 1 2 2222222222222222222222222222222222222222222222222222222222222222", 0);
    // consola("GUARDAR_PAGINA 1 3 1111111111111111111111111111111111111111111111111111111111111111", 0);
    // consola("GUARDAR_PAGINA 1 4 2222222222222222222222222222222222222222222222222222222222222222", 0);
    // consola("GUARDAR_PAGINA 1 5 1111111111111111111111111111111111111111111111111111111111111111", 0);

    // consola("GUARDAR_PAGINA 2 6 2222222222222222222222222222222222222222222222222222222222222222", 0);
    // consola("GUARDAR_PAGINA 2 7 1111111111111111111111111111111111111111111111111111111111111111", 0);
    // consola("GUARDAR_PAGINA 2 8 2222222222222222222222222222222222222222222222222222222222222222", 0);
    // consola("GUARDAR_PAGINA 2 9 3333333333333333333333333333333333333333333333333333333333333333", 0);
    // consola("GUARDAR_PAGINA 2 10 ASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDASDA", 0);

    // consola("GUARDAR_PAGINA 1 11 JEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJE", 0);

    // consola("FINALIZAR_PROCESO 1", 0);
    // consola("GUARDAR_PAGINA 7 4 NINININININININININININININININININININININININININININININININI", 0);
    // consola("GUARDAR_PAGINA 7 5 UWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUWUW", 0);
    // consola("GUARDAR_PAGINA 6 5 GAULESGAULESGAULESGAULESGAULESGAULESGAULESGAULESGAULESGAULESGAUL", 0);
    // consola("GUARDAR_PAGINA 6 4 BOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSKYBOSK", 0);
    // //consola("FINALIZAR_PROCESO 7", 0);
    // consola("GUARDAR_PAGINA 9 5 BOCABOOOOOCABOCABOOOOOCABOCABOOOOOCABOCABOOOOOCABOCABOOOOOCABOOO", 0);
    // consola("GUARDAR_PAGINA 9 4 DALEBODALEBODALEBODALEBODALEBODALEBODALEBODALEBODALEBODALEBOOOOO", 0);

    // t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, "1");
    // t_list_iterator* list_iterator4 = list_iterator_create(tabla_paginas);
    // while (list_iterator_has_next(list_iterator4)) {
    //     fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator4);
    //     if (nodo_actual->proceso == 7) {
    //         log_error(log_file, "nodo_actual->proceso: %d", nodo_actual->proceso);
    //         log_warning(log_file, "nodo_actual->pagina: %d", nodo_actual->pagina);
    //     }
    // }
    // log_info(log_file, "size: %d", list_size(tabla_paginas));
    // list_iterator_destroy(list_iterator4);

    recibir_mensajes();
    cerrar_swamp();
}

void cerrar_swamp() {
    log_info(log_file, "Cerrando Swamp (∪︿∪)...");
    // free_t_mensaje(recibido);
    close(server_socket);
    dictionary_destroy(swap_dict);
    list_destroy_and_destroy_elements(swap_list, nodo_swap_list_destroy);
    config_destroy(config_file);
    log_destroy(log_file);
    exit(EXIT_SUCCESS);
}

void port_fixer() {

    // FIX PUERTO SWAMP

    if (config_get_int_value(config_file, "PUERTO") == 5180) {
        char* puerto = string_from_format("%d", 5181);
        config_set_value(config_file, "PUERTO", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config_file, "PUERTO") == 5181) {
        char* puerto = string_from_format("%d", 5182);
        config_set_value(config_file, "PUERTO", puerto);
        free(puerto);
    }

    else if (config_get_int_value(config_file, "PUERTO") == 5182) {
        char* puerto = string_from_format("%d", 5183);
        config_set_value(config_file, "PUERTO", puerto);
        free(puerto);
    }

    else {
        char* puerto = string_from_format("%d", 5180);
        config_set_value(config_file, "PUERTO", puerto);
        free(puerto);
    }
}