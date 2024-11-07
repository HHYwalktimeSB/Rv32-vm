#include"../Rv32-vm/RVVM32.h"

int main() {
	CPUdebugger debug;
	debug.quick_setup(1024 * 1024 * 4);
	debug.loadmem_fromhexfile("../x64/Debug/test.txt",0x80000000);
	debug.simple_run();
	return 0;
}