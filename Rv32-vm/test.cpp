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
	while (continue_running) {
		//add your code here
		
		//example of modify memory
		strcpy_s(buf,"ahahhaa");
		debug.memwrite(buf, 0 , 1,8);
		

		ecode = debug.run_1_cycle();

		//example:
		cout << debug.getregs()->x[1] << endl;


		cin >> continue_running;
	}

	system("pause");
	return 0;
}	