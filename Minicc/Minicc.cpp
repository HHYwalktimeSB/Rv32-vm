#include"../minicc_dll/tks.h"
#include <iostream>
#include<fstream>
#include"../jittools/jittool.h"
#include "../rv32masm_dll/rv32masm.h"
#pragma comment(lib, "../x64/Debug/minicc_dll.lib")
#pragma comment(lib, "../x64/Debug/jittools.lib" )
#pragma comment(lib, "../x64/Debug/rv32masm_dll.lib")

template<typename T, unsigned N>
int gothrough_array( T (&arr)[N], void (*Fn) (T&)  ) {
    for (unsigned i = 0; i < N; ++i)
        Fn(arr[i]);
    return 0;
}

unsigned bufa[128];
unsigned cntrv = 0;
char bufi[2048];

void fuck(const char*& c) {
    bufa[cntrv] = make_1(c, -1);
    ++cntrv;
}

int main()
{
    unsigned xxxxx;
    const char* str[] = { "add x1, x2, x3" ,"add x0, x1, x0", "blt,  x1, x2, 16", "bge, x1,x2, 16"};
    gothrough_array(str, fuck);
    unsigned int cnt = 0;
    unsigned cnt_sz = 0;
    for (unsigned i = 0; i < cntrv; ++i) {
        call_test_fn();
    }
    FILE* fs;
    fopen_s(&fs, "ins_86", "wb");
    if (fs) {
        fwrite(bufi, 1, cnt_sz, fs);
        fclose(fs);
    }
    return 0;
}
