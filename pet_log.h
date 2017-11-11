/* 
 * PetLab logging utility functions
 * (c) Jack lange, 2015
 */


#ifndef __PET_LOG_H__
#define __PET_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


extern FILE * pet_log_stream;

#define ERROR(fmt, args...)                                             \
    do {                                                                \
	FILE * output = pet_log_stream;					\
	if (!output) output = stderr;					\
	fprintf(output, "Error> %s(%d): " fmt,  __FILE__, __LINE__, ##args); \
    } while (0)


#define WARN(fmt, args...)                                             \
    do {                                                                \
	FILE * output = pet_log_stream;					\
	if (!output) output = stdout;					\
	fprintf(output, "Warning> %s(%d): " fmt,  __FILE__, __LINE__, ##args); \
    } while (0)


#define DEBUG(fmt, args...)                                             \
    do {                                                                \
	FILE * output = pet_log_stream;					\
	if (!output) output = stdout;					\
        fprintf(output, "Debug> " fmt, ##args);			\
    } while (0)



int pet_log_file_init(char * filename);


#ifdef __cplusplus
}
#endif



#endif
