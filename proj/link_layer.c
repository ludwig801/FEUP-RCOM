#include "link_layer.h"
#include <math.h>

#define BIT(x,b)	((x >> b) & 1)
#define CUT(x)		(x & 0x0F)
#define _ADDR(x, b) (&(x[b]))

struct termios oldtio;

void ll_set_params(int b, int t) {

	ll.baudrate = (b < 0) ? BAUDRATE : b;
	ll.timeout = (t < 0) ? TIMEOUT : t;
}

void print_message(message msg) {
	int i;
	
	printf("Frame: ");
	
	for(i = 0; i < msg.length; i++) {
		printf("%x ", msg.buf[i]);
	}
	
	printf("\n"); 
}

int calc_next_nr() {
	return (ll.nr + 1) % MODULE;
}

message stuff(message msg) {

	message stuffed;
	
	int i;
	
	int extra = 0;
	for(i = 1; i < (msg.length - 1); i++) {
		if(msg.buf[i] == FLAG || msg.buf[i] == ESCAPE)
			extra++;
	}
	
	stuffed.length = msg.length + extra;
	
	int j = 0;
	stuffed.buf[j] = msg.buf[0];
	j++;
	
	for(i = 1; i < (msg.length - 1); i++) {
		if(msg.buf[i] == FLAG) {
			stuffed.buf[j] = ESCAPE;
			j++;
			stuffed.buf[j] = FLAG ^ STUFF;
		}
		else if(msg.buf[i] == ESCAPE) {
			stuffed.buf[j] = ESCAPE;
			j++;
			stuffed.buf[j] = ESCAPE ^ STUFF;
		}
		else {
			stuffed.buf[j] = msg.buf[i];
		}	
		j++;
	}
	
	stuffed.buf[j] = msg.buf[i];
	
	return stuffed;
}

message destuff(message msg) {
	
	message destuffed;

	int j = 0;
	
	int i;
	for(i = 0; i < msg.length; i++) {
		if(msg.buf[i] == ESCAPE) {
			destuffed.buf[j] = msg.buf[i+1] ^ STUFF;
			i++;
		}
		else {
			destuffed.buf[j] = msg.buf[i];
		}
		
		j++;
	}
	
	destuffed.length = j;
	
	return destuffed;
}

char get_bcc(char * buf, int size) {

    char result = 0;
    int i;
	
    for (i = 0; i < size; ++i) {
        result ^= buf[i];
    }
    
    return result;
}

command build_cmd(Adress addr, Command_Type cmd, int nr) {
	
    command result;
    result.length = CMD_LENGTH;
	
    result.buf[0] = FLAG;
    result.buf[1] = addr;
	
	if(cmd == CTRL_RR || cmd == CTRL_REJ)
		result.buf[2] = cmd | (nr << 7);
	else
		result.buf[2] = cmd;

    result.buf[3] = get_bcc(_ADDR(result.buf, ADDR), 2);
    result.buf[4] = FLAG;

    return result;
}

message build_msg(char * msg, int size) {

	message temp;
	
	temp.length = size + HTS;
	
	temp.buf[0] = FLAG;
	temp.buf[1] = T_TO_R;
	temp.buf[2] = (ll.nr << 6);
	temp.buf[3] = get_bcc(_ADDR(temp.buf, 1), 2);
	
	int i;
	for(i = 0; i < size; i++) {
		temp.buf[i + 4] = msg[i];
	}
	
	temp.buf[size + 4] = get_bcc(_ADDR(temp.buf, 4), size);
	temp.buf[size + 5] = FLAG;

	return stuff(temp);
}

int check_cmd() {

	ll.err = ERROR_NONE;
	ll.type = CTRL_NONE;
	
	if(ll.frame.length > CMD_LENGTH) {
		ll.err = ERROR_NOT_CMD;
		return ERROR;
	}
	
	
	if(check_bcc(ll.frame.buf[BCC1], ADDR, 2)) {
		ll.err = ERROR_BCC1;
		return ERROR;
	}
		
	char cmd_c = CUT(ll.frame.buf[2]);
	
	switch(cmd_c) {
	
		case CTRL_SET:
			ll.type = CTRL_SET;
			break;
			
		case CTRL_DISC:
			ll.type = CTRL_DISC;
			break;
			
		case CTRL_UA:
			ll.type = CTRL_UA;
			break;
			
		case CTRL_RR:
			ll.type = CTRL_RR;
			break;
			
		case CTRL_REJ:
			ll.type = CTRL_REJ;
			break;
			
		default:
			ll.err = ERROR_CONTENT;
			return ERROR;
	}
		
	return OK;
}

int frame_is_cmd(Command_Type cmd) {

	if(check_cmd() != OK) {
		switch(ll.err) {
				case ERROR_NOT_CMD:
					break;
			
				case ERROR_BCC1:
					break;
			
				case ERROR_CONTENT:
					break;
			
				default:
					break;
			}
		return ERROR;
	}
	
	if(ll.type == cmd)
		return OK;
		
	return ERROR;
}

int read_frame() {

	char buf[2];
	char c;
	
	if(ll.status == TRANSMITTER) {
	
		tcflush(ll.fd, TCIFLUSH);
		ll.frame.length = 0;
		ll.sm_state = 0;
		ll.index = 0;
	
		while (!get_alarm_flag()) {
		
			if(read(ll.fd, buf, 1))
			{
				c = buf[0];
				
				switch(ll.sm_state) {
				case 0:
					if(c == FLAG)
					{
						ll.index = 0;
						ll.frame.buf[ll.index] = c;
						ll.index = 1;
						ll.sm_state = 1;
						ll.frame.length = 1;
					}
					break;
				case 1: 
					if(c == T_TO_R)
					{
						ll.index = 1;
						ll.frame.buf[ll.index] = c;
						ll.index = 2;
						ll.sm_state = 2;
						ll.frame.length = 2;
					}
					else if(c == FLAG)
							ll.sm_state = 1;
						else
							ll.sm_state = 0;
					break;
				case 2:
					if(c == FLAG){
					ll.sm_state = 1;
					}
					else
					{
						ll.frame.buf[ll.index] = c;
						ll.index = 3;
						ll.sm_state = 3;
						ll.frame.length = 3;
					}
					break;
				case 3:
					if(c == FLAG)
						ll.sm_state = 1;
					else {
						ll.frame.buf[ll.index] = c;
						ll.index = 4;
						ll.sm_state = 4;
						ll.frame.length = 4;
					}
					break;
				case 4:
					if(c == FLAG) {
						ll.frame.buf[ll.index] = c;
						ll.sm_state = 0;
						ll.frame.length = 5;
						return OK;
					}
					else
						ll.sm_state = 0;
					break;
				}		
			}	
		}
	}
	else if(ll.status == RECEIVER) {
	
		if(ll.in_transition == FALSE) {
			tcflush(ll.fd, TCIFLUSH);
			ll.sm_state = 0;
			ll.frame.length = 0;
			ll.index = 0;
		}
		else {
			ll.in_transition = FALSE;
		}
		
	
		while (!get_alarm_flag()) {
			
			if(read(ll.fd, buf, 1)) {
		
				c = buf[0];
								
				switch(ll.sm_state) {
				case 0:
					if(c == FLAG)
					{
						ll.index = 0;
						ll.frame.buf[ll.index] = c;
						ll.sm_state = 1;
						ll.frame.length = 1;
					}
					break;
				case 1: 
					if(c == T_TO_R)
					{
						ll.index = 1;
						ll.frame.buf[ll.index] = c;
						ll.sm_state = 2;
						ll.frame.length = 2;
					}
					else if(c == FLAG)
						ll.sm_state = 1;
					else
						ll.sm_state = 0;
					break;
				case 2:
					if(c == FLAG)
						ll.sm_state = 1;
					else {
						ll.index = 2;
						ll.frame.buf[ll.index] = c;
						ll.sm_state = 3;
						ll.frame.length = 3;

						if((ll.state != TRANSMIT) && (_ctrl(c) != OK)) {
							ll.in_transition = TRUE;
							ll.state = TRANSMIT; // data transmission has started
							return OK;
						}
					}
					break;
				case 3:
					if(c == FLAG)
						ll.sm_state = 1;
					else {
						ll.index = 3;
						ll.frame.buf[ll.index] = c;
						ll.frame.length = 4;
						
						if(ll.state != TRANSMIT)
							ll.sm_state = 5; // if not transmitting, the state machine espects a frame with length 5
						else
							ll.sm_state = 4; // if transmitting, data will be received from now on
					}
					break;
				case 4: // state only for data transmission
					if(c == FLAG) {
						ll.index++;
						ll.frame.buf[ll.index] = c;
						ll.frame.length++;
						return OK; // we can receive the end FLAG, meaning this is a command and not a data message (that shall be determined by the calling method)
					}
					else {
						// data is received here
						ll.index++;
						ll.frame.buf[ll.index] = c;
						ll.frame.length++;
					}
					break;

				case 5:
					if(c == FLAG)
						return OK;
					else
						ll.index++;
						ll.frame.buf[ll.index] = c;
						ll.frame.length += 1;
						ll.sm_state = 1;
					break;
				} // while end
			} // if end
		}
	}
		
	return ERROR;
}



int send_cmd(command cmd) {

	tcflush(ll.fd, TCOFLUSH);
	
	int written = write(ll.fd, cmd.buf, cmd.length);
	
	return (written == cmd.length ? OK : ERROR);
}

int send_msg(message msg) {

	tcflush(ll.fd, TCOFLUSH);
	
	int written = write(ll.fd, msg.buf, msg.length);
	
	return (written == msg.length ? OK : ERROR);
}

int open_port(char * serial_port) {

	struct termios newtio;
	
	int fd;

	fd = open(serial_port, O_RDWR | O_NOCTTY );

	if (fd < 0)
		return ERROR;

	if (tcgetattr(fd, &oldtio) != OK) { /* save current port settings */

		perror("tcgetattr");
		return ERROR;
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = (ll.baudrate) | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = OPOST;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]  = 0;
	
	tcflush(fd, TCIFLUSH);
	tcflush(fd, TCOFLUSH);

	if ( tcsetattr(fd, TCSANOW, &newtio) != OK)
	{
		perror("tcsetattr");
		return ERROR;
	}
	
	ll.fd = fd;

 	return fd;
}

int llopen(char * port, int status) {

	install_alarm();
	alarm(0);
	
	int fd = open_port(port);

	if(fd < 0) {
		printf("    > [LL] Could not open serial port\n");
		return ERROR;
	}
	
	printf("    > [LL] Serial port open\n");

	ll.fd = fd;
	ll.max_attempts = 3;	
	ll.state = CONNECT;
	ll.status = status;

	stats.num_frames_sent = 0;
	stats.num_frames_received = 0;

	stats.num_frames_resent= 0;

	stats.num_timeouts = 0;
	stats.num_rej_sent = 0;
	stats.num_rej_received = 0;
	
	if(ll.status == TRANSMITTER) {

		command cmd = build_cmd(T_TO_R, CTRL_SET, ll.nr);
		
		int attempts = 0;

		while(attempts < ll.max_attempts && ll.state != TRANSMIT) {

			printf("    > [LL] Sending SET...\n");

			if(send_cmd(cmd) != OK) {

				printf("      > Error sending SET\n");
			}
			else {
				
				set_alarm_flag(0);
				alarm(ll.timeout);
			
				if(read_frame() == OK) {

					if(frame_is_cmd(CTRL_UA) == OK) {
					
						printf("      > Received UA...\n");
						ll.state = TRANSMIT;
						break;
					}		
				}
			
				printf("      > [LL] Limit time(%d) exceeded...\n", ll.timeout);
			}

			attempts++;
		}
	}
	else if(ll.status == RECEIVER) {
		
		command cmd;
		
		printf("    > [LL] Connecting...\n");

		set_alarm_flag(0);
		
		while(TRUE) {

			if(ll.state == TRANSMIT) {
				break;
			}

			if(read_frame() == OK) {
				
				if(frame_is_cmd(CTRL_SET) == OK) {
				
					printf("    > [LL] Received SET\n");
				
					cmd = build_cmd(T_TO_R, CTRL_UA, ll.nr);
					
					printf("    > [LL] Sending UA...\n");
	
					if(send_cmd(cmd) != OK) {
						printf("      > Error sending UA...\n");
					}
				}
			}
		}
	}
	
	if(ll.state == TRANSMIT) {		
		return fd;
	}

	return ERROR;
}

int llwrite(char* msg, int size) {

	alarm(0);

	message to_send = build_msg(msg, size);
	
	int attempts = 0;
	set_alarm_flag(1); // for the first if

	while(attempts < ll.max_attempts) {
	
		if(get_alarm_flag()) {

			set_alarm_flag(0);
		
			printf("    > [LL] Sending frame...\n");

			if(send_msg(to_send) != OK) {
				
				printf("      > Could not write frame\n");
			}
			else {
				stats.num_frames_sent++;		
				alarm(ll.timeout);
			
				if(read_frame() == OK) {

					if(frame_is_cmd(CTRL_RR) == OK) {
					
						if( BIT(ll.frame.buf[2], 7) != ll.nr) {
							
							printf("	  > Received RR(next)\n");
							ll.nr = calc_next_nr();
							alarm(0);
							return OK;
						}
						else {
							printf("	  > Received RR(current)\n");
						}
					}
					else if(ll.err != ERROR_NOT_CMD) {
						switch(ll.type) {
							case CTRL_REJ:
							
								if( BIT(ll.frame.buf[2], 7) == ll.nr) {
									printf("	  > REJ received\n");
									stats.num_rej_received++;
								}
								else {
									printf("      > REJ received but not for this frame (something is not quite right...)\n");
								}
								break;
							default:
								break;
						}
					}	
				}
			
				printf("      > [LL] Limit time(%d) exceeded...\n", ll.timeout);
				stats.num_timeouts++;
			}	
			
			attempts++;
			stats.num_frames_resent++;
		}
	}
	
	return ERROR;
}

int llread(char* msg) {

	int done = 0;

	set_alarm_flag(0);

	while(!done){
		
		if(read_frame() == OK) {
		
			if( frame_is_cmd(CTRL_DISC) == OK) {
				ll.state = DISCONNECT;
				return -2;
			}
			
			check_msg();
			stats.num_frames_received++;
			command cmd;
			cmd.length = 0;
			
			switch(ll.resp) {
				case RR_CURRENT:
					cmd = build_cmd(T_TO_R, CTRL_RR, ll.nr);
					break;
					
				case RR_NEXT:
					cmd = build_cmd(T_TO_R, CTRL_RR, calc_next_nr());
					done = 1;
					break;
					
				case REJ:
					printf("      > Sending REJ...\n");
					stats.num_rej_sent++;
					cmd = build_cmd(T_TO_R, CTRL_REJ, ll.nr);
					break;
					
				default:
					break;
			}
			
			if(cmd.length > 0)
				send_cmd(cmd);
				
		}
	}


	ll.nr = calc_next_nr();
	memcpy(msg, _ADDR(ll.frame.buf, 4), ll.frame.length - 2); // copy frame to msg variable
	return (ll.frame.length - HTS); // Number of chars read
}

int check_msg() {

	message temp = destuff(ll.frame);
	
	memcpy(ll.frame.buf, temp.buf, temp.length);
	ll.frame.length = temp.length;
	
	int bcc1_start = 1;
	int bcc1_length = 2;
	
	// check BCC1
	if( check_bcc(ll.frame.buf[BCC1], bcc1_start, bcc1_length) != OK) {
		
		printf("      > Detected BCC1 error (control)\n");
		ll.resp = REJ;
		return ERROR;
	}
	
	int BCC2 = ll.frame.length - 2;
	
	int bcc2_start = 4;
	int bcc2_length = (ll.frame.length - HTS);
	
	// check BCC2
	if( check_bcc(ll.frame.buf[BCC2], bcc2_start, bcc2_length) != OK) {
		
		printf("      > Detected BCC2 error (data)\n");
		
		if(ll.nr == BIT(ll.frame.buf[CC], 6))
			ll.resp = REJ;
		else
			ll.resp = RR_NEXT;
			
		return ERROR;
	}
	
	if(ll.nr == BIT(ll.frame.buf[CC], 6)) {
		ll.resp = RR_NEXT;
	}
	else {
		ll.resp = RR_CURRENT;
	}		
	
	return OK;
}

int check_bcc(char bcc_cmp, int bcc_start, int bcc_length) {

	if (bcc_cmp == get_bcc(_ADDR(ll.frame.buf, bcc_start), bcc_length))
		return OK;
		
	return ERROR;
}

int close_port() {
	
    if ( tcsetattr(ll.fd, TCSANOW, &oldtio) != OK) {
    
		perror("tcsetattr");
		return ERROR;
    }
    
    if(close(ll.fd) != OK) {
    
		perror("close fd");
		return ERROR;
	}
    
    return OK;
}

int llclose() {
	
	ll.state = DISCONNECT;
	
	if(ll.status == TRANSMITTER) {

		alarm(0);

		command cmd = build_cmd(T_TO_R, CTRL_DISC, ll.nr);
		
		int attempts = 0;

		while(attempts < ll.max_attempts) {

			printf("    > [LL] Sending DISC...\n");

			if(send_cmd(cmd) != OK) {

				printf("      > Error sending DISC...\n");
				break;
			}
			
			printf("    > [LL] Reading DISC response...\n");
			
			set_alarm_flag(0);
			alarm(ll.timeout);
			
			if(read_frame() == OK) {

				if(frame_is_cmd(CTRL_DISC) == OK) {
				
					printf("    > [LL] DISC received...\n");
					printf("    > [LL] Sending UA...\n");

					cmd = build_cmd(T_TO_R, CTRL_UA, ll.nr);
				
					if(send_cmd(cmd) != OK) {

						printf("      > Error sending UA...\n");
						break;
					}
			
					ll.state = OVER;
					printf("====STATISTICS====\n");
					printf("\tNumber of [I] frames sent: %d\n", stats.num_frames_sent);
					printf("\tNumber of [I] frames resent: %d\n", stats.num_frames_resent);	
					printf("\tNumber of timeouts: %d\n", stats.num_timeouts);	
					printf("\tNumber of Rej CMD received: %d\n", stats.num_rej_received);	
					break;
				}	
			}
			
			printf("      > [LL] Limit time(%d) exceeded...\n", ll.timeout);
			attempts++;
		}
	}
	else if(ll.status == RECEIVER) {

		alarm(0);

		command cmd;

		set_alarm_flag(0);

		while(TRUE) {
			
			if(read_frame() == OK) {
				if(frame_is_cmd(CTRL_DISC) == OK) {
		
					printf("    > [LL] Received DISC...\n");
					break;
				}
			}
		}

		int attempts = 0;

		while(attempts < ll.max_attempts) {

			cmd = build_cmd(T_TO_R, CTRL_DISC, ll.nr);
			
			printf("    > [LL] Sending DISC...\n");

			send_cmd(cmd);

			set_alarm_flag(0);
			alarm(ll.timeout);

			if(read_frame() == OK) {
				
				if(frame_is_cmd(CTRL_UA) == OK) {
		
					printf("    > [LL] Received UA...\n");
			
					ll.state = OVER;
					break;
				}
			}

			printf("      > [LL] Limit time(%d) exceeded...\n", ll.timeout);
			
			attempts++;
			if(attempts == ll.max_attempts) {
				ll.state = OVER;
			}
		}
	}
	else {
		return ERROR;
	}
	
	if(ll.state == OVER) {

		printf("=== STATISTICS ===\n");
		printf("\tNumber of [I] frames received: %d\n", stats.num_frames_received);
		printf("\tNumber of Rej CMD sent: %d\n", stats.num_rej_sent);
		
		if(close_port() != OK) {
	
			perror("    > [LL] Could not close the serial port\n");
			return ERROR;
		}
	}
	else {
		if(close_port() != OK) {
	
			perror("    > [LL] Could not close the serial port\n");
			return ERROR;
		}
		
		return ERROR;
	}	

	return OK;
} 

int _ctrl(char p) {
	char c = CUT(p);
	if(c == CTRL_SET || c == CTRL_DISC || c == CTRL_UA || c == CTRL_RR || c == CTRL_REJ) {
		return OK;
	}
	return ERROR;
}
