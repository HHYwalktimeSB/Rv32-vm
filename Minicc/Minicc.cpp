#include"../minicc_dll/tks.h"
#include <iostream>
#pragma comment(lib, "D:/sdx/pjx/Rv32-vm/x64/Debug/minicc_dll.lib")

int main()
{
    const char* str = "int main(int,char**) { return 0; const char* c = \"asn sdkr\\\" sdujder \"; }";
    Token* tk = Tokenlize(str, 73);
    print_tk(tk);
    return 0;
}
