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

#define SWAP_FILES_PATH "./Swap Files/"
#define ASIGNACION_FIJA 10
#define ASIGNACION_DINAMICA 20

t_dictionary* swap_dict; // Diccionario donde la key representa a un proceso y el valor es un puntero a la tabla de paginas del archivo de swap que utiliza
t_list* swap_list;       // Lista que contiene todas las listas que representan a las tablas de paginas de los diferentes archivos de swap
int swap_file_size;
int swap_page_size;
int tipo_asignacion;

typedef struct {
    char* proceso;
    int pagina;
} fila_tabla_paginas;

void inicializar_directorios();
void inicializar_swap_files();

#endif