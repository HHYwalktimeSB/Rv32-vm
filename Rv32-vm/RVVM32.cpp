#include "RVVM32.h"
#include<Windows.h>

int MyVm::InstallDevice(const char* path, unsigned addr, unsigned flag)
{
	return 0;
}

_DevBase* MyVm::_device_attach(const char* path)
{
	HANDLE Hmod = LoadLibraryA(path);
	if (Hmod == NULL)return nullptr;
	void* fn = GetProcAddress((HMODULE)Hmod, "CreateObj");
	if (fn == NULL)return nullptr;
	return ((DevEntry_Fn)fn)(Hmod, nullptr);
}

void MyVm::init_(unsigned memory_size)
{
	this->quick_setup(memory_size);
	ps = new Schedule(100000);
}

MyVm::MyVm()
{
	ps = nullptr;
}
