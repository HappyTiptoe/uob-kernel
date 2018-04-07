#include <stdio.h>
#include <stdint.h>

int main(){
	int x;
	int* xptr;
	int num = 0x00001000;
	xptr = &num;
	x = (uint32_t)(&num);

	printf("num: %d\nxptr: %d\nx: %d\n", num, xptr, x);

}