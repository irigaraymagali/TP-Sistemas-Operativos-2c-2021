#include "consola.h"

void consola(char* buffer, int socket_conexion) {
    char** parametros = string_split(buffer, " ");
    int cantidad_de_parametros = contar_parametros(parametros);

    if (string_starts_with(buffer, "TIPO_ASIGNACION")) { // Ejemplo: TIPO_ASIGNACION ASIGNACION_FIJA
        if (cantidad_de_parametros == 2) {
            if (string_equals_ignore_case(parametros[1], "ASIGNACION_FIJA")) {
                tipo_asignacion = ASIGNACION_FIJA;
            }

            else if (string_equals_ignore_case(parametros[1], "ASIGNACION_DINAMICA")) {
                tipo_asignacion = ASIGNACION_DINAMICA;
            }

            else {
                log_error(log_file, "Tipo de asignacion invalido.");
            }
        }

        else {
            log_error(log_file, "Cantidad de parametros incorrecta.");
        }
    }

    else if (string_starts_with(buffer, "GUARDAR_PAGINA")) { // Ejemplo: GUARDAR_PAGINA PROCESO NUMERO_PAGINA CONTENIDO
        if (cantidad_de_parametros == 4) {
            // Funcion que guarda la pagina en un archivo de swap y actualiza las estructuras administrativas para guardar su referencia
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

    else if (string_starts_with(buffer, "SALIR") || string_starts_with(buffer, "EXIT")) { // Ejemplo: SALIR || EXIT
        cerrar_swamp();
    }

    else {
        log_error(log_file, "El comando ingresado no existe.");
    }

    for (int i = 0; parametros[i] != NULL; i++) {
        free(parametros[i]);
    }
    free(parametros);
}

int contar_parametros(char **parametros) {
    int i = 0;
    while (parametros[i] != NULL) {
        i++;
    }
    return i;
}