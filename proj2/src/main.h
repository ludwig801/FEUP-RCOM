#ifndef _MAIN_FTP_
#define _MAIN_FTP_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FTP_PORT	21

#define MAX_SIZE_SHORT	32
#define MAX_SIZE_MEDIUM	256
#define MAX_SIZE_LONG	2048

// ftp message composition: ftp://[<user>:<password>@]<host>/<url-path>
struct msg_ftp {

	char username[MAX_SIZE_SHORT];		// <user>
	char password[MAX_SIZE_SHORT];		// <password>
	char hostname[MAX_SIZE_SHORT];		// <host>
	char url_path[MAX_SIZE_LONG];		// <url-path>
};

struct control_ftp {
	int	socket_fd;
	int pasv[6];	// response to 'pasv' command
	
	int port;		// New port received from the server
	char ip[MAX_SIZE_MEDIUM];			// New IP address received from the server
};

struct receiver_ftp {
	int	socket_fd;
};

struct output_file {
	FILE* fd;
	char filename[MAX_SIZE_LONG];
};

void print_error_usage();

void print_ftp_struct();

int parse_args(char *arg);

int print_server_response(int fd);

int receive_pasv_response(int fd);

int send_and_rcv(int cmd, int fd);

int open_tcp_control_connection();

int receive_tcp_control_data();

int open_tcp_receiver_connection();

int open_output_file();

int receive_file();

int main(int argc, char **argv);

#endif 
