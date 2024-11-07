#include"_dev_impl.h"

class PythonModule:public _DevBase {
protected:
	static void* pLib;
	void* pmodule;
	void* cb_update;
	void* cb_write;
	void* cb_read;
protected:
	PythonModule(const char* n, unsigned sz, Schedule* s,const std::function<void(unsigned)>&);
public:
	static void SetPathToPythonRuntime(const char* path);
	void InitRuntime();
	void FinaliseRuntime();
	virtual void _cb_writedata();
	virtual void _cb_readdata();
	virtual void update();
	_DevBase* CreateObj(const char* name_to_py);
	~PythonModule();
};