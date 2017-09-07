#include "stdio.h"

int main(int argc, char const *argv[]) {

    // int i;
    // for(i = 0; i < argc; ++i) {
    //     printf("arg %d: %s\n", i, argv[i]);
    // }


    // $ ./a.out -b 16 -s 64 -n 2 < test.trace

    int i,b;
    for(i = 0; i < argc; ++i) {
        if(strcmp("-b", argv[i] == 0)) { // if argv[i] is "-b"
             b = atoi(argv[i+1]);
        }
    }

    return 0;
}
