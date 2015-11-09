#include "application.h"

int open_file(const char * filepath, const char * mode) {
	
	FILE * f = fopen(filepath, mode);

	if(f <= 0) {
	
		printf("Could not open file\n");
		return ERROR;
	}

	fstatus.f = f;
	
	return OK;
}

int read_file_size(const char * filepath) {

	if( open_file(filepath, "r+b") != OK) {
		return ERROR;
	}
	
	if( fseek(fstatus.f, 0L, SEEK_END) < 0) {
	
		printf("Could not seek the end of file\n");
		return ERROR;
	}
	
	int size = -1;
	
	if( (size = ftell(fstatus.f)) < 0) {
	
		printf("Could not tell the end of file\n");
		return ERROR;
	}
	
	fclose(fstatus.f);
	
	fstatus.size = size;
	
	return OK;
}

int app_send_data(int sequence_nr, int data_field_size, char* data) {

	int final_size = data_field_size + 4;
	
	char* buf = malloc(final_size);

	char n = (char)((pack.sequence_nr) % 255);
	
	char l2 = (char)(data_field_size / 256);
	char l1 =  (char)(data_field_size % 256);
	
	buf[0] = PACKET_DATA;
	buf[1] = n;
	buf[2] = l2;
	buf[3] = l1;
	
	int i;
	for(i = 0; i < data_field_size; i++) {
		buf[i + 4] = data[i];
	}
	
	if( llwrite(buf, final_size) != OK) {
		printf("> Could not write data packet nr %c\n", n);
		return ERROR;
	}
	
	return OK;
}

int app_send_control(Packet_Control control, Packet_Type type, int file_size) {

	int size = 3 * sizeof(char) + sizeof(int); // type + size + int
	
	unsigned char* buf = malloc(size);
	
	int fsize = file_size;
	unsigned char * tmp;
	tmp  = convert_int32_to_byte(fsize);
	
	buf[0] = control;
	buf[1] = type;
	buf[2] = 4;
	buf[3] = tmp[0];
	buf[4] = tmp[1];
	buf[5] = tmp[2];
	buf[6] = tmp[3];
	
	if(llwrite(buf, size) != OK) {
		printf("Could not send control packet\n");
		return ERROR;
	}
	
	return OK;
}


int app_send(char * port, const char * filepath) {

	printf("> [APP] Connecting...\n");

	// Getting file attributes
	if( read_file_size(filepath) != OK)
		return ERROR;
	
	// Open the file for reading and sending the data packets
	if( open_file(filepath, "r") != OK) {
		printf("> [APP] Error opening input file\n");
		return ERROR;
	}

	int fd = llopen(port, TRANSMITTER);

	if(fd < 0) {
		return ERROR;
	}
	
	printf("> [APP] Connection established...\n");
	
	printf("\n> [APP] Sending start control packet...\n");
	if( app_send_control(PACKET_START, PACKET_FILE_SIZE, fstatus.size) != OK) {
		return ERROR;
	}
	
	pack.sequence_nr = 0;
	int number_of_packets = (fstatus.size / MAX_PACKET_SIZE)
							+ (fstatus.size % MAX_PACKET_SIZE > 0 ? 1 : 0);
	
	char buf[MAX_PACKET_SIZE];
	int res = 0;
	int i;
	for(i = 0; i < number_of_packets; i++) {
	
		printf("\n> [APP] Sending data packet(%d)...\n", pack.sequence_nr);
		
		int read = fread(buf, sizeof(char), MAX_PACKET_SIZE, fstatus.f);
		
		res += read;
				
		if( app_send_data(pack.sequence_nr, read, buf) != OK) {
		
			printf("> [APP] Could not write packet(%d)\n", pack.sequence_nr);	
			fclose(fstatus.f);
			llclose();
			return ERROR;
		}
		
		pack.sequence_nr++;
	}
	printf("\n> [APP] Written bytes: [ %d / %d ]\n", res, fstatus.size);
	
	// Ending connection
	printf("\n> [APP] Sending end control packet...\n");
	if( app_send_control(PACKET_END, PACKET_FILE_SIZE, fstatus.size) != OK) {
		return ERROR;
	}
	
	printf("\n> [APP] Closing connection...\n");
	
	if(fclose(fstatus.f) < 0) {
		printf("  > [APP] Error closing the input file\n");	
		llclose();
		return ERROR;
	}
	
	llclose();

	return OK;
}

int app_check_packet(char * buf, int size) {

	switch(buf[0]) {
	
		case PACKET_START:
			pack.ctrl = PACKET_START;
			pack.sequence_nr = -1;
			if(buf[1] == PACKET_FILE_SIZE) {
				 fstatus.size = convert_byte_to_int32(&(buf[3]), buf[2]);
			}
			
			printf("> [APP] Received start packet\n");
		break;
		
		case PACKET_DATA:
			pack.ctrl = PACKET_DATA;
			if(pack.sequence_nr == buf[1]) {
				printf("> [APP] Received duplicate packet\n");
				break;
			}
			pack.sequence_nr = buf[1];
			pack.data.length = (256 * (unsigned char)buf[2]) + (unsigned char)buf[3];
			
			memcpy(pack.data.buf, &buf[4], pack.data.length);
			
			printf("> [APP] Received data packet(%d) with length(%d)\n", pack.sequence_nr, pack.data.length);
		break;
		
		case PACKET_END:
			pack.ctrl = PACKET_END;
			if(buf[1] == PACKET_FILE_SIZE) {
				 fstatus.size = convert_byte_to_int32(&(buf[3]), buf[2]);
			}
			
			printf("> [APP] Received end packet\n");
		break;
		
		default:
			return ERROR;
	}
	
	return OK;
}

int app_read(char * port) {

	printf("> [APP] Connecting...\n");

	int fd = llopen(port, RECEIVER);
	
	if(fd < 0) {
		return ERROR;
	}
	
	char packet[MAX_FRAME_SIZE];
	fstatus.size = 0;
	fstatus.valid = FALSE;
	int fsize = 0;
	
	printf("> [APP] Connection established...\n");

	printf("> [APP] Receiving...\n");

	do{
		int size = llread(&packet[0]);
		
		if(size == -2) { // Disconnect (forced)
			break;
		}
		else if(size < 0) {
			printf("  > [APP] Some error ocurred reading a packet. Exiting...\n");
			return ERROR;
		}
		else {
			if(app_check_packet(packet, size) != OK){
					printf("  > [APP] Unrecognized packet type\n");
					return ERROR;
			}
			else {
				if(pack.ctrl == PACKET_START) {
					FILE *f = fopen(((fstatus.name == NULL) ? "out" : fstatus.name), "w");
					if( f < 0 ) {	
						printf("> [APP] Error opening output file\n");
						return ERROR;
					}
					fstatus.f = f;
					
					fstatus.valid = TRUE;
				}
				else if(fstatus.valid != TRUE) {
					printf("> [APP] No file info received\n");
					return ERROR;
				}
				else if(pack.ctrl == PACKET_DATA) {
					//writing data to file 
					fwrite(pack.data.buf, sizeof(char), pack.data.length, fstatus.f);
					fsize += pack.data.length;
				}
				else if(pack.ctrl == PACKET_END) {
					//
					break;
				}
			}
		}

	} while((fstatus.size != fsize) || (pack.ctrl != PACKET_END));
	
	printf("> [APP] Bytes read: [ %d / %d ]\n", fsize, fstatus.size);
	
	printf("> [APP] Closing connection...\n");
	
	llclose();

	return OK;
}

