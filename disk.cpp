#include "disk.h"
#include <sstream>

namespace utility
{
	namespace detail
	{
		handle_wrapper::handle_wrapper(HANDLE handle, std::string &err)
			: _handle(handle)
			, _err(err)
		{

		}

		handle_wrapper::~handle_wrapper()
		{
			_err = get_win32_error_string();
		}

		handle_wrapper::operator HANDLE() const
		{
			return _handle;
		}
	}

	disk::disk(int number)
		: _num(number)
		, _exists(false)
		, _size(0)
	{
		std::stringstream ss;
		ss << "\\\\.\\PhysicalDrive" << _num;
		_filename = ss.str();
		identify();
	}

	disk::~disk()
	{

	}

	void disk::identify()
	{
		using namespace detail;
		auto h = CreateFileA(_filename.c_str()
			, GENERIC_READ
			, FILE_SHARE_READ | FILE_SHARE_WRITE
			, nullptr
			, OPEN_EXISTING
			, 0
			, nullptr);
		
		if (h == INVALID_HANDLE_VALUE)
		{
			_err = get_win32_error_string();
			return;
		}

		_exists = true;

		BOOL rv = FALSE;
		DWORD bytes_returned;

		DISK_GEOMETRY_EX geo;
		rv = DeviceIoControl(handle_wrapper(h, _err)
			, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
			, nullptr
			, 0
			, &geo
			, sizeof(geo)
			, &bytes_returned
			, nullptr);
		if (rv)
		{
			_size = geo.DiskSize.QuadPart;
		}

		int bufsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + kwin32_max_parts * sizeof(PARTITION_INFORMATION_EX);
		char *buf = new char[bufsize];
		DRIVE_LAYOUT_INFORMATION_EX *di = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX *>(buf);
		rv = DeviceIoControl(handle_wrapper(h, _err)
			, IOCTL_DISK_GET_DRIVE_LAYOUT_EX
			, nullptr
			, 0
			, di
			, bufsize
			, &bytes_returned
			, nullptr);
		if (rv)
		{

		}
	}

	int disk::number() const
	{
		return _num;
	}

	disk_size_t disk::size() const
	{
		return _size;
	}

	bool disk::exists() const
	{
		return _exists;
	}

	volume::volume(char drive_letter)
		: _filename("\\\\.\\")
	{
		_filename += drive_letter;
		_filename += ':';
	}

	volume::volume(const std::string &name)
		: _filename(name)
	{
		if (_filename.back() == '\\') _filename.pop_back();
	}

	volume::~volume()
	{

	}

	const disk_set &volume::extents() const
	{
		return _extents;
	}

	bool operator==(const disk &disk1, const disk &disk2)
	{
		return disk1.number() == disk2.number();
	}

	bool operator!=(const disk &disk1, const disk &disk2)
	{
		return !(disk1 == disk2);
	}

	bool operator<(const disk &disk1, const disk &disk2)
	{
		return disk1.number() < disk2.number();
	}

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
			disk d(i);
			if (d.exists()) map[std::move(d)];
		}

		char buf[MAX_PATH]{};
		auto h = FindFirstVolumeA(buf, sizeof(buf));
		do
		{
			volume v(buf);
			for (auto &disk : v.extents())
			{
				map[disk].push_back(v);
			}
		} while (FindNextVolumeA(h, buf, sizeof(buf)));

		return map;
	}
}
