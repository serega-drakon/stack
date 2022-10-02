#include <stdio.h>
#include "stack.h"

int main() {
    Stack *ptrStack;
    ptrStack = stackInit(sizeof(int));
    if(stackErrorCheck(ptrStack))
        return 1;

    printf("%d\n", *((int*)stack_r(ptrStack, -1)));
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
    stackFree(ptrStack);
    return 0;
}
