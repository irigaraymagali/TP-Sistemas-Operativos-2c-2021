#ifndef SWAP_H
#define SWAP_H

#include "main.h"
#include <commons/collections/list.h>
#include <commons/collections/node.h>
#include <commons/collections/dictionary.h>
#include <sys/mman.h>    // Biblioteca mmap()
#include <sys/stat.h>    // Biblioteca open(), mkdir(), fstat()
#include <sys/types.h>   // Biblioteca ftruncate(), opendir()
#include <fcntl.h>       // Biblioteca open(), fstat()
#include <unistd.h>      // Biblioteca pwrite(), close(), sleep()
#include <dirent.h>      // Biblioteca opendir()
#include <errno.h>

#define SWAP_FILES_PATH "./swapdir/"
#define ASIGNACION_FIJA 0
#define ASIGNACION_DINAMICA 1

t_dictionary* swap_dict; // Diccionario donde la key representa a un proceso y el valor es un puntero a la tabla de paginas del archivo de swap que utiliza
t_list* swap_list;       // Lista que contiene todas las listas que representan a las tablas de paginas de los diferentes archivos de swap
int swap_file_size;
int swap_page_size;
int marcos_por_carpincho;
int tipo_asignacion;
int retardo_swamp;

typedef struct {
    int proceso;
    int pagina;
} fila_tabla_paginas;

typedef struct {
    char* swap_file_name;
    t_list* tabla_paginas;
} nodo_swap_list;

void inicializar_directorios();
void inicializar_swap_files();
// void guardar_pagina(int proceso, int pagina, char* contenido);
// void guardar_pagina_asignacion_fija(int proceso, int pagina, char* contenido);
// void guardar_pagina_asignacion_dinamica(int proceso, int pagina, char* contenido);
// char* obtener_pagina(int proceso, int pagina);
int guardar_pagina(int proceso, int pagina, void* contenido);
int guardar_pagina_asignacion_fija(int proceso, int pagina, void* contenido);
int guardar_pagina_asignacion_dinamica(int proceso, int pagina, void* contenido);
void* obtener_pagina(int proceso, int pagina);
void finalizar_proceso(int proceso);
nodo_swap_list* swap_file_menos_ocupado();
int frames_ocupados(t_list* tabla_paginas);
int get_first_free_frame_number(fila_tabla_paginas* nodo, void* swap_file_map);
int get_frame_number(fila_tabla_paginas* nodo);
bool frame_is_empty(int frame, void* swap_file_map);
bool pagina_esta_en_swap(t_list* tabla_paginas, int proceso, int pagina);
char* get_swap_file_name(t_list* tabla_paginas);
int tabla_paginas_size(t_list* tabla_paginas);
void fila_tabla_paginas_destroy(void* fila);
void nodo_swap_list_destroy(void* nodo);

#endif
