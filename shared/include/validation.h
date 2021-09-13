#ifndef VALIDATION_H
#define VALIDATION_H

// STANDARD LIBRARIES

#include <stddef.h>
#include <commons/config.h>

// FUNCTIONS

/**
 * METHOD: _check_config
 * 
 * valida un archivo de configuracion
 * 
 * @params: 
 *      config -> Archivo de configuracion
 *
 *      keys -> Array de claves utilizadas para validar el archivo
 *
 * @example:
 *      Call with _check_config(config, KEYS);
 *
 *      char *KEYS[] = {
 *        "KEY_1",
 *        "KEY_2",
 *        "KEY_3",
 *      }
 */
int _check_config(t_config* config, char* keys[]);

#endif