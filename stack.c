#include "stack.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define READ 0
#define WRITE 1
#define HAS_USED 1
#define RESET 2
#define STEP 10

#define KAN_SIZE 1

enum Errors { //Не больше 8 ошибок! иначе надо расширять переменную error
    PtrStackNull = 0, //number of right bit in error
    DataArrayNull,
    BufferNull,
    MetaNull,
};

/** Structure of stack\n
 * Думаю на канарейки выделю по одному элементу размера size слева и справа */
struct Stack_ {
    void *data; ///< Pointer to data
    void *buffer;///< buffer returns if error occurred
    int size; ///< Size of one element of data in bytes
    int num; ///< Number of elements of data (malloced memory)
    int pos; ///< Next free position of stack (pop/push/getlast)
    unsigned char *meta; ///< "Poison" check of data
    int metaNum; ///< Number of elements of meta (malloced memory)
    unsigned char error;///< is an array of bools
};

int error_main(Stack *ptrStack, int flag, int numOfError);

int meta_main(Stack *ptrStack, int flag, int x);

void stack_extend(Stack *ptrStack, int x);

void myMemCpy(void *toPtr, void *fromPtr, int sizeInBytes){
    assert(toPtr != NULL);
    assert(sizeInBytes >= 0);
    assert(fromPtr != NULL || sizeInBytes == 0);

    for (int i = 0; i < sizeInBytes; i++)
        ((char *) toPtr)[i] = ((char *) fromPtr)[i];
}

/** Main function of stack array
 * @param ptrStack Pointer to stack
 * @param flag READ or WRITE
 * @param x Position to operate
 * @param ptrValue Pointer to value of (ptrStack->size) size */
void *stack_main(Stack *ptrStack, int flag, int x, void *ptrValue) {
    assert(ptrStack != NULL); //ассерты имба//ассерты имба//ассерты имба//ассерты имба//ассерты имба//ассерты имба//ассерты имба//ассерты имба
    assert(flag != WRITE || ptrValue != NULL);//формулу получил из упрощения логического выражения
    assert(flag == READ || flag == WRITE);
    assert(x >= 0);

    stack_extend(ptrStack, x);//все проверки внутри этой ф.
    if (stackErrorCheck(ptrStack)) {
        printf("stack_main: memory error\n");
        if (error_main(ptrStack, READ, BufferNull) == 0)
            return ptrStack->buffer;
        else
            return NULL;//возвращает нулл в крайнем случае
    }

    switch (flag) {
        case READ:
            //check here needed
            if (!meta_main(ptrStack, READ, x))
                printf("stack_main: using before undefined value X = %d\n", x);
            return &((char *) ptrStack->data)[x * ptrStack->size];
        case WRITE:
            myMemCpy(&(((char *) ptrStack->data)[x * ptrStack->size]), ptrValue, ptrStack->size);
            meta_main(ptrStack, HAS_USED, x);
            return &((char *) ptrStack->data)[x * ptrStack->size];
    }
}

/** Extends given Stack_ by STEP const \n
 * (ptrStack->data == Null || ptrStack->meta == NULL) if memory error occurred \n */
void stack_extend(Stack *ptrStack, int x) {
    assert(ptrStack != NULL);
    assert(x >= 0);

    void *buffArray = ptrStack->data; ///< Saves previous pointer's value
    int i; ///< Counter

    //Сначала выделяем память под data
    if (x >= ptrStack->num) {
        x = x + 1 + (STEP - (x + 1) % STEP);///< new number of elements
        ptrStack->data = malloc(x * ptrStack->size);
        //проверка на ошибку
        if (ptrStack->data != NULL) {
            //возвращаем элементы
            myMemCpy(ptrStack->data, buffArray, ptrStack->num * ptrStack->size);
            ptrStack->num = x;
            free(buffArray);
        } else
            error_main(ptrStack, WRITE, DataArrayNull); //отчет об ошибке
    }
    //Потом выделяем память для meta
    if (x > ptrStack->metaNum * 8) {
        x = x / 8 + (x % 8 == 0 ? 0 : 1);
        buffArray = ptrStack->meta;
        ptrStack->meta = calloc(x, sizeof(char)); //калок чтобы нулики везде были
        //проверка на ошибку
        if (ptrStack->meta != NULL) {
            //возвращаем элементы
            myMemCpy(ptrStack->meta, buffArray, ptrStack->metaNum);
            ptrStack->metaNum = x;
            free(buffArray);
        } else
            error_main(ptrStack, WRITE, MetaNull);
    }

}

/** "Poison" check function */
int meta_main(Stack *ptrStack, int flag, int x) {
    assert(ptrStack != NULL);
    assert(flag == READ || flag == HAS_USED || flag == RESET);

    switch (flag) {
        case READ:
            return (ptrStack->meta[x / 8] >> (7 - x % 8)) & 1; //достаю нужный бит
        case HAS_USED:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] | (1 << (7 - (char) (x % 8)));
            return 1;
        case RESET:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] & ~(1 << (7 - (char) (x % 8)));
            return 0;
    }
}

///максимум вариантов ошибок - 8 с таким размером error \n
///идея такова что мы храним ака массив булов, которые содержат информацию о том, была ли совершена ошибка
int error_main(Stack *ptrStack, int flag, int numOfError) { //заметим что сброса ошибок нет
    assert(ptrStack != NULL);
    assert(flag == READ || flag == WRITE);
    assert(numOfError >= 0);

    switch (flag) {
        case READ:
            return ptrStack->error >> numOfError & 1; //достаю нужный бит
        case WRITE:
            ptrStack->error = ptrStack->error | (1 << numOfError);
            return 1;
    }
}

/** Checks pointers for NULL value \n*/
///returns !0 if error occurred and prints messages about
int stackErrorCheck(Stack *ptrStack) {
    if (ptrStack != NULL) {
        if (error_main(ptrStack, READ, DataArrayNull))
            printf("error DataArrayNull\n");
        if (error_main(ptrStack, READ, BufferNull))
            printf("error BufferNull\n");
        if (error_main(ptrStack, READ, MetaNull))
            printf("error MetaNull\n");

        return ptrStack->error;
    } else
        return 1;
    //ака так бы было если бы переменная error существовала
}

/** Constructor of stack. \n
 * Outputs "memory error" to command line if error occurred */
void *stackInit(int size) {
    if (size <= 0)
        return NULL;
    Stack *ptrStack;
    ptrStack = malloc(sizeof(Stack));
    if (ptrStack != NULL) {
        ptrStack->size = size;
        ptrStack->num = 0;
        ptrStack->pos = 0;
        ptrStack->metaNum = 0;
        ptrStack->meta = NULL;
        ptrStack->buffer = calloc(ptrStack->size, sizeof(char));//буфер из нулей, вместо NULL))
        if (ptrStack->buffer == NULL)
            error_main(ptrStack, WRITE, BufferNull);
        stack_extend(ptrStack, 0);
    } else
        printf("stackInit: memory error\n");
    stackErrorCheck(ptrStack);
    return ptrStack;
}

/** Array READ function \n
 * короче рассказываю анекдот:\n
 * Знаете какой наркотик гомосексуалисты предпочитают больше всего?\n
 * \n
 * \n
 * \n - кокс */
void *stack_r(Stack *ptrStack, int x) {
    if (!stackErrorCheck(ptrStack) && x >= 0)
        return stack_main(ptrStack, READ, x, NULL);
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BufferNull) == 0)
        return ptrStack->buffer;
    else
        return NULL;
}

/** Array WRITE function \n
 * @param ptrStack - pointer to stack struct
 * @param x - position in which value will be written
 * @param ptrValue - pointer to value */
void *stack_w(Stack *ptrStack, int x, void *ptrValue) {
    if (!stackErrorCheck(ptrStack) && ptrValue != NULL && x >= 0)
        return stack_main(ptrStack, WRITE, x, ptrValue);
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BufferNull) == 0)
        return ptrStack->buffer;
    else
        return NULL;
}

/** Stack function: Push \n
 * Adds a new element to the end of the stack
 * @param ptrStack - pointer to stack struct
 * @param ptrValue - pointer to value
 * @warning Value form ptrValue is needed to be the same size as one of stack's element */
void *push(Stack *ptrStack, void *ptrValue) {
    if (!stackErrorCheck(ptrStack) && ptrValue != NULL)
        return stack_main(ptrStack, WRITE, ptrStack->pos++, ptrValue);
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BufferNull) == 0)
        return ptrStack->buffer;
    else
        return NULL;
}

/** Stack function: Pop \n
 * Gets a new element from the end of the stack
 * @param ptrStack - pointer to stack struct */
void *pop(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0) {
        void *buffPtr = stack_main(ptrStack, READ, --ptrStack->pos, NULL); ///< Saves result's pointer
        meta_main(ptrStack, RESET, ptrStack->pos); ///< resets that position
        return buffPtr;
    } else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BufferNull) == 0)
        return ptrStack->buffer;
    else
        return NULL;
}

/** Stack function: GetLast \n
 * Gets a last element of stack
 * @param ptrStack - pointer to stack struct */
void *getLast(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0)
        return stack_main(ptrStack, READ, ptrStack->pos - 1, NULL);
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BufferNull) == 0)
        return ptrStack->buffer;
    else
        return NULL;
}

