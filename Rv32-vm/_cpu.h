#ifndef mCPUH
#define mCPUH
#include "mem.h"
#define RV32_
#define MODE_USR 0
#define MODE_SUPER 1
#define MODE_MACHINE 3
#define MASK_STAP_MODE 0x80000000
#define MASK_STAP_ASID 0x7fc00000
#define MASK_STAP_PPN 0x3fffff

//TODO
#define CSR_HAVE_FLAG_SUM true
//TODO

namespace tagCSR {
	struct tagmtvec
	{
		unsigned int base : 30;
		unsigned int mode : 2;
	};
	struct tagmcause {
		unsigned int Interrupt : 1;
		unsigned int exception_code : 31;
	};
	struct tagmstatus {
		unsigned int SD : 1;
		unsigned int WPRI_1 : 8;
		unsigned int TSR : 1;
		unsigned int TW : 1;
		unsigned int TVM : 1;
		unsigned int MXR : 1;
		unsigned int SUM : 1;
		unsigned int MPRV : 1;
		unsigned int XS : 2;
		unsigned int FS : 2;
		unsigned int MPP : 2;
		unsigned int WPRI_2 : 2;
		unsigned int SPP : 1;
		unsigned int MPIE : 1;
		unsigned int WPRI_3 : 1;
		unsigned int SPIE : 1;
		unsigned int UPIE : 1;
		unsigned int MIE : 1;
		unsigned int WPRI_4 : 1;
		unsigned int SIE : 1;
		unsigned int UIE : 1;
	};
}

enum CSRid {
	//user level

	fflags = 1,//URW
	frm = 2, //URW
	fcsr = 3, // URW

	cycle = 0xc00, //URO
	time = 0xc01, // URO
	instret = 0xc02, //URO
	hpmcounter3 = 0xc03, //URO
	hpmcounter4 = 0xc04, //URO
	hpmcounter5 = 0xc05, //URO
	hpmcounter6 = 0xc06, //URO
	hpmcounter7 = 0xc07, //URO
	hpmcounter8 = 0xc08, //URO
	hpmcounter9 = 0xc09, //URO
	hpmcounter10 = 0xc0a, //URO
	hpmcounter11 = 0xc0b, //URO
	hpmcounter12 = 0xc0c, //URO
	hpmcounter13 = 0xc0d, //URO
	hpmcounter14 = 0xc0e, //URO
	hpmcounter15 = 0xc0f, //URO
	hpmcounter16 = 0xc10, //URO
	hpmcounter17 = 0xc11, //URO
	hpmcounter18 = 0xc12, //URO
	hpmcounter19 = 0xc13, //URO
	//...
#ifdef RV32_
	cycleh = 0xc80, //URO 
	timeh = 0xc81, //URO
	instreth = 0xc82, //URO
	hpmcounter3h = 0xc83, //URO
	hpmcounter4h = 0xc84, //URO
	//...
#endif 

	//S level
	stap = 0x180, //SRW

	//machine level

	mstatus = 0x300, //MRW
	misa = 0x301, //MRW
	medeleg = 0x302, //MRW
	mideleg = 0x303, //MRW
	mie = 0x304, //MRW
	mtvec = 0x305, //MRW
	mcounteren = 0x306, //MRW
#ifdef RV32_
	mstatush = 0x307, //MRW
#endif // RV32_

	mscratch = 0x340, //MRW
	mepc = 0x341, //MRW
	mcause = 0x342, //MRW
	mtval = 0x343, //MRW
	mip = 0x344, //MRW
	mtinst = 0x34a, //MRW
	mtval2 = 0x34b, //MRW
};

struct REGS
{
	unsigned int x[32];
	int pc;
};

template <unsigned N>
unsigned int _sign_ext(unsigned int val) {
	if (val & (1 << (N - 1)))
		val |= 0xffffffff << N;
	return val;
}

union Instruction
{
	struct tagRtype {
		unsigned int opcode : 7;
		unsigned int rd : 5;
		unsigned int funct3 : 3;
		unsigned int rs1 : 5;
		unsigned int rs2 : 5;
		unsigned int funct7 :  7;
	} rType;
	struct tagIty {
		unsigned int opcode : 7;
		unsigned int rd : 5;
		unsigned int funct3 : 3;
		unsigned int rs1 : 5;
		unsigned int imm : 11;
	}iType;
	struct tagSty {
		unsigned int opcode : 7;
		unsigned int imm_4_0 : 5;
		unsigned int funct3 : 3;
		unsigned int rs1 : 5;
		unsigned int rs2 : 5;
		unsigned int imm_11_5 : 7;
	}sType;
	struct tagBTy {
		unsigned int opcode : 7;
		unsigned int imm_11 : 1;
		unsigned int imm_4_1 : 4;
		unsigned int funct3 : 3;
		unsigned int rs1 : 5;
		unsigned int rs2 : 5;
		unsigned int imm_10_5 : 6;
		unsigned int imm_12 : 1;
	}bType;
	struct tagUTy {
		unsigned int opcode : 7;
		unsigned int rd : 5;
		unsigned int imm_31_12 : 20;
	}uType;
	struct tagJTy {
		unsigned int opcode : 7;
		unsigned int rd : 5;
		unsigned int imm_19_12 : 8;
		unsigned int imm_11 : 1;
		unsigned int imm_10_1 : 10;
		unsigned int imm_20 : 1;
	}jType;
};

#define OP_LUI 0b0110111
#define OP_AUIPC 0b0010111
#define OP_JAL 0b1101111
#define OP_JALR 0b1100111
#define OP_BTYPE 0b1100011
#define OP_LOAD 0b0000011
#define OP_STORE 0b0100011
#define OP_ALU_IMM 0b0010011
#define OP_ALU 0b0110011
#define OP_SYSTEM 0b1110011

unsigned int immgen(Instruction ins);

#define MEME_OK 0
#define MEME_ADDR_NOT_ALIGNED 1
#define MEME_ACCESS_DENIED 2
#define MEME_PAGE_FAULT 3
#define IOF_READ 0
#define IOF_WRITE 1
#define IOF_EXEC 2
#define IOF_USR 4

 
class Memioresult {
public:
	unsigned int val;
	int ecode;
	inline constexpr Memioresult(unsigned int v,int ec ):val(v),ecode
	(ec){	}
	inline Memioresult(unsigned long long i64val) {
		val = i64val >> 32;
		ecode = i64val & 0xffffffff;
	}
	inline unsigned long long i64resforregret()const {
		unsigned long long res = ((unsigned long long )val) << 32;
		res |= ecode;
		return res;
	}
	inline static int getecode(unsigned long long i64val) {
		return i64val & 0xffffffff;
	}
	inline static unsigned int getval(unsigned long long i64val) {
		return i64val >> 32;
	}
};

class MemController {
	unsigned int* cpuCSRs;
	myMem memory;
public:
	unsigned long long vaddr_to_paddr(unsigned int addr, int io_flag) ;//assume can access
	unsigned int read32(unsigned int addr);
	unsigned int read16(unsigned int addr);
	unsigned int read8(unsigned int addr);
	void write32(unsigned int addr, unsigned int val);
	void write16(unsigned int addr, unsigned short val);
	void write8(unsigned int addr, unsigned char val);
};

class Cpu_
{
public:
	void Invoke_err();//reserved
	void Invoke_int();//reserved

private:
	REGS regs;
	bool sig_abort_exec1;
	unsigned int CSRs[4096];
	struct
	{
		unsigned int MP : 2;
		unsigned int reserved : 30;
	}mflags;
	MemController memctrl;
	unsigned int ALUoperation(unsigned int a, unsigned int b, Instruction ins);
	void ins_exec(Instruction ins);
	void _into_trap(tagCSR::tagmcause cause, unsigned int mtval);
};

#endif