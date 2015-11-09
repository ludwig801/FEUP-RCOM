#include "helpers.h"

unsigned char * convert_int32_to_byte(int w) {

	unsigned char * b = malloc(4);

	int i;
	for(i = 3; i >= 0 ; i--) {
		b[i] = (char) (w & 0xFF);
		w >>= 8;
	}
	
	return b;

}

int convert_byte_to_int32(unsigned char * b, int size) {

	int res = 0;

	int i;
	for(i = 0; i < size; i++) {
		res |= b[i] << ( 8 * ( size - (i+1)));
	}

	return res;
}

int get_baudrate(int baudrate) {

	switch (baudrate)
	{
		case 1200: return 0xB1200;
		case 1800: return 0xB1800;
		case 2400: return 0xB2400;
		case 4800: return 0xB4800;
		case 9600: return 0xB9600;
		case 19200: return 0xB19200;
		case 38400: return 0xB38400;
		default:
			return -1;
	}
}

