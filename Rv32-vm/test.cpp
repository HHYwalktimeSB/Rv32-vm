#include "_cpu.h"

#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string.h>
#include<iomanip>

using namespace std;

char buf[8192];

int main() {
	CPUdebugger debug;
	debug.quick_setup(1024 * 1024 * 4);//4m
	int continue_running = 1;
	int ecode;
	_cpustate stat;
	unsigned int is;
	//add your codes bellow

	//example of modify memory
	debug.loadmem_fromhexfile("test.txt", 0);



	while (continue_running!=0) {

		debug.writecpustate(&stat);

		ecode = debug.run_1_cycle();

		switch (ecode)
		{
		case EXC_INSTRUCTION_ADDR_NOT_ALIGNED:
			cout << "instruction address not aligned at 0x" << hex << debug.getregs()->pc << endl;
			break;
		case EXC_INV_INSTRUCTION:
			debug.readmem(debug.getregs()->pc, (char*)(&is), 4, 1);
			cout <<  "invalid instruction at 0x" <<hex <<  debug.getregs()->pc <<"\nval = 0x" <<is  << endl;
			break;
		case EXC_LOAD_ADDR_NOT_ALIGNED:
			break;
		case EXC_STORE_ADDR_NOT_ALIGNED:
			break;
		default:
			break;
		}

		debug.cmpcpustate(&stat);

		//cout << debug.getregs()->x[1] << endl;


		cin >> continue_running;
	}

	system("pause");
	return 0;
}	