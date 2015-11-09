/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define F 0x7e
#define A 0x03
#define Cr 0x07
#define Cs 0x03
#define UALENGT 8




volatile int STOP=FALSE;

int stateMachine(unsigned char c, int state,char temp[])
{
		switch(state){
			case 0:
				if(c == F)
				{
					temp[state] = c;
					state++;
				}
				break;
			case 1: 
				if(c == A)
				{
					temp[state] = c;
					state++;
				}
				else if(c == F)
						state = 1;
					else
						state = 0;
				break;
			case 2: 
				if(c == Cs)
				{
					temp[state] = c;
					state++;
				}
				else if(c == F)
						state = 1;
					else
						state = 0;
				break;
			case 3:
				if(c == (temp[1]^temp[2]))
				{
					temp[state] = c;
					state++;
				}
				else if(c == F)
						state = 1;
					else
						state = 0;
				break;
			case 4:
				if(c == F)
				{
					temp[state] = c;
					STOP = TRUE;
				}
				else
					state = 0;
				break;
		}
return state;
	
}

int llread(int fd, char * buffer)
{
	char temp[UALENGT];
	int res;
	int state = 0;

	while (STOP==FALSE) 
	{	
      		res = read(fd,buffer,1);
		state = stateMachine(buffer[0],state,temp);
	}

	if(temp[3] != (temp[1]^temp[2]))
		res = -1;
	else
		printf("%x, %x, %x, %x, %x\n", temp[0], temp[1], temp[2],temp[3],temp[4]);

	return res;
}

int llwrite(int fd, char * buffer, int length)
{
	int res;
	tcflush(fd, TCOFLUSH);
	sleep(1);
	res = write(fd, buffer, length);
	sleep(1);

	return res;
}

int main(int argc, char** argv)
{
    int fd, res;
    struct termios oldtio,newtio;
	unsigned char * buffer;
	unsigned char UA[UALENGT]; 

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

      
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    tcgetattr(fd,&oldtio); /* save current port settings */

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0.1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);

    printf("New termios structure set\n");

	res = llread(fd, buffer);
	
	if(res < 0)
	{
		printf("Error recieving msg!");
		exit(1);
	}

	UA[0] = F;
	UA[1] = A;
	UA[2] = 0x08;
	UA[3] = F;
	UA[4] = A;
	UA[5] = Cr;
	UA[6] = A^Cr;
	UA[7] = F;	

	res = llwrite(fd,UA, UALENGT);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}



