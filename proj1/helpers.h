#ifndef _HELPERS_H_
#define _HELPERS_H_

#include "constants.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {

	char buf[MAX_FRAME_SIZE];
	int length;
} message;

unsigned char * convert_int32_to_byte(int w);

int convert_byte_to_int32(unsigned char * b,int size);

int get_baudrate(int baudrate);

#endif
