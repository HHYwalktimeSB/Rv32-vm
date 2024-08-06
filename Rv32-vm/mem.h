#ifndef _Mymem_h_
#define _Mymem_h_

#define MASK_VPN_1 0xffc00000
#define MASK_VPN_0 0x3ff000
#define MASK_OFFSET4K 0xfff
#define MASK_XWR 0xe

struct tagVaddr
{
	unsigned int offset : 12;
	unsigned int vpn_0 : 10;
	unsigned int vpn_1 : 10;
};

//��С��ת������32λ
inline unsigned int bl_endian_switch32(unsigned int val) {
	unsigned int ret = val << 24;
	ret |= val >> 24;
	ret |= (val & 0x00ff0000) >> 8;
	ret |= (val & 0x0000ff00) << 8;
	return ret;
}

//��С��ת������32λ
inline unsigned short bl_endian_switch16(unsigned short val) {
	unsigned short ret = val << 8;
	ret |= val >> 8;
	return ret;
}

//�ڴ�Ԥ�ȷ��䣬ʹ��ǰ�ύ������ռ��read/wrirte[size in bits] p(�����ڴ�)
class myMem {
protected:
	unsigned char* membase;
	unsigned int memsize;
	unsigned int _mask;
	void* cbdata;//����
	void (*_invoke_int)(void*, int);//����
public:
	void _init(unsigned int size);
	unsigned char _readp8(unsigned int _addr);

	unsigned short _readp16(unsigned int _addr);

	unsigned int _readp32(unsigned int _addr);

	void _writep8(unsigned int _addr, unsigned char val);
	void _writep16(unsigned int _addr, unsigned short val);
	void _writep32(unsigned int _addr, unsigned int val);

};

//ҳ����
struct sv32pagetable_entry {
	unsigned int V : 1;
	unsigned int R : 1;
	unsigned int W : 1;
	unsigned int X : 1;
	unsigned	int U : 1;
	unsigned int G : 1;
	unsigned int A : 1;
	unsigned	int D : 1;
	unsigned int RSW : 2;
	unsigned int PPN_0 : 10;
	unsigned int PPN_1 : 12;
};

#endif // !_Mymem_h_
