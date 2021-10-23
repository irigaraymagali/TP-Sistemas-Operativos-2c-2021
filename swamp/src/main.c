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
    consola("GUARDAR_PAGINA 1 2 HOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLACHAU", 0);
    consola("GUARDAR_PAGINA 3 3 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);
    consola("GUARDAR_PAGINA 4 3 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);

    // nodo_swap_list* nodo_swap1 = list_get(swap_list, 0);
    // fila_tabla_paginas* estruc1 = malloc(sizeof(fila_tabla_paginas));
    // estruc1->proceso = 1;
    // estruc1->pagina = 0;
    // list_add(nodo_swap1->tabla_paginas, estruc1);
    // fila_tabla_paginas* estruc2 = malloc(sizeof(fila_tabla_paginas));
    // estruc2->proceso = 2;
    // estruc2->pagina = 12;
    // list_add(nodo_swap1->tabla_paginas, estruc2);
    // fila_tabla_paginas* estruc3 = malloc(sizeof(fila_tabla_paginas));
    // estruc3->proceso = 3;
    // estruc3->pagina = 16;
    // list_add(nodo_swap1->tabla_paginas, estruc3);
    // int frame = get_frame_number(estruc3);
    // log_error(log_file, "Si el numero que sigue no es 2 esta mal -> %d", frame);

    // fila_tabla_paginas* estruc1 = malloc(sizeof(fila_tabla_paginas));
    // estruc1->proceso = 1;
    // estruc1->pagina = 0;
    // nodo_swap_list* nodo_swap1 = list_get(swap_list, 0);
    // list_add(nodo_swap1->tabla_paginas, estruc1);
    // fila_tabla_paginas* estruc2 = malloc(sizeof(fila_tabla_paginas));
    // estruc2->proceso = 2;
    // estruc2->pagina = 12;
    // list_add(nodo_swap1->tabla_paginas, estruc2);
    // fila_tabla_paginas* estruc3 = malloc(sizeof(fila_tabla_paginas));
    // estruc3->proceso = 3;
    // estruc3->pagina = 16;
    // nodo_swap_list* nodo_swap2 = list_get(swap_list, 1);
    // list_add(nodo_swap2->tabla_paginas, estruc3);
    // // fila_tabla_paginas* respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 0);
    // // log_warning(log_file, "LEIDO");
    // // log_warning(log_file, "%d", respuesta->proceso);
    // // log_warning(log_file, "%d", respuesta->pagina);
    // // respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 1);
    // // log_warning(log_file, "LEIDO");
    // // log_warning(log_file, "%d", respuesta->proceso);
    // // log_warning(log_file, "%d", respuesta->pagina);
    // // respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 2);
    // // log_warning(log_file, "LEIDO");
    // // log_warning(log_file, "%d", respuesta->proceso);
    // // log_warning(log_file, "%d", respuesta->pagina);
    // nodo_swap_list* respuesta = swap_file_menos_ocupado();
    // log_error(log_file, "%s", respuesta->swap_file_name);

    recibir_mensajes();
    cerrar_swamp();
}

void cerrar_swamp() {
    log_info(log_file, "Cerrando Swamp (∪︿∪)...");
    close(server_socket);
    config_destroy(config_file);
    log_destroy(log_file);
    exit(EXIT_SUCCESS);
}