#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include "TaggedTuple.hpp"
#include "Mutex.hpp"

struct name {};
struct age {};
struct email {};

tagged_tuple<name, std::string, age, int, email, std::string> get_record() 
{
	return{ "Bob", 32, "bob@bob.bob" };
}

template<class T>
bool CAS(T *addr, T exp, T val)
{
	if (*addr == exp)
	{
		*addr = val;
		return true;
	}
	return false;
}

int main(void)
{
	std::map<std::string, int> tokens{
		{ "name0", 20 },
		{ "name1", 21 },
		{ "name2", 23 }
	};

	std::cout << "Age: " << get_record().get<age>() << std::endl;

	hierarchical_mutex mutex(32);
	hierarchical_mutex mutex1(100);
	//std::lock(mutex, mutex1);
	//std::lock_guard<hierarchical_mutex> lk(mutex);
	//std::lock_guard<hierarchical_mutex> lk1(mutex1);

	std::atomic_int ai;
	ai.store(200);
	std::atomic_pointer ss;
	//ai.compare_exchange_weak(2,1);

	int *a = new int{ 32 };
	int *b = new int{ 234 };

	auto r = CAS(&a, a, b);

	auto x = [](int x){ return ++x; }(0);

	return 0;
}
