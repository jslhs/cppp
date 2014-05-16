#include <iostream>

#ifdef _WIN32
#include "disk.h"
#endif

#include "socket.h"
#include "console.h"

int main(void)
{
#ifdef _WIN32
	using namespace utility;
	auto disks = get_disk_volume_map();

	for (auto &p : disks)
	{
		auto &disk = p.first;
		auto &volumes = p.second;

		std::cout << "Disk " 
			<< disk.number() 
			<< ", " 
			<< disk.model() 
			<< ", " 
			<< (disk.size() >> 30) 
			<< "GB" 
			<< ", "
			<< disk.bus_type()
			<< std::endl;
		std::cout << "Logical Drives: " << std::endl;
		for (auto &volume : volumes)
		{
			std::cout << "    "
				<< volume.root_path()
				<< ", "
				<< volume.label()
				<< ", "
				<< volume.fs_name()
				<< ", "
				<< (volume.size() >> 30)
				<< "GB"
				<< std::endl;
		}
		std::cout << std::endl;
	}
#endif

	std::cout << seta(console::color_red) << "Press <Enter> to continue...";

	std::cin.ignore(1);

	return 0;
}
