#include "consola.h"

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

void consola(char* buffer, int socket_conexion) {
    char** parametros = string_split(buffer, " ");
    int cantidad_de_parametros = contar_parametros(parametros);

    if (string_starts_with(buffer, "TIPO_ASIGNACION")) { // Ejemplo: TIPO_ASIGNACION ASIGNACION_FIJA
        if (cantidad_de_parametros == 2) {
            if (string_equals_ignore_case(parametros[1], "ASIGNACION_FIJA")) {
                tipo_asignacion = ASIGNACION_FIJA;
                log_info(log_file, "La asignacion fija fue establecida.");
            }

            else if (string_equals_ignore_case(parametros[1], "ASIGNACION_DINAMICA")) {
                tipo_asignacion = ASIGNACION_DINAMICA;
                log_info(log_file, "La asignacion dinamica fue establecida.");
            }

            else {
                log_error(log_file, "Tipo de asignacion invalido.");
            }
        }

        else {
            log_error(log_file, "Cantidad de parametros incorrecta.");
        }
    }

    else if (string_starts_with(buffer, "GUARDAR_PAGINA")) { // Ejemplo: GUARDAR_PAGINA PROCESO PAGINA CONTENIDO
        if (cantidad_de_parametros == 4) {
            guardar_pagina(atoi(parametros[1]), atoi(parametros[2]), parametros [3]);
        }

        else {
            log_error(log_file, "Cantidad de parametros incorrecta.");
        }
    }

    else if (string_starts_with(buffer, "OBTENER_PAGINA")) { // Ejemplo: OBTENER_PAGINA PROCESO NUMERO_PAGINA
        if (cantidad_de_parametros == 3) {
            // char* pagina = Funcion que devuelve el contenido de la pagina especificada en los parametros del mensaje

            // if (send(socket_conexion, pagina, strlen(pagina) + 1, 0) < 0) {
            //         log_error(log_file, "Error al enviar la pagina %s del proceso %s.", parametros[2], parametros[1]);
            // }
            // free(pagina);
        }

        else {
            log_error(log_file, "Cantidad de parametros incorrecta.");
        }
    }

    else if (string_starts_with(buffer, "FINALIZAR_PROCESO")) { // Ejemplo: FINALIZAR_PROCESO PROCESO
        if (cantidad_de_parametros == 2) {
            // Funcion que elimina de swap todas las paginas del proceso especificado en los parametros (reemplaza su contenido por '\0')
            // y ademas elimina su informacion de las estructuras administrativas
        }

        else {
            log_error(log_file, "Cantidad de parametros incorrecta.");
        }
    }

    else if (string_starts_with(buffer, "SALIR") || string_starts_with(buffer, "EXIT")) { // Ejemplo: SALIR || EXIT
        cerrar_swamp();
    }

    else {
        log_error(log_file, "El comando ingresado no existe.");
    }

    // Libera memoria
    for (int i = 0; parametros[i] != NULL; i++) {
        free(parametros[i]);
    }
    free(parametros);
}

int contar_parametros(char** parametros) {
    int i = 0;
    while (parametros[i] != NULL) {
        i++;
    }
    return i;
}