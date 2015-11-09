/*      
	FEUP 2014/2015
	
	2nd Project - RCOM
	
	1st part - Download application
	
*/
#include "main.h"

#define USER_CMD	0
#define PASS_CMD	1
#define PASV_CMD	2
#define RETR_CMD	3

struct msg_ftp msg;
struct control_ftp ctrl_ftp;
struct receiver_ftp recv_ftp;
struct output_file o_file;

void print_error_usage() {
	printf("Usage: ftp://<user>:<password>@<host>/<url-path>");
}

void print_ftp_struct() {
	printf("Username: %s\n", msg.username);
	printf("Password: %s\n", msg.password);
	printf("Hostname: %s\n", msg.hostname);
	printf("URL_Path: %s\n", msg.url_path);
}

int parse_args(char *arg) {
	// Format: ftp://[<user>:<password>@]<host>/<url-path>
	if(4 == sscanf(arg, "ftp://%[^:]:%[^@]@%[^/]/%s\n", msg.username, msg.password, msg.hostname, msg.url_path)) {
		return 0;
	}
	
	// Format: ftp://<host>/<url-path>
	if(2 == sscanf(arg, "ftp://%[^/]/%s\n", msg.hostname, msg.url_path)){
		strcpy(msg.username, "anonymous");
		strcpy(msg.password, "123");
		return 0;
	}
	
	// Wrong number of parameters or wrong format received
	return -1;
}

int print_server_response(int fd) {

	int bytes_read = 0;
	char res[MAX_SIZE_MEDIUM];
	
	memset(res, 0, MAX_SIZE_MEDIUM);
	
	bytes_read = read(fd, res, MAX_SIZE_MEDIUM);
	if(bytes_read > 0) {
		printf("Server response: %s\n", res);
		return 0;
	}
	
	printf("Could not read response form server socket\n");
	return -1;
}

int receive_pasv_response(int fd) {
	int bytes_read = 0;
	char res[MAX_SIZE_MEDIUM];
	
	memset(res, 0, MAX_SIZE_MEDIUM);
	
	bytes_read = read(fd, res, MAX_SIZE_MEDIUM);
	if(bytes_read > 0) {
		printf("Server response: %s\n", res);

		if(6 != sscanf(res, "%*[^(](%d,%d,%d,%d,%d,%d)\n", &(ctrl_ftp.pasv[0]), &(ctrl_ftp.pasv[1]), &(ctrl_ftp.pasv[2]), &(ctrl_ftp.pasv[3]), &(ctrl_ftp.pasv[4]), &(ctrl_ftp.pasv[5])))
		{
			printf("Could not read the 6 bytes from the server response: %s\n", res);
			return -1;
		}
		
		return 0;
	}
	
	printf("Could not read response form server socket\n");
	return -1;
}

int send_and_rcv(int cmd, int fd) {

	if(USER_CMD == cmd) {
		// Sending 'user' command
		char user_cmd[50];
		memset(user_cmd, 0, 50);
		strcpy(user_cmd, "user ");
		strcat(user_cmd, msg.username);
		strcat(user_cmd, "\n");
		printf("Command: %s\n", user_cmd);
		if(write(fd, user_cmd, strlen(user_cmd)) < 0) {
			printf("Could not send the 'user' command\n");
			return -1;
		}
	
		sleep(1);
		
		print_server_response(fd);
		return(0);
	}
	else if(PASS_CMD == cmd) {
	
		// Sending 'pass' command
		char pass_cmd[50];
		memset(pass_cmd, 0, 50);
		strcpy(pass_cmd, "pass ");
		strcat(pass_cmd, msg.password);
		strcat(pass_cmd, "\n");
		printf("Command: %s\n", pass_cmd);
		if(write(fd, pass_cmd, strlen(pass_cmd)) < 0) {
			printf("Could not send the 'pass' command\n");
			return -1;
		}

		sleep(1);
		
		print_server_response(fd);
		return(0);
	}
	else if(PASV_CMD == cmd) {
	
		// Sending 'pasv' command
		char pasv_cmd[50];
		memset(pasv_cmd, 0, 50);
		strcpy(pasv_cmd, "pasv");
		strcat(pasv_cmd, "\n");
		printf("Command: %s\n", pasv_cmd);
		if(write(fd, pasv_cmd, strlen(pasv_cmd)) < 0) {
			printf("Could not send the 'pasv' command\n");
			return -1;
		}

		sleep(1);
		
		if(0 == receive_pasv_response(fd)) {
			
			// Parse new IP address
			ctrl_ftp.port = ctrl_ftp.pasv[4] * 256 + ctrl_ftp.pasv[5];
			memset(ctrl_ftp.ip, 0, MAX_SIZE_MEDIUM); // clearing the array, "just in case"
			sprintf(ctrl_ftp.ip, "%d.%d.%d.%d", ctrl_ftp.pasv[0], ctrl_ftp.pasv[1], ctrl_ftp.pasv[2], ctrl_ftp.pasv[3]);
			printf("IP address to receive file: %s\n", ctrl_ftp.ip);
			printf("Port to receive file      : %d\n", ctrl_ftp.port);
			
			return 0;
		}

		printf("Could not receive the 'pasv' response\n");
		return -1;
	}
	
	printf("Unrecognised command to send\n");
	return -1;
}

int open_tcp_control_connection() {
	struct hostent * host;
	struct sockaddr_in server_addr;
	int	socket_fd;
	char *host_ip;
	
	
	if(NULL == (host = gethostbyname(msg.hostname))) {
		printf("Could not get host\n");
		return -1;
	}

	host_ip = inet_ntoa(*((struct in_addr *)host->h_addr));

	printf("Host name  : %s\n", host->h_name);
	printf("IP Address : %s\n", host_ip);

	// Configuring server address
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host_ip);	// 32 bit Internet address network byte ordered
	server_addr.sin_port = htons(FTP_PORT);				// server TCP port (21) must be network byte ordered

	// Opening the control TCP socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		printf("Error opening control TCP socket\n");
		return -1;
	}

	// Connecting to the server...
	if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Error connecting to the server to open the control connection\n");
		return -1;
	}
	
	ctrl_ftp.socket_fd = socket_fd;
	return 0;
}

int receive_tcp_control_data() {
	// Sending the necessary commands to get the control TCP connection data
	
	if(send_and_rcv(USER_CMD, ctrl_ftp.socket_fd) != 0) {
		return -1;
	}
	
	if(send_and_rcv(PASS_CMD, ctrl_ftp.socket_fd) != 0) {
		return -1;
	}
	
	if(send_and_rcv(PASV_CMD, ctrl_ftp.socket_fd) != 0) {
		return -1;
	}
	
	return 0;
}

int open_tcp_receiver_connection() {

	struct hostent * host;
	struct sockaddr_in server_addr;
	int	socket_fd;
	char *host_ip;

	if (NULL == (host = gethostbyname(ctrl_ftp.ip))) {
		printf("Could not get host\n");
		return -1;
	}

	host_ip = inet_ntoa(*((struct in_addr *)host->h_addr));

	printf("Host name  : %s\n", host->h_name);
	printf("IP Address : %s\n", host_ip);

	// Configuring server address
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host_ip);	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(ctrl_ftp.port);		//server TCP port must be network byte ordered | this is the new port received from the control TCP connection!
	
	// Opening the receiver TCP socket
	socket_fd = socket(AF_INET, SOCK_STREAM,0);
	if (socket_fd < 0) {
		printf("Error opening receiver TCP socket\n");
		return -1;
	}
	
	// Connecting to the server...
	if(connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		printf("Error connecting to the server to open the receiver connection\n");
		return -1;
	}
	
	recv_ftp.socket_fd = socket_fd;
	return 0;
}

int open_output_file() {
	// Get the filename
	int url_path_length = strlen(msg.url_path);
	int i = (url_path_length - 1);
	while(i > 0) {
		if('/' == msg.url_path[i]) { // The filename is the name after the last "forward slash"
			i++;
			break;
		}
		i--;
	}
	
	if(i == (url_path_length - 1)) {
		printf("Could not read the filename\n");
		return -1;
	}

	int j;
	for(j = 0; i < url_path_length; i++, j++) {
		o_file.filename[j] = msg.url_path[i];
	}
	o_file.filename[j]='\0'; // <-- VERY IMPORTANT !

	printf("Filename: %s\n", o_file.filename);
	
	
	o_file.fd = fopen(o_file.filename, "wb");	// binary write mode
	
	if(NULL == o_file.fd) {
		return -1;
	}
	
	return 0;
}

int receive_file() {
	
	// Create an output file with the url_path name
	if(0 != open_output_file()) {
		printf("Could not open the output file\n");
		return -1;
	}
	
	// Sending 'retr' command
	char retr_cmd[50];
	memset(retr_cmd, 0, 50);
	strcpy(retr_cmd, "retr ");
	strcat(retr_cmd, msg.url_path);
	strcat(retr_cmd, "\n");
	printf("Command: %s\n", retr_cmd);
	if(write(ctrl_ftp.socket_fd, retr_cmd, strlen(retr_cmd)) < 0){
		printf("Could not send 'retr' command\n");
		return -1;
	}
	
	// Reading the server response
	char buf[MAX_SIZE_MEDIUM];
	int bytes_read = 0;
	memset(buf, 0, MAX_SIZE_MEDIUM - 1);
	
	while((bytes_read = read(recv_ftp.socket_fd, buf, 255)) > 0){
		buf[bytes_read] = '\0';
		//printf("Read : %s\n", buf);
		fwrite(buf, sizeof(char), bytes_read, o_file.fd);
		memset(buf, 0, 255);
	}
	
	fclose(o_file.fd);

	return 0;
}

int main(int argc, char **argv) {

	if(2 != argc) {
		print_error_usage();
		return -1;
	}
	
	if(0 != parse_args(argv[1])) {
		print_error_usage();
		return -1;
	}
	
	printf("\n> FTP Structure:\n");
	print_ftp_struct();
	
	printf("\n> Opening TCP control connection...\n");
	if(0 != open_tcp_control_connection()) {
		printf("Could not open tcp control connection\n");
		return -1;
	}
	
	printf("\n> Receiving TCP control data...\n");
	if(0 != receive_tcp_control_data()) {
		printf("Could not receive tcp control data\n");
		return -1;
	}
	
	printf("\n> Opening TCP receiver connection...\n");
	if(0 != open_tcp_receiver_connection()) {
		printf("Could not open tcp receiver connection\n");
		
		close(ctrl_ftp.socket_fd);
		return -1;
	}
	
	printf("\n> Receiving file...\n");
	if(0 != receive_file()) {
		printf("Could not receive file data\n");
		close(ctrl_ftp.socket_fd);
		close(recv_ftp.socket_fd);
		return -1;
	}
	else {
		printf("File received successfully\n");
	}

	close(ctrl_ftp.socket_fd);
	close(recv_ftp.socket_fd);

	printf("\n> Sockets closed. Terminating...\n");

	return 0;
}
