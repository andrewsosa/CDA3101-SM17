#include "stdio.h"

#define BUFFER_SIZE 80

int main(void) {


	char buffer[BUFFER_SIZE];

	while(fgets(buffer, BUFFER_SIZE, stdin) != NULL)
		printf("hello");

	return 0;

}
