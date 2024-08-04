#ifndef _Mymem_h_
#define _Mymem_h_

#define _mMEM_ADDR_NOT_AL 1

class myMem {
protected:
	unsigned char* membase;
	unsigned int memsize;
	unsigned int _mask;
	void _init(unsigned int size);
	void* cbdata;
	void (*_invoke_int)(void*, int);
public:

	unsigned char _readp8(unsigned int _addr);

	unsigned short _readp16(unsigned int _addr);

	unsigned int _readp32(unsigned int _addr);

	void _writep8(unsigned int _addr, unsigned char val);
	void _writep16(unsigned int _addr, unsigned short val);
	void _writep32(unsigned int _addr, unsigned int val);

};

#endif // !_Mymem_h_
