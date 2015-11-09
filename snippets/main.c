#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BIT(v,b)	((v >> b) & 1)

#define ADDRESS(x, b) (&(x[b]))

// ==========================================
// HELPERS
// ==========================================
unsigned char * convert_int32_to_byte(int w) {

	unsigned char * b = malloc(4);

	int i;
	for(i = 3; i >= 0 ; i--) {
		b[i] = (char) (w & 0xFF);
		w >>= 8;
	}
	
	return b;

}

int convert_byte_to_int32(unsigned char * b) {

	int res = 0;

	int i;
	for(i = 0; i < 4; i++) {
		res |= b[i] << ( 8 * ( 3 - i ) );
	}

	return res;
}

char get_bcc(char * buf, int size) {

    char result = 0;
    int i;
	
    for (i = 0; i < size; ++i) {
        result ^= buf[i];
    }
    
    return result;
}

int main(int argc, char** argv) {

	char c[3] = { 0x07, 0x05, (0x07 ^ 0x05) };  // 0000 1000
	
	
	printf("%x vs %x\n", c[2], get_bcc(&c[0], 2));
	

	return 0;

}
