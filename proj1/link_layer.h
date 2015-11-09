#ifndef _SERIAL_PORT_DRIVER_H_
#define _SERIAL_PORT_DRIVER_H_

#include "constants.h"
#include "alarm.h"
#include "helpers.h"

#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#define BAUDRATE B38400
#define TIMEOUT 3

#define TRANSMITTER		0
#define RECEIVER			1

#define CMD_LENGTH		5

#define	FLAG	0x7e
#define ESCAPE	0x7d
#define STUFF	0x20

#define ADDR	1
#define CC		2
#define BCC1	3

#define HTS		6 // frame (header + trailer)

#define MODULE 2

typedef enum {
	ERROR_NONE,
	ERROR_BCC1,
	ERROR_BCC2,
	ERROR_NOT_CMD,
	ERROR_CONTENT
} Error;

typedef enum {
	T_TO_R = 0x03,
	R_TO_T = 0x01,
} Adress;

typedef enum {
	CTRL_NONE = 0x00,
	CTRL_SET = 0x03,
	CTRL_DISC = 0x0b,
	CTRL_UA	= 0x07,
	CTRL_RR	= 0x05,
	CTRL_REJ = 0x01
} Command_Type;

typedef enum {
	CONNECT,
	TRANSMIT,
	DISCONNECT,
	OVER
} State;

typedef enum {
	RR_CURRENT,
	RR_NEXT,
	REJ
} Response;

typedef struct {
	int fd;
	int status;
	
	State state;
	
	unsigned int nr; // mod 2
	
	unsigned int baudrate;
	unsigned int timeout;
	unsigned int max_attempts;
	
	message frame;
	
	Error err;
	Command_Type type;
	Response resp;
	
	
	int sm_state; // state machine state
	int index;
	int in_transition;
	
} link_layer;

typedef struct{
	unsigned int num_frames_sent;
	unsigned int num_frames_received;

	unsigned int num_frames_resent;

	unsigned int num_timeouts;
	unsigned int num_rej_sent;
	unsigned int num_rej_received;

} Statistics;

typedef struct {

	char buf[CMD_LENGTH];
	int length;
} command;



static link_layer ll;
static Statistics stats;

void ll_set_params(int baudrate, int timeout);

void print_message(message msg);

int calc_next_nr();

// Uses byte stuffing to prevent the appearance of the end flag in the data packet,
//	which could lead to an early stop in the reading process.
message stuff(message msg);

// Destuffes a previously stuffed message, returning it to its original state.
message destuff(message msg);

// Returns the given buf's BCC correspondent, so that this result can be compared with the given BCC.
// 	If they coincide, then it is most likely the received message was not corrupted during the transfer.
char get_bcc(char* buf, int size);

int check_bcc(char bcc_cmp, int bcc_start, int bcc_length);

// Builds a data message.
message build_msg(char* msg, int size);

// Builds a command type message: [F, A, C, Bcc, F]
command build_cmd(Adress addr, Command_Type cmd, int nr);

int check_cmd();

int check_msg();

int frame_is_cmd(Command_Type cmd);

// Reads a command type message: [F, A, C, Bcc, F]
// 	Implements a state machine. Will exit only if it has read a valid command message
// 	 or if it has been signaled by the alarm.
int read_frame();

int send_msg(message msg);

// Sends a command type message: [F, A, C, Bcc, F]
// 	No rocket science, only writes the message.
//	Returns ERROR if the written char count is less than the cmd size.
int send_cmd(command cmd);

// Opens the port for connection (sets the termios configuration).
int open_port(char * port);

// Opens the port for connection (status indicates whether it is sender or receive)
//	and establishes the connection returning to the SET and UA message sequence.
int llopen(char * port, int status);

int llwrite(char* msg, int size);

int llread(char* msg);

// Closes the port of connection (resets the termios configuration).
int close_port();

// Closes the port of connection and the file descriptor associated to it.
int llclose();

int _ctrl(char c);
#endif
