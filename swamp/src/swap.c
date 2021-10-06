#include "swap.h"

void inicializar_directorios() {
    DIR *swap_files_dir = opendir(SWAP_FILES_PATH);
    if (swap_files_dir == NULL) {
        mkdir(SWAP_FILES_PATH, 0777);
        log_info(log_file, "Directorio Swap Files creado correctamente.");
    }

    else {
        log_info(log_file, "Directorio Swap Files abierto correctamente.");
    }

    closedir(swap_files_dir);
}

void inicializar_swap_files() {
    char **swap_file_names = config_get_array_value(config_file, "ARCHIVOS_SWAP");
    for (int i = 0; swap_file_names[i] != NULL; i++) {
        char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_names[i]);
        int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777); // Intenta abrir el archivo con permisos de lectura y escritura.
        if (swap_file_fd == -1) {
            swap_file_fd = open(swap_file_path, O_CREAT | O_RDWR, (mode_t)0777); // Si no existe el archivo lo crea con permisos de lectura y escritura.
            ftruncate(swap_file_fd, swap_file_size);
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            memset(swap_file_map, '\0', swap_file_size);
            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i]," creado correctamente.");
        }
        else {
            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i]," abierto correctamente.");
        }
        close(swap_file_fd);
        free(swap_file_names[i]);
        free(swap_file_path);
    }
    free(swap_file_names);
}