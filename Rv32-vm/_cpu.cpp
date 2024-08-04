#include "_cpu.h"

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
	sig_abort_exec1 = false;
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
		switch (ins.iType.funct3)
		{
		case 0://lb
			if (ins.iType.rd != 0)
				regs.x[ins.iType.rd] = _sign_ext<8>(memctrl.read8(
					static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins))));
			break;
		case 1:
			if (ins.iType.rd != 0)
				regs.x[ins.iType.rd] = _sign_ext<16>(memctrl.read16(
					static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins))));
			break;
		case 2:
			if (ins.iType.rd != 0)
				regs.x[ins.iType.rd] = memctrl.read32(
					static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins)));
			break;
		case 3:
			if (ins.iType.rd != 0)
				regs.x[ins.iType.rd] = memctrl.read8(
					static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins)));
			break;
		case 4:
			if (ins.iType.rd != 0)
				regs.x[ins.iType.rd] = memctrl.read16(
					static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins)));
			break;
		}
	}
	break;
	case OP_STORE: 
	{
		switch (ins.sType.funct3)
		{
		case 0:
			memctrl.write8(static_cast<int>(regs.x[ins.sType.rs1]) + static_cast<int>(immgen(ins))
				, regs.x[ins.sType.rs2]);
			break;
		case 1:
			memctrl.write16(static_cast<int>(regs.x[ins.sType.rs1]) + static_cast<int>(immgen(ins))
				, regs.x[ins.sType.rs2]);
			break;
		case 3:
			memctrl.write32(static_cast<int>(regs.x[ins.sType.rs1]) + static_cast<int>(immgen(ins))
				, regs.x[ins.sType.rs2]);
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
			sig_abort_exec1 = true;
		}
	}
	break;
	case OP_JAL:
		if (ins.jType.rd != 0)
			regs.x[ins.jType.rd] = regs.pc + 4;
		regs.pc += static_cast<int>(immgen(ins));
		sig_abort_exec1 = true;
		break;
	case OP_JALR:
		if (ins.iType.rd != 0)
			regs.x[ins.iType.rd] = regs.pc + 4;
		regs.pc = static_cast<int>(regs.x[ins.iType.rs1]) + static_cast<int>(immgen(ins));
		sig_abort_exec1 = true;
		break;
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
		break;
	}
	if (!sig_abort_exec1)  regs.pc += 4;
}

void Cpu_::_into_trap(tagCSR::tagmcause cause, unsigned int mtval)
{
	*reinterpret_cast<tagCSR::tagmcause*>(&CSRs[(int)CSRid::mcause]) = cause;
	CSRs[(int)CSRid::mepc] = regs.pc;
	if (cause.Interrupt == 0 && 
		(cause.exception_code == 8 || cause.exception_code == 9 || cause.exception_code == 11))
		CSRs[(int)CSRid::mepc] += 4;
	CSRs[(int)CSRid::mtval] = mtval;
	auto mst = reinterpret_cast<tagCSR::tagmstatus*>(&CSRs[(int)CSRid::mstatus]);
	mst->MPIE = mst->MIE;
	mst->MIE = 0;
	mst->MPP = mflags.MP;
	if (reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->mode == 1 
		&& cause.Interrupt == 1)regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base +
		cause.exception_code * 4;
	else regs.pc = reinterpret_cast<tagCSR::tagmtvec*>(&CSRs[(int)CSRid::mtvec])->base;
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

unsigned int MemController::read32(unsigned int addr)
{
	return this->memory._readp32(addr);
}

unsigned int MemController::read16(unsigned int addr)
{
	return memory._readp16(addr);
}

unsigned int MemController::read8(unsigned int addr)
{
	return memory._readp8(addr);
}

void MemController::write32(unsigned int addr, unsigned int val)
{
	memory._writep32(addr, val);
}

void MemController::write16(unsigned int addr, unsigned short val)
{
	memory._writep16(addr, val);
}

void MemController::write8(unsigned int addr, unsigned char val)
{
	memory._writep8(addr, val);
}
