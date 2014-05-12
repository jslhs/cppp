#pragma once

#include <mutex>

class hierarchical_mutex
{
public:
	explicit hierarchical_mutex(unsigned long val)
		: _hval(val)
		, _prev_hval(0)
	{
	}

	void lock()
	{
		check_for_violation();
		_m.lock();
		update_hval();
	}

	void unlock()
	{
		_this_thread_hval = _prev_hval;
		_m.unlock();
	}

	bool try_lock()
	{
		check_for_violation();
		if (!_m.try_lock())
			return false;
		update_hval();
		return true;
	}

private:
	void check_for_violation()
	{
		if (_this_thread_hval <= _hval)
		{
			throw std::logic_error("mutex hierarchy violated!");
		}
	}

	void update_hval()
	{
		_prev_hval = _this_thread_hval;
		_this_thread_hval = _hval;
	}

private:
	std::mutex _m;
	unsigned long const _hval;
	unsigned long _prev_hval;
#ifdef _MSC_VER
	static __declspec(thread) unsigned long _this_thread_hval;
#else
	static thread_local unsigned long _this_thread_hval;
#endif
};

#ifdef _MSC_VER
__declspec(thread) unsigned long hierarchical_mutex::_this_thread_hval(ULONG_MAX);
#else
thread_local unsigned long hierarchical_mutex::_this_thread_hval(ULONG_MAX);
#endif
