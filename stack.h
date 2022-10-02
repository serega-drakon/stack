#ifndef UNTITLED_STACK_H
#define UNTITLED_STACK_H

///Structure Stack_ type
typedef struct Stack_ Stack;

int stackErrorCheck(Stack *ptrStack);
void *stackInit(int size);
void *stack_r(Stack *ptrStack, int x);
void *stack_w(Stack *ptrStack, int x, void *ptrValue);
void *push(Stack *ptrStack, void *ptrValue);
void *pop(Stack *ptrStack);
void *getLast(Stack *ptrStack);
void stackFree(Stack *ptrStack);

unsigned long long int hash_sedgwick(Stack *ptrStack); //debug


#endif //UNTITLED_STACK_H
