#include <stdio.h>
#include "stack.h"

GENERATE_CREATE(int)  //этот макрос должен возвращать не int* (у меня компилятор жэто сразу показал)

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
        //c = getchar();
        scanf("%d", ptrValue); //scanf удобная функция
        printf("push: %d\n", *push_int(ptrStack, ptrValue));
    }
    printf("\n|%c, size of stack: %d|\n", GET(int, ptrStack, 1), getsize(ptrStack)); //считать размер с нуля это прям сильно

    for(int i = 0; i < 10; i++){
        printf("%d\n", *pop_int(ptrStack));
    }
    getchar();
    stackFree(ptrStack);
    return 0;
}
