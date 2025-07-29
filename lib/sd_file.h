#ifndef SD_FILE_H
#define SD_FILE_H

#include <stdbool.h>

// Inicializa o SD card (monta)
bool sd_mount(void);

// Desmonta o SD card
bool sd_unmount(void);

// Escreve (anexa) uma string em um arquivo existente ou cria se não existir
bool sd_append_line(const char *filename, const char *line);

// Lê e exibe o conteúdo de um arquivo no terminal
bool sd_read_file(const char *filename);

#endif
