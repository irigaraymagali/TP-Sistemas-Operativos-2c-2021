#ifndef SWAP_H
#define SWAP_H

#include "main.h"
#include <sys/mman.h>  // Biblioteca mmap()
#include <sys/stat.h>  // Biblioteca open(), mkdir(), fstat()
#include <sys/types.h> // Biblioteca ftruncate(), opendir()
#include <fcntl.h>     // Biblioteca open(), fstat()
#include <unistd.h>    // Biblioteca pwrite(), close(), sleep()
#include <dirent.h>    // Biblioteca opendir()
#include <errno.h>

#define SWAP_FILES_PATH "./Swap Files/"

void inicializar_directorios();
void inicializar_swap_files();

#endif