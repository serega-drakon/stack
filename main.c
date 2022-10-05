#include <stdio.h>
#include "stack.h"

GENERATE_CREATE(int)

GENERATE_POP(int)

GENERATE_PUSH(int)

int main() {  //debug
    Stack *ptrStack = create_int();
    if(stackErrorCheck(ptrStack))
        return 1;

    printf("%d\n", *((int*)stack_r(ptrStack, -1)));
    int c;
    int *ptrValue = &c;
    for(int i = 0; i < 10; i++){
        c = getchar();
        push_int(ptrStack, ptrValue);
    }
    printf("\n|%c, size of stack: %d|\n", GET(int, ptrStack, 1), getsize(ptrStack));

    for(int i = 0; i < 10; i++){
        printf("%c", *pop_int(ptrStack));
    }
    getchar();
    stackFree(ptrStack);
    return 0;
}
