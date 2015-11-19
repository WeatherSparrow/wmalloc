#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "wmalloc.h"

#define testcase 20000

using namespace Wolf::Sys;

int main(void)
{
    CompleMem cmem = initCompleMem(1, 256, complemem_blocksize_default);
    SysMem mem = initSysMem(1, 16, 32);
    mem = combine(mem, cmem);

    if (mem == NULL) {
        puts("mem is NULL.");
        return EXIT_FAILURE;
    }
    //アクティベート完了

    intptr_t ptr[testcase];
    int i;
    for (i = 0; i < testcase; i++) {
        ptr[i] = (intptr_t)i;
    }
    //void *wp;
    //wp = wmalloc(mem);
    //printf("%p\n", wp);
    printf("%p\n", cmem->data);
    printf("%p\n", mem->data);

    SysStack stk = initSysStack(mem);
    printf("StackAddr:%p\n", stk);
    //printf("%p\n", stk->data);
    //printf("%p\n", stk->upper);

    for (i = 0; i < testcase; i++) {
        printf("%d\n", i);
        push(mem, stk, (void *)ptr[i]);
    }
/*
    puts("stack-------------------------------------------");
    for (i = 0; i < 64; i++) {
        printf("%p\n", stk->data[i]);
    }
    puts("cur---");
    for (i = 0; i < 36; i++) {
        printf("%p\n", stk->cur->data[i]);
    }
*/
    puts("------------------------------------------------");

    void *mp;
    for (i = 0; i < testcase; i++) {
        mp = pop(mem, stk);
        printf("%p\n", mp);
    }

    deleteCompleMem(&cmem);
    deleteSysMem(&mem);
    return EXIT_SUCCESS;
}
