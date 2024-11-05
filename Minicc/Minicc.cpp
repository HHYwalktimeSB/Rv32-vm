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

using namespace std;

int main()
{
    unsigned xxxx;
    fstream fss;
    string str;
    call_test_fn();
    fss.open("../x64/Debug/test.txt", ios::in);
    unsigned int cnt = 0;
    unsigned cnt_sz = 0;
    char* ffff;
    while (fss >> str) {
        str = "0x" + str;
        xxxx = strtol(str.c_str(),&ffff,16) ;
        cnt_sz += c_instruction(xxxx, bufi + cnt_sz);
    }
    FILE* fs;
    fopen_s(&fs, "ins_86", "wb");
    if (fs) {
        fwrite(bufi, 1, cnt_sz, fs);
        fclose(fs);
    }
    return 0;
}
