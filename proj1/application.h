#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "link_layer.h"

#include <stdio.h>
#include <stdint.h>

#define MAX_PACKET_SIZE 100

#define FALSE	0
#define TRUE	1

typedef enum {
	PACKET_DATA = 1,
	PACKET_START = 2,
	PACKET_END = 3
} Packet_Control;

typedef enum {
	PACKET_FILE_SIZE = 0,
	PACKET_FILE_NAME = 1
} Packet_Type;

typedef struct {
	FILE * f;
	int size;
	char *name;
	
	int valid;
} File;

typedef struct {
	Packet_Control ctrl;
	int sequence_nr;
	
	message data;
	
} Packet;

static File fstatus;
static Packet pack;

int open_file(const char * filepath, const char * mode);

int read_file_size(const char * filepath);

int app_send_data(int sequence_nr, int data_field_size, char* data);

int app_read_data(char* data);

int app_send_control(Packet_Control control, Packet_Type type, int file_size);

int app_read_control(Packet_Control control);

int app_check_packet(char * buf, int size);

int app_send(char * port, const char * filepath);

int app_read(char * port);

#endif
