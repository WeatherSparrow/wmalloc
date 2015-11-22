
/****************************************************************************/
/*                  Copyright 2014-2015 Yoshinobu Ogura                     */
/*                                                                          */
/*                      This file is part of Sirius.                        */
/*                                                                          */
/*  Sirius is free software: you can redistribute it and/or modify          */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation, either version 3 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  Sirius is distributed in the hope that it will be useful,               */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/*  GNU General Public License for more details.                            */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License       */
/*  along with Sirius.  If not, see <http://www.gnu.org/licenses/>.         */
/*                                                                          */
/****************************************************************************/

#include "wmalloc.h"

namespace Wulf {
    namespace Sys {
        const size_t complemem_blocksize_default = 8;

        int MallocErrorDefault(int err, const void *p)
        {
            perror((const char *)p);
            exit(EXIT_FAILURE);
            return err;
        }

        Error_f MallocErrorPtr = MallocErrorDefault;

        int SystemMallocError(int err, const void *p)
        {
            return (*MallocErrorPtr)(err, p);
        }

        Error_f SetErrorFun(Error_f ptr)
        {
            MallocErrorPtr = ptr;
            return MallocErrorPtr;
        }

        SysMem combine(SysMem mem, CompleMem cmem)
        {
            if (mem == NULL || cmem == NULL) {
                return NULL;
            }
            mem->partner = cmem;
            cmem->partner = mem;
            return mem;
        }

        //pagenum 初期のページ数
        //blockperpage ページあたりのブロック数
        //blocksize ブロックあたりの要素数
        CompleMem initCompleMem(size_t pagenum, size_t blockperpage, size_t blocksize)
        {
            CompleMem cmem = (CompleMem)malloc(sizeof(struct complemem_t));
            if (cmem == NULL) {
                SystemMallocError(0, (const void *)"System Memory Exhaustion");
                return NULL;
            }

            cmem->allsize = pagenum * blockperpage * blocksize;
            cmem->pagesize = blockperpage * blocksize;
            cmem->blocksize = blocksize;
            cmem->last = 0;
            cmem->partner = NULL;
            cmem->freestack = (SysStack)WMPNULL;
            cmem->data = (Pointer *)malloc(cmem->allsize * sizeof(Pointer));
            if (cmem->data == NULL) {
                free(cmem);
                SystemMallocError(0, (const void *)"System Memory Exhaustion");
                return NULL;
            }
            return cmem;
        }

        void deleteCompleMem(CompleMem *cmem)
        {
            if (cmem == NULL) {
                return;
            }

            if (*cmem != NULL) {
                free((*cmem)->data);
                free(*cmem);
                *cmem = NULL;
            }
            return;
        }

        wmptr_t complemalloc(CompleMem cmem)
        {
            if (cmem == NULL) {
                return WMPNULL;
            }

            //フリー済みがあればそっちを優先
            void *ret;
            if (cmem->freestack != (SysStack)WMPNULL) {
                ret = pop(cmem->partner, cmem->freestack);
                if (ret != NULL) {
                    return (wmptr_t)ret;
                }
            }

            //cmem->last + 1 == cmem->allsizeで満杯
            if ((cmem->last + 1) * cmem->blocksize >= cmem->allsize) {
                size_t newsize = cmem->allsize + cmem->pagesize;
                Pointer *buf = (Pointer *)realloc(cmem->data, newsize * sizeof(Pointer));
                if (buf == NULL) {
                    SystemMallocError(0, (const void *)"System Memory Exhaustion");
                    return WMPNULL;
                }
                cmem->data = buf;
                cmem->allsize = newsize;
            }
            return (wmptr_t)((cmem->last)++ * cmem->blocksize);
        }

        void complefree(CompleMem cmem, wmptr_t p)
        {
            if (cmem == NULL || p == (wmptr_t)NULL || p == WMPNULL) {
                return;
            }

            if(cmem->freestack == (SysStack)WMPNULL) {
                cmem->freestack = initSysStack(cmem->partner);
            } 

            push(cmem->partner, cmem->freestack, (void *)p);
            return;
        }

        //pagenum 初期のページ数
        //blockperpage ページあたりのブロック数
        //blocksize ブロックあたりの要素数
        SysMem initSysMem(size_t pagenum, size_t blockperpage, size_t blocksize)
        {
            SysMem mem = (SysMem)malloc(sizeof(sysmem_t));
            if (mem == NULL) {
                return NULL;
            }
            mem->allsize = pagenum * blockperpage * blocksize;
            mem->pagesize = blockperpage * blocksize;
            mem->blocksize = blocksize;
            mem->last = 0;
            mem->partner = NULL;
            mem->freestack = (SysStack)WMPNULL;
            mem->data = (Pointer *)malloc(mem->allsize * sizeof(Pointer));
            if (mem->data == NULL) {
                free(mem);
                SystemMallocError(0, (const void *)"System Memory Exhaustion");
                return NULL;
            }
            return mem;
        }

        void deleteSysMem(SysMem *mem)
        {
            if (mem == NULL) {
                return;
            }
            if (*mem != NULL) {
                free((*mem)->data);
                free(*mem);
                *mem = NULL;
            }
            return;
        }

        wmptr_t wmalloc(SysMem mem)
        {
            if (mem == NULL) {
                return WMPNULL;
            }

            void *ret;
            if (mem->freestack != (SysStack)WMPNULL) {
                SysStack freestack = (SysStack)&mem->partner->data[(wmptr_t)mem->freestack];
                if (freestack->head != WMPNULL) {
                    ret = pop(mem, mem->freestack);
                    if (ret != NULL) {
                        return (wmptr_t)ret;
                    }
                }
            }

            //(mem->last + 1) * mem->blocksize == mem->allsizeで満杯
            if ((mem->last + 1) * mem->blocksize >= mem->allsize) {
                size_t newsize = mem->allsize + mem->pagesize;
                Pointer *buf = (Pointer *)realloc(mem->data, newsize * sizeof(Pointer));
                if (buf == NULL) {
                    return WMPNULL;
                }
                mem->data = buf;
                mem->allsize = newsize;
            }

            return (wmptr_t)((mem->last)++ * mem->blocksize);
        }

        //wmallocしたもの以外をwmfreeした場合は、次にwmallocしたときに
        //そこにpagesize分を確保するということである。
        //十分な長さを持ちstableな領域であれば問題ないが、
        //スタック領域のメモリなどを使うと動作は未定となる。
        //なお、グローバル変数として確保した配列をあらかじめwmfreeしておくことで
        //そこを優先的に使用するというハックも可能。
        void wmfree(SysMem mem, wmptr_t p)
        {
            if (mem->freestack == NULL) {
                mem->freestack = initSysStack(mem);
            }

            push(mem, mem->freestack, (void *)p);
            
            return;
        }

        SysStack initSysStack(SysMem mem)
        {
            wmptr_t ptr = complemalloc(mem->partner);
            if (ptr == WMPNULL) {
                return (SysStack)WMPNULL;
            }
            SysStack stk = (SysStack)&mem->partner->data[ptr];
            //SysStack stk = (SysStack)ptr;
            stk->dim = 0;
            stk->cursol = WMPNULL;
            stk->head = WMPNULL;
            stk->cur = (SysStack)WMPNULL;
            stk->upper = (SysStack)WMPNULL;

            //partnerからアドレスを決め打ちするとrealloc時に死ぬのでstkを返してはならない。
            return (SysStack)ptr;
        }

        void *look(SysMem mem, SysStack stk)
        {
            if (mem == NULL || stk == (SysStack)WMPNULL) {
                return NULL;
            }

            stk = (SysStack)&mem->partner->data[(wmptr_t)stk];
            
            if (stk->cur == (SysStack)WMPNULL) {
                if (stk->cursol == WMPNULL) {
                    return NULL;
                }
                return mem->data[stk->head + stk->cursol];
            }

            SysStack cur = (SysStack)&mem->partner->data[(wmptr_t)stk->cur];

            return mem->data[cur->head + cur->cursol];
        }

        void deleteCurStack(SysMem mem, SysStack *stk)
        {
            if (mem == NULL || stk == NULL) {
                return;
            }

            
            if (*stk == (SysStack)WMPNULL) {
                return;
            }

            SysStack buf = (SysStack)&mem->partner->data[(wmptr_t)*stk];

            //deleteSysStackとは違い、下位のスタックは放置する
            wmfree(mem, buf->head);
            complefree(mem->partner, (wmptr_t)*stk);
            *stk = (SysStack)WMPNULL;

            return;
        }

        SysStack alterStack(SysMem mem, SysStack stk)
        {
            if (mem == NULL || stk == (SysStack)WMPNULL) {
                return (SysStack)WMPNULL;
            }

            stk = (SysStack)&mem->partner->data[(wmptr_t)stk];
            
            if (stk->cur == (SysStack)WMPNULL) {
                //代わりはない
                return NULL;
            }

            if (stk->upper == (SysStack)WMPNULL) {
                //データが壊れている
                SystemMallocError(0, "Broken Stack");
                return (SysStack)WMPNULL;
            }

            //stk->cur != WMPNULL
            SysStack upper = (SysStack)&mem->partner->data[(wmptr_t)stk->upper];

            if (upper->cur == (SysStack)WMPNULL) {
                if (upper->cursol > 0) {
                    stk->cur = (SysStack)mem->data[upper->head + (upper->cursol)--];
                    return stk->cur;
                }
                //upper->cursol == 0
                //upperは不要となる
                //stk->upper = (SysStack)WMPNULL;
                stk->cur = (SysStack)WMPNULL;
                deleteCurStack(mem, &stk->upper);
                return (SysStack)WMPNULL;
            }

            //upper->cur != NULL;
            SysStack cur = (SysStack)&mem->partner->data[(wmptr_t)upper->cur];
            if (cur->cursol > 0) {
                stk->cur = (SysStack)mem->data[cur->head + (cur->cursol)--];
                return stk->cur;
            }
            //cur->cursol == 0
            SysStack ret = (SysStack)mem->data[cur->head];
            alterStack(mem, stk->upper);
            return ret;
        }

        void *pop(SysMem mem, SysStack stk)
        {
            if (mem == NULL || stk == (SysStack)WMPNULL) {
                return NULL;
            }

            SysStack stkptr = stk;
            stk = (SysStack)&mem->partner->data[(wmptr_t)stk];

            //基底スタック
            if (stk->cur == (SysStack)WMPNULL) {
                if (stk->cursol == WMPNULL) {
                    //スタックは空
                    return NULL;
                }

                if (stk->cursol == 0) {
                    //今回のポップでスタックは空
                    stk->cursol = WMPNULL;
                    void *ret = mem->data[stk->head];
                    wmfree(mem, stk->head);
                    return ret;
                }

                //stk->cursol > 0
                return mem->data[stk->head + (stk->cursol)--];
            }

            //stk->cur != NULL
            //n次スタック
            SysStack cur = (SysStack)&mem->partner->data[(wmptr_t)stk->cur];
            if (cur->cursol == 0) {
                SysStack ret = (SysStack)mem->data[cur->head];
                //今回のポップで現在のスタックは空。
                //専用の関数alterStackでstk->curに有効なスタックを装填
                //上位スタックが空ならstkはそのように書き換えられる
                stk->cur = alterStack(mem, stkptr);
                return ret;
            } else if (cur->cursol < mem->blocksize) {
                return mem->data[cur->head + (cur->cursol)--];
            }
            //puts("error3");

            //ここまで来たらエラー

            return NULL;
        }

        void push(SysMem mem, SysStack stk, void *p)
        {
            if (mem == NULL || stk == (SysStack)WMPNULL) {
                return;
            }

            stk = (SysStack)&mem->partner->data[(wmptr_t)stk];

            //基底スタック
            if (stk->cur == (SysStack)WMPNULL) {
                if (stk->head == WMPNULL) {
                    stk->cursol = 0;
                    stk->head = wmalloc(mem);
                    mem->data[stk->head] = p;
                    return;
                }

                if (stk->cursol == WMPNULL) {
                    //スタックは空
                    stk->cursol = 0;
                    mem->data[stk->head] = p;
                    return;
                }

                //ここで悪魔的なことが起こっておりエラーになる
                if (stk->cursol + 1 < mem->blocksize) {
                    stk->cursol += 1;
                    mem->data[stk->head + stk->cursol] = p;
                    return;
                }

                //stk->cursol + 1 >= mem->blocksize
                if (stk->upper == (SysStack)WMPNULL) {
                    stk->upper = initSysStack(mem);
                    SysStack upper = (SysStack)&mem->partner->data[(wmptr_t)stk->upper];
                    upper->dim = stk->dim + 1;
                }

                push(mem, stk->upper, (void *)stk);
                stk->cur = initSysStack(mem);
                SysStack cur = (SysStack)&mem->partner->data[(wmptr_t)stk->cur];
                cur->cursol = 0;
                cur->upper = stk->upper;
                cur->head = wmalloc(mem);
                mem->data[cur->head] = p;
                return;
            }

            //stk->cur != NULL
            SysStack cur = (SysStack)&mem->partner->data[(wmptr_t)stk->cur];
            if (cur->cursol == WMPNULL) {
                //スタックは空
                if (cur->head == WMPNULL) {
                    cur->head = wmalloc(mem);
                }
                cur->cursol = 0;
                mem->data[cur->head] = p;
                return;
            }
            if (cur->cursol + 1 < mem->blocksize) {
                cur->cursol += 1;
                mem->data[cur->head + cur->cursol] = p;
                return;
            }

            //cur->cursol + 1 >= mem->blocksize
            push(mem, stk->upper, (void *)stk->cur);
            stk->cur = initSysStack(mem);
            cur = (SysStack)&mem->partner->data[(wmptr_t)stk->cur];
            cur->cursol = 0;
            cur->upper = stk->upper;
            cur->head = wmalloc(mem);
            mem->data[cur->head] = p;
 
            return;
        }

        void deleteSysStack(SysMem mem, SysStack *stk)
        {
            puts("deleteSysStack");
            if (mem == NULL || stk == NULL) {
                return;
            }

            if (*stk == (SysStack)WMPNULL) {
                return;
            }
            //本来は全体を解放するより複雑な再帰処理
            wmfree(mem, (*stk)->head);
            complefree(mem->partner, (wmptr_t)*stk);
            *stk = (SysStack)WMPNULL;

            return;
        }

        Block initBlock(SysMem mem)
        {
            Block blk = (Block)complemalloc(mem->partner);
            blk->hcur = WMPNULL;
            blk->lcur = WMPNULL;
            blk->head = wmalloc(mem);
            blk->next = NULL;
            return blk;
        }

        void freeBlock(SysMem mem, Block *blk)
        {
            if (mem == NULL || blk == NULL) {
                return;
            }
            if (*blk != NULL) {
                wmfree(mem, (*blk)->head);
                complefree(mem->partner, (wmptr_t)*blk);
                *blk = NULL;
            }
            return;
        }
/*
        SysQueue initSysQueue(SysMem mem)
        {
            SysQueue sysq = (SysQueue)malloc(sizeof(sysqueue_t));
            if (sysq == NULL) {
                return NULL;
            }
            sysq->headblk = initBlock(mem);
            sysq->lastblk = NULL;
            return NULL;
        }

        void *deq(SysMem mem, SysQueue sysq)
        {
            if (mem == NULL || sysq == NULL) {
                return NULL;
            }

            if (sysq->head == NULL) {
                return NULL;
            }

            Block target = sysq->head;
            void *ret;

            if (target->last >= mem->blocksize) {
                return NULL;
            }

            if (target->head == target->last) {
                return target->data[target->head];
            }

            ret = target->data

            return NULL;
        }

        void enq(SysMem mem, SysQueue sysq, void *p)
        {
            if (mem == NULL || sysq == NULL) {
                return;
            }

            if (sysq->head == NULL) {
                sysq->headblk = initBlock(mem)
            }

            Block target = sysq->lastblk;

            if (target->last == mem->blocksize) {
                //そのブロックに入る最初の値
                target->last = 0;
                target->data[0] = p;
            }

            if (target->last + 1 < mem->blocksize) {
                target->data[++target->last] = p;
                return;
            }

            //sysq->lastblk->last + 1 >= mem->blocksize
            Block buf = initBlock(mem);
            sysq->lastblk->next = buf;
            sysq->lastblk = buf;

            buf->data[0] = p;

            return;
        }

        void deleteSysQueue(SysMem mem, SysQueue *sysq)
        {
            if (mem == NULL || sysq == NULL) {
                return;
            }

            if (*sysq == NULL) {
                return;
            }

            Block cur = (*sysq)->headblk;
            Block next;
            while (cur != NULL) {
                next = cur->next;
                wmfree(cur);
                cur = next;
            }

            free(*sysq);
            *sysq = NULL;

            return;
        }*/
    }
}
