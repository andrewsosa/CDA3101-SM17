#include "stdio.h"

int main(void) {

	char buffer[80];
	while(fgets(buffer, 80, stdin) != NULL) {

		char a[20];
		char b[20];

		// hi sosa,andrew

		if(sscanf(buffer, "hi %[^,]%s", a, b) == 2) 
			printf("b: %s \n", b);
		else
			printf("nope \n");

	}


	return 0;

}
