#include <stdio.h>
#include <stdlib.h>

///Structure Stack_ type
typedef struct Stack_ Stack;
int stackMemErr(Stack *ptrStack);
void* stackInit(int size);
void* stack_r(Stack *ptrStack, int x);
void* stack_w(Stack *ptrStack, int x, void* ptrValue);
void* push(Stack *ptrStack, void* ptrValue);
void* pop(Stack *ptrStack);
void* getLast(Stack *ptrStack);

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

#define READ 0
#define WRITE 1
#define HAS_USED 1
#define RESET 2
#define STEP 10

/** Structure of stack\n
 * Думаю на канарейки выделю по одному элементу размера size слева и справа */
struct Stack_{
    void* data; ///< Pointer to data
    int size; ///< Size of one element of data in bytes
    int num; ///< Number of elements of data (malloced memory)
    int pos; ///< Next free position of stack (pop/push/getlast)
    unsigned char* meta; ///< "Poison" check of data
    int metaNum; ///< Number of elements of meta (malloced memory)
};

int meta_main(Stack *ptrStack, int flag, int x);
void stack_extend(Stack *ptrStack, int x);

/** Main function of stack array
 * @param ptrStack Pointer to stack
 * @param flag READ or WRITE
 * @param x Position to operate
 * @param ptrValue Pointer to value of (ptrStack->size) size */
void* stack_main(Stack *ptrStack, int flag, int x, void* ptrValue){
    if(x >= ptrStack->num){
        stack_extend(ptrStack, x);
        if(stackMemErr(ptrStack)){
            printf("stack_main: memory error\n");
            return NULL;
        }
    }
    switch(flag){
        case READ:
            //check here needed
            if(!meta_main(ptrStack, READ, x))
                printf("stack_main: using before undefined value X = %d\n", x);
            return &((char*)ptrStack->data)[x * ptrStack->size];
        case WRITE:
            for(int i = 0; i < ptrStack->size; i++)
                ((char*)ptrStack->data)[x * ptrStack->size + i] = ((char*)ptrValue)[i];
            meta_main(ptrStack, HAS_USED, x);
            return &((char*)ptrStack->data)[x * ptrStack->size];
    }
}

/** Extends given Stack_ by STEP const \n
 * (ptrStack->data == Null || ptrStack->meta == NULL) if memory error occurred \n */
void stack_extend(Stack *ptrStack, int x){
    void* buffArray = ptrStack->data; ///< Saves previous pointer's value
    //Сначала выделяем память под data
    x = x + 1 + (STEP - (x + 1) % STEP);///< new number of elements
    ptrStack->data = malloc(x * ptrStack->size);
    //проверка на ошибку
    if(ptrStack->data != NULL){
        int i; //возвращаем элементы
        for(i = 0; i < ptrStack->num * ptrStack->size; i++)
            ((char*)ptrStack->data)[i] = ((char*)buffArray)[i];
        ptrStack->num = x;
        free(buffArray);
        //Потом выделяем память для meta
        if(x > ptrStack->metaNum * 8){
            x = x / 8 + (x % 8 == 0 ? 0 : 1);
            buffArray = ptrStack->meta;
            ptrStack->meta = calloc(x, sizeof(char)); //калок чтобы нулики везде были
            //проверка на ошибку
            if(ptrStack->meta != NULL) {
                //возвращаем элементы
                for(i = 0; i < ptrStack->metaNum; i++)
                    ((char*)ptrStack->meta)[i] = ((char*)buffArray)[i];
                ptrStack->metaNum = x;
                free(buffArray); //при ошибках с выделением памяти мб утечка памяти, да и хуй с ней
            }
        }
    }
}

/** "Poison" check function */
int meta_main(Stack *ptrStack, int flag, int x){
    switch (flag) {
        case READ:
            return (ptrStack->meta[x / 8] >> (7 - x % 8)) & 1; //достаю нужный бит
        case HAS_USED:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] | (1 << (7 - (char)(x % 8)));
            return 1;
        case RESET:
            ptrStack->meta[x / 8] = ptrStack->meta[x / 8] & ~(1 << (7 - (char)(x % 8)));
            return 0;
    }
}

/** Constructor of stack. \n
 * Outputs "memory error" to command line if error occurred */
void* stackInit(int size){
    if(size <= 0)
        return NULL;
    Stack *ptrStack;
    ptrStack = malloc(sizeof(Stack));
    if(ptrStack != NULL) {
        ptrStack->size = size;
        ptrStack->num = 0;
        ptrStack->pos = 0;
        ptrStack->metaNum = 0;
        ptrStack->meta = NULL;
        stack_extend(ptrStack, 0);
    }
    if(stackMemErr(ptrStack))
        printf("stackInit: memory error\n");
    return ptrStack;
}

/** Checks pointers for NULL value */
int stackMemErr(Stack *ptrStack){
    return (ptrStack == NULL || ptrStack->data == NULL || ptrStack->meta == NULL);
}

/** Array READ function \n
 * короче рассказываю анекдот:\n
 * Знаете какой наркотик гомосексуалисты предпочитают больше всего?\n
 * \n
 * \n
 * \n - кокс */
void* stack_r(Stack *ptrStack, int x){
    if(!stackMemErr(ptrStack) && x >= 0)
        return stack_main(ptrStack, READ, x, NULL);
    else
        return NULL;                                        //FIXME!!!! в противном условии возвращать адрес канарейки (и проверять ее на всякий)
}

/** Array WRITE function \n
 * @param ptrStack - pointer to stack struct
 * @param x - position in which value will be written
 * @param ptrValue - pointer to value */
void* stack_w(Stack *ptrStack, int x, void* ptrValue){
    if(!stackMemErr(ptrStack) && ptrValue!= NULL && x >= 0)
        return stack_main(ptrStack, WRITE, x, ptrValue);
    else
        return NULL;                                        //FIXME!!!! в противном условии возвращать адрес канарейки (и проверять ее на всякий)
}

/** Stack function: Push \n
 * Adds a new element to the end of the stack
 * @param ptrStack - pointer to stack struct
 * @param ptrValue - pointer to value
 * @warning Value form ptrValue is needed to be the same size as one of stack's element */
void* push(Stack *ptrStack, void* ptrValue){
    if(!stackMemErr(ptrStack) && ptrValue != NULL)
        return stack_main(ptrStack, WRITE, ptrStack->pos++, ptrValue);
    else
        return NULL;                                        //FIXME!!!! в противном условии возвращать адрес канарейки (и проверять ее на всякий)
}

/** Stack function: Pop \n
 * Gets a new element from the end of the stack
 * @param ptrStack - pointer to stack struct */
void* pop(Stack *ptrStack){
    if(!stackMemErr(ptrStack) && ptrStack->pos > 0){
        void* buffPtr = stack_main(ptrStack, READ, --ptrStack->pos, NULL); ///< Saves result's pointer
        meta_main(ptrStack, RESET, ptrStack->pos); ///< resets that position
        return buffPtr;
    }
    else
        return NULL;                                        //FIXME!!!! в противном условии возвращать адрес канарейки (и проверять ее на всякий)
}

/** Stack function: GetLast \n
 * Gets a last element of stack
 * @param ptrStack - pointer to stack struct */
void* getLast(Stack *ptrStack){
    if(!stackMemErr(ptrStack) && ptrStack->pos > 0)
        return stack_main(ptrStack, READ, ptrStack->pos - 1, NULL);
    else
        return NULL;                                        //FIXME!!!! в противном условии возвращать адрес канарейки (и проверять ее на всякий)
}

