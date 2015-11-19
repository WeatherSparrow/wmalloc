# wmalloc

wmallocはWolf Frameworkのために実装されたメモリアロケータです。固定長の領域を動的に確保・解放することに特化しており、
C/C++のmallocとは少し役割や様相が異なりますが、十分に実用的な性能を有しています。具体的には、一般のmallocはメモリの確保に
最悪でO(n)の計算量を必要とするのに対し、wmallocは最悪でもほぼO(1)の計算量しか要さず、また、メモリの断片化も比較的軽微なものに
止まります。

wmallocは以下の構造により構成されています。

* SysMem
* CompleMem
* SysStack
* SysQueue

## 使い方
`make wmtest`でテスト用のサンプルソースがビルドされます。実行すると20,000個の要素をpush/popするデモを行います。
機能を利用するには`wmalloc.h`をインクルードし、サンプルソース、および以下の説明にしたがってください。

## SysMem / CompleMem
wmallocを使うには、SysMemとCompleMemが必要になります。SysMemはwmallocにより確保されるメモリ領域、CompleMemは
wmalloc全体で用いる構造体を管理するためのメモリ領域を管理します。以下の手順により、wmallocに必要な構造体をアクティベートしてください。

```c++:sample.cpp
using namespace Wolf::Sys;

int main(void)
{
    CompleMem cmem = initCompleMem(1, 256, complemem_blocksize_default);
    SysMem mem = initSysMem(1, 16, 32);
    mem = combine(mem, cmem);

    //処理

    return 0;
}
```

SysMemとCompleMemは対になっており、両方を確保してcombineにより結合する必要があります。
引数についての詳細は`wmalloc.h`を参照してください。SysMemとCompleMemは、領域が不足した場合には
自身を拡張する機能がついているため、通常の使用においてサイズの上限を気にする必要はありません。
以降、解放するとき以外はCompleMemは使いません。解放は次のコードで行います。

```c++:sample.cpp
using namespace Wolf:Sys;

int main(void)
{
    //処理

    deleteCompleMem(&cmem);
    deleteSysMem(&mem);

    return 0;
}

```

この処理により、SysMem上にwmallocで確保されたすべての領域は安全に解放されます。
wmallocで取得した領域は、計算の直前で次の式によりアドレスに変換してください。

```c++:sample.cpp

SysMem mem;
wmptr_t wmptr = wmalloc(mem);
if (wmptr == WMPNULL) {
    perror("WMPNULLはエラー値です");
}

//使用直前に次の式で(void *)型に変換する
void *ptr = (void *)&mem->data[wmptr];

//解放は次のように行う
wmfree(&wmptr);

```

wmallocで確保された領域はSysMemを解放するときに自動的にすべて解放されるので、個別に解放する必要のない場合があります。
SysStack、SysQueueは管理用の構造体も含めてすべてSysMemとComleMem上に確保されるため、同様に個別に解放する必要がない場合があります。

## SysStack
SysStackは、wmallocを用いて自身を拡張することが可能なスタックです。自身の大きさを最適化し、SysMemに指定されたブロックサイズ
以上に余計な領域を消費することはありません。1つのSysMemの中に複数のSysStackを作成することができるため、
「どのスタックにいくつの要素が積まれるかはわからないが、全体ではn個以上積まれることはない」という状況で真価を発揮します。
自己再帰した高階構造を取り、理論的には無限のサイズ拡張が可能です。


## SysQueue
実装中です。

## 注意点
wmallocはスレッドセーフではありません。複数のスレッドで共通に使用する場合、各自で排他制御を実装してください。


