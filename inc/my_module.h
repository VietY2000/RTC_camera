#ifndef __MY_MODULE_H__
#define __MY_MODULE_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <rtsavapi.h>

typedef enum {
    H264,
    ALAW,
    JPG,
} format;

typedef struct {	
	FILE *file;
	char folder[64];
	char name[128];
	char path[64];
	uint8_t channel;
    format fm;
} save_struct;

void func_creat_new_folder(save_struct *file_save, time_t time);
void func_set_file_name(save_struct *file_save, time_t time_start, uint32_t time_save);
void func_save(save_struct *file_save, time_t *start, time_t now, uint32_t time_save);

#endif /* __MY_MODULE_H__ */