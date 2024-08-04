#ifndef mCPUH
#define mCPUH
#include "mem.h"
#define RV32_

inline unsigned int bl_endian_switch(unsigned int val) {
	unsigned int ret = val << 24;
	ret |= val >> 24;
	ret |= (val & 0x00ff0000) >> 8;
	ret |= (val & 0x0000ff00) << 8;
	return ret;
}

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

class MemController {
	REGS* cpuregs;//todo
	myMem memory;
public:
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
	unsigned int CSRs[4096];
	MemController memctrl;
	unsigned int ALUoperation(unsigned int a, unsigned int b, Instruction ins);
	void ins_exec(Instruction ins);
	void _into_trap(tagCSR::tagmcause cause);
};

#endif