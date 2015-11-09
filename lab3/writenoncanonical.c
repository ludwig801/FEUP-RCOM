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

#define FALSE 0
#define TRUE 1

const int setlen = 5;
volatile int tries = 3;
volatile int timeout = 3;

volatile int STOP = FALSE;

int flag = 1,count = 0;


// argv : /dev/ttyS4

int llwrite(int fd, char * buffer, int length) {

	printf("Sending signal... ");
	
	tcflush(fd, TCOFLUSH); // Clean output buffer

    write(*fd, msg, length);

    printf("[OK]\n");
}

int readResponse(const int * fd) {

 	// **********************
    // READ RESPONSE
    // **********************
    int res;
    char buf[setlen];
	char temp[setlen];
    
    tcflush(*fd, TCIFLUSH);

	int i = 0;
	printf("Reading UA response... \n");
    while (STOP==FALSE && !flag) /* loop for input */
    {      
		res = read(*fd,buf,1);   /* returns after 1 chars have been input */
		if(res != 0)
		{
			temp[i] = buf[0];
			if(temp[i] == F && i!=0)
			{
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

void triggerAlarm()
{
	flag = 1;
	count++;
}

void installAlarm()
{
	// instala  rotina que atende interrupcao
	(void) signal(SIGALRM, triggerAlarm);
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio;
    
    struct link_layer link;
    
    link.port = "/dev/ttyS0";
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1]) != 0) && 
  	      (strcmp("/dev/ttyS1", argv[1]) != 0) &&
	      (strcmp("/dev/ttyS4", argv[1]) != 0)))
	    {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
    }
    
    printf("\n\n ============== \n SIGNAL SENDER \n ============== \n\n");
    

    configure(&fd, argv[1], &oldtio);
    
    installAlarm();
    
    if(llopen(&fd, tries, timeout) == 0) {
    	// Fazemos as merdas
    }
    
    return llclose(&fd);
}
