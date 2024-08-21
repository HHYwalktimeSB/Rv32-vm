#include "_cpu.h"

#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string.h>

using namespace std;

char buf[8192];

int main() {
	CPUdebugger debug;
	debug.quick_setup(1024 * 1024 * 4);//4m
	bool continue_running = true;
	int ecode;
	_cpustate stat;

	//add your codes bellow

	//example of modify memory
	debug.loadmem_fromhexfile("test.txt", 0);

	while (continue_running) {

		debug.writecpustate(&stat);

		ecode = debug.run_1_cycle();

		debug.cmpcpustate(&stat);

		//cout << debug.getregs()->x[1] << endl;


		cin >> continue_running;
	}

	system("pause");
	return 0;
}	