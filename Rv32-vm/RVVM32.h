#ifndef RVVM32_H_
#define RVVM32_H_

#include"_cpu.h"

class MyVm :public CPUdebugger {
public:
	int InstallDevice(const char* path, unsigned addr, unsigned flag);
protected:
	_DevBase* _device_attach(const char* path);
	Schedule* ps;
	public:
	void init_(unsigned memory_size);
	MyVm();
};

#endif