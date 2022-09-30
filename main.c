#include <stdio.h>
#include <stdlib.h>
#include "stack.h"


int main() {
    Stack *ptrStack;
    ptrStack = stackInit(sizeof(int));
    if(stackMemErr(ptrStack))
        return 1;

    int c;
    int *ptrValue = &c;
    for(int i = 0; i < 10; i++){
        c = getchar();
        push(ptrStack, ptrValue);
    }
    for(int i = 0; i < 10; i++){
        printf("%c", *((int*)pop(ptrStack)));
    }
    getchar();

    return 0;
}
