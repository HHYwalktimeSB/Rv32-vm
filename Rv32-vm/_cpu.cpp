#include "_cpu.h"

#define MASK_PPN_1 0xfff00000

#define MAKE_MEM_RW_RESULT_OK(_VAL_) ((unsigned long long)(_VAL_))<<32
#define GET_MEM_RW_RESULT_VAL(_VAL_) ((unsigned int)(_VAL_>>32))

int (Cpu_::*Cpu_::_decodeheperfuncs[128]) (Instruction);

void Cpu_::_init()
{
	regs.pc = 0x80000000;
	for (auto i = 0; i < 32; i++)
		regs.x[i] = 0;
	for (auto i = 0; i < 4096; i++)
		CSRs[i] = 0;
	Mode = MODE_MACHINE;
	regs.instruction_ecode = -1;
	//regs.csrs = CSRs;
	*reinterpret_cast<unsigned int*>( &debugflags) = 0;
	debugflags.flag_run = 1;
	debugflags.flag_exit_when_inv_ins = 1;
}

bool Cpu_::_csr_readable(int csrid)
{
	return this->Mode >= ((csrid >> 8) & 3);
}

bool Cpu_::_csr_writeable(int csrid)
{
	return ( this->Mode >= ((csrid >> 8) & 3) ) && 
		((csrid&0xc00) != 0xc00);
}

unsigned int Cpu_::ALUoperation(unsigned int a, unsigned int b, Instruction ins)
{
	unsigned int ret = 0;
	if (ins.bType.opcode == OP_ALU_IMM && 
		ins.rType.funct3 != 0b101) 
		ins.rType.funct7 = 0;

	switch (ins.rType.funct3)
	{
	case 0:
		ret = ins.rType.funct7 == 0b0100000 ? a - b : a + b;
		break;
	case 1:
		ret = a << b;
		break;
	case 2:
		ret = static_cast<int>(a) < static_cast<int>(b) ? 1 : 0;
		break;
	case 3:
		ret = a < b ? 1 : 0;
		break;
	case 4:
		ret = a ^ b;
		break;
	case 5:
		b &= 31;
		if (ins.rType.funct7 == 0b0100000) {
			ret = a >> b;
			ret |= ((0xffffffff >> b) ^ 0xffffffff);
		}
		else ret = a >> b;
		break;
	case 6:
		ret = a | b;
		break;
	case 7:
		ret = a & b;
		break;
	}
	return ret;
}

void Cpu_::_into_trap()
{
	auto mst = reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus]);
	mst->MPIE = mst->MIE;
	mst->MIE = 0;
	mst->MPP = Mode;
	Mode = MODE_MACHINE;
	regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base;
}

void Cpu_::_into_int()
{

	auto mst = reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus]);
	mst->MPIE = mst->MIE;
	mst->MIE = 0;
	mst->MPP = Mode;
	Mode = MODE_MACHINE;

	if (reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->mode == 1)
		regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base +
		reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause])->exception_code * 4;
	else 
		regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base;
}

void Cpu_::_make_exception(int code, unsigned int mtval)
{
	if (reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus])->MIE == 0)return;
	this->debugflags.eflag = 1;
	CSRs[(int)CSRid::mtval] = mtval;
	reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause])->Interrupt = 0;
	reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause])->exception_code = code;
	switch (code) {
	case EXC_BREAKPOINT:
	case EXC_ECALL_FROM_MMODE:
	case EXC_ECALL_FROM_SMODE:
	case EXC_ECALL_FROM_UMODE:
		CSRs[(int)CSRid::mepc] = regs.pc + 4;
		break;
	default:
		CSRs[(int)CSRid::mepc] = regs.pc;
		break;
	}
}

void Cpu_::_make_exception(int code)
{
	if (reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus])->MIE == 0)return;
	this->debugflags.eflag = 1;
	reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause])->Interrupt = 0;
	reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause])->exception_code = code;
	switch (code) {
	case EXC_BREAKPOINT:
	case EXC_ECALL_FROM_MMODE:
	case EXC_ECALL_FROM_SMODE:
	case EXC_ECALL_FROM_UMODE:
		CSRs[(int)CSRid::mepc] = regs.pc + 4;
		break;
	default:
		CSRs[(int)CSRid::mepc] = regs.pc;
		break;
	}
}

void Cpu_::_make_mem_exception(unsigned int meme_code, unsigned io_code)
{
	switch (meme_code)
	{
	case MEME_ADDR_NOT_ALIGNED:
		if (io_code == IOF_READ)_make_exception(EXC_LOAD_ADDR_NOT_ALIGNED,0);
		else _make_exception(EXC_STORE_ADDR_NOT_ALIGNED,0);
		break;
	case MEME_ACCESS_DENIED:
		if (io_code == IOF_READ) _make_exception(EXC_LOAD_ACCESS_FAULT,0);
		else _make_exception(EXC_STORE_ACCESS_FAULT,0);
		break;
	case MEME_PAGE_FAULT:
		if (io_code == IOF_READ)_make_exception(EXC_LOAD_PAGE_FAULT,0);
		else _make_exception(EXC_STORE_PAGE_FAULT,0);
		break;
	default:
		break;
	}
}

void Cpu_::Invoke_int(unsigned usrreason)
{
	if (reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus])->MIE == 0)return;
	debugflags.flag_int = 1;
	debugflags.reason_for_int = usrreason;
}

int Cpu_::_ins_exec_op_alu(Instruction ins)
{
	register int res = 0,a = regs.x[ins.rType.rs1],b = regs.x[ins.rType.rs2];
	switch (ins.rType.funct3)
	{
	case 0:
		res = ins.rType.funct7 == 0b0100000 ? a - b : a + b;
		break;
	case 1:
		res = a << b;
		break;
	case 2:
		res = static_cast<int>(a) < static_cast<int>(b) ? 1 : 0;
		break;
	case 3:
		res = a < b ? 1 : 0;
		break;
	case 4:
		res = a ^ b;
		break;
	case 5:
		b &= 31;
		if (ins.rType.funct7 == 0b0100000) {
			res = a >> b;
			res |= ((0xffffffff >> b) ^ 0xffffffff);
		}
		else res = a >> b;
		break;
	case 6:
		res = a | b;
		break;
	case 7:
		res = a & b;
		break;
	}
	if (ins.rType.rd != 0)regs.x[ins.rType.rd] = res;
	return EXC_NO_EXCEPTION;
}

int Cpu_::_ins_exec_op_aluimm(Instruction ins)
{
	register int res = 0, a = regs.x[ins.rType.rs1], b = immgen(ins);
	switch (ins.rType.funct3)
	{
	case 0:
		res = a + b;
		break;
	case 1:
		res = a << ins.rType.rs2;
		break;
	case 2:
		res = static_cast<int>(a) < static_cast<int>(b) ? 1 : 0;
		break;
	case 3:
		res = a < b ? 1 : 0;
		break;
	case 4:
		res = a ^ b;
		break;
	case 5:
		b &= 31;
		if (ins.rType.funct7 == 0b0100000) {
			res = a >> b;
			res |= ((0xffffffff >> b) ^ 0xffffffff);
		}
		else res = a >> b;
		break;
	case 6:
		res = a | b;
		break;
	case 7:
		res = a & b;
		break;
	}
	if (ins.rType.rd != 0)regs.x[ins.rType.rd] = res;
	return EXC_NO_EXCEPTION;
}

int Cpu_::_ins_exec_op_load(Instruction ins)
{
	register unsigned long long _ret_buf;
	unsigned int addr = static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins));
	switch (ins.iType.funct3)
	{
	case 0://lb
		_ret_buf = memctrl.read_unsafe(addr, Mode | (1 << 2));
		if (_ret_buf & 0xffffffff) {
			CSRs[(int)CSRid::mtval] = addr;
			return (int)_ret_buf;
		}
		if (ins.iType.rd != 0) regs.x[ins.iType.rd] = _sign_ext<8>(GET_MEM_RW_RESULT_VAL(_ret_buf));
		break;
	case 1:
		_ret_buf = memctrl.read_unsafe(addr, Mode | (2 << 2));
		if (_ret_buf & 0xffffffff) {
			CSRs[(int)CSRid::mtval] = addr;
			return (int)_ret_buf;
		}
		if (ins.iType.rd != 0) regs.x[ins.iType.rd] = _sign_ext<16>(GET_MEM_RW_RESULT_VAL(_ret_buf));
		break;
	case 2:
		_ret_buf = memctrl.read_unsafe(addr, Mode | (4 << 2));
		if (_ret_buf & 0xffffffff) {
			CSRs[(int)CSRid::mtval] = addr;
			return (int)_ret_buf;
		}
		if (ins.iType.rd != 0) regs.x[ins.iType.rd] = GET_MEM_RW_RESULT_VAL(_ret_buf);
		break;
	case 4:
		_ret_buf = memctrl.read_unsafe(addr, Mode | (1 << 2));
		if (_ret_buf & 0xffffffff) {
			CSRs[(int)CSRid::mtval] = addr;
			return (int)_ret_buf;
		}
		if (ins.iType.rd != 0) regs.x[ins.iType.rd] = GET_MEM_RW_RESULT_VAL(_ret_buf);
		break;
	case 5:
		_ret_buf = memctrl.read_unsafe(addr, Mode | (2 << 2));
		if (_ret_buf & 0xffffffff) {
			CSRs[(int)CSRid::mtval] = addr;
			return (int)_ret_buf;
		}
		if (ins.iType.rd != 0) regs.x[ins.iType.rd] = GET_MEM_RW_RESULT_VAL(_ret_buf);
		break;
	default:
		return EXC_INV_INSTRUCTION;
	}
	return EXC_NO_EXCEPTION;
}

int Cpu_::_ins_exec_op_store(Instruction ins)
{
	unsigned int _ret_buf;
	unsigned int addr = static_cast<int>(regs.x[ins.sType.rs1]) + static_cast<int>(immgen(ins));
	switch (ins.sType.funct3)
	{
	case 0:
		_ret_buf = memctrl.write_unsafe(addr, regs.x[ins.sType.rs2], Mode | (1 << 2));
		if (_ret_buf != 0) {
			CSRs[(int)CSRid::mtval] = addr;
			return _ret_buf;
		}
		break;
	case 1:
		_ret_buf = memctrl.write_unsafe(addr, regs.x[ins.sType.rs2], Mode | (2 << 2));
		if (_ret_buf != 0) {
			CSRs[(int)CSRid::mtval] = addr;
			return _ret_buf;
		}
		break;
	case 3:
		_ret_buf = memctrl.write_unsafe(addr, regs.x[ins.sType.rs2], Mode | (4 << 2));
		if (_ret_buf != 0) {
			CSRs[(int)CSRid::mtval] = addr;
			return _ret_buf;
		}
		break;
	default:
		return EXC_INV_INSTRUCTION;
	}
	return -1;
}

int Cpu_::_ins_exec_op_system(Instruction ins)
{
	return EXC_INV_INSTRUCTION;
}

int Cpu_::_ins_exec_op_bty(Instruction ins)
{
	bool if_jmp;
	switch (ins.bType.funct3)
	{
	case 0:
		if_jmp = regs.x[ins.bType.rs1] == regs.x[ins.bType.rs2];
		break;
	case 1:
		if_jmp = regs.x[ins.bType.rs1] != regs.x[ins.bType.rs2];
		break;
	case 4:
		if_jmp = static_cast<int>(regs.x[ins.bType.rs1]) < static_cast<int>(regs.x[ins.bType.rs2]);
		break;
	case 5:
		if_jmp = static_cast<int>(regs.x[ins.bType.rs1]) >= static_cast<int>(regs.x[ins.bType.rs2]);
		break;
	case 6:
		if_jmp = regs.x[ins.bType.rs1] < regs.x[ins.bType.rs2];
		break;
	case 7:
		if_jmp = regs.x[ins.bType.rs1] >= regs.x[ins.bType.rs2];
		break;
	default:
		return EXC_INV_INSTRUCTION;
	}
	if (if_jmp) {
		regs.pc += (static_cast<unsigned int>(immgen(ins)) - 4 );
	}
	return EXC_NO_EXCEPTION;
}

int Cpu_::_ins_exec_op_jal(Instruction ins)
{
	if (ins.jType.rd != 0)
		regs.x[ins.jType.rd] = regs.pc + 4;
	regs.pc += (static_cast<int>(immgen(ins)) -4 );
	return -1;
}

int Cpu_::_ins_exec_op_jalr(Instruction ins)
{
	if (ins.iType.rd != 0)
		regs.x[ins.iType.rd] = regs.pc + 4;
	regs.pc = static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins)) -4;
	return -1;
}

int Cpu_::_ins_exec_op_lui(Instruction ins)
{
	if (ins.uType.rd != 0)
		regs.x[ins.uType.rd] =
		immgen(ins);
	return -1;
}

int Cpu_::_ins_exec_op_auipc(Instruction ins)
{
	if (ins.uType.rd != 0)
		regs.x[ins.uType.rd] = regs.pc +
		static_cast<int>(immgen(ins));
	return -1;
}

int Cpu_::_ins_exec_op_unknown(Instruction ins)
{
	return EXC_INV_INSTRUCTION;
}

void Cpu_::_init_ftable()
{
	for (unsigned i = 0; i < 128; ++i)
		_decodeheperfuncs[i] = &Cpu_::_ins_exec_op_unknown;
	_decodeheperfuncs[OP_ALU] = &Cpu_::_ins_exec_op_alu;
	_decodeheperfuncs[OP_ALU_IMM] = &Cpu_::_ins_exec_op_aluimm;
	_decodeheperfuncs[OP_LOAD] = &Cpu_::_ins_exec_op_load;
	_decodeheperfuncs[OP_STORE] = &Cpu_::_ins_exec_op_store;
	_decodeheperfuncs[OP_SYSTEM] = &Cpu_::_ins_exec_op_system;
	_decodeheperfuncs[OP_BTYPE] = &Cpu_::_ins_exec_op_bty;
	_decodeheperfuncs[OP_JAL] = &Cpu_::_ins_exec_op_jal;
	_decodeheperfuncs[OP_JALR] = &Cpu_::_ins_exec_op_jalr;
	_decodeheperfuncs[OP_LUI] = &Cpu_::_ins_exec_op_lui;
	_decodeheperfuncs[OP_AUIPC] = &Cpu_::_ins_exec_op_auipc;

}

Cpu_::Cpu_(unsigned int mem_sz):memctrl(mem_sz, this->CSRs)
{
	thandle = nullptr;
	Mode = 0;
	cache = 0;
	debugflags = { 0 };
	regs = { 0 };
	shandle = nullptr;
	tstste_sus = 0;
	CSRs = new unsigned[4096];
	memctrl.cpuCSRs = CSRs;
#ifdef _JITTOOLS_ENABLED
	cache = new MambaCache_((char*)memctrl.memory.native_ptr(), memctrl.memory.mask());
#endif // _JITTOOLS_ENABLED
}

Cpu_::~Cpu_()
{
	delete[] CSRs;
#ifdef _JITTOOLS_ENABLED
	if (cache)delete cache;
#endif // _JITTOOLS_ENABLED
}

unsigned int __fastcall chk_can_rw_(unsigned int pe, int io_flag) {
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return MEME_PAGE_FAULT;
	switch (io_flag & 3)
	{
	case IOF_READ:
		if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->R)) return MEME_ACCESS_DENIED;
		if (io_flag & IOF_USR) {
			if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return MEME_ACCESS_DENIED;
		}
		else
		{
			if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U &&
				!CSR_HAVE_FLAG_SUM)return MEME_ACCESS_DENIED;
		}
		break;
	case IOF_WRITE:
		if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->W)) return MEME_ACCESS_DENIED;
		if (io_flag & IOF_USR) {
			if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return MEME_ACCESS_DENIED;
		}
		else
		{
			if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U &&
				!CSR_HAVE_FLAG_SUM)return MEME_ACCESS_DENIED;
		}
		break;
	case IOF_EXEC:
		if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->X)) return MEME_ACCESS_DENIED;
		if (io_flag & IOF_USR) {
			if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return MEME_ACCESS_DENIED;
		}
		else
		{
			if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U)return MEME_ACCESS_DENIED;
		}
		break;
	}
	return 0;
}

#include<stdarg.h>

unsigned long MemController::shared_io_interfence(unsigned int addr, unsigned int ioinfo, ...)
{
	unsigned long long r = 0;
	_DevBase* dev;
	if (addr < 0x40000000) {
		r = (addr & 0x70000000) / (0x40000000 / _DEV_lower_allocsz);
		dev = loweraddrdevs[(unsigned short)r];
		addr -= r * (0x40000000 / _DEV_lower_allocsz);
	}
	else {
		r = (addr & 0x70000000) / (0x40000000 / _DEV_upper_allocsz);
		dev = upperaddrdev[(unsigned short)r];
		addr -= r * (0x40000000 / _DEV_upper_allocsz);
	}
	if (dev == nullptr)return 0;
	if (mioflag_read & ioinfo) {
		r = dev->read(addr, ioinfo & 7);
	}
	else {
		va_list ap; va_start(ap, ioinfo);
		dev->write(addr, va_arg(ap, unsigned), ioinfo & 7);
	}
	return r;
}

unsigned long long MemController::read_ins(unsigned int addr, unsigned int mode)
{
	if ((addr & 3) != 0)return MEME_ADDR_NOT_ALIGNED;
	unsigned long long ret;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr(addr, IOF_EXEC | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) ret = MAKE_MEM_RW_RESULT_OK(memory._readp32(
			GET_MEM_RW_RESULT_VAL(ret)));
	}
	else {
		ret = (addr & 0x80000000) ?
			MAKE_MEM_RW_RESULT_OK ( memory._readp32(addr & 0x7fffffff) )
			: shared_io_interfence(addr, mioflag_read | 4);
	}
	return ret;
}

unsigned long long MemController::vaddr_to_paddr(unsigned int addr, int io_flag)
{
	unsigned int pmemaddr = (cpuCSRs[(int)CSRid::stap] & MASK_STAP_PPN )<< 10;
	pmemaddr |= (reinterpret_cast<tagVaddr*>(&addr)->vpn_1);
	pmemaddr <<= 2;
	auto pe = memory._readp32(pmemaddr);
	unsigned int rx;
	if (pe & MASK_XWR)
	{//LARGE PAGE
		if ((rx = chk_can_rw_(pe, io_flag))) return rx;
		//set flags
		reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
		if ((io_flag & 3) == IOF_WRITE)
			reinterpret_cast<sv32pagetable_entry*>(&pe)->D = 1;
		memory._writep32(pmemaddr, pe);
		//make 4m page paddr
		return MAKE_MEM_RW_RESULT_OK(((pe & MASK_PPN_1) << 2) |
			(addr & (MASK_VPN_0 | MASK_OFFSET4K)));
	}else if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return MEME_PAGE_FAULT;
	//2nd entry
	pmemaddr = pe & (MASK_PPN_1 | MASK_VPN_0);
	pmemaddr = reinterpret_cast<tagVaddr*>(&addr)->vpn_0;
	pe = memory._readp32(pmemaddr);
	if ((rx = chk_can_rw_(pe, io_flag)))return rx;
	//set flags
	reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	if ((io_flag & 3) == IOF_WRITE)
		reinterpret_cast<sv32pagetable_entry*>(&pe)->D = 1;
	memory._writep32(pmemaddr, pe);
	//make 4k page paddr
	return MAKE_MEM_RW_RESULT_OK(((pe & (MASK_PPN_1|MASK_VPN_0)) << 2) |
		(addr &  MASK_OFFSET4K));
}

unsigned long long MemController::read32(unsigned int addr, unsigned int mode)
{
	if ((addr & 3) != 0)return MEME_ADDR_NOT_ALIGNED;
	unsigned long long ret;
	if (mode!=3&&(cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr(addr, IOF_READ | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) ret = (unsigned long long)memory._readp32(GET_MEM_RW_RESULT_VAL(ret));
		else return ret;
	}
	else {
		ret = (addr & 0x80000000) ? (memory._readp32(addr & 0x7fffffff))
			: shared_io_interfence(addr, mioflag_read | 4);
	}
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
		ret = (unsigned long long)bl_endian_switch32((unsigned int)ret);
	return ret << 32;
}

unsigned long long MemController::read16(unsigned int addr, unsigned int mode)
{
	if ((addr & 1) != 0)return MEME_ADDR_NOT_ALIGNED;
	unsigned long long ret;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr(addr, IOF_READ | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) ret = (unsigned long long)memory._readp16(GET_MEM_RW_RESULT_VAL(ret));
		else return ret;
	}
	else {
		ret = (addr & 0x80000000) ? (memory._readp16(addr & 0x7fffffff))
			: shared_io_interfence(addr, mioflag_read | 2);
	}
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
		ret = (unsigned long long)bl_endian_switch16((unsigned short)ret);
	return ret<<32;
}

unsigned long long MemController::read8(unsigned int addr, unsigned int mode)
{
	unsigned long long ret;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr(addr, IOF_READ | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) ret = MAKE_MEM_RW_RESULT_OK(memory._readp8(
			GET_MEM_RW_RESULT_VAL(ret)));
	}
	else {
		ret = (addr & 0x80000000) ? (memory._readp8(addr & 0x7fffffff))
			: shared_io_interfence(addr,mioflag_read | 1);
	}
	return ret<<32;
}

unsigned int MemController::write32(unsigned int addr, unsigned int val, unsigned int mode)
{
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
		val = bl_endian_switch32(val);
	if ((addr & 3) != 0)return MEME_ADDR_NOT_ALIGNED;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		unsigned long long ret;
		ret = vaddr_to_paddr(addr, IOF_WRITE | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) addr = GET_MEM_RW_RESULT_VAL(ret);
		else return ret & 0xffffffff;
	}
	else {
		if (addr & 0x80000000)
			memory._writep32(addr & 0x7fffffff, val);
		else shared_io_interfence(addr, mioflag_write | 4, val);
	}
	return MEME_OK;
}

unsigned int MemController::write16(unsigned int addr, unsigned short val, unsigned int mode)
{
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
		val = bl_endian_switch16(val);
	if ((addr & 1) != 0)return MEME_ADDR_NOT_ALIGNED;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		unsigned long long ret;
		ret = vaddr_to_paddr(addr, IOF_WRITE | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) addr = GET_MEM_RW_RESULT_VAL(ret);
		else return ret & 0xffffffff;
	}
	else {
		if (addr & 0x80000000)
			memory._writep16(addr & 0x7fffffff, val);
		else shared_io_interfence(addr, mioflag_write | 2, (unsigned int)val);
	}
	return MEME_OK;
}

unsigned int MemController::write8(unsigned int addr, unsigned char val, unsigned int mode)
{
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		unsigned long long ret;
		ret = vaddr_to_paddr(addr, IOF_WRITE | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) addr = GET_MEM_RW_RESULT_VAL(ret);
		else return ret & 0xffffffff;
	}
	else {
		if (addr & 0x80000000)
			memory._writep8(addr & 0x7fffffff, val);
		else shared_io_interfence(addr, mioflag_write | 1, (unsigned)val);
	}
	return MEME_OK;
}

MemController::MemController(unsigned memsz, unsigned int* csr)
{
	memory._init(memsz);
	cpuCSRs = csr;	
	membase = memory.native_ptr();
	mem_mask = memory.mask();
	for (unsigned i = 0; i < _DEV_lower_allocsz; ++i) loweraddrdevs[i] = nullptr;
	for (unsigned i = 0; i < _DEV_upper_allocsz; ++i) upperaddrdev[i] = nullptr;
}

inline unsigned long long MemController::vaddr_to_paddr_read_unsafe(unsigned int addr, unsigned int mode)
{
	unsigned int pmemaddr = (cpuCSRs[(int)CSRid::stap] & MASK_STAP_PPN) << 10;
	pmemaddr |= (reinterpret_cast<tagVaddr*>(&addr)->vpn_1);
	pmemaddr <<= 2;
	unsigned int pe = *reinterpret_cast <unsigned int*>(membase +  (pmemaddr&mem_mask ));

	//check can read
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_LOAD_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->R))return EXC_LOAD_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_LOAD_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_LOAD_ACCESS_FAULT;
	}
	//set flags
	//reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast <sv32pagetable_entry*>(membase + (pmemaddr & mem_mask))->A = 1;

	if (pe & MASK_XWR)
	{//LARGE PAGE
		//make 4m page paddr
		return MAKE_MEM_RW_RESULT_OK(((pe & MASK_PPN_1) << 2) |
			(addr & (MASK_VPN_0 | MASK_OFFSET4K)));
	}
	//handle 2nd entry
	pmemaddr = pe & (MASK_PPN_1 | MASK_VPN_0);
	pmemaddr = reinterpret_cast<tagVaddr*>(&addr)->vpn_0;
	pe = *reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask));
	//check read or write
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_LOAD_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->R))return EXC_LOAD_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_LOAD_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_LOAD_ACCESS_FAULT;
	}
	//set flags
	//reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast <sv32pagetable_entry*>(membase + (pmemaddr & mem_mask))->A =1;

	//make 4k page paddr
	return MAKE_MEM_RW_RESULT_OK(((pe & (MASK_PPN_1 | MASK_VPN_0)) << 2) |
		(addr & MASK_OFFSET4K));
}

inline unsigned long long MemController::vaddr_to_paddr_exec_unsafe(unsigned int addr, unsigned int mode)
{
	unsigned int pmemaddr = (cpuCSRs[(int)CSRid::stap] & MASK_STAP_PPN) << 10;
	pmemaddr |= (reinterpret_cast<tagVaddr*>(&addr)->vpn_1);
	pmemaddr <<= 2;
	unsigned int pe = *reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask));

	//check can read
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_INSTRUCTION_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->X))return EXC_INSTRUCTION_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_INSTRUCTION_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_INSTRUCTION_ACCESS_FAULT;
	}
	//set flags
	//reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast <sv32pagetable_entry*>(membase + (pmemaddr & mem_mask))->A = 1;

	if (pe & MASK_XWR)
	{//LARGE PAGE
		//make 4m page paddr
		return MAKE_MEM_RW_RESULT_OK(((pe & MASK_PPN_1) << 2) |
			(addr & (MASK_VPN_0 | MASK_OFFSET4K)));
	}
	//handle 2nd entry
	pmemaddr = pe & (MASK_PPN_1 | MASK_VPN_0);
	pmemaddr = reinterpret_cast<tagVaddr*>(&addr)->vpn_0;
	pe = *reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask));
	//check read or write
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_INSTRUCTION_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->X))return EXC_INSTRUCTION_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_INSTRUCTION_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_INSTRUCTION_ACCESS_FAULT;
	}
	//set flags
	//reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast <sv32pagetable_entry*>(membase + (pmemaddr & mem_mask))->A = 1;

	//make 4k page paddr
	return MAKE_MEM_RW_RESULT_OK(((pe & (MASK_PPN_1 | MASK_VPN_0)) << 2) |
		(addr & MASK_OFFSET4K));
}

inline unsigned long long MemController::vaddr_to_paddr_write_unsafe(unsigned int addr, unsigned int mode)
{
	unsigned int pmemaddr = (cpuCSRs[(int)CSRid::stap] & MASK_STAP_PPN) << 10;
	pmemaddr |= (reinterpret_cast<tagVaddr*>(&addr)->vpn_1);
	pmemaddr <<= 2;
	unsigned int pe = *reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask));

	//check can read
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_STORE_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->W))return EXC_STORE_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_STORE_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_STORE_ACCESS_FAULT;
	}
	//set flags
	reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast<sv32pagetable_entry*>(&pe)->D = 1;
	*reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask)) = pe;

	if (pe & MASK_XWR)
	{//LARGE PAGE
		//make 4m page paddr
		return MAKE_MEM_RW_RESULT_OK(((pe & MASK_PPN_1) << 2) |
			(addr & (MASK_VPN_0 | MASK_OFFSET4K)));
	}
	//handle 2nd entry
	pmemaddr = pe & (MASK_PPN_1 | MASK_VPN_0);
	pmemaddr = reinterpret_cast<tagVaddr*>(&addr)->vpn_0;
	pe = *reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask));
	//check read or write
	if (!reinterpret_cast<sv32pagetable_entry*>(&pe)->V)return EXC_STORE_PAGE_FAULT;
	if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->W))return EXC_STORE_ACCESS_FAULT;
	if (mode == MODE_USR) { if (!(reinterpret_cast<sv32pagetable_entry*>(&pe)->U)) return EXC_STORE_ACCESS_FAULT; }
	else {
		if (reinterpret_cast<sv32pagetable_entry*>(&pe)->U && !CSR_HAVE_FLAG_SUM)return EXC_STORE_ACCESS_FAULT;
	}
	//set flags
	reinterpret_cast<sv32pagetable_entry*>(&pe)->A = 1;
	reinterpret_cast<sv32pagetable_entry*>(&pe)->D = 1;
	*reinterpret_cast <unsigned int*>(membase + (pmemaddr & mem_mask)) = pe;

	//make 4k page paddr
	return MAKE_MEM_RW_RESULT_OK(((pe & (MASK_PPN_1 | MASK_VPN_0)) << 2) |
		(addr & MASK_OFFSET4K));
}

unsigned long long MemController::read_unsafe(unsigned int addr, unsigned int mode)
{
	unsigned long long ret = 0;
	if ((mode&3) != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr_read_unsafe(addr, mode & 3);
		if ((ret & 0xffffffff) == 0) {
			addr = GET_MEM_RW_RESULT_VAL(ret);
			addr &= mem_mask;
			switch (mode >> 2)
			{
			case 1:
				ret = *reinterpret_cast<unsigned char*> (membase + addr);
				break;
			case 2:
				if (addr & 1) return EXC_LOAD_ADDR_NOT_ALIGNED;
				ret = *reinterpret_cast<unsigned short*> (membase + addr);
				break;
			case 4:
				if (addr & 3) return EXC_LOAD_ADDR_NOT_ALIGNED;
				ret = *reinterpret_cast<unsigned int*> (membase + addr);
				break;
			}
		}
		else return ret;
	}
	else if(addr& 0x80000000){
		addr &= 0x7fffffff;
		switch (mode >> 2)
		{
		case 1:
			ret =  *reinterpret_cast<unsigned short*>(membase + (addr & mem_mask));
			break;
		case 2:
			if (addr & 1) return EXC_LOAD_ADDR_NOT_ALIGNED;
			ret = *reinterpret_cast<unsigned short*>(membase + (addr & mem_mask));
			break;
		case 4:
			if (addr & 3) return EXC_LOAD_ADDR_NOT_ALIGNED;
			ret = *reinterpret_cast<unsigned int*>(membase + (addr & mem_mask));
			break;
		}
	}
	else ret = shared_io_interfence(addr, mioflag_read | (mode>>2) );
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
	{
		switch (mode >> 2)
		{
		case 2:
			ret = bl_endian_switch16((unsigned short)ret);
			break;
		case 4:
			ret = bl_endian_switch32((unsigned int)ret);
		}
	}
	return ret << 32;
}

unsigned int MemController::write_unsafe(unsigned int addr, unsigned int val, unsigned int mode)
{
	if (reinterpret_cast<tagCSR::tagmstatush*>(&cpuCSRs[(int)CSRid::mstatush])->MBE)
		val = bl_endian_switch32(val);
	if ((mode &3) != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		unsigned long long ret;
		ret = vaddr_to_paddr_write_unsafe(addr, mode);
		if ((ret & 0xffffffff) == 0) addr = GET_MEM_RW_RESULT_VAL(ret);
		else return ret & 0xffffffff;
		addr &= mem_mask;
		switch (mode>>2)
		{
		case 1:
			*reinterpret_cast<unsigned char*> (membase + addr) = (unsigned char)val;
			break;
		case 2:
			if (addr & 1) return EXC_STORE_ADDR_NOT_ALIGNED;
			*reinterpret_cast<unsigned short*> (membase + addr) = (unsigned short)val;
			break;
		case 4:
			if (addr & 3) return EXC_STORE_ADDR_NOT_ALIGNED;
			*reinterpret_cast<unsigned int*> (membase + addr) = val;
			break;
		}
	}
	else {
		if (addr & 0x80000000) {
			addr &= (mem_mask & 0x7fffffff);
			switch (mode>>2)
			{
			case 4:
				if (addr & 3) return EXC_STORE_ADDR_NOT_ALIGNED;
				*reinterpret_cast<unsigned *>(membase + addr ) = val;
				break;
			case 2:
				if (addr & 1) return EXC_STORE_ADDR_NOT_ALIGNED;
				*reinterpret_cast<unsigned short*>(membase + addr) = (unsigned short )val;
				break;
			case 1:
				*reinterpret_cast<unsigned char*>(membase + addr) = (unsigned char)val;
				break;
			}
		}
		else shared_io_interfence(addr, mioflag_write | 4, val);
	}
	return MEME_OK;
}

unsigned long long MemController::read_ins_unsafe(unsigned int addr, unsigned int mode)
{
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		unsigned long long ret = vaddr_to_paddr_write_unsafe(addr, mode);
		if ((ret & 0xffffffff) == 0) addr = GET_MEM_RW_RESULT_VAL(ret);
		else return ret & 0xffffffff;
		return MAKE_MEM_RW_RESULT_OK (*reinterpret_cast<unsigned int*>(membase + ( addr & mem_mask ) )) ;
	}
	else
	{
		if (addr & 0x80000000) {
			return MAKE_MEM_RW_RESULT_OK(*reinterpret_cast<unsigned int*>(membase + (addr & mem_mask & 0x7fffffff)));
		}
	}
	return 0;
}

MemController::~MemController()
{
}

void CPUdebugger::setpc(unsigned int val)
{
	pcpu->regs.pc = val;
}

void CPUdebugger::setreg(unsigned int id, int val)
{
	if (id == 0 || id >= 32)return;
	pcpu->regs.x[id] = val;
}

const REGS* CPUdebugger::getregs()
{
	return &(pcpu->regs);
}

void CPUdebugger::writecpustate(_cpustate* s)
{
	memcpy_s(s, sizeof(REGS), &(pcpu->regs), sizeof(REGS));
}

#include<iomanip>

int CPUdebugger::cmpcpustate(const _cpustate* prevstate)
{
	int detect = 0;
	for (int i = 0; i < 32; ++i) {
		if (prevstate->regs.x[i] != pcpu->regs.x[i])
		{
			//printf("change in x%d: %u->%u\n", i, prevstate->regs.x[i],pcpu->regs.x[i]);
			std::cout << "change in x" << std::oct << i << ": " << std::hex << prevstate->regs.x[i]
				<< "->" << pcpu->regs.x[i] << std::endl;
			detect++;
		}
	}
	if (prevstate->regs.pc != pcpu->regs.pc) {
		//printf("change in pc: %x->%x\n", prevstate->regs.pc, pcpu->regs.pc);
		std::cout << "change in pc: " << std::hex << prevstate->regs.pc
			<< "->" << pcpu->regs.pc << std::endl;
		detect++;
	}
	return detect;
}

void CPUdebugger::reset_cpu_regs()
{
	pcpu->regs.pc = 0;
	pcpu->regs.cycles = 0;
	for (unsigned i = 0; i < 32; ++i)pcpu->regs.x[i] = 0;
}

#include<Windows.h>
#include<fstream>
#include<stdio.h>

void CPUdebugger::printregs(unsigned int start, unsigned int end)
{
}

void CPUdebugger::commit_command(const std::string& comm)
{
}

void CPUdebugger::bind(Cpu_* pc)
{
	this->pcpu = pc;
	mode = sync;
}

void CPUdebugger::simple_run()
{
	_cpustate st;
	this->writecpustate(&st);
	auto s = clock();
	pcpu->runsync_with_jit();
	s = clock() - s;
	std::cout <<"time taked " << s << "ms\ncycles: " << pcpu->regs.cycles <<std::endl;
	cmpcpustate(&st);
	return;
}

void CPUdebugger::quick_setup(unsigned int memsize)
{
	bind(new Cpu_(memsize));
	pcpu->debugflags.one_step = true;
	pcpu->_init();
}

void CPUdebugger::memwrite(const char* src, unsigned int dst_paddr, unsigned element_sz, unsigned element_cnt, bool endian_switch)
{
	dst_paddr &= 0x7fffffff;
	unsigned char* prev_excptionaddr = nullptr;
	if (element_cnt * element_sz + dst_paddr > this->pcpu->memctrl.memory.size())return;
	unsigned int i = 0;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	membase += dst_paddr;
#ifdef VMMEM_RESERVE_THEN_COMMIT
_func_start:
	__try {
#endif // VMMEM_RESERVE_THEN_COMMIT
		switch (element_sz)
		{
		case 1:
			while (i < element_cnt) 
				membase[i] = src[i],
					++i;
			break;
		case 2:
			while (i < element_cnt) {
				reinterpret_cast<unsigned short*>(membase)[i] = endian_switch ?
					bl_endian_switch16( reinterpret_cast<const unsigned short*>(src)[i]):
					reinterpret_cast<const unsigned short*>(src)[i];
					++i;
			}
			break;
		case 4:
			while (i < element_cnt) {
				reinterpret_cast<unsigned int*>(membase)[i] = endian_switch ?
					bl_endian_switch32(reinterpret_cast<const unsigned short*>(src)[i]) :
					reinterpret_cast<const unsigned int*>(src)[i];
				++i;
			}
			break;
		default:
			return;
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except ((GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION&& prev_excptionaddr!=membase
		)? EXCEPTION_EXECUTE_HANDLER:
		EXCEPTION_CONTINUE_SEARCH) {
		prev_excptionaddr = membase;
		VirtualAlloc( reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(
			membase + i * element_sz )&0xFFFFFFFFFFFFF000), 
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (i < element_cnt)goto _func_start;
#endif // VMMEM_RESERVE_THEN_COMMIT
}

void CPUdebugger::readmem(unsigned int src_paddr, char* dst, unsigned element_sz, unsigned element_cnt, bool endian_switch)
{
	src_paddr &= 0x7fffffff;
	unsigned char* prev_excptionaddr = nullptr;
	if (element_cnt * element_sz + src_paddr > this->pcpu->memctrl.memory.size())return;
	unsigned int i = 0;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	membase += src_paddr;
#ifdef VMMEM_RESERVE_THEN_COMMIT
_func_start:
	__try {
#endif // VMMEM_RESERVE_THEN_COMMIT
		switch (element_sz)
		{
		case 1:
			while (i < element_cnt)
				dst[i] = membase[i],
				++i;
			break;
		case 2:
			while (i < element_cnt) {
				reinterpret_cast<unsigned short*>(dst)[i] = endian_switch ?
					bl_endian_switch16(reinterpret_cast<const unsigned short*>(membase)[i]) :
					reinterpret_cast<const unsigned short*>(membase)[i];
				++i;
			}
			break;
		case 4:
			while (i < element_cnt) {
				reinterpret_cast<unsigned int*>(dst)[i] = endian_switch ?
					bl_endian_switch32(reinterpret_cast<const unsigned short*>(membase)[i]) :
					reinterpret_cast<const unsigned int*>(membase)[i];
				++i;
			}
			break;
		default:
			return;
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION && 
		prev_excptionaddr != membase) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		prev_excptionaddr = membase;
		VirtualAlloc(reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(
			membase + i * element_sz) & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (i < element_cnt)goto _func_start;
#endif // VMMEM_RESERVE_THEN_COMMIT
}

unsigned int CPUdebugger::getCSR(CSRid id)
{
	return pcpu->CSRs[(int)id];
}

void CPUdebugger::writeCSR(CSRid id, unsigned int val)
{
	pcpu->CSRs[(int)id] = val;
}

unsigned int CPUdebugger::loadmem_fromfile(const char* filename, unsigned int dst_paddr)
{
	unsigned sz_write = 0,sz_read=0;
	char iobuf[4096];
	FILE* fp;
	fopen_s(&fp, filename, "rb");
	if (!fp)return 0;
	while (!feof(fp)) {
		sz_read = (unsigned)fread(iobuf, 1, 4096, fp);
		this->memwrite(iobuf, dst_paddr, 1, sz_read);
		sz_write += sz_read;
	}
	fclose(fp);
	return sz_write;
}

unsigned int CPUdebugger::loadmem_fromhexfile(const char* filename, unsigned int dst_paddr, bool endian_switch)
{
	dst_paddr &= 0x7fffffff;
	int i = 0;
	unsigned int memleft = this->pcpu->memctrl.memory.size() - dst_paddr;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	unsigned char* prev_excptionaddr = nullptr;
	FILE* fp;
	char readbuf[64];
	readbuf[0] = '0';
	readbuf[1] = 'x';
	fopen_s(&fp, filename, "r");
	if (!fp)return 0;
	long ptrprev = ftell(fp);
#ifdef VMMEM_RESERVE_THEN_COMMIT
_func_start:
	__try {
#endif // VMMEM_RESERVE_THEN_COMMIT
		while ( !feof(fp) && memleft > 0) {
			fgets(readbuf + 2, 62, fp);
			reinterpret_cast<unsigned int*>(membase)[i] =
				endian_switch ? bl_endian_switch32(strtol(readbuf,nullptr,16)) : strtol(readbuf, nullptr, 16);
			++i; --memleft;
			ptrprev = ftell(fp);
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION &&
		prev_excptionaddr != membase) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		fseek(fp, ptrprev - ftell(fp), SEEK_CUR);
		prev_excptionaddr = membase;
		VirtualAlloc(reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(
			membase + i * 4) & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (memleft > 0 && !feof(fp)) goto _func_start;
#endif // VMMEM_RESERVE_THEN_COMMIT
	return i * 4;
}

unsigned int CPUdebugger::storemem_tohexfile(const char* filename, unsigned addr,unsigned size)
{
	addr &= 0x7fffffff;
	int i = 0;
	unsigned int memleft = this->pcpu->memctrl.memory.size() - addr;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	unsigned char* prev_excptionaddr = nullptr;
	FILE* fp;
	unsigned val;
	fopen_s(&fp, filename, "w");
	if (!fp)return -1;
#ifdef VMMEM_RESERVE_THEN_COMMIT
	_func_start :
	__try {
#endif // VMMEM_RESERVE_THEN_COMMIT
		while (size > 0) {
			val = reinterpret_cast<unsigned int*>(membase)[i];
			fprintf(fp, "%d\n", val);
			++i; --memleft; --size;
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION &&
		prev_excptionaddr != membase) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		prev_excptionaddr = membase;
		VirtualAlloc(reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(
			membase + i * 4) & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (memleft > 0 && size>0) goto _func_start;
#endif // VMMEM_RESERVE_THEN_COMMIT
	return 0;
}

#include<iostream>

using namespace std;

void Cpu_::ins_exec_with_jit()
{
	unsigned paddr = 0;
	auto res = call_my_fn(cache->read(regs.pc & 0x7fffffff), &regs);
	switch (res)
	{case 0:
		break;
	case RC_RRT_MEMLOAD:
	case RC_RRT_MEMSTORE:
	case RC_RRT_INV_INSTRUCTION:
		printf("err! invins\n");
		break;
	case RC_RRT_SYSCALL:
		break;
	}
}

#ifdef _JITTOOLS_ENABLED

unsigned long Cpu_::runsync_with_jit()
{
	unsigned long long addr = 0;
	unsigned long long ecode;
	unsigned ix = -1;
#ifdef VMMEM_RESERVE_THEN_COMMIT
	__func_start:
	__try {
#endif // VMMEM_RESERVE_THEN_COMMIT
		while (debugflags.flag_run) {
			if (Mode != MODE_MACHINE) { 
				addr = memctrl.vaddr_to_paddr_exec_unsafe(regs.pc, Mode);
			}
			else addr = regs.pc & 0x7fffffff;
			if (addr & 3) {
				ix = 0;
			_make_exception(ix, *(unsigned*)(memctrl.memory.native_ptr() + (addr & memctrl.memory.mask())));
			}
			ecode = call_my_fn(cache->read_withoutHAJIfunction(regs.pc & 0x7fffffff), &regs);
			switch (ecode& 0x3fffff) {
			case 0:
				if (debugflags.flag_async && (debugflags.sleep || debugflags.sleep_req)) _wait_for_signal_run();
				continue;
			case RC_RRT_MEMLOAD:
				addr = memctrl.read_unsafe(ecode>>32, ((ecode>>22) & 0b11100) |(Mode)&3);
				if (addr & 0xffffffff) {
					ix = addr & 0xffffffff;
					--regs.cycles;
					_make_exception(ix, ecode >> 32);
				}
				ix = GET_MEM_RW_RESULT_VAL(addr);
				if ((ecode >> 22) & 3) {
					if (((ecode >> 22) & 3) == 1)ix = _sign_ext<8>(ix);
					else ix = _sign_ext<16>(ix);
				}
				if (((ecode >> 27) & 31) != 0)regs.x[(ecode >> 27) & 31] = ix;
				ix = -1;
				regs.pc += 4;
				break;
			case RC_RRT_MEMSTORE:
				ix = memctrl.write_unsafe(ecode>>32, regs.x[((ecode >> 27) & 31)],
					((ecode >> 22) & 0b11100) | (Mode) & 3);
				if (ix == 0) ix = -1;
				else { --regs.cycles; _make_exception(ix, ecode >> 32); }
				regs.pc += 4;
				break;
			case RC_RRT_INV_INSTRUCTION:
				if (debugflags.flag_exit_when_inv_ins)return 0;
				ix = EXC_INV_INSTRUCTION;
				_make_exception(ix);
				break;
			case RC_RRT_SYSCALL:
				_ins_exec_op_system(*reinterpret_cast<Instruction*>(
					memctrl.memory.native_ptr() + (addr & memctrl.memory.mask())));
				break;
			}
			if (ix != -1) {
				_into_trap();
				++regs.cycles;
			}
			if (debugflags.flag_async && (debugflags.sleep || debugflags.sleep_req))_wait_for_signal_run();
			if (debugflags.flag_int) {
				debugflags.flag_int = 0;
				CSRs[(int)CSRid::mepc] = regs.pc + 4;
				_into_int();
			}
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except (addr = reinterpret_cast<unsigned long long>(GetExceptionInformation()),
		GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(reinterpret_cast<void*>(
			reinterpret_cast<_EXCEPTION_POINTERS*>(addr)->ExceptionRecord->ExceptionInformation[1] & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (debugflags.flag_run) goto __func_start;
#endif // VMMEM_RESERVE_THEN_COMMIT
	return 0;
}

#include<thread>

void Cpu_::run_with_jit()
{
	std::thread th(&Cpu_::runsync_with_jit, this);
	thandle = th.native_handle();
	th.detach();
}

#endif // _JITTOOLS_ENABLED

void Cpu_::set_flag_async()
{
	debugflags.flag_async = 1;
}

void Cpu_::_invoke()
{
	if (debugflags.flag_async&&debugflags.one_step) {
		if (this->tstste_sus == 1&& thandle!=nullptr) {
			ResumeThread(thandle);
		}
	}
}

void Cpu_::_wait_for_signal_run()
{
	if (thandle != nullptr)
	{
		tstste_sus = 1;
		debugflags.pause = 1;
		debugflags.sleep = 0;
		debugflags.sleep_req = 0;
		SuspendThread(thandle);
	}
}

void Cpu_::runsync()
{
	int ecode;
	unsigned int ins;
	unsigned long long iid;
#ifdef VMMEM_RESERVE_THEN_COMMIT
_funcstart:
	__try {
#endif
		while (debugflags.flag_run) {
			if (regs.pc & 3) {
				ecode = EXC_INSTRUCTION_ADDR_NOT_ALIGNED;
				goto _handle_exceptions;
			}
			iid = this->memctrl.read_ins_unsafe(regs.pc, Mode);
			if (iid & 0xffffffff) {
				ecode = iid & 0xffffffff;
				goto _handle_exceptions;
			}
			ins = GET_MEM_RW_RESULT_VAL(iid);
			ecode = (this->*_decodeheperfuncs[reinterpret_cast<Instruction*>(&ins)->rType.opcode])(
				*reinterpret_cast<Instruction*>(&ins));
		_handle_exceptions:
			if (ecode != -1) {
				_make_exception(ecode);
				_into_trap();
			}
			else regs.pc += 4;
			regs.cycles++;
			if (debugflags.flag_async && debugflags.one_step) _wait_for_signal_run();
		}
#ifdef VMMEM_RESERVE_THEN_COMMIT
	}
	__except (iid = reinterpret_cast<unsigned long long>(GetExceptionInformation()),
		GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		VirtualAlloc(reinterpret_cast<void*>(
			reinterpret_cast<_EXCEPTION_POINTERS*>(iid)->ExceptionRecord->ExceptionInformation[1] & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);

	}
	if (debugflags.flag_run) goto _funcstart;
#endif // VMMEM_RESERVE_THEN_COMMIT
}