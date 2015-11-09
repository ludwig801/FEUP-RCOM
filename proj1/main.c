#include "application.h"

#define PORT	1
#define TYPE	2
#define BAUD_R	3
#define BAUD_T	4
#define FILE	3
#define TIME	5

const int DEFAULT = -1;

void print_usage() {
	printf("Usage (Receiver)   : [/dev/ttySX] [R] ([baudrate])\n");
	printf("Usage (Transmitter): [/dev/ttySX] [T] [filepath] ([baudrate]) ([timeout])\n");
}

int main(int argc, char *argv[])
{
    if ( (argc < 3) || 
  	     ((strcmp("/dev/ttyS0", argv[PORT]) != 0) && 
  	      (strcmp("/dev/ttyS1", argv[PORT]) != 0) &&
	      (strcmp("/dev/ttyS4", argv[PORT]) != 0)))
	    {
	    
	    print_usage();
		
		return ERROR;
    }
    
    switch(argc) {
    
    	case 3:
    		if(strcmp(argv[TYPE], "R") == 0) { // Receiver
    		
				ll_set_params(DEFAULT, DEFAULT);
				app_read(argv[PORT]);
			}
			else if(strcmp(argv[TYPE], "T") == 0) { // Transmitter
			
				print_usage();
				return ERROR;
			}
    	break;
    	
    	case 4:
    		if(strcmp(argv[TYPE], "R") == 0) { // Receiver
    		
    			int baudrate = get_baudrate(atoi(argv[BAUD_R]));
				ll_set_params(baudrate, DEFAULT);
				app_read(argv[PORT]);
			}
			else if(strcmp(argv[TYPE], "T") == 0) { // Transmitter
			
				ll_set_params(DEFAULT, DEFAULT);
				app_send(argv[PORT], argv[FILE]);
			}
    	break;
    	
    	case 5:
    		if(strcmp(argv[TYPE], "R") == 0) { // Receiver
    		
    			print_usage();
				return ERROR;
			}
			else if(strcmp(argv[TYPE], "T") == 0) { // Transmitter
			
				int baudrate = get_baudrate(atoi(argv[BAUD_T]));
				ll_set_params(baudrate, DEFAULT);
				
				app_send(argv[PORT], argv[FILE]);
			}
    	break;
    	
    	case 6:
    		if(strcmp(argv[TYPE], "R") == 0) { // Receiver
    		
    			print_usage();
				return ERROR;
			}
			else if(strcmp(argv[TYPE], "T") == 0) { // Transmitter
			
				int baudrate = get_baudrate(atoi(argv[BAUD_T]));
				int timeout = atoi(argv[TIME]);
				ll_set_params(baudrate, timeout);
				
				app_send(argv[PORT], argv[FILE]);
			}
    	break;
    }

	printf("\n== Terminated ==\n\n"); 

    return OK;
}
