#ifndef _Mymem_h_
#define _Mymem_h_

inline unsigned int bl_endian_switch32(unsigned int val) {
	unsigned int ret = val << 24;
	ret |= val >> 24;
	ret |= (val & 0x00ff0000) >> 8;
	ret |= (val & 0x0000ff00) << 8;
	return ret;
}


inline unsigned short bl_endian_switch16(unsigned short val) {
	unsigned short ret = val << 8;
	ret |= val >> 8;
	return ret;
}

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

struct sv32pagetable_entry {
	unsigned int PPN_1 : 12;
	unsigned int PPN_0 : 10;
	unsigned int RSW : 2;
	unsigned	int D : 1;
	unsigned int A : 1;
	unsigned int G : 1;
	unsigned	int U : 1;
	unsigned int X : 1;
	unsigned int W : 1;
	unsigned int R : 1;
	unsigned int V : 1;
};

#endif // !_Mymem_h_
