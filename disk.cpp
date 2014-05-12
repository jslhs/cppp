#include "disk.h"

namespace utility
{
	std::string get_win32_error_string(DWORD code, DWORD lang)
	{
		char *buf = nullptr;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_IGNORE_INSERTS
			, nullptr
			, code
			, lang
			, (LPSTR)&buf
			, 0
			, nullptr);

		std::string msg;
		if (buf)
		{
			msg = buf;
			LocalFree(buf);
		}

		return msg;
	}

	disk_volume_map get_disk_volume_map(int max_disks)
	{
		disk_volume_map map;

		for (int i = 0; i < max_disks; i++)
		{
			map[disk(i)];
		}

		return map;
	}
}
