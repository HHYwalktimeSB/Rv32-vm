from ctypes import *
from ctypes.wintypes import POINT
from email.headerregistry import UniqueSingleAddressHeader
from os import *

mylib = CDLL("../X64/Debug/Rv32-vm_dll")

vmm_loadhex = mylib.Vmmem_load_hex
vmu_record_reg = mylib.VmCPU_RecordRegs
vmu_cmp_reg = mylib.VmCPU_CmpRegs
vm_reset = mylib.RV32vm_Reset
vm_run = mylib.Vm_RunSync

class SreuctReg(Structure):
    _fields_ = [('x', POINT*32),('pc', c_uint),('ecode', c_int),('cycles', c_ulonglong)]


def initialize_vm():
    vm_init = mylib.RV32vm_Init
    vm_init.argtypes = (c_uint)
    vm_init.restype = c_int
    res = vm_init(1024*1024*4)
    if(res!=0):return res
    vmm_loadhex.restype = c_int
    vmm_loadhex.argtypes(c_uint, c_char_p)
    vmu_record_reg.restype = c_int
    vmu_record_reg.argtype = (c_int)
    vmu_cmp_reg.restype = c_int
    vmu_cmp_reg.argtypes = (c_int)
    vm_reset.restype = c_int
    vm_reset.argtypes = (c_int)
    vm_run.argtypes = ()
    vm_run.restype  = c_int

if(mylib.RV32vm_Init(c_int(1024*1024*4))!=0):
    print("fuck")
print(mylib.Vmmem_load_hex('../x64/Debug/test.txt',c_uint(0x80000000)))
mylib.Vm_RunSync()


