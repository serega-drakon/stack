#ifndef UNTITLED_STACK_H
#define UNTITLED_STACK_H

#define GET(type, ptrStack, x) (*((type*)stack_r(ptrStack, x)))
#define GENERATE_POP(type) type* pop_##type(Stack *ptrStack) { return (type*) pop(ptrStack); }
#define GENERATE_PUSH(type) type* push_##type(Stack *ptrStack, void *ptrValue) { return (type*) push(ptrStack, ptrValue); }
#define GENERATE_CREATE(type) type* create_##type() { return stackInit(sizeof(type)); }

typedef struct Stack_ Stack;

void stackErrorPrint(Stack *ptrStack);
int stackErrorCheck(Stack *ptrStack);
void *stackInit(int size);
void *stack_r(Stack *ptrStack, int x);
void *stack_w(Stack *ptrStack, int x, void *ptrValue);
void *push(Stack *ptrStack, void *ptrValue);
void *pop(Stack *ptrStack);
void *getLast(Stack *ptrStack);
int getsize(Stack *ptrStack);
void stackFree(Stack *ptrStack);

#endif //UNTITLED_STACK_H
