/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

// lab2 constants
#define F 0x7e
#define A 0x03
#define C 0x03
#define SETLEN 5
#define TIMEOUT 3
#define ATTEMPTS 3

volatile int STOP = FALSE;

int flag = 1,count = 0;


// argv : /dev/ttyS4

void writeMsg(int * fd) {

	printf("Sending signal... ");
	
	tcflush(*fd, TCOFLUSH); // Clean output buffer

    unsigned char SET[SETLEN];
    SET[0] = F;
    SET[1] = A;
    SET[2] = C;
    SET[3] = SET[1] ^ SET[2];
    SET[4] = F;
    
    write(*fd, SET, SETLEN);

    printf("[OK]\n");	
    
}

int readResponse(int * fd) {

 	// **********************
    // READ RESPONSE
    // **********************
    int res;
    char buf[SETLEN];
	char temp[SETLEN];
    
    tcflush(*fd, TCIFLUSH);

	int i = 0;
	printf("Reading UA response... \n");
    while (STOP==FALSE && !flag) /* loop for input */
    {      
		res = read(*fd,buf,1);   /* returns after 1 chars have been input */
		if(res != 0) {
			temp[i] = buf[0];
			if(temp[i] == F && i!=0) {
				STOP = TRUE;
			}
			else {
				i++;
			}
		}
    }
    
    if(flag) return -1;

	if(temp[3] != (temp[1]^temp[2]))
	{
		printf("Error recieving msg!");
		//exit(1);
		return -1;
	}
	printf("[OK]\n");

	printf("%x, %x, %x, %x, %x\n", temp[0], temp[1], temp[2],temp[3],temp[4]);	
	
	return 0;
	
}

void configure(int * fd, char * serial_port, struct termios * oldtio) {

	struct termios newtio;

	printf("Opening...");

    *fd = open(serial_port, O_RDWR | O_NOCTTY );
    
    if (*fd <0) {perror(serial_port); exit(-1); }
    
    printf(" [OK]\n");

    if ( tcgetattr(*fd,oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }
    
    printf("Current setting saved. [OK]\n");

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0.1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */


	printf("Saving new settings... ");
    if ( tcsetattr(*fd,TCSANOW,&newtio) == -1)
    {
      perror("tcsetattr");
      exit(-1);
    }
    
    printf("[OK]\n");
	
}

void resetConfiguration(int * fd, struct termios * oldtio) {

	printf("Restoring default setting...");
	
    if ( tcsetattr(*fd,TCSANOW,oldtio) == -1)
    {
      perror("tcsetattr");
      exit(-1);
    }
    
    printf("[OK]\n");
    close(*fd);
    
    printf("Goodbye.\n\n");
    
}

void triggerAlarm() {
	flag = 1;
	count++;
	printf("Timeout Expired: %ds\n", TIMEOUT);
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) &&
	      (strcmp("/dev/ttyS4", argv[1])!=0)))
	    {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
    }
    
    printf("\n\n ============== \n SIGNAL SENDER \n ============== \n\n");
    

    configure(&fd, argv[1], &oldtio);
    
    (void) signal(SIGALRM, triggerAlarm);  // instala  rotina que atende interrupcao
    // Resend
    while(count < ATTEMPTS) {
    	if(flag) {
    	    alarm(TIMEOUT);
    	    printf("\nAttempts remaining: %d \n", (ATTEMPTS - count - 1));
    		writeMsg(&fd);
    		flag = 0;
    		if(readResponse(&fd) == 0) {
    			printf("-> Response received with success!\n");
    			break;
    		}
    	}
    }
    
    
    resetConfiguration(&fd, &oldtio);


    return 0;
}
