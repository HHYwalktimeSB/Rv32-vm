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
	CoreMem(Mode md,unsigned sz,const char* n,unsigned flag...);
	~CoreMem();
public:
	CoreMem(const CoreMem& o) = delete;
	constexpr static unsigned crflag_exist = 1;
};

class _DevBase : public CoreMem{
public:
	virtual void _cb_writedata();
	virtual void _cb_readdata();
	virtual void update();
	_DevBase(Mode md, unsigned sz, const char* n);
	virtual ~_DevBase();
	_DevBase* CreateObj(unsigned sz, const char* n);
	void write(unsigned addr, unsigned val, unsigned sz);
	unsigned read(unsigned addr, unsigned sz);
};

class Schedule {
public:
protected:
	struct Ts {
		unsigned handle;
		unsigned interval;
		long long cnt;
	};
	std::mutex guard;
	std::list<std::pair<Ts, std::function<void()> > > tasklist;
	int sig_krunning;
	void* handle;
public:
	unsigned clk_definterval;
	void clk_update();
	void add_fn(std::function<void()> && _Fn, unsigned interval, long long delay = 0);
	void add_fn(const std::function<void()>& _Fn, unsigned interval, long long delay = 0);
	Schedule(unsigned di);
	~Schedule();
};

#endif
