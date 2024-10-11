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
	std::string str;
	cin >> str;
	//add your codes bellow

	//example of modify memory
	debug.loadmem_fromhexfile(str.c_str(), 0x80000000);
	debug.setpc(0x80000000);
	debug.simple_run();

	system("pause");
	return 0;
}	