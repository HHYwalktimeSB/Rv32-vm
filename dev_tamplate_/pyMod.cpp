#include "pch.h"
#include "pyMod.h"
#include "C:/Python27/include/Python.h"
#include<Windows.h>

#define DECLARE_FUNCTION(_FUNC_) decltype(&_FUNC_) myLib##_FUNC_
#define GETFUNCTION(_FUNC_) myLib##_FUNC_

class LibPy {
public:
	HMODULE handle_dll;
	static char* path_to_python_dll;
	void LoadFunctions();
	void Init();
	DECLARE_FUNCTION(Py_Initialize);
	DECLARE_FUNCTION(Py_Finalize);
	DECLARE_FUNCTION(PyImport_ImportModule);
	DECLARE_FUNCTION(Py_BuildValue);
	DECLARE_FUNCTION(PyArg_Parse);
	DECLARE_FUNCTION(Py_DecRef);
	DECLARE_FUNCTION(PyObject_CallFunction);
	DECLARE_FUNCTION(PyCallable_Check);
	DECLARE_FUNCTION(PyObject_GetAttrString);
	DECLARE_FUNCTION(PyObject_CallObject);

	LibPy();
};

#define ToText(_val_) #_val_

#define MY_LIB_LOADFUNC(_ProcName__) this->GETFUNCTION(_ProcName__) = \
(decltype(&_ProcName__))reinterpret_cast<void*>(GetProcAddress(handle_dll, #_ProcName__));\
if(this->GETFUNCTION(_ProcName__)==nullptr)throw ToText(Cannot find function: ##_ProcName__)


PythonModule::PythonModule(const char* n, unsigned sz,Schedule* s,const std::function<void(unsigned)>& Fn):
	_DevBase(Mode::fmmem,sz,n, s,nullptr,Fn)
{
	if(pLib==nullptr)pLib = new LibPy;
}

void PythonModule::SetPathToPythonRuntime(const char* path)
{
	auto len = strlen(path) + 1;
	LibPy::path_to_python_dll = new char[len];
	memcpy_s(LibPy::path_to_python_dll, len, path, len);
}

void PythonModule::InitRuntime()
{
	((LibPy*)pLib)->myLibPy_Initialize();
}

void PythonModule::FinaliseRuntime()
{
	((LibPy*)pLib)->myLibPy_Finalize();
}

#define GFn(_Fn_) ((LibPy*)pLib)->GETFUNCTION(_Fn_)

void PythonModule::_cb_writedata()
{
	if (cb_write) {
		GFn(PyObject_CallObject)((PyObject*)cb_write, NULL);
	}
}

void PythonModule::_cb_readdata()
{
	if (cb_read) {
		GFn(PyObject_CallObject)((PyObject*)cb_read, NULL);
	}
}

void PythonModule::update()
{
	if (cb_update) {
		GFn(PyObject_CallObject)((PyObject*)cb_update, NULL);
	}
}

PythonModule::~PythonModule()
{
}

void LibPy::LoadFunctions()
{
	MY_LIB_LOADFUNC(Py_Initialize);
	MY_LIB_LOADFUNC(Py_Finalize);
	MY_LIB_LOADFUNC(PyImport_ImportModule);
	MY_LIB_LOADFUNC(Py_BuildValue);
	MY_LIB_LOADFUNC(PyArg_Parse);
	MY_LIB_LOADFUNC(Py_DecRef);
	MY_LIB_LOADFUNC(PyObject_CallFunction);
	MY_LIB_LOADFUNC(PyCallable_Check);
	MY_LIB_LOADFUNC(PyObject_GetAttrString);
	MY_LIB_LOADFUNC(PyObject_CallObject);


}

void LibPy::Init()
{
	this->LoadFunctions();
	GETFUNCTION(Py_Initialize)();
}

LibPy::LibPy()
{
	handle_dll = LoadLibraryA(path_to_python_dll);
	this->LoadFunctions();
}
