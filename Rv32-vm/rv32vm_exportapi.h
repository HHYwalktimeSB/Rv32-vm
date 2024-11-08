#ifndef RV32VM_EXPORTAPI_H_
#define RV32VM_EXPORTAPI_H_ 
#ifndef MY_API
#ifdef _DLL
#define MY_API extern "C" __declspec(dllexport)
#else
#define MY_API extern "C"
#endif
#endif
#ifndef MYAPI_CC
#define MYAPI_CC __cdecl
#endif

#ifndef mCPUH
struct REGS
{
	unsigned int x[32];
	unsigned int pc;//_128
	int instruction_ecode;//132
	unsigned long long cycles;
};
#endif


MY_API int MYAPI_CC RV32vm_Init(int memory_size);

MY_API int MYAPI_CC RV32vm_Reset(int flags);

MY_API int MYAPI_CC VmCPU_SetPc(unsigned val);

MY_API REGS* MYAPI_CC VmCPU_GetAllRegs();

MY_API unsigned MYAPI_CC VmCPU_GetReg(int index);

MY_API int MYAPI_CC VmCPU_SetReg(int index, unsigned val);

MY_API int MYAPI_CC RV32vm_Destruct();

MY_API int MYAPI_CC Vmmem_write(const char* src, unsigned dst_addr, int elem_sz, int elem_cnt, int endian_switch);

MY_API int MYAPI_CC Vmmem_read(unsigned src_addr, char* buf, int elem_sz, int elem_cnt, int endian_switch);

MY_API int MYAPI_CC Vmmem_load_binary(unsigned dst_paddr, const char* fn);

MY_API int MYAPI_CC Vmmem_load_hex(const char* fn,unsigned);

MY_API int MYAPI_CC Vmmem_store_hex(unsigned dst_paddr, const char* fn, unsigned sz);

MY_API int MYAPI_CC VmCPU_RecordRegs(unsigned index);

MY_API int MYAPI_CC VmCPU_CmpRegs(unsigned index);

MY_API int MYAPI_CC Vm_RunSync();


#endif // !RV32VM_EXPORTAPI
