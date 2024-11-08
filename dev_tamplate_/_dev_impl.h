#ifndef _DEV_IMPL_H_
#define _DEV_IMPL_H_

#include<functional>
#include<list>
#include<mutex>

class CoreMem {
private:
	char* buf;
	void* hfm;
	void* hse;
	char* name;
	int* reference_cnt;
	unsigned size;
public:
	enum class Mode {
		sync, async, fmmem
	};
protected:
	struct SharedMemctrlInfo {
		int sigkill;
		int flag;
		unsigned long long reserved;
		constexpr inline SharedMemctrlInfo() :sigkill(0), flag(0), reserved(0) { };
	};
	Mode mode;
	void writedw(unsigned addr, unsigned val);
	void writew(unsigned addr, unsigned short val);
	void writec(unsigned addr, unsigned char val);
	void writex(unsigned addr, unsigned cbcnt, const char* v);
	void readx(unsigned addr, unsigned cbcnt, char* buf);
	unsigned readdw(unsigned addr);
	unsigned short readw(unsigned addr);
	unsigned char readc(unsigned addr);
	void _shared_data_sync_guard_init();
	void _shared_data_sync_guard_end();
	CoreMem(Mode md,unsigned sz,const char* n);
	CoreMem(const CoreMem&);
	~CoreMem();
	inline char* get_bufptr() { return buf; }
public:
	int get_refcnt()const;
};

class Schedule;

class _DevBase : public CoreMem{
protected:
	void* phandle;
	Schedule* pSchedule;
	std::function<void(unsigned)> FnCallbackInt;
	void FnBindingud();
public:
	virtual void _cb_writedata();
	virtual void _cb_readdata();
	virtual void update();
	_DevBase(Mode md, unsigned sz, const char* n, Schedule*, void*, const std::function<void(unsigned)>&);
	_DevBase(const _DevBase&);
	virtual ~_DevBase();
	void write(unsigned addr, unsigned val, unsigned sz);
	unsigned read(unsigned addr, unsigned sz);
	void InvokeInt(unsigned code);
	void update_loop_async();//never call on host
	void RunLoopinMode1();
	void* GetHandle();
};

//never call it in a module
int __cdecl unload_dll(void*);

class Schedule {
public:
protected:
	struct Ts {
		unsigned rep;
		unsigned interval;
		long long cnt;
	};
	std::mutex guard;
	std::list<std::pair<Ts, std::function<int()> > > tasklist;
	int sig_krunning;
	void* handle;
public:
	unsigned clk_definterval;
	void clk_update();
	void add_fn(std::function<int()> && _Fn, unsigned interval, long long delay = 0, unsigned rep = 0xffffffff);
	void add_fn(const std::function<int()>& _Fn, unsigned interval, long long delay = 0, unsigned rep = 0xffffffff);
	Schedule(unsigned di);
	~Schedule();
};

#ifndef MY_API
#ifdef _DLL
#define MY_API extern "C" __declspec(dllexport)
#else
#define MY_API extern "C"
#endif // _DLL
#endif
#ifndef MYAPI_CC
#define MYAPI_CC __cdecl
#endif

MY_API _DevBase* CreateObj(void* handle, Schedule*);

typedef _DevBase*(MYAPI_CC *DevEntry_Fn)(void*, Schedule*);

#endif
