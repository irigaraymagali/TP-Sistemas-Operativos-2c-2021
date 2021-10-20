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

    inicializar_directorios();
    inicializar_swap_files();

    t_list* tabla_paginas = list_create();
    fila_tabla_paginas* estruc1 = malloc(sizeof(fila_tabla_paginas));
    estruc1->proceso = "Proceso A";
    estruc1->pagina = 0;
    list_add(tabla_paginas, estruc1);
    fila_tabla_paginas* estruc2 = malloc(sizeof(fila_tabla_paginas));
    estruc2->proceso = "Proceso B";
    estruc2->pagina = 12;
    list_add(tabla_paginas, estruc2);
    fila_tabla_paginas* estruc3 = malloc(sizeof(fila_tabla_paginas));
    estruc3->proceso = "Proceso C";
    estruc3->pagina = 16;
    list_add(tabla_paginas, estruc3);
    fila_tabla_paginas* respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 0);
    log_warning(log_file, "LEIDO");
    log_warning(log_file, "%s", respuesta->proceso);
    log_warning(log_file, "%d", respuesta->pagina);
    respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 1);
    log_warning(log_file, "LEIDO");
    log_warning(log_file, "%s", respuesta->proceso);
    log_warning(log_file, "%d", respuesta->pagina);
    respuesta = (fila_tabla_paginas*) list_get(tabla_paginas, 2);
    log_warning(log_file, "LEIDO");
    log_warning(log_file, "%s", respuesta->proceso);
    log_warning(log_file, "%d", respuesta->pagina);

    recibir_mensajes();
    cerrar_swamp();
}

void recibir_mensajes() {
    server_socket = _create_socket_listenner(config_get_string_value(config_file, "PUERTO"), log_file);
    int client_socket = _listen(server_socket, 1, log_file);
    int len_mensaje;

    log_info(log_file, "Conexion exitosa. Esperando mensajes...");

    while ((len_mensaje = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[len_mensaje - 1] = '\0';
        log_info(log_file, "Mensaje recibido: %s", buffer);
        consola(buffer, client_socket);
    }

    if (len_mensaje <= 0)
    {
        log_warning(log_file, "Conexion finalizada con el socket %d", client_socket);
    }
}

void cerrar_swamp() {
    log_info(log_file, "Cerrando Swamp (∪︿∪)...");
    close(server_socket);
    config_destroy(config_file);
    log_destroy(log_file);
    exit(EXIT_SUCCESS);
}