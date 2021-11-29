#include "swap.h"

void inicializar_directorios() {
    DIR* swap_files_dir = opendir(SWAP_FILES_PATH);

    if (swap_files_dir == NULL) {
        mkdir(SWAP_FILES_PATH, 0777);
        log_info(log_file, "swapdir creado correctamente.");
    }

    else {
        system("rm -r swapdir");
        mkdir(SWAP_FILES_PATH, 0777);
        log_info(log_file, "swapdir creado correctamente.");
    }
    closedir(swap_files_dir);
}

void inicializar_swap_files() {
    swap_dict = dictionary_create();
    swap_list = list_create();
    char** swap_file_names = config_get_array_value(config_file, "ARCHIVOS_SWAMP");

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
            log_error(log_file, "No deberia haber entrado a este else.");
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

int guardar_pagina(int proceso, int pagina, void* contenido) {
    if (tipo_asignacion == ASIGNACION_FIJA) {
        return guardar_pagina_asignacion_fija(proceso, pagina, contenido);
    }

    else if (tipo_asignacion == ASIGNACION_DINAMICA) {
        return guardar_pagina_asignacion_dinamica(proceso, pagina, contenido);
    }

    else {
        log_error(log_file, "El tipo de asignacion de frames no fue definido.");
        return 0;
    }
}

int guardar_pagina_asignacion_fija(int proceso, int pagina, void* contenido) {
    char* string_proceso = string_itoa(proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso); // Devuelve un puntero al t_list que representa a la tabla de paginas del archivo de swap que esta utilizando
    if (tabla_paginas == NULL) { // Si es la primera pagina del proceso que se va a guardar en swap
        nodo_swap_list* swap_file_asignado = swap_file_menos_ocupado();
        int swap_file_frames_count;
        int swap_file_used_frames;
        if (swap_file_asignado != NULL) {
            swap_file_frames_count = swap_file_size / swap_page_size;
            swap_file_used_frames = tabla_paginas_size(swap_file_asignado->tabla_paginas);
        }

        if (swap_file_asignado == NULL) { // Si no hay frames disponibles en ningun archivo de swap
            log_error(log_file, "No hay espacio disponible en swap.");
            return 0;
        }
        
        else if (marcos_por_carpincho > swap_file_frames_count - swap_file_used_frames) { // Si el archivo de swap con mas frames disponibles no tiene suficientes para satisfacer los marcos por carpincho
            log_error(log_file, "No hay espacio disponible en swap.");
            return 0;
        }

        else { // Si hay un archivo de swap que tenga suficientes frames disponibles para satisfacer los marcos por carpincho
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
            dictionary_put(swap_dict, string_proceso, (void*) swap_file_asignado->tabla_paginas);
            int frame_asignado = get_first_free_frame_number(nuevo, swap_file_map);

            // Guardo la pagina recibida en el archivo de swap
            memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
            munmap(swap_file_map, swap_file_size);
            close(swap_file_fd);
            free(swap_file_path);

            // list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
            //
            // // Reservo los frames contiguos como pide el enunciado
            // for (int i = 0; i < marcos_por_carpincho - 1; i++) {
            //     fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
            //     nuevo->proceso = proceso;
            //     nuevo->pagina = 9999;
            //     list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
            // }

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////// RECONTRA BETA VERSION HAY QUE PROBARLO ///////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            if (list_size(swap_file_asignado->tabla_paginas) - 1 > frame_asignado) { // Si el frame que se le asigno habia sido utilizado anteriormente por un proceso que fue finalizado.
                // Cambio el numero del proceso eliminado (9999) por el del nuevo proceso en la tabla de paginas.
                list_replace_and_destroy_element(swap_file_asignado->tabla_paginas, frame_asignado, (void*) nuevo, fila_tabla_paginas_destroy);

                // Hago lo mismo con el resto de los frames pertenecientes al proceso eliminado para reservar los frames contiguos como pide el enunciado.
                for (int i = 0; i < marcos_por_carpincho - 1; i++) {
                fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
                nuevo->proceso = proceso;
                nuevo->pagina = 9999;
                // frame_asignado = get_first_free_frame_number(nuevo, swap_file_map);
                frame_asignado++;
                list_replace_and_destroy_element(swap_file_asignado->tabla_paginas, frame_asignado, (void*) nuevo, fila_tabla_paginas_destroy);
                }
            }

            else { // Si el frame asignado no habia sido utilizado anteriormente por ningun otro proceso.
                list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);

                // Reservo los frames contiguos como pide el enunciado
                for (int i = 0; i < marcos_por_carpincho - 1; i++) {
                    fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
                    nuevo->proceso = proceso;
                    nuevo->pagina = 9999;
                    list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
                }
            }

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////// RECONTRA BETA VERSION HAY QUE PROBARLO ///////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            return 1;
        }
    }

    else { // Si el proceso tiene paginas en swap
        if (pagina_esta_en_swap(tabla_paginas, proceso, pagina)) { // Si tengo que sobreescribir una pagina que ya esta en swap
            // Guardo la pagina recibida en el archivo de swap
            char* swap_file_name = get_swap_file_name(tabla_paginas);
            char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
            int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
            if (swap_file_fd == -1) {
                log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
            }
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
            nuevo->proceso = proceso;
            nuevo->pagina = pagina;
            int frame_asignado = get_frame_number(nuevo);
            memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
            munmap(swap_file_map, swap_file_size);
            close(swap_file_fd);
            free(swap_file_path);
            free(nuevo);

            return 1;
        }

        else { // Si tengo que guardar una pagina nueva
            char* swap_file_name = get_swap_file_name(tabla_paginas);
            char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
            int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
            if (swap_file_fd == -1) {
                log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
            }
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
            nuevo->proceso = proceso;
            nuevo->pagina = 9999;
            int frame_asignado = get_frame_number(nuevo);
            if (frame_asignado == tabla_paginas_size(tabla_paginas)) {
                log_error(log_file, "El proceso %d no posee frames libres en el archivo %s.", proceso, swap_file_name);
                munmap(swap_file_map, swap_file_size);
                close(swap_file_fd);
                free(swap_file_path);
                free(nuevo);
                return 0;
            }
            else {
                memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
                fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
                nuevo->proceso = proceso;
                nuevo->pagina = pagina;
                list_replace_and_destroy_element(tabla_paginas, frame_asignado, (void*) nuevo, fila_tabla_paginas_destroy);
                munmap(swap_file_map, swap_file_size);
                close(swap_file_fd);
                free(swap_file_path);
                free(nuevo);
                return 1;
            }
        }
    free(string_proceso);
    }
}


int guardar_pagina_asignacion_dinamica(int proceso, int pagina, void* contenido) {
    char* string_proceso = string_itoa(proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso); // Devuelve un puntero al t_list que representa a la tabla de paginas del archivo de swap que esta utilizando
    free(string_proceso);
    if (tabla_paginas == NULL) { // Si es la primera pagina del proceso que se va a guardar en swap
        nodo_swap_list* swap_file_asignado = swap_file_menos_ocupado();
        if (swap_file_asignado == NULL) { // Si no hay frames disponibles en ningun archivo de swap
            log_error(log_file, "No hay espacio disponible en swap.");
            return 0;
        }

        else { // Si hay un archivo de swap que tenga al menos un marco disponible
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
            dictionary_put(swap_dict, string_proceso, (void*) swap_file_asignado->tabla_paginas);
            int frame_asignado = get_first_free_frame_number(nuevo, swap_file_map);
            // list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////// RECONTRA BETA VERSION HAY QUE PROBARLO ///////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            if (tabla_paginas_size(swap_file_asignado->tabla_paginas) - 1 > frame_asignado) { // Si el frame que se le asigno habia sido utilizado anteriormente por un proceso que fue finalizado.
                // Cambio el numero del proceso eliminado (9999) por el del nuevo proceso en la tabla de paginas.
                list_replace_and_destroy_element(swap_file_asignado->tabla_paginas, frame_asignado, (void*) nuevo, fila_tabla_paginas_destroy);
            }

            else { // Si el frame asignado no habia sido utilizado anteriormente por ningun otro proceso.
                list_add(swap_file_asignado->tabla_paginas, (void*) nuevo);
            }

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ////////////////////////////// RECONTRA BETA VERSION HAY QUE PROBARLO ///////////////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            // Guardo la pagina recibida en el archivo de swap
            memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
            munmap(swap_file_map, swap_file_size);
            close(swap_file_fd);
            free(swap_file_path);

            return 1;
        }
    }

    else { // Si el proceso tiene paginas en swap
        if (pagina_esta_en_swap(tabla_paginas, proceso, pagina)) { // Si tengo que sobreescribir una pagina que ya esta en swap
            // Guardo la pagina recibida en el archivo de swap
            char* swap_file_name = get_swap_file_name(tabla_paginas);
            char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
            int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
            if (swap_file_fd == -1) {
                log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
            }
            void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
            fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
            nuevo->proceso = proceso;
            nuevo->pagina = pagina;
            int frame_asignado = get_frame_number(nuevo);
            memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
            munmap(swap_file_map, swap_file_size);
            close(swap_file_fd);
            free(swap_file_path);
            free(nuevo);

            return 1;
        }

        else { // Si tengo que guardar una pagina nueva
            if (tabla_paginas_size(tabla_paginas) == swap_file_size / swap_page_size) { // Si todos los frames estan ocupados
                log_error(log_file, "No hay frames disponibles en el archivo %s correspondiente al proceso %d.", get_swap_file_name(tabla_paginas), proceso);
                return 0;
            }

            else if (tabla_paginas_size(tabla_paginas) > swap_file_size / swap_page_size) {
                log_error(log_file, "Se rompio todo.");
                return 0;
            }

            else { // Si quedan frames libres en el archivo de swap utilizado por el proceso que quiere guardar una pagina
                char* swap_file_name = get_swap_file_name(tabla_paginas);
                char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
                int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
                if (swap_file_fd == -1) {
                    log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
                }
                void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
                fila_tabla_paginas* nuevo = malloc(sizeof(fila_tabla_paginas));
                nuevo->proceso = proceso;
                nuevo->pagina = pagina;
                list_add(tabla_paginas, (void*) nuevo);
                int frame_asignado = get_first_free_frame_number(nuevo, swap_file_map);

                // Guardo la pagina recibida en el archivo de swap
                memcpy(swap_file_map + swap_page_size * frame_asignado, contenido, swap_page_size);
                munmap(swap_file_map, swap_file_size);
                close(swap_file_fd);
                free(swap_file_path);

                return 1;
            }
        }
    }
}

void* obtener_pagina(int proceso, int pagina) {
    // char* contenido = malloc(swap_page_size + 1);
    void* contenido = malloc(swap_page_size);
    char* string_proceso = string_itoa(proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso); // Devuelve un puntero al t_list que representa a la tabla de paginas del archivo de swap que esta utilizando
    if (tabla_paginas == NULL) {
        log_error(log_file, "El proceso %d no se encuentra utilizando swap.", proceso);
        free(contenido);
    }

    else {
        char* swap_file_name = get_swap_file_name(tabla_paginas);
        char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
        int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
        if (swap_file_fd == -1) {
            log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
        }
        void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
        fila_tabla_paginas* aux = malloc(sizeof(fila_tabla_paginas));
        aux->proceso = proceso;
        aux->pagina = pagina;
        int frame_asignado = get_frame_number(aux);
        memcpy(contenido, swap_file_map + swap_page_size * frame_asignado, swap_page_size);
        //contenido[swap_page_size] = '\0';
        free(aux);
        free(swap_file_path);
    }
    free(string_proceso);

    return contenido;
}

void finalizar_proceso(int proceso) {
    char* string_proceso = string_itoa(proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso);
    if (tabla_paginas != NULL) {
        // Abro el archivo de swap y lo mapeo para eliminar las paginas guardadas.
        char* swap_file_name = get_swap_file_name(tabla_paginas);
        char* swap_file_path = string_from_format("%s%s", SWAP_FILES_PATH, swap_file_name);
        int swap_file_fd = open(swap_file_path, O_RDWR, (mode_t)0777);
        if (swap_file_fd == -1) {
            log_error(log_file, "Error al abrir el archivo %s", swap_file_name);
        }
        void* swap_file_map = mmap(NULL, swap_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, swap_file_fd, 0);
        char* vacio = malloc(swap_page_size);
        memset((void*) vacio, '\0', swap_page_size);
        t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
        while (list_iterator_has_next(list_iterator)) {
            fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator);
            if (nodo_actual->proceso == proceso) {
                int frame_asignado = get_frame_number(nodo_actual);
                memcpy(swap_file_map + swap_page_size * frame_asignado, vacio, swap_page_size);
                nodo_actual->proceso = 9999;
                nodo_actual->pagina = 9999;
            }
        }
        dictionary_remove(swap_dict, string_proceso);
        free(vacio);
        list_iterator_destroy(list_iterator);
        free(swap_file_path);
        munmap(swap_file_map, swap_file_size);
        close(swap_file_fd);
        log_info(log_file, "El proceso %d fue eliminado de swap.", proceso);
    }
    else {
    log_info(log_file, "El proceso %d no se encontraba utilizando swap.", proceso);
    }
    free(string_proceso);
}

nodo_swap_list* swap_file_menos_ocupado() {
    int most_free_frames = 0;
    int swap_file_frames_count = swap_file_size / swap_page_size;
    t_list_iterator* list_iterator = list_iterator_create(swap_list);
    nodo_swap_list* resultado;
    while (list_iterator_has_next(list_iterator)) {
        nodo_swap_list* nodo_actual = list_iterator_next(list_iterator);
        int used_frames = tabla_paginas_size(nodo_actual->tabla_paginas);
        int swap_file_free_frames = swap_file_frames_count - used_frames;
        if (swap_file_free_frames > most_free_frames) {
            resultado = nodo_actual;
            most_free_frames = swap_file_free_frames;
        }
    }
    list_iterator_destroy(list_iterator);

    if (most_free_frames == 0) {
        return NULL;
    }

    else {
        return resultado;
    }
}

int frames_ocupados(t_list* tabla_paginas) {
    t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
    int i = 0;
    while (list_iterator_has_next(list_iterator)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->pagina != 9999) {
            i++;
        }
    }
    list_iterator_destroy(list_iterator);

    return i;
}

int get_first_free_frame_number(fila_tabla_paginas* nodo, void* swap_file_map) {
    char* string_proceso = string_itoa(nodo->proceso);
    t_list* tabla_paginas = (t_list*) dictionary_get(swap_dict, string_proceso);
    t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
    int i = 0;
    while (list_iterator_has_next(list_iterator)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->proceso == 9999) {
            break;
        }

        else if (nodo_actual->proceso != nodo->proceso) {
            i++;
        }

        else {
            if (!frame_is_empty(i, swap_file_map)) {
                i++;
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

int get_frame_number(fila_tabla_paginas* nodo) {
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
            if (nodo_actual->pagina != nodo->pagina) {
                i++;
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

bool pagina_esta_en_swap(t_list* tabla_paginas, int proceso, int pagina) {
    t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
    int i = false;
    while (list_iterator_has_next(list_iterator)) {
        fila_tabla_paginas* nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->proceso == proceso && nodo_actual->pagina == pagina) {
            i = true;
            break;
        }
    }
    list_iterator_destroy(list_iterator);

    return i;
}

char* get_swap_file_name(t_list* tabla_paginas) {
    t_list_iterator* list_iterator = list_iterator_create(swap_list);
    nodo_swap_list* nodo_actual;
    while (list_iterator_has_next(list_iterator)) {
        nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->tabla_paginas == tabla_paginas) {
            break;
        }
    }
    list_iterator_destroy(list_iterator);

    return nodo_actual->swap_file_name;
}

int tabla_paginas_size(t_list* tabla_paginas) {
    t_list_iterator* list_iterator = list_iterator_create(tabla_paginas);
    fila_tabla_paginas* nodo_actual;
    int size = 0;
    while (list_iterator_has_next(list_iterator)) {
        nodo_actual = list_iterator_next(list_iterator);
        if (nodo_actual->proceso != 9999) {
            size++;
        }
    }
    list_iterator_destroy(list_iterator);

    return size;
}

void fila_tabla_paginas_destroy(void* fila) {
    fila_tabla_paginas* aux = (fila_tabla_paginas*) fila;
    if (aux != NULL){
        free(aux);
    }
}

void nodo_swap_list_destroy(void* nodo) {
    nodo_swap_list* aux = (nodo_swap_list*) nodo;
    list_destroy_and_destroy_elements(aux->tabla_paginas, fila_tabla_paginas_destroy);
    if (aux != NULL){
        free(aux->swap_file_name);
        free(aux);
    }
}
