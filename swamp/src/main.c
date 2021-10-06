#include "main.h"

int main(int argc, char ** argv) {
    if (argc > 1 && strcmp(argv[1], "-test") == 0)
    {
        return run_tests();
    }
    
    config_file = config_create(CONFIG_PATH);
    log_file = log_create(LOG_PATH, "[Swamp ᶘ◕ᴥ◕ᶅ]", 1, LOG_LEVEL_INFO);
    swap_file_size = config_get_int_value(config_file, "TAMANIO_SWAP");
    swap_page_size = config_get_int_value(config_file, "TAMANIO_PAGINA");

    signal(SIGINT, cerrar_swamp);

    inicializar_directorios();
    inicializar_swap_files();
    
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