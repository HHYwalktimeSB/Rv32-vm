#include "pch.h"
#include "_dev_impl.h"
#include<Windows.h>

unsigned short CoreMem::readw(unsigned addr)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	auto r = *(unsigned short*)(buf + addr);
	_shared_data_sync_guard_end();
	return r;
}

unsigned char CoreMem::readc(unsigned addr)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	auto r = *(unsigned char*)(buf + addr);
	_shared_data_sync_guard_end();
	return r;
}

void CoreMem::_shared_data_sync_guard_init()
{
	if(mode!=Mode::sync)
	WaitForSingleObject(hse, INFINITE);
}

void CoreMem::_shared_data_sync_guard_end()
{
	if (mode != Mode::sync)
		ReleaseSemaphore(hse, 1, NULL);
}

CoreMem::CoreMem(Mode md, unsigned sz, const char* n,unsigned flag ...):mode(md),size(sz)
{
	if (n) {
		auto ll = strlen(n) + 1;
		name = new char[ll];
		memcpy_s(name, ll, n, ll);
	}
	else name = nullptr;
	switch (mode)
	{
	case CoreMem::Mode::sync:
		hfm = NULL;
		hse = NULL;
		buf = new char[sz];
		reference_cnt = NULL;
		break;
	case CoreMem::Mode::async:
		hfm = NULL;
		if ((flag & 1)==0) {
			hse = CreateSemaphore(NULL, 1, 1, NULL);
			buf = new char[sz];
			reference_cnt = new int;
			reference_cnt[0] = 1;
		}
		else {
			va_list va;
			va_start(va, flag);
			hse = va_arg(va, void*);
			buf = va_arg(va, char*);
			reference_cnt = va_arg(va, int*);
			va_end(va);
			++*reference_cnt;
		}
		break;
	case CoreMem::Mode::fmmem:
		if (n == nullptr)throw "error empty name";
		hfm = OpenFileMappingA(FILE_MAP_ALL_ACCESS, TRUE, name);
		if (!hfm) {
			hfm = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, FILE_MAP_ALL_ACCESS,
				0, sz, n);
			if (!hfm) throw "err no enough mem";
			hse = CreateSemaphoreA(NULL, 1, 1, n);
		}
		else {
			hse = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, TRUE, n);
		}
		buf = (char*)MapViewOfFile(hfm, PAGE_READWRITE, 0, 0, sz);
		break;
	default:
		hfm = hse = NULL;
		buf = nullptr;
		break;
	}
}

CoreMem::~CoreMem()
{
	switch (mode)
	{
	case CoreMem::Mode::sync:
		delete[] buf;
		break;
	case CoreMem::Mode::async:
		--*reference_cnt;
		if (*reference_cnt == 0)delete[] buf;
		break;
	case CoreMem::Mode::fmmem:
		UnmapViewOfFile(buf);
		CloseHandle(hfm);
		break;
	default:
		break;
	}
	delete[] name;
}

void CoreMem::writedw(unsigned addr, unsigned val)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	*(unsigned*)(buf + addr) = val;
	_shared_data_sync_guard_end();
}

void CoreMem::writew(unsigned addr, unsigned short val)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	*(unsigned short*)(buf + addr) = val;
	_shared_data_sync_guard_end();
}

void CoreMem::writec(unsigned addr, unsigned char val)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	*(unsigned char*)(buf + addr) = val;
	_shared_data_sync_guard_end();
}

void CoreMem::writex(unsigned addr, unsigned cbcnt, const char* v)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	memcpy_s(buf + addr, (unsigned long long)size - addr, v, cbcnt);
	_shared_data_sync_guard_end();
}

void CoreMem::readx(unsigned addr, unsigned cbcnt, char* buf)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	memcpy_s(buf, cbcnt, buf + addr, (unsigned long long)size - addr);
	_shared_data_sync_guard_end();
}

unsigned CoreMem::readdw(unsigned addr)
{
	if (addr >= size)return;
	_shared_data_sync_guard_init();
	auto r = *(unsigned*)(buf + addr);
	_shared_data_sync_guard_end();
	return r;
}

#include<chrono>
using namespace std;

void Schedule::clk_update()
{
	long long sleepus;
	long long elapsed;
	long long diff = 0;
	chrono::time_point<chrono::high_resolution_clock> ts = chrono::high_resolution_clock::now();
	decltype(ts) tp = ts;
	while (sig_krunning) {
		ts = chrono::high_resolution_clock::now();
		elapsed = chrono::duration_cast<chrono::microseconds>(ts - tp).count();
		guard.lock();
		sleepus = clk_definterval;
		for (auto& a : tasklist) {
			a.first.cnt -= elapsed;
			if (a.first.cnt <= 0) {
				a.first.cnt = a.first.interval;
				a.second();
			}
			if (a.first.cnt < sleepus) sleepus = a.first.cnt;
		}
		guard.unlock();
		tp = chrono::high_resolution_clock::now();
		diff = chrono::duration_cast<chrono::microseconds>(ts - tp).count();
		sleepus -= diff;
		if(sleepus>0)std::this_thread::sleep_for(chrono::microseconds( sleepus));
	}
}

void Schedule::add_fn(std::function<void()>&& _Fn, unsigned interval, long long delay)
{
	Ts ttt;
	ttt.cnt = delay;
	ttt.interval = interval;
	ttt.handle = tasklist.size();
	guard.lock();
	tasklist.emplace_back(ttt, std::move(_Fn));
	guard.unlock();
}

void Schedule::add_fn(const std::function<void()>& _Fn, unsigned interval, long long delay)
{
	Ts ttt;
	ttt.cnt = delay;
	ttt.interval = interval;
	ttt.handle = tasklist.size();
	guard.lock();
	tasklist.emplace_back(ttt, _Fn);
	guard.unlock();
}

Schedule::Schedule(unsigned di)
{
	clk_definterval = di>=50?di:50;
	sig_krunning = true;
	std::thread th(Schedule::clk_update, this);
	handle = th.native_handle();
	th.detach();
}

Schedule::~Schedule()
{
	sig_krunning = 0;
	WaitForSingleObject(handle, 20);
}

void _DevBase::_cb_writedata()
{
}

void _DevBase::_cb_readdata()
{
}

void _DevBase::update()
{
}

_DevBase::_DevBase(Mode md, unsigned sz, const char* n):CoreMem(md,sz,n,0)
{
}

_DevBase::~_DevBase()
{
}

_DevBase* _DevBase::CreateObj(unsigned sz, const char* n)
{
	return new _DevBase(Mode::sync, sz, n);
}

void _DevBase::write(unsigned addr, unsigned val, unsigned sz)
{
	switch (sz)
	{
	case 1:
		writec(addr, val);
		break;
	case 2:
		writew(addr, val);
		break;
	case 4:
		writedw(addr, val);
		break;
	default:
		break;
	}
	if (mode == Mode::sync) this->_cb_writedata();
}

unsigned _DevBase::read(unsigned addr, unsigned sz)
{
	unsigned r;
	switch (sz)
	{
	case 1:
		r = readc(addr);
		break;
	case 2:
		r= readw(addr);
		break;
	case 4:
		r= readdw(addr);
		break;
	default:
		r = 0;
		break;
	}
	if (mode == Mode::sync) this->_cb_writedata();
	return r;
}