#pragma once

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef ERRFUNTYPE
#define ERRFUNTYPE
typedef int (*Error_f)(int, const void *);
#endif

#define WMPNULL UINTPTR_MAX
typedef uintptr_t wmptr_t;
typedef void* Pointer;

namespace Wolf {
    namespace Sys {
        typedef struct complemem_t* CompleMem;
        typedef struct sysmem_t* SysMem;
        typedef struct sysstack_t* SysStack;
        typedef struct block_t* Block;
        typedef struct sysqueue_t* SysQueue;

        struct complemem_t {
            size_t allsize;         //全体の個数
            size_t pagesize;        //ページあたりの個数
            size_t blocksize;       //ブロックあたりの個数
            wmptr_t last;           //使用可能メモリの末尾+1
            Pointer *data;          //使用可能メモリの先頭
            SysMem partner;
            SysStack freestack;
        };

        struct sysmem_t {
            size_t allsize;     //全体の個数
            size_t pagesize;    //ページあたりの個数
            size_t blocksize;   //ブロックあたりの個数
            wmptr_t last;       //使用可能メモリの末尾+1
            Pointer *data;      //使用可能メモリの先頭
            CompleMem partner;
            SysStack freestack;
        };

        struct sysstack_t {
            size_t dim;
            wmptr_t cursol;  //cursol >= sysmem_t blocksizeのとき、スタックは空。
            wmptr_t head;   //wmallocしたデータの先頭の添字
            SysStack cur;   //現在のスタック
            SysStack upper; //ひとつ上の階
            //mem->data[stk->data+stk->cursol]でPointerが返る
        };

        struct block_t {
            wmptr_t hcur;   //ブロック内での使用中領域の先頭
            wmptr_t lcur;   //ブロック内での使用中領域の末尾
            wmptr_t head;   //wmallocしたデータの先頭の添字
            Block next;     //次のブロック
        };

        struct sysqueue_t {
            Block headblk;  //キューの先頭ブロック
            Block lastblk;  //キューの末尾ブロック
        };

        extern const size_t complemem_blocksize_default;

        Error_f SetErrorFun(Error_f ptr);
        CompleMem initCompleMem(size_t pagenum, size_t pagesize, size_t blocksize);
        //pagenum: 初期のページ数
        //pagesize: pageのブロック数
        //blocksize: ブロックに格納できるポインタの個数(void*)型
        void deleteCompleMem(CompleMem *cmem);
        wmptr_t complemalloc(CompleMem cmem);
        void complefree(CompleMem cmem, wmptr_t p);
        SysMem initSysMem(size_t pagenum, size_t pagesize, size_t blocksize);
        //pagenum: 初期のページ数
        //pagesize: pageのブロック数
        //blocksize: ブロックに格納できるポインタの個数(void*)型
        void deleteSysMem(SysMem *mem);
        wmptr_t wmalloc(SysMem mem);
        void wmfree(SysMem mem, wmptr_t p);
        SysMem combine(SysMem mem, CompleMem cmem);
        SysStack initSysStack(SysMem mem);
        void *pop(SysMem mem, SysStack stk);
        void push(SysMem mem, SysStack stk, void *p);
        void deleteSysStack(SysMem mem, SysStack *stk);
    }
}
