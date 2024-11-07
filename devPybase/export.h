#pragma once
#include "../dev_tamplate_/_dev_impl.h"

MY_API int MYAPI_CC Pycall_CoreMemoryInit(const char* name, unsigned long long ptr, unsigned sz);

MY_API int MYAPI_CC ReadArrayInt(unsigned index);

MY_API int MYAPI_CC WriteArrayInt(unsigned index,unsigned val);

MY_API int MYAPI_CC InvokeInt(unsigned val);

MY_API int MYAPI_CC Pycall_CoreMemoryDestruct(const char* name);