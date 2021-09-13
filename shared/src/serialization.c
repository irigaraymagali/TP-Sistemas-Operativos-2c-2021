#include "serialization.h"

int get_type(char *type) {
    return 
        !strcmp(type, "s") ? STRING :
        !strcmp(type, "d") ? INT :
        !strcmp(type, "c") ? CHAR :
        !strcmp(type, "f") ? DOUBLE :
        !strcmp(type, "u") ? UINT32 :
    -1;
}

void *_serialize(int size, char *format, ...) {
    if (!string_starts_with(format, "%")) {
        fprintf(stderr, "[Shared Library]: Incorrect Format\n");
        return NULL;
    }

    void *stream = malloc(size);
    int offset = 0;

    char *string;
    int stringLength;
    int value;
    char c_value;
    double d_value;
    uint32_t u_value;

    int arg_c = 0;

    for (int n = 0; format[n]; n++) {
        if (format[n] == '%') arg_c++;
    }
    // Separo los tipos de datos
    char **types = string_split(format, "%");

    // Declaro la lista de argumentos de la funcion
    va_list lista_argumentos;
    // Inicializo la lista
    va_start(lista_argumentos, format);

    // Verifico primero los parametros que se enviaron
    for(int i = 1; i <= arg_c; i++) {

        if(get_type(types[i]) < 0) {
            printf("[Shared Library]: %s Format Does not exist\n", types[i]);
            free(stream);
            for(int j = 0; j <= arg_c; j++){
                free(types[j]);
            }
            free(types);
            va_end(lista_argumentos);
            return NULL;
        }
    }

    for(int i = 1; i <= arg_c; i++) {

        switch(get_type(types[i])) {

            case STRING:
                // printf("Got a string..\n");
                string = va_arg(lista_argumentos, char *);
                // printf("[Shared Library]: Serializing type of String: %s\n", string);

                stringLength = strlen(string);
                memcpy(stream + offset, &stringLength, sizeof(int));
                offset += sizeof(int);
                memcpy(stream + offset, string, stringLength);
                offset += stringLength;

            break;
            
            case INT:
                // printf("Got an Integer..\n");
                value = va_arg(lista_argumentos, int);
                // printf("[Shared Library]: Serializing type of Int: %d\n", value);

                memcpy(stream + offset, &value, sizeof(int));
                offset += sizeof(int);

            break;
            
            case CHAR:
                // printf("Got a Characater..\n");
                c_value = va_arg(lista_argumentos, int);
                // printf("[Shared Library]: Serializing type of Char: %c\n", c_value);

                memcpy(stream + offset, &c_value, sizeof(char));
                offset += sizeof(char);

            break;
            
            case DOUBLE:
                // printf("Got a Double..\n");
                d_value = va_arg(lista_argumentos, double);
                // printf("[Shared Library]: Serializing type of Double: %f\n", d_value);

                memcpy(stream + offset, &d_value, sizeof(double));
                offset += sizeof(double);

            break;

            case UINT32:
                // printf("Got an 32b Unsigned Int..\n");
                u_value = va_arg(lista_argumentos, uint32_t);
                // printf("[Shared Library]: Serializing type of UInt32_t: %d\n", u_value);

                memcpy(stream + offset, &u_value, sizeof(uint32_t));
                offset += sizeof(uint32_t);

            break;

            default:
                // printf("[Shared Library]: %s Format Does not exist\n", types[i]);
                free(stream);
                for(int j = 0; j <= arg_c; j++){
                    free(types[j]);
                }
                free(types);
                va_end(lista_argumentos);
                return NULL;
        }

    }

    for(int j = 0; j <= arg_c; j++){
        free(types[j]);
    }
    free(types);
    va_end(lista_argumentos);

    return stream;
    
}

// int main () {

//     void *stream = _serialize(5 + sizeof(int), "%s", "holis");

//     //send()

//     // Libero la memoria del buffer
//     free(stream);

//     return 0;
// }