#include"RVVM32.h"
#include "rv32vm_exportapi.h"

MyVm* pVM = nullptr;

MY_API int MYAPI_CC RV32vm_Init(int memory_size)
{
    if (pVM)return -1;
    pVM = new MyVm;
    pVM->init_(memory_size);
    return 0;
}

MY_API int MYAPI_CC RV32vm_Reset(int flags)
{
    if (pVM == NULL)return -1;
    pVM->reset_cpu_regs();
    return 0;
}

MY_API int MYAPI_CC VmCPU_SetPc(unsigned val)
{
    if (pVM == NULL)return -1;
    pVM->setpc(val);
    return 0;
}

MY_API REGS* MYAPI_CC VmCPU_GetAllRegs()
{
    if (pVM == NULL)return nullptr;
    return const_cast<REGS*>(pVM->getregs());
}

MY_API unsigned MYAPI_CC VmCPU_GetReg(int index)
{
    if (pVM == NULL)return 0;
    if (index < 32)return pVM->getregs()->x[index];
    switch (index)
    {
    case 32:
        return pVM->getregs()->pc;
    case 33:
        return (unsigned) (pVM->getregs()->cycles);
    case 34:
        return (unsigned)(pVM->getregs()->cycles>>32);
    }
    return 0;
}

MY_API int MYAPI_CC VmCPU_SetReg(int index, unsigned val)
{
    if (pVM == NULL)return -1;
    if (index < 32)pVM->setreg(index, val);
    else pVM->setpc(val);
    return 0;
}

MY_API int MYAPI_CC RV32vm_Destruct()
{
    if (pVM == NULL)return -1;
    delete pVM;
    pVM = nullptr;
    return 0;
}

MY_API int MYAPI_CC Vmmem_write(const char* src, unsigned dst_addr, int elem_sz, int elem_cnt, int endian_switch)
{
    if (pVM == NULL)return -1;
    pVM->memwrite(src, dst_addr, elem_sz, elem_sz, endian_switch != 0);
    return 0;
}

MY_API int MYAPI_CC Vmmem_read(unsigned src_addr, char* buf, int elem_sz, int elem_cnt, int endian_switch)
{
    if (pVM == NULL)return -1;
    pVM->readmem(src_addr,buf, elem_sz, elem_sz, endian_switch != 0);
    return 0;
}

MY_API int MYAPI_CC Vmmem_load_binary(unsigned src_paddr, const char* fn)
{
    if (pVM == NULL)return -1;
    return pVM->loadmem_fromfile(fn, src_paddr);
}

MY_API int MYAPI_CC Vmmem_load_hex(unsigned dst_paddr, const char* fn)
{
    if (pVM == NULL)return -1;
    return pVM->loadmem_fromhexfile(fn, dst_paddr);
}

MY_API int MYAPI_CC Vmmem_store_hex(unsigned dst_paddr, const char* fn, unsigned sz)
{
    if (pVM == NULL)return -1;
    return pVM->storemem_tohexfile(fn, dst_paddr,sz);
}