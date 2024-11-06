#include "mem.h"
#include<Windows.h>

void myMem::_init(unsigned int size)
{
	auto x = size;
	while (x % 2 == 0) {
		x /= 2;                     
	}
	if (x != 1)throw -1;
	memsize = size;
	_mask = 0xffffffff;
	while (_mask > memsize)_mask >>= 1;
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	membase = (UCHAR*)VirtualAlloc(NULL, memsize, MEM_RESERVE, PAGE_READWRITE);
#else
	membase = new unsigned char[memsize];
#endif
}

unsigned char myMem::_readp8(unsigned int _addr)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	unsigned char ret;
	__try {
		ret = *(unsigned char*)(membase + (_addr & _mask));
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION? 
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 1, MEM_COMMIT, PAGE_READWRITE);
		ret = *(membase + (_addr & _mask));
	}
	return ret; 
#else
	return *(unsigned char*)(membase + (_addr & _mask));
#endif
}

unsigned short myMem::_readp16(unsigned int _addr)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	unsigned short ret;
	__try {
		ret = *(unsigned short*)(membase + (_addr & _mask));
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION?
		EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 2, MEM_COMMIT, PAGE_READWRITE);
		ret = *(membase + (_addr & _mask));
	}
	return ret;
#else
	return *(unsigned short*)(membase + (_addr & _mask));
#endif
}

unsigned int myMem::_readp32(unsigned int _addr)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	unsigned int ret;
	__try {
		ret = *(unsigned int*)(membase + (_addr & _mask));
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 4, MEM_COMMIT, PAGE_READWRITE);
		ret = *(membase + (_addr & _mask));
	}
	return ret;
#else
return *(unsigned int*)(membase + (_addr & _mask));
#endif
}

void myMem::_writep8(unsigned int _addr, unsigned char val)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	__try {
		*reinterpret_cast<unsigned char*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 1, MEM_COMMIT, PAGE_READWRITE);
		*(membase + (_addr & _mask)) = val;
	}
#else
	* reinterpret_cast<unsigned char*>(membase + (_addr & _mask)) = val;
#endif
}

void myMem::_writep16(unsigned int _addr, unsigned short val)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	__try {
		*reinterpret_cast<unsigned short*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 2, MEM_COMMIT, PAGE_READWRITE);
		*reinterpret_cast<unsigned short*>(membase + (_addr & _mask)) = val;
	}
#else
* reinterpret_cast<unsigned char*>(membase + (_addr & _mask)) = val;
#endif
}

void myMem::_writep32(unsigned int _addr, unsigned int val)
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	__try {
		*reinterpret_cast<unsigned int*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 4, MEM_COMMIT, PAGE_READWRITE);
		*reinterpret_cast<unsigned int*>(membase + (_addr & _mask)) = val;
	}
#else
* reinterpret_cast<unsigned char*>(membase + (_addr & _mask)) = val;
#endif
}

myMem::myMem()
{
	this->cbdata = 0;
	this->membase = 0;
	this->memsize = 0;
	this->_invoke_int = 0;
	this->_mask = 0;
}

myMem::~myMem()
{
#ifdef VMMEM_RESERVE_THEN_COMMIT 
	if (memsize > 0 && membase != NULL) {
		VirtualFree(membase, 0, MEM_RELEASE);
	}
#else
	delete[] membase;
#endif
}
