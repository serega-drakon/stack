#include "stack.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#define READ 0
#define WRITE 1
#define HAS_USED 1
#define RESET 2
#define FIXED 2

#define KAN_NUM 1
#define KAN_VALUE 255    //1111.1111
#define POISON_VALUE 255 //1111.1111

#define HASH_SIZE ULLONG_MAX

#define EXIT_MAIN do{ \
if (error_main(ptrStack, READ, BuffForErrNull) == 0)\
    return ptrStack->buffForErr;                    \
else                                                \
    return NULL;                                    \
}while(0)

#define EXIT do{ \
if (ptrStack != NULL && error_main(ptrStack, READ, BuffForErrNull) == 0)\
    return ptrStack->buffForErr;                    \
else                                                \
    return NULL;                                    \
}while(0)
                                                                                //добавить потом продвинутую проверку по пойзонам, а то они есть, но никаких проверок толком нет

/// Structure of stack
struct Stack_ {
    void *data; ///< Pointer to data
    void *buffForErr;///< buffForErr returns if error occurred
    void *buffForRes;///< buffForRes points to result of stack_main() func
    unsigned long long int hash; ///< Hash value of data array //ебать объявление конечно
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

int kanareiyka_check(Stack *ptrStack);

//хеш считается от всего массива кроме канареек
///Returns hash of ptrStack->data
unsigned long long int hash_sedgwick(Stack *ptrStack){
    unsigned long long int hash = 0;
    unsigned int i, a = 31415, b = 27183;
    for(i = KAN_NUM; i < (ptrStack->num + KAN_NUM) * ptrStack->size; i++, a = a * b % HASH_SIZE)
        hash = (a * hash + ((char*)ptrStack->data)[i]) % HASH_SIZE;
    return hash;
}

///Копирует участок помяти по байтам с fromPtr на toPtr
void myMemCpy(const void *toPtr, const void *fromPtr, int sizeInBytes){
    assert(toPtr != NULL);
    assert(sizeInBytes >= 0);
    assert(fromPtr != NULL || sizeInBytes == 0);

    for (int i = 0; i < sizeInBytes; i++)
        ((char *) toPtr)[i] = ((char *) fromPtr)[i];
}

void saveResToBuff(Stack *ptrStack, int x){
    const int shift_in_stack = (x + KAN_NUM) * ptrStack->size;
    const void* from_buf = &((char *) ptrStack->data)[shift_in_stack];
    myMemCpy(ptrStack->buffForRes, from_buf, ptrStack->size);
}

/// Main function of stack array
void *stack_main(Stack *ptrStack, int flag, int x, void *ptrValue) {
    assert(ptrStack != NULL);
    assert(flag != WRITE || ptrValue != NULL);
    assert(flag == READ || flag == WRITE);
    assert(x >= 0);

    if(kanareiyka_check(ptrStack))
        error_main(ptrStack, WRITE, KanareiykePizda);
    if(ptrStack->hash != hash_sedgwick(ptrStack)) {
        error_main(ptrStack, WRITE, HashMismatch);
        stackErrorPrint(ptrStack);
    }
    if(stackErrorCheck(ptrStack)) {
        stackErrorPrint(ptrStack);
        EXIT_MAIN;
    }
    //extend check
    stack_extend(ptrStack, x);
    if (stackErrorCheck(ptrStack)) {
        printf("stack_main: error\n");
        stackErrorPrint(ptrStack);
        EXIT_MAIN;
    }

    switch (flag) {
        case READ:
            //check here needed
            if (!meta_main(ptrStack, READ, x))
                printf("stack_main: using before undefined value X = %d\n", x);
            saveResToBuff(ptrStack, x);
            return ptrStack->buffForRes;
        case WRITE:
            //const int toPtr = &(((char *) ptrStack->data)[(x + KAN_NUM) * ptrStack->size]); //выдает ошибку
            myMemCpy(&(((char *) ptrStack->data)[(x + KAN_NUM) * ptrStack->size]), ptrValue, ptrStack->size);
            meta_main(ptrStack, HAS_USED, x);
            ptrStack->hash = hash_sedgwick(ptrStack);
            saveResToBuff(ptrStack, x);
            return ptrStack->buffForRes;
    }
}

/// Extends given Stack_ by STEP const
void stack_extend(Stack *ptrStack, int x) {
    assert(ptrStack != NULL);
    assert(x >= 0);

    void *buffPtr;

    //Сначала выделяем память под data
    if (x >= ptrStack->num) {
        buffPtr = ptrStack->data;
        x = x * 2; //new number of elements
        ptrStack->data = malloc((x + 2 * KAN_NUM) * ptrStack->size); //+канарейка слева и справа
        //проверка на ошибку
        if (ptrStack->data != NULL) {
            //возвращаем элементы (включил канарейку слева)
            const void *ptrKanRightNew = &((char*)ptrStack->data)[(KAN_NUM + x) * ptrStack->size];//указатель на новую канарейку справа
            const void *ptrKanRightOld = &((char*)buffPtr)[(KAN_NUM + ptrStack->num) * ptrStack->size];//указатель на старую канарейку справа

            myMemCpy(ptrStack->data, buffPtr, (KAN_NUM + ptrStack->num) * ptrStack->size);
            myMemCpy(ptrKanRightNew,ptrKanRightOld, KAN_NUM * ptrStack->size);

            if(!error_main(ptrStack, READ, BuffForErrNull))
                //заполняю пустоты пойзонами
                for(int i = 0; i < (x - ptrStack->num) * ptrStack->size ; i++){
                    ((char*)ptrStack->data)[(KAN_NUM + ptrStack->num) * ptrStack->size + i] = POISON_VALUE;
                }
            ptrStack->num = x;
            free(buffPtr);
        } else {
            error_main(ptrStack, WRITE, DataArrayNull); //отчет об ошибке
            ptrStack->data = buffPtr;
            return;
        }
    }
    //Потом выделяем память для meta
    if (x > ptrStack->metaNum * 8) {
        buffPtr = ptrStack->meta;
        int y;
        for(y = ptrStack->metaNum; y * 8 < x; y *= 2)
            ;
        ptrStack->meta = calloc(y, sizeof(char));
        if (ptrStack->meta != NULL) {
            //возвращаем элементы
            myMemCpy(ptrStack->meta, buffPtr, ptrStack->metaNum);
            ptrStack->metaNum = y;
            free(buffPtr);
        } else {
            error_main(ptrStack, WRITE, MetaNull);
            ptrStack->meta = buffPtr;
            return;
        }
    }

}

///Битовый массив, который содержит информацию об использовании каждого из элементов массива
int meta_main(Stack *ptrStack, int flag, int x) {
    assert(ptrStack != NULL);
    assert(flag == READ || flag == HAS_USED || flag == RESET);

    switch (flag) {
        case READ:
            break;
        case HAS_USED:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] | (1 << (7 - (char) (x % 8)));
            break;
        case RESET:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] & ~(1 << (7 - (char) (x % 8)));
            break;
    }
    return (ptrStack->meta[x / 8] >> (7 - x % 8)) & 1; //достаю нужный бит
}

///Максимум вариантов ошибок - 8 с таким размером error.
int error_main(Stack *ptrStack, int flag, int numOfError) {
    assert(ptrStack != NULL);
    assert(flag == READ || flag == WRITE); //flag == FIXED пока не включаю
    assert(numOfError >= 0);

    switch (flag) {
        case READ:
            break;
        case WRITE:
            ptrStack->error = ptrStack->error | (1 << numOfError);
            break;
        case FIXED: //пока пусть будет, хоть я это нигде и не использовал
            ptrStack->error = ptrStack->error & ~(1 << numOfError);
            break; //мб можно придумать функцию, которая занимается "починкой" этой структуры
    }
    return ptrStack->error >> numOfError & 1; //достаю нужный бит
}

///проверяет канарейки массива и возвращает 1 если они повреждены, 0 если нет.
int kanareiyka_check(Stack *ptrStack){
    int check = 0;
    const int shift = (KAN_NUM + ptrStack->num) * ptrStack->size;

    for(int i = 0; i < KAN_NUM * ptrStack->size; i++)
        if(((unsigned char*)ptrStack->data)[i] != KAN_VALUE
        || ((unsigned char*)ptrStack->data)[shift + i] != KAN_VALUE)
            check = 1;
    return check;
}

///сбрасывает текущую позицию массива до пойзона.
void stack_reset_pos(Stack *ptrStack, int x){
    int i;

    for(i = 0; i < ptrStack->size; i++)
        ((char*)ptrStack->buffForErr)[i] = POISON_VALUE;
    stack_main(ptrStack, WRITE, x, ptrStack->buffForErr);
    meta_main(ptrStack, RESET, ptrStack->pos);
    for(i = 0; i < ptrStack->size; i++)
        ((char*)ptrStack->buffForErr)[i] = 0;
}

///returns !0 if error occurred
int stackErrorCheck(Stack *ptrStack) {
    if (ptrStack != NULL) {
        return ptrStack->error;
    } else
        return 1 << PtrStackNull;
    //ака так бы было если бы переменная error существовала
}


///Выводит инфу об ошибках в консоль.
void stackErrorPrint(Stack *ptrStack){
        if (ptrStack != NULL) {
            if (error_main(ptrStack, READ, DataArrayNull))
                printf("error DataArrayNull\n");
            if (error_main(ptrStack, READ, BuffForErrNull))
                printf("error BuffForErrNull\n");
            if (error_main(ptrStack, READ, BuffForResNull))
                printf("error BuffForResNull\n");
            if (error_main(ptrStack, READ, MetaNull))
                printf("error MetaNull\n");
            if(error_main(ptrStack, READ, HashMismatch))
                printf("error HashMismatch\n");
            if(error_main(ptrStack, READ, KanareiykePizda)){
                printf("error KanareiykePizda\n");
            }
        } else
            printf("error PtrStackNull\n");
};

/// Constructor of stack.
void *stackInit(int size) {
    if (size <= 0)
        return NULL;
    Stack *ptrStack;
    ptrStack = malloc(sizeof(Stack));
    if (ptrStack != NULL) {
        ptrStack->size = size;
        ptrStack->pos = 0;
        ptrStack->metaNum = 0;//FIXME 1
        ptrStack->error = 0;

        ptrStack->buffForRes = malloc(ptrStack->size);
        if (ptrStack->buffForRes == NULL)
            error_main(ptrStack, WRITE, BuffForResNull);
        ptrStack->buffForErr = calloc(ptrStack->size, sizeof(char));
        if (ptrStack->buffForErr == NULL)
            error_main(ptrStack, WRITE, BuffForErrNull);

        ptrStack->data = malloc((2 * KAN_NUM + 1) * ptrStack->size);
        if(ptrStack->data != NULL) {    //заполняем канарейки
            ptrStack->num = 1;
            int i;
            const int shift = (KAN_NUM + ptrStack->num) * ptrStack->size;

            for(i = 0; i < KAN_NUM * ptrStack->size; i++){
                ((char*)ptrStack->data)[i] = KAN_VALUE;
                ((char*)ptrStack->data)[shift + i] = KAN_VALUE;
            }
            for(i = 0; i < ptrStack->size; i++)
                ((char*)ptrStack->data)[KAN_NUM * ptrStack->size + i]= POISON_VALUE;
            ptrStack->meta = calloc(1, sizeof(char));
            ptrStack->metaNum = 1;
            if(ptrStack->meta == NULL)
                error_main(ptrStack, WRITE, MetaNull);
            //считаем хеш
            ptrStack->hash = hash_sedgwick(ptrStack);
        }
        else
            error_main(ptrStack, WRITE, DataArrayNull);
    } else
        printf("stackInit: memory error\n");
    return ptrStack;
}

/// Деструктор стека
void stackFree(Stack *ptrStack){
    if(ptrStack != NULL){
        free(ptrStack->data);
        free(ptrStack->buffForErr);
        free(ptrStack->buffForRes);
        free(ptrStack->meta);
    }
    free(ptrStack);
}

/// Array READ function
void *stack_r(Stack *ptrStack, int x) {
    if (!stackErrorCheck(ptrStack) && x >= 0)
        return stack_main(ptrStack, READ, x, NULL);
    else
        EXIT;
}

/// Array WRITE function
void *stack_w(Stack *ptrStack, int x, void *ptrValue) {
    if (!stackErrorCheck(ptrStack) && ptrValue != NULL && x >= 0)
        return stack_main(ptrStack, WRITE, x, ptrValue);
    else
        EXIT;
}

/// Stack function: Push
void *push(Stack *ptrStack, void *ptrValue) {
    if (!stackErrorCheck(ptrStack) && ptrValue != NULL)
        return stack_main(ptrStack, WRITE, ptrStack->pos++, ptrValue);
    else
        EXIT;
}

/// Stack function: Pop
void *pop(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0) {
        --ptrStack->pos;
        void *buffPtr = malloc(ptrStack->size);
        myMemCpy(buffPtr, stack_main(ptrStack, READ, ptrStack->pos, NULL), ptrStack->size);
        stack_reset_pos(ptrStack, ptrStack->pos);
        return buffPtr;
    }
    else
        EXIT;
}

/// Stack function: GetLast
void *getLast(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0)
        return stack_main(ptrStack, READ, ptrStack->pos - 1, NULL);
    else
        EXIT;
}

