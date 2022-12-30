#include <stdio.h>

int main(int argc, char** argv){
    for(int i = 0; i < 16; i++)
    {
        printf("__attribute__ ((interrupt))  void isr%d(interruptFrame* frame){isrs[%d] = true;sendEOI(%d);}\n", i, i, i);
    }

    for(int i = 0; i < 32; i++){
        printf("__attribute__ ((interrupt))  void exc%d(interruptFrame* frame){printf(exceptions[%d]);}\n", i, i);
    }
    return 0;
}