#include <iostream>
#include "disk.h"

int main(void)
{
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

	std::cin.ignore(1);

	return 0;
}
