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
#define STEP 10

#define KAN_NUM 1
#define KAN_VALUE 255    //1111.1111
#define POISON_VALUE 255 //1111.1111

#define HASH_SIZE ULLONG_MAX
                                                                                //добавить потом продвинутую проверку по пойзонам, а то они есть, но никаких проверок толком нету
enum Errors { //Не больше 8 ошибок! иначе надо расширять переменную error
    PtrStackNull = 0, //number of right bit in error
    DataArrayNull,
    BuffForErrNull,
    BuffForResNull,
    MetaNull,
    KanareiykePizda,
    HashMismatch,
};

/** Structure of stack\n
 * Думаю на канарейки выделю по одному элементу размера size слева и справа */
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

//хеш считается от всего массива кроме канареек
unsigned long long int hash_sedgwick(Stack *ptrStack){ //returns hash of ptrStack->data
    unsigned long long int hash = 0;
    unsigned int i, a = 31415, b = 27183;
    for(i = KAN_NUM; i < (ptrStack->num + KAN_NUM) * ptrStack->size; i++, a = a * b % HASH_SIZE)
        hash = (a * hash + ((char*)ptrStack->data)[i]) % HASH_SIZE;
    return hash;
}

void myMemCpy(void *toPtr, void *fromPtr, int sizeInBytes){
    assert(toPtr != NULL);
    assert(sizeInBytes >= 0);
    assert(fromPtr != NULL || sizeInBytes == 0);

    for (int i = 0; i < sizeInBytes; i++)
        ((char *) toPtr)[i] = ((char *) fromPtr)[i];
}

void saveResToBuff(Stack *ptrStack, int x){
    myMemCpy(ptrStack->buffForRes, &((char *) ptrStack->data)[(x + KAN_NUM) * ptrStack->size], ptrStack->size);
}

/** Main function of stack array
 * @param ptrStack Pointer to stack
 * @param flag READ or WRITE
 * @param x Position to operate
 * @param ptrValue Pointer to value of (ptrStack->size) size */
void *stack_main(Stack *ptrStack, int flag, int x, void *ptrValue) {
    assert(ptrStack != NULL); //ассерты имба
    assert(flag != WRITE || ptrValue != NULL);//формулу получил из упрощения логического выражения
    assert(flag == READ || flag == WRITE);
    assert(x >= 0);

    //hash check
    if(ptrStack->hash != hash_sedgwick(ptrStack)) {
        error_main(ptrStack, WRITE, HashMismatch);
    }

    stack_extend(ptrStack, x);//все проверки внутри этой ф.
    if (stackErrorCheck(ptrStack)) {
        printf("stack_main: error\n");
        if (error_main(ptrStack, READ, BuffForErrNull) == 0)
            return ptrStack->buffForErr;
        else
            return NULL;//возвращает нулл в крайнем случае
    }

    switch (flag) {
        case READ:
            //check here needed
            if (!meta_main(ptrStack, READ, x))
                printf("stack_main: using before undefined value X = %d\n", x);
            saveResToBuff(ptrStack, x);
            return ptrStack->buffForRes;
        case WRITE:
            myMemCpy(&(((char *) ptrStack->data)[(x + KAN_NUM) * ptrStack->size]), ptrValue, ptrStack->size);
            meta_main(ptrStack, HAS_USED, x);
            ptrStack->hash = hash_sedgwick(ptrStack);
            saveResToBuff(ptrStack, x);
            return ptrStack->buffForRes;
    }
}

/** Extends given Stack_ by STEP const \n
 * (ptrStack->data == Null || ptrStack->meta == NULL) if memory error occurred \n */
void stack_extend(Stack *ptrStack, int x) {
    assert(ptrStack != NULL);
    assert(x >= 0);

    void *buffPtr; ///< Saves previous pointer's value

    //Сначала выделяем память под data
    if (x >= ptrStack->num) {
        buffPtr = ptrStack->data;
        x = x + 1 + (STEP - (x + 1) % STEP);///< new number of elements   //заменить на степень двойки для O(n) (в пределе num -> inf)
        ptrStack->data = malloc((x + 2 * KAN_NUM) * ptrStack->size); //+канарейка слева и справа
        //проверка на ошибку
        if (ptrStack->data != NULL) {
            //возвращаем элементы (включил канарейку слева)
            myMemCpy(ptrStack->data, buffPtr, (KAN_NUM + ptrStack->num) * ptrStack->size);
            myMemCpy(&((char*)ptrStack->data)[(KAN_NUM + x) * ptrStack->size], //указатель на канарейку справа
                     &((char*)buffPtr)[(KAN_NUM + ptrStack->num) * ptrStack->size], //указатель на другую канарейку справа
                     KAN_NUM * ptrStack->size);//размер

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
        }
    }
    //Потом выделяем память для meta
    if (x > ptrStack->metaNum * 8) {
        buffPtr = ptrStack->meta;
        x = x / 8 + (x % 8 == 0 ? 0 : 1);
        ptrStack->meta = calloc(x, sizeof(char)); //калок чтобы нулики везде были
        //проверка на ошибку
        if (ptrStack->meta != NULL) {
            //возвращаем элементы
            myMemCpy(ptrStack->meta, buffPtr, ptrStack->metaNum);
            ptrStack->metaNum = x;
            free(buffPtr);
        } else {
            error_main(ptrStack, WRITE, MetaNull);
            ptrStack->meta = buffPtr;
        }
    }

}

/** "Poison" check function */
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

///Максимум вариантов ошибок - 8 с таким размером error. \n
///Идея такова что мы храним ака массив булов, которые содержат информацию о том, была ли совершена конкретная ошибка. \n
///И если мы захотим решить проблему точечно, а не пересоздавать полностью структуру, то мы можем это сделать. \n
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

int kanareiyka_check(Stack *ptrStack){
    int check = 0;
    for(int i = 0; i < KAN_NUM * ptrStack->size; i++)
        if(((unsigned char*)ptrStack->data)[i] != KAN_VALUE
        || ((unsigned char*)ptrStack->data)[(KAN_NUM + ptrStack->num) * ptrStack->size + i] != KAN_VALUE)
            check = 1;
    return check;
}

void stack_reset_pos(Stack *ptrStack, int x){
    //мб тут можно сделать лучше, но я оч задолбался
    int i;
    for(i = 0; i < ptrStack->size; i++)
        ((char*)ptrStack->buffForErr)[i] = POISON_VALUE;
    stack_main(ptrStack, WRITE, x, ptrStack->buffForErr);
    meta_main(ptrStack, RESET, ptrStack->pos);
    for(i = 0; i < ptrStack->size; i++)
        ((char*)ptrStack->buffForErr)[i] = 0;
}

/** Checks pointers for NULL value \n*/
///returns !0 if error occurred and prints messages about
int stackErrorCheck(Stack *ptrStack) {
    if (ptrStack != NULL) {
        //вывод инфы об ошибках
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
        //проверка канареек
        if(kanareiyka_check(ptrStack)){
            error_main(ptrStack, WRITE, KanareiykePizda);
            printf("error KanareiykePizda\n");
        }
        return ptrStack->error;
    } else
        return 1 << PtrStackNull;
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
        ptrStack->error = 0;
        ptrStack->buffForRes = malloc(ptrStack->size);//буфер из нулей, вместо NULL))
        if (ptrStack->buffForRes == NULL)
            error_main(ptrStack, WRITE, BuffForResNull);
        ptrStack->buffForErr = calloc(ptrStack->size, sizeof(char));//буфер из нулей, вместо NULL))
        if (ptrStack->buffForErr == NULL)
            error_main(ptrStack, WRITE, BuffForErrNull);
        ptrStack->data = malloc(KAN_NUM * ptrStack->size);
        //заполняем канарейки
        if(ptrStack->data != NULL) {
            for(int i = 0; i < KAN_NUM * ptrStack->size; i++){
                ((char*)ptrStack->data)[i] = KAN_VALUE;
                ((char*)ptrStack->data)[(KAN_NUM + ptrStack->num) * ptrStack->size + i] = KAN_VALUE;
            }
            //считаем хеш
            ptrStack->hash = hash_sedgwick(ptrStack);
        }
        else
            error_main(ptrStack, WRITE, DataArrayNull);
    } else
        printf("stackInit: memory error\n");
    stackErrorCheck(ptrStack);
    return ptrStack;
}

void stackFree(Stack *ptrStack){
    if(ptrStack != NULL){
        free(ptrStack->data); //внутри free() есть проверка на NULL
        free(ptrStack->buffForErr);
        free(ptrStack->buffForRes);
        free(ptrStack->meta);
    }
    free(ptrStack);
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

    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BuffForErrNull) == 0)
        return ptrStack->buffForErr;
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
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BuffForErrNull) == 0)
        return ptrStack->buffForErr;
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
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BuffForErrNull) == 0)
        return ptrStack->buffForErr;
    else
        return NULL;
}

/** Stack function: Pop \n
 * Gets a new element from the end of the stack
 * @param ptrStack - pointer to stack struct */
void *pop(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0) {
        --ptrStack->pos;
        void *buffPtr = malloc(ptrStack->size);
        myMemCpy(buffPtr, stack_main(ptrStack, READ, ptrStack->pos, NULL), ptrStack->size);
        stack_reset_pos(ptrStack, ptrStack->pos);///< resets that position
        return buffPtr;
    }
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BuffForErrNull) == 0)
        return ptrStack->buffForErr;
    else
        return NULL;
}

/** Stack function: GetLast \n
 * Gets a last element of stack
 * @param ptrStack - pointer to stack struct */
void *getLast(Stack *ptrStack) {
    if (!stackErrorCheck(ptrStack) && ptrStack->pos > 0)
        return stack_main(ptrStack, READ, ptrStack->pos - 1, NULL);
    else if (stackErrorCheck(ptrStack) != 1 && error_main(ptrStack, READ, BuffForErrNull) == 0)
        return ptrStack->buffForErr;
    else
        return NULL;
}

