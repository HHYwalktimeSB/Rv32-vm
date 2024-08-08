#include "_cpu.h"

#define MASK_PPN_1 0xfff00000

#define MAKE_MEM_RW_RESULT_OK(_VAL_) ((unsigned long long)(_VAL_))<<32
#define GET_MEM_RW_RESULT_VAL(_VAL_) ((unsigned int)(_VAL_>>32))

void Cpu_::_init()
{
	regs.pc = 0;
	for (auto i = 0; i < 32; i++)
		regs.x[i] = 0;
	for (auto i = 0; i < 4096; i++)
		CSRs[i] = 0;
	Mode = MODE_MACHINE;
	*reinterpret_cast<unsigned int*>(&debugflags) = 0;
	debugflags.flag_run = 1;
	debugflags.one_step = 1;
}

unsigned int Cpu_::ALUoperation(unsigned int a, unsigned int b, Instruction ins)
{
	unsigned int ret;
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

void Cpu_::ins_exec(Instruction ins)
{
	switch (ins.rType.opcode)
	{
	case OP_ALU:
		if (ins.rType.rd != 0)
			regs.x[ins.rType.rd] =
			ALUoperation(regs.x[ins.rType.rs1], regs.x[ins.rType.rs2], ins);
		break;
	case OP_ALU_IMM:
		if (ins.rType.rd != 0)
			regs.x[ins.iType.rd] =
			ALUoperation(regs.x[ins.iType.rs1], immgen(ins), ins);
		break;
	case OP_LOAD:
	{
		unsigned long long _ret_buf;
		unsigned int addr = static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins));
		switch (ins.iType.funct3)
		{
		case 0://lb
			_ret_buf = memctrl.read8(addr, Mode);
			if (_ret_buf & 0xffffffff) {
				_make_mem_exception(_ret_buf & 0xffffffff, IOF_READ);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			if (ins.iType.rd != 0) regs.x[ins.iType.rd] = _sign_ext<8>(GET_MEM_RW_RESULT_VAL(_ret_buf));
			break;
		case 1:
			_ret_buf = memctrl.read16(addr, Mode);
			if (_ret_buf & 0xffffffff) {
				_make_mem_exception(_ret_buf & 0xffffffff,IOF_READ);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			if (ins.iType.rd != 0) regs.x[ins.iType.rd] = _sign_ext<16>(GET_MEM_RW_RESULT_VAL(_ret_buf));
			break;
		case 2:
			_ret_buf = memctrl.read32(addr, Mode);
			if (_ret_buf & 0xffffffff) {
				_make_mem_exception(_ret_buf & 0xffffffff,IOF_READ);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			if (ins.iType.rd != 0) regs.x[ins.iType.rd] =GET_MEM_RW_RESULT_VAL(_ret_buf);
			break;
		case 3:
			_ret_buf = memctrl.read32(addr, Mode);
			if (_ret_buf & 0xffffffff) {
				_make_mem_exception(_ret_buf & 0xffffffff, IOF_READ);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			if (ins.iType.rd != 0) regs.x[ins.iType.rd] = GET_MEM_RW_RESULT_VAL(_ret_buf);
			break;
		case 4:
			_ret_buf = memctrl.read32(addr, Mode);
			if (_ret_buf & 0xffffffff) {
				_make_mem_exception(_ret_buf & 0xffffffff, IOF_READ);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			if (ins.iType.rd != 0) regs.x[ins.iType.rd] = GET_MEM_RW_RESULT_VAL(_ret_buf);
			break;
		}
	}
	break;
	case OP_STORE: 
	{
		unsigned int _ret_buf;
		unsigned int addr = static_cast<int>(regs.x[ins.sType.rs1]) + static_cast<int>(immgen(ins));
		switch (ins.sType.funct3)
		{
		case 0:
			_ret_buf = memctrl.write8(addr, regs.x[ins.sType.rs2], Mode);
			if (_ret_buf != 0) {
				_make_mem_exception(_ret_buf, IOF_WRITE);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			break;
		case 1:
			_ret_buf = memctrl.write16(addr, regs.x[ins.sType.rs2], Mode);
			if (_ret_buf != 0) {
				_make_mem_exception(_ret_buf, IOF_WRITE);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			break;
		case 3:
			_ret_buf = memctrl.write32(addr, regs.x[ins.sType.rs2], Mode);
			if (_ret_buf != 0) {
				_make_mem_exception(_ret_buf, IOF_WRITE);
				CSRs[(int)CSRid::mtval] = addr;
				EXEC_INS1_ABORT;
			}
			break;
		}
	}
	break;
	case OP_BTYPE:
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
		}
		if (if_jmp) {
			regs.pc += static_cast<int>(immgen(ins));
			EXEC_INS1_ABORT;
		}
	}
	break;
	case OP_JAL:
		if (ins.jType.rd != 0)
			regs.x[ins.jType.rd] = regs.pc + 4;
		regs.pc += static_cast<int>(immgen(ins));
		EXEC_INS1_ABORT;
	case OP_JALR:
		if (ins.iType.rd != 0)
			regs.x[ins.iType.rd] = regs.pc + 4;
		regs.pc = static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins));
		EXEC_INS1_ABORT;
	case OP_LUI:
		if (ins.uType.rd != 0)
			regs.x[ins.uType.rd] =
			immgen(ins);
		break;
	case OP_AUIPC:
		if (ins.uType.rd != 0)
			regs.x[ins.uType.rd] = regs.pc +
			static_cast<int>(immgen(ins));
		break;
	case OP_SYSTEM:
		break;
	default://TODO
		_make_exception(EXC_INV_INSTRUCTION);
		EXEC_INS1_ABORT;
		break;
	}
	regs.pc += 4;
}

void Cpu_::_into_trap(tagCSR::tagmcause cause)
{
	debugflags.flag_int = 0;
	*reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause]) = cause;
	CSRs[(int)CSRid::mepc] = regs.pc;
	if (cause.Interrupt == 0 && 
		(cause.exception_code == 8 || cause.exception_code == 9 || cause.exception_code == 11))
		CSRs[(int)CSRid::mepc] += 4;
	auto mst = reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus]);
	mst->MPIE = mst->MIE;
	mst->MIE = 0;
	mst->MPP = Mode;
	if (reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->mode == 1 
		&& cause.Interrupt == 1)regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base +
		cause.exception_code * 4;
	else regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base;
}

void Cpu_::_make_exception(int code, bool is_int)
{
	this->exeption.exception_code = code;
	this->exeption.Interrupt = is_int;
	this->debugflags.flag_int = 1;
}

void Cpu_::_make_mem_exception(unsigned int meme_code, unsigned io_code)
{
	switch (meme_code)
	{
	case MEME_ADDR_NOT_ALIGNED:
		if (io_code == IOF_READ)_make_exception(EXC_LOAD_ADDR_NOT_ALIGNED);
		else _make_exception(EXC_STORE_ADDR_NOT_ALIGNED);
		break;
	case MEME_ACCESS_DENIED:
		if (io_code == IOF_READ) _make_exception(EXC_LOAD_ACCESS_FAULT);
		else _make_exception(EXC_STORE_ACCESS_FAULT);
		break;
	case MEME_PAGE_FAULT:
		if (io_code == IOF_READ)_make_exception(EXC_LOAD_PAGE_FAULT);
		else _make_exception(EXC_STORE_PAGE_FAULT);
		break;
	default:
		break;
	}
}

void Cpu_::runsync()
{
	unsigned long long result;
	Instruction ins;
	while (debugflags.flag_run)
	{
		result = memctrl.read_ins(regs.pc, Mode);
		switch (result&0xffffffff)
		{
		case MEME_OK:
			*reinterpret_cast<unsigned int*>(&ins) = GET_MEM_RW_RESULT_VAL(result);
			this->ins_exec(ins);
				break;
		case	MEME_ACCESS_DENIED:
			_make_exception(EXC_INSTRUCTION_ACCESS_FAULT);
			break;
		case	MEME_ADDR_NOT_ALIGNED:
			_make_exception(EXC_INSTRUCTION_ADDR_NOT_ALIGNED);
			break;
		case MEME_PAGE_FAULT:
			_make_exception(EXC_INSTRUCTION_PAGE_FAULT);
			break;
		default:
			break;
		}
		if (debugflags.flag_int) {
			debugflags.flag_int = 0;
			_into_trap(exeption);
		}
		if (debugflags.one_step) {
			std::string str;
			std::cin >> str;

		}
	}
}

Cpu_::Cpu_(unsigned int mem_sz):memctrl(mem_sz, this->CSRs)
{
	thandle = nullptr;
}

unsigned int immgen(Instruction ins)
{
	unsigned int ret = 0;
	switch (ins.rType.opcode)
	{
	case OP_LOAD:
	case OP_ALU_IMM://itype
	case OP_JALR:
		ret = _sign_ext<12>(ins.iType.imm);
		break;
	case OP_BTYPE:
		ret |= ((unsigned)(ins.bType.imm_4_1)) << 1;
		ret |= ((unsigned)(ins.bType.imm_10_5)) << 5;
		ret |= ((unsigned)(ins.bType.imm_11)) << 11;
		ret |= ((unsigned)(ins.bType.imm_12)) << 12;
		ret = _sign_ext<13>(ret);
		break;
	case OP_LUI:
	case OP_AUIPC:
		ret = ((unsigned)ins.uType.imm_31_12) << 12;
		break;
	case OP_STORE:
		ret = ((unsigned)ins.sType.imm_4_0);
		ret |= ((unsigned)ins.sType.imm_11_5) << 5;
		ret = _sign_ext<12>(ret);
		break;
	case OP_JAL:
		ret = ((unsigned)ins.jType.imm_10_1) << 1;
		ret |= ((unsigned)ins.jType.imm_11) << 11;
		ret |= ((unsigned)ins.jType.imm_19_12) << 12;
		ret |= ((unsigned)ins.jType.imm_20) << 20;
		break;
	default:
		break;
	}
	return ret;
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

unsigned long long MemController::read_ins(unsigned int addr, unsigned int mode)
{
	if ((addr & 3) != 0)return MEME_ADDR_NOT_ALIGNED;
	unsigned long long ret;
	if (mode != 3 && (cpuCSRs[(int)CSRid::stap] & MASK_STAP_MODE)) {
		ret = vaddr_to_paddr(addr, IOF_EXEC | (mode == MODE_USR ? IOF_USR : 0));
		if ((ret & 0xffffffff) == 0) ret = MAKE_MEM_RW_RESULT_OK(memory._readp32(
			GET_MEM_RW_RESULT_VAL(ret)));
	}
	else ret = MAKE_MEM_RW_RESULT_OK(memory._readp32(addr));
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
	else ret = (unsigned long long)memory._readp32(addr);
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
	else ret = (unsigned long long)memory._readp16(addr);
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
	else ret = MAKE_MEM_RW_RESULT_OK(memory._readp8(addr));
	return ret;
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
	else memory._writep32(addr,val);
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
	else memory._readp16(addr);
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
	else memory._readp8(addr);
	return MEME_OK;
}

MemController::MemController(unsigned memsz, unsigned int* csr)
{
	memory._init(memsz);
	cpuCSRs = csr;	
}

int CPUdebugger::run_1_cycle()
{
	auto rr = pcpu->memctrl.read_ins(pcpu->regs.pc, pcpu->Mode);
	if ((rr & 0xffffffff) != 0) {
		throw (rr & 0xffffffff);
	}
	Instruction ins;
	*reinterpret_cast<unsigned int*>(&ins) = GET_MEM_RW_RESULT_VAL(rr);
	pcpu->ins_exec(ins);
	if ((pcpu->debugflags.flag_int)) {
		pcpu->debugflags.flag_int = 0;
		return pcpu->exeption.exception_code;
	}
	return EXC_NO_EXCEPTION;
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

#include<Windows.h>
#include<fstream>
#include<stdio.h>

void CPUdebugger::bind(Cpu_* pc)
{
	this->pcpu = pc;
	mode = sync;
}

void CPUdebugger::quick_setup(unsigned int memsize)
{
	bind(new Cpu_(memsize));
	pcpu->debugflags.one_step = true;
	pcpu->_init();
}

void CPUdebugger::memwrite(const char* src, unsigned int dst_paddr, unsigned element_sz, unsigned element_cnt, bool endian_switch)
{
	unsigned char* prev_excptionaddr = nullptr;
	if (element_cnt * element_sz + dst_paddr > this->pcpu->memctrl.memory.size())return;
	int i = 0;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	membase += dst_paddr;
_func_start:
	__try {
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
}

void CPUdebugger::readmem(unsigned int src_paddr, char* dst, unsigned element_sz, unsigned element_cnt, bool endian_switch)
{
	unsigned char* prev_excptionaddr = nullptr;
	if (element_cnt * element_sz + src_paddr > this->pcpu->memctrl.memory.size())return;
	int i = 0;
	unsigned char* membase = this->pcpu->memctrl.memory.native_ptr();
	membase += src_paddr;
_func_start:
	__try {
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
	}
	__except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION && 
		prev_excptionaddr != membase) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
		prev_excptionaddr = membase;
		VirtualAlloc(reinterpret_cast<void*>(reinterpret_cast<unsigned long long>(
			membase + i * element_sz) & 0xFFFFFFFFFFFFF000),
			4096, MEM_COMMIT, PAGE_READWRITE);
	}
	if (i < element_cnt)goto _func_start;
}

unsigned int CPUdebugger::loadmem_fromfile(const char* filename, unsigned int dst_paddr)
{
	unsigned sz_write = 0,sz_read=0;
	char iobuf[4096];
	FILE* fp;
	fopen_s(&fp, filename, "rb");
	if (!fp)return 0;
	while (!feof(fp)) {
		sz_read = fread(iobuf, 1, 4096, fp);
		this->memwrite(iobuf, dst_paddr, 1, sz_read);
		sz_write += sz_read;
	}
	fclose(fp);
	return sz_write;
}
