#include "stdio.h"

int main(void) {


	int i;

	for(i = 0; i < 64; i = i+4) {

		printf("0x%06X \n", i);

	}
}
