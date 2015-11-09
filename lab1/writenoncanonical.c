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
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd, res = 0;
    struct termios oldtio,newtio;
    char buf[255], temp[255];
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) &&
	      (strcmp("/dev/ttyS4", argv[1])!=0)
	    )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }
    
    printf("Opening...");

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    
    if (fd <0) {perror(argv[1]); exit(-1); }
    
    printf(" [OK]\n");

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCOFLUSH);


    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    
    printf("New setting saved. [OK]\n");
    
    printf("...> ");
    
    gets(buf);
    
    printf("\nSending...\n");

    write(fd, buf, strlen(buf) + 1);
    sleep(1);

    printf("[OK]\n");	

    printf("\nSent: %s \n\n", buf);

    printf("Waiting for response...\n");
    tcflush(fd, TCIFLUSH);
    strcpy(buf, "");
    strcpy(temp, "");

    while (STOP==FALSE) {       /* loop for input */
      res = read(fd,buf,255);   /* returns after 1 chars have been input */
      strncat(temp,buf,res);
      if(buf[res-1] == '\0')
      	STOP = TRUE;
      else
      	buf[res]=0;               /* so we can printf... */
		
    }

    printf("> %s\n", temp);	

    printf("[OK]\n\n");

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);

    return 0;
}
