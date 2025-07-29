#include "sd_file.h"
#include "sd_card.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>
#include "hw_config.h"

#define SD_DRIVE "0:"

static FATFS *p_fs = NULL;

bool sd_mount(void)
{
    p_fs = &sd_get_by_num(0)->fatfs;
    FRESULT res = f_mount(p_fs, SD_DRIVE, 1);
    if (res != FR_OK)
    {
        printf("Erro ao montar o SD: %d\n", res);
        return false;
    }
    sd_get_by_num(0)->mounted = true;
    return true;
}

bool sd_unmount(void)
{
    FRESULT res = f_unmount(SD_DRIVE);
    if (res != FR_OK)
    {
        printf("Erro ao desmontar o SD: %d\n", res);
        return false;
    }
    sd_get_by_num(0)->mounted = false;
    return true;
}

bool sd_append_line(const char *filename, const char *line)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (res != FR_OK)
    {
        printf("Erro ao abrir arquivo para escrita: %d\n", res);
        return false;
    }
    UINT bw;
    res = f_write(&file, line, strlen(line), &bw);
    f_write(&file, "\n", 1, &bw); // Adiciona quebra de linha
    f_close(&file);
    return (res == FR_OK);
}

bool sd_read_file(const char *filename)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("Erro ao abrir arquivo para leitura: %d\n", res);
        return false;
    }
    char buffer[128];
    UINT br;
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    return true;
}
