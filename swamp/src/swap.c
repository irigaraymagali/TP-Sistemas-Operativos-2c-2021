#include "swap.h"

void inicializar_directorios() {
    DIR* swap_files_dir = opendir(SWAP_FILES_PATH);

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
    swap_dict = dictionary_create();
    swap_list = list_create();
    char** swap_file_names = config_get_array_value(config_file, "ARCHIVOS_SWAP");

    for (int i = 0; swap_file_names[i] != NULL; i++) {
        char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_names[i]);

        int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777); // Intenta abrir el archivo con permisos de lectura y escritura.
        if (swap_file_fd == -1) {
            swap_file_fd = open(swap_file_path, O_CREAT | O_RDWR, (mode_t)0777); // Si no existe el archivo lo crea con permisos de lectura y escritura.
            
            // Crea el archivo, setea el tamaño segun config file y pone todos sus caracteres en '\0'.
            ftruncate(swap_file_fd, swap_file_size);
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            memset(swap_file_map, '\0', swap_file_size);

            // Crea tabla de paginas invertida para el archivo de swap.
            t_list* tabla_paginas = list_create();
            nodo_swap_list* nodo = malloc(sizeof(nodo_swap_list));
            nodo->swap_file_name = malloc(strlen(swap_file_names[i]) + 1);
            memcpy(nodo->swap_file_name, swap_file_names[i], strlen(swap_file_names[i]));
            nodo->swap_file_name[strlen(swap_file_names[i])] = '\0';
            nodo->tabla_paginas_swap_file = tabla_paginas;
            list_add(swap_list, (void *) nodo);

            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i], " creado correctamente.");
        }

        else {
            // Si el archivo de swap existe se pierde la referencia de que procesos tienen sus paginas dentro de el porque la estructura que maneja esto son
            // un diccionario y una lista que estan en memoria, es decir, desaparece su contenido cuando se reinicia el programa.

            // Crea tabla de paginas invertida para el archivo de swap.
            t_list* tabla_paginas = list_create();
            nodo_swap_list* nodo = malloc(sizeof(nodo_swap_list));
            nodo->swap_file_name = malloc(strlen(swap_file_names[i]) + 1);
            memcpy(nodo->swap_file_name, swap_file_names[i], strlen(swap_file_names[i]));
            nodo->swap_file_name[strlen(swap_file_names[i])] = '\0';
            nodo->tabla_paginas_swap_file = tabla_paginas;
            list_add(swap_list, (void *) nodo);

            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i], " abierto correctamente.");
        }
        close(swap_file_fd);
        free(swap_file_names[i]);
        free(swap_file_path);
    }
    free(swap_file_names);
}

void guardar_pagina(int proceso, int pagina, char* contenido) {
    char* string_proceso = string_itoa(proceso);
    t_list* tabla_paginas_swap_file = (t_list*) dictionary_get(swap_dict, string_proceso); // Devuelve un puntero al t_list que representa a la tabla de paginas del archivo de swap que esta utilizando
    free(string_proceso);
    if (tabla_paginas_swap_file == NULL) { // Si el proceso aun no esta utilizando swap
        if(tipo_asignacion == ASIGNACION_FIJA) {
            
        }

        else if (tipo_asignacion == ASIGNACION_DINAMICA) {
            
        }

        else {
            log_error(log_file, "El tipo de asignacion de frames no fue definido.");
        }
    }

    else { // Si el proceso ya tiene paginas en swap
        if(tipo_asignacion == ASIGNACION_FIJA) {

        }

        else if(tipo_asignacion == ASIGNACION_DINAMICA) {

        }

        else {
            log_error(log_file, "El tipo de asignacion de frames no fue definido.");
        }
    }
}

nodo_swap_list* swap_file_menos_ocupado() {
    int most_free_frames = 0;
    int swap_file_frames_count = swap_file_size / swap_page_size;
    t_list_iterator* list_iterator = list_iterator_create(swap_list);
    nodo_swap_list* resultado;
    while (list_iterator_has_next(list_iterator)) {
        nodo_swap_list* nodo_actual = list_iterator_next(list_iterator);
        int used_frames = list_size(nodo_actual->tabla_paginas_swap_file);
        int swap_file_free_frames = swap_file_frames_count - used_frames;
        if (swap_file_free_frames > most_free_frames) {
            resultado = nodo_actual;
            most_free_frames = swap_file_free_frames;
        }
    }
    list_iterator_destroy(list_iterator);
    // Free/destroy de nodo_swap_list* nodo_actual;

    if (most_free_frames == 0) {
        return NULL;
    }

    else {
        return resultado;
    }
}