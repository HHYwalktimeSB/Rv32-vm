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
	membase = (UCHAR*)VirtualAlloc(NULL, memsize, MEM_RESERVE, PAGE_READWRITE);
}

unsigned char myMem::_readp8(unsigned int _addr)
{
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
}

unsigned short myMem::_readp16(unsigned int _addr)
{
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
}

unsigned int myMem::_readp32(unsigned int _addr)
{
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
}

void myMem::_writep8(unsigned int _addr, unsigned char val)
{
	__try {
		*reinterpret_cast<unsigned char*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 1, MEM_COMMIT, PAGE_READWRITE);
		*(membase + (_addr & _mask)) = val;
	}
}

void myMem::_writep16(unsigned int _addr, unsigned short val)
{
	__try {
		*reinterpret_cast<unsigned short*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 2, MEM_COMMIT, PAGE_READWRITE);
		*reinterpret_cast<unsigned short*>(membase + (_addr & _mask)) = val;
	}
}

void myMem::_writep32(unsigned int _addr, unsigned int val)
{
	__try {
		*reinterpret_cast<unsigned int*>(membase + (_addr & _mask)) = val;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
		EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(membase + (_addr & _mask), 4, MEM_COMMIT, PAGE_READWRITE);
		*reinterpret_cast<unsigned int*>(membase + (_addr & _mask)) = val;
	}
}