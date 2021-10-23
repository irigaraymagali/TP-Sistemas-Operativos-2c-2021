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
            munmap(swap_file_map, swap_file_size);

            // Crea tabla de paginas invertida para el archivo de swap.
            t_list* tabla_paginas = list_create();
            nodo_swap_list* nodo = malloc(sizeof(nodo_swap_list));
            nodo->swap_file_name = malloc(strlen(swap_file_names[i]) + 1);
            memcpy(nodo->swap_file_name, swap_file_names[i], strlen(swap_file_names[i]));
            nodo->swap_file_name[strlen(swap_file_names[i])] = '\0';
            nodo->tabla_paginas = tabla_paginas;
            list_add(swap_list, (void *) nodo);

            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i], " creado correctamente.");
        }

        else { // Si existe el archivo lo setea como si recien se creara.
            ftruncate(swap_file_fd, swap_file_size);
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            memset(swap_file_map, '\0', swap_file_size);
            munmap(swap_file_map, swap_file_size);

            // Crea tabla de paginas invertida para el archivo de swap.
            t_list* tabla_paginas = list_create();
            nodo_swap_list* nodo = malloc(sizeof(nodo_swap_list));
            nodo->swap_file_name = malloc(strlen(swap_file_names[i]) + 1);
            memcpy(nodo->swap_file_name, swap_file_names[i], strlen(swap_file_names[i]));
            nodo->swap_file_name[strlen(swap_file_names[i])] = '\0';
            nodo->tabla_paginas = tabla_paginas;
            list_add(swap_list, (void *) nodo);

            log_info(log_file, "%s%s%s", "Archivo ", swap_file_names[i], " creado correctamente.");
        }
        close(swap_file_fd);
        free(swap_file_names[i]);
        free(swap_file_path);
    }
    free(swap_file_names);
}

void guardar_pagina(int proceso, int pagina, char* contenido) {
    nodo_swap_list* swap_file_asignado = swap_file_menos_ocupado();
    if (swap_file_asignado == NULL) {
        log_error(log_file, "No hay espacio disponible en swap.");
    }

    else {
        char* string_proceso = string_itoa(proceso);
        t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso); // Devuelve un puntero al t_list que representa a la tabla de paginas del archivo de swap que esta utilizando

        if (tabla_paginas == NULL) { // Si es la primera pagina del proceso que se va a guardar en swap
            char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_asignado->swap_file_name);
            int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
            if (swap_file_fd == -1) {
                log_error(log_file, "Error al abrir el archivo %s", swap_file_asignado->swap_file_name);
            }
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);

            // Actualizo estructuras administrativas
            fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
            nuevo->proceso = proceso;
            nuevo->pagina = pagina;
            list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
            dictionary_put(swap_dict, string_proceso, (void*) swap_file_asignado->tabla_paginas);
            int frame_asignado = get_frame_number(nuevo, swap_file_map);
            
            if (tipo_asignacion == ASIGNACION_FIJA) {
                // Reservo los frames contiguos como pide el enunciado
                for (int i = 0; i < marcos_por_carpincho - 1; i++) {
                    fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
                    nuevo->proceso = proceso;
                    nuevo->pagina = 999999;
                    list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
                }

                // Guardo la pagina recibida en el archivo de swap
                memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
                munmap(swap_file_map, swap_file_size);
                close(swap_file_fd);
                free(swap_file_path);
            }

            else if (tipo_asignacion == ASIGNACION_DINAMICA) {
                // Guardo la pagina recibida en el archivo de swap
                memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
                munmap(swap_file_map, swap_file_size);
                close(swap_file_fd);
                free(swap_file_path);
            }

            else {
                log_error(log_file, "El tipo de asignacion de frames no fue definido.");
            }
        }

        else { // Si el proceso tiene otras paginas en swap
            if(tipo_asignacion == ASIGNACION_FIJA) {
                
            }

            else if(tipo_asignacion == ASIGNACION_DINAMICA) {

            }

            else {
                log_error(log_file, "El tipo de asignacion de frames no fue definido.");
            }
        }
    free(string_proceso);
    }
}

nodo_swap_list* swap_file_menos_ocupado() {
    int most_free_frames = 0;
    int swap_file_frames_count = swap_file_size / swap_page_size;
    t_list_iterator* list_iterator = list_iterator_create(swap_list);
    nodo_swap_list* resultado;
    while (list_iterator_has_next(list_iterator)) {
        nodo_swap_list* nodo_actual = list_iterator_next(list_iterator);
        int used_frames = list_size(nodo_actual->tabla_paginas);
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


int get_frame_number(fila_tabla_paginas* nodo, void* swap_file_map) {
    char* string_proceso = string_itoa(nodo->proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso);
    t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
    int i = 0;
    while (list_iterator_has_next(list_iterator)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->proceso != nodo->proceso) {
            i++;
        }

        else {
            if (!frame_is_empty(i, swap_file_map)) {
                i ++;
            }

            else {
                break;
            }
        }
    }
    free(string_proceso);
    list_iterator_destroy(list_iterator);

    return i;
}

bool frame_is_empty(int frame, void* swap_file_map) {
    char* leido = malloc(swap_page_size + 1);
    memcpy(leido, swap_file_map + frame * swap_page_size, swap_page_size);
    leido[swap_page_size] = '\0';

    char* vacio = malloc(swap_page_size + 1);
    memset((void*) vacio, '\0', swap_page_size);
    vacio[swap_page_size] = '\0';

    int resultado = strcmp(leido, vacio);

    free(vacio);
    free(leido);

    if (!resultado) {
        return 1;
    }

    else {
        return 0;
    }
}