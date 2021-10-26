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

    // consola("TIPO_ASIGNACION ASIGNACION_FIJA", 0);
    // consola("GUARDAR_PAGINA 1 2 HOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLACHAU", 0);
    // consola("GUARDAR_PAGINA 3 3 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);
    // consola("GUARDAR_PAGINA 4 3 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);

    consola("TIPO_ASIGNACION ASIGNACION_FIJA", 0);
    consola("GUARDAR_PAGINA 1 2 HOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLAHOLACHAU", 0);
    consola("GUARDAR_PAGINA 1 2 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);
    consola("GUARDAR_PAGINA 4 3 BOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCABOCA", 0);
    consola("GUARDAR_PAGINA 5 3 JEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJEJE", 0);

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