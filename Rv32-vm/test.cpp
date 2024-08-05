#include "_cpu.h"

#include<stdio.h>
#include<stdlib.h>

int main() {
	Instruction ins{0};
	ins.uType.opcode = OP_LUI;
	ins.uType.rd = 5;
	ins.uType.imm_31_12 = 114;
	printf("%x\n", *reinterpret_cast<unsigned int*>(&ins));
	Cpu_ _u;
	_u._init();
	_u.runsync();
	system("pause");
	return 0;
}	