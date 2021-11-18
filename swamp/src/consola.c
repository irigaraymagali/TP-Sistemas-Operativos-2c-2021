#include "consola.h"

void recibir_mensajes() {
    server_socket = _create_socket_listenner(config_get_string_value(config_file, "PUERTO"), log_file);
    int client_socket = _listen(server_socket, 1, log_file);

    log_info(log_file, "Conexion exitosa. Esperando mensajes...");

    // int len_mensaje;
    // while ((len_mensaje = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
    //     buffer[len_mensaje - 1] = '\0';
    //     log_info(log_file, "Mensaje recibido: %s", buffer);
    //     consola(buffer, client_socket);
    // }

    // if (len_mensaje <= 0) {
    //     log_warning(log_file, "Conexion finalizada con el socket %d", client_socket);
    // }

    recibido = _receive_message(client_socket, log_file);
    while (recibido != NULL) {
        consola(recibido, client_socket);
        free_t_mensaje(recibido);
        recibido = _receive_message(client_socket, log_file);
    }

    if (recibido == NULL) {
        log_warning(log_file, "Conexion finalizada con el socket %d", client_socket);
        cerrar_swamp();
    }
}

void consola(t_mensaje* recibido, int socket_conexion) {
    if (recibido->command == TIPO_ASIGNACION) { // Ejemplo: TIPO_ASIGNACION ASIGNACION_FIJA
        if (recibido->pay_len == sizeof(int)) {
            memcpy(&tipo_asignacion, recibido->payload, sizeof(int));

            if (tipo_asignacion == ASIGNACION_FIJA) {
                log_info(log_file, "La asignacion fija fue establecida.");
            }

            else if (tipo_asignacion == ASIGNACION_DINAMICA) {
                log_info(log_file, "La asignacion dinamica fue establecida.");
            }

            else {
                log_error(log_file, "Tipo de asignacion invalido.");
            }
        }

        else {
            log_error(log_file, "TIPO_ASIGNACION Cantidad de parametros incorrecta.");
        }
    }

    else if (recibido->command == MEMORY_SEND_SWAP_RECV) { // Ejemplo: GUARDAR_PAGINA PROCESO NUMERO_PAGINA CONTENIDO
        if (recibido->pay_len == 3 * sizeof(int) + swap_page_size) {
            int offset = 0;
            int proceso, pagina;
            void* contenido = malloc(swap_page_size);
            memcpy(&proceso, recibido->payload + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&pagina, recibido->payload + offset, sizeof(int));
            offset += 2 * sizeof(int); // Lo avanzo dos veces ya que el proximo es el tamaño del void* que tiene el contenido de la pagina y no me interesa porque se que es igual al swap_page_size.
            memcpy(contenido, recibido->payload + offset, swap_page_size);

            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ///////////////////// VER SI ESTA BIEN EL CASTEO O HAY QUE CAMBIAR TODO A VOID //////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////
            guardar_pagina(proceso, pagina, (char*) contenido);
            //////////////////////////////////////////////////////////////////////////////////////////////////////
            ///////////////////// VER SI ESTA BIEN EL CASTEO O HAY QUE CAMBIAR TODO A VOID //////////////////////
            ////////////////////////////////////////////////////////////////////////////////////////////////////
        }

        else {
            log_error(log_file, "MEMORY_SEND_SWAP_RECV Cantidad de parametros incorrecta.");
        }
    }

    else if (recibido->command == MEMORY_RECV_SWAP_SEND) { // Ejemplo: OBTENER_PAGINA PROCESO NUMERO_PAGINA
        if (recibido->pay_len == 2 * sizeof(int)) {
            int offset = 0;
            int proceso, pagina;
            memcpy(&proceso, recibido->payload + offset, sizeof(int));
            offset += sizeof(int);
            memcpy(&pagina, recibido->payload + offset, sizeof(int));

            void* pagina_leida =  obtener_pagina(proceso, pagina);
            if (pagina_leida != NULL) {
                void* pagina_a_enviar = _serialize(swap_page_size, "%v", pagina_leida);
                if (_send_message(socket_conexion, "SWP", MEMORY_RECV_SWAP_SEND, pagina_a_enviar, swap_page_size, log_file)) { // Estoy mandando n bytes(n = tamaño pagina + 2) ya que obtener_pagina devuelve la pagina con el '\0' al final y en el send reservo otro byte mas para el mismo caracter. Anoto esto por las dudas de que se lea/escriba basura en un futuro.
                    log_info(log_file, "Pagina %d del proceso %d enviada.", pagina, proceso);
                }

                else {
                    log_error(log_file, "Error al enviar la pagina %d del proceso %d.", pagina, proceso);
                }
                free(pagina_leida);
                free(pagina_a_enviar);
            }
        }

        else {
            log_error(log_file, "MEMORY_RECV_SWAP_SEND Cantidad de parametros incorrecta.");
        }
    }

    else if (recibido->command == FINISH_PROCESS) { // Ejemplo: FINALIZAR_PROCESO PROCESO
        if (recibido->pay_len == sizeof(int)) {
            int proceso;
            memcpy(&proceso, recibido->payload, sizeof(int));
            finalizar_proceso(proceso);
        }

        else {
            log_error(log_file, "FINISH_PROCESS Cantidad de parametros incorrecta.");
        }
    }

    else {
        log_error(log_file, "El comando solicitado no existe.");
    }
}

// void consola(char* buffer, int socket_conexion) {
//     char** parametros = string_split(buffer, " ");
//     int cantidad_de_parametros = contar_parametros(parametros);


//     // recibido->payload = 1 si es asignacion dinamica o 0 si es fija
//     if (string_starts_with(buffer, "TIPO_ASIGNACION")) { // Ejemplo: TIPO_ASIGNACION ASIGNACION_FIJA
//         if (cantidad_de_parametros == 2) {
//             if (string_equals_ignore_case(parametros[1], "ASIGNACION_FIJA")) {
//                 tipo_asignacion = ASIGNACION_FIJA;
//                 log_info(log_file, "La asignacion fija fue establecida.");
//             }

//             else if (string_equals_ignore_case(parametros[1], "ASIGNACION_DINAMICA")) {
//                 tipo_asignacion = ASIGNACION_DINAMICA;
//                 log_info(log_file, "La asignacion dinamica fue establecida.");
//             }

//             else {
//                 log_error(log_file, "Tipo de asignacion invalido.");
//             }
//         }

//         else {
//             log_error(log_file, "Cantidad de parametros incorrecta.");
//         }
//     }

//     else if (string_starts_with(buffer, "GUARDAR_PAGINA")) { // Ejemplo: GUARDAR_PAGINA PROCESO NUMERO_PAGINA CONTENIDO
//         if (cantidad_de_parametros == 4) {
//             guardar_pagina(atoi(parametros[1]), atoi(parametros[2]), parametros [3]);
//         }

//         else {
//             log_error(log_file, "Cantidad de parametros incorrecta.");
//         }
//     }

//     else if (string_starts_with(buffer, "OBTENER_PAGINA")) { // Ejemplo: OBTENER_PAGINA PROCESO NUMERO_PAGINA
//         if (cantidad_de_parametros == 3) {
//             char* pagina = obtener_pagina(atoi(parametros[1]), atoi(parametros[2]));
//             if (pagina != NULL) {
//                 if (send(socket_conexion, pagina, strlen(pagina) + 1, 0) < 0) { // Estoy mandando n bytes(n = tamaño pagina + 2) ya que obtener_pagina devuelve la pagina con el '\0' al final y en el send reservo otro byte mas para el mismo caracter. Anoto esto por las dudas de que se lea/escriba basura en un futuro.
//                     //////////////////////////////////////////////////////////////////////////////////////////////////////
//                     ////////////////////// DESCOMENTAR CUANDO PRUEBE TODO ESTO CON LAS CONEXIONES ///////////////////////
//                     ////////////////////////////////////////////////////////////////////////////////////////////////////
//                     // log_error(log_file, "Error al enviar la pagina %s del proceso %s.", parametros[2], parametros[1]);
//                     //////////////////////////////////////////////////////////////////////////////////////////////////////
//                     ////////////////////// DESCOMENTAR CUANDO PRUEBE TODO ESTO CON LAS CONEXIONES ///////////////////////
//                     ////////////////////////////////////////////////////////////////////////////////////////////////////
//                 }
//                 log_warning(log_file, "%s", pagina);
//                 free(pagina);
//             }
//         }

//         else {
//             log_error(log_file, "Cantidad de parametros incorrecta.");
//         }
//     }

//     else if (string_starts_with(buffer, "FINALIZAR_PROCESO")) { // Ejemplo: FINALIZAR_PROCESO PROCESO
//         if (cantidad_de_parametros == 2) {
//             finalizar_proceso(atoi(parametros[1]));
//         }

//         else {
//             log_error(log_file, "Cantidad de parametros incorrecta.");
//         }
//     }

//     else if (string_starts_with(buffer, "SALIR") || string_starts_with(buffer, "EXIT")) { // Ejemplo: SALIR || EXIT
//         cerrar_swamp();
//     }

//     else {
//         log_error(log_file, "El comando ingresado no existe.");
//     }

//     // Libera memoria
//     for (int i = 0; parametros[i] != NULL; i++) {
//         free(parametros[i]);
//     }
//     free(parametros);
// }

void free_t_mensaje(t_mensaje* mensaje) {
    free(mensaje->identifier);
    free(mensaje->payload);
    free(mensaje);
}

int contar_parametros(char** parametros) {
    int i = 0;
    while (parametros[i] != NULL) {
        i++;
    }
    return i;
}