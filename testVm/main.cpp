#include"../Rv32-vm/rv32vm_exportapi.h"
#include<iostream>

using namespace std;

int main() {
	if (RV32vm_Init(1024 * 1024 * 4) != 0) {
		cout << "fuck" << endl;
		return 0;
	}
	Vmmem_load_hex("../x64/Debug/test.txt", 0x80000000);
	Vm_RunSync();
	return 0;
}