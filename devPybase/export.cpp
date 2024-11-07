#include "pch.h"
#include "../dev_tamplate_/_dev_impl.h"
#include<string>
#include "export.h"

#ifdef _DEBUG
#pragma comment(lib, "../x64/Debug/dev_tamplate_.lib")
#endif // _DEBUG


_DevBase* pCoreObj = nullptr;

MY_API int MYAPI_CC Pycall_CoreMemoryInit(const char* name, unsigned long long ptr, unsigned size)
{
    if (pCoreObj != nullptr)return -1;
    try {
        pCoreObj = new _DevBase(*reinterpret_cast<_DevBase*>(ptr));
    }
    catch (...) {
        return -2;
    }
    return 0;
}

MY_API int MYAPI_CC ReadArrayInt(unsigned index)
{
    if (pCoreObj != nullptr)return 0;
    return pCoreObj->read(index << 2,4);
}

MY_API int MYAPI_CC WriteArrayInt(unsigned index, unsigned val)
{
    if (pCoreObj != nullptr)return -1;
    pCoreObj->write(index << 2, val, 4);
    return 0;
}

MY_API int MYAPI_CC InvokeInt(unsigned val)
{
    if (pCoreObj != nullptr)return -1;
    pCoreObj->InvokeInt(val);
    return 0;
}

MY_API int MYAPI_CC Pycall_CoreMemoryDestruct(const char* name)
{
    if (pCoreObj != nullptr)return -1;
    delete pCoreObj;
    return 0;
}
