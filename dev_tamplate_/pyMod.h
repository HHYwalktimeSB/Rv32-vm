#include"_dev_impl.h"

class PythonModule:public _DevBase {
protected:
	static void* pLib;
	void* pmodule;
	void* cb_update;
	void* cb_write;
	void* cb_read;
protected:
	PythonModule(const char* n, unsigned sz,const std::function<void(unsigned)>&, const char* name, void* pmod);
public:
	static void SetPathToPythonRuntime(const char* path);
	void InitRuntime();
	void FinaliseRuntime();
	virtual void _cb_writedata();
	virtual void _cb_readdata();
	virtual void update();
	_DevBase* CreateObj(const char* name_to_py, const std::function<void(unsigned)>&);
	~PythonModule();
};