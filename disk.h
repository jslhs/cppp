#pragma once

#include <map>
#include <vector>
#include <string>
#include <cstdint>

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

namespace utility
{
	using disk_size_t = uint64_t;

	struct partition
	{
		disk_size_t starting_offset;
		disk_size_t length;
		bool unused;
	};

	using partition_set = std::vector<partition>;

	namespace detail
	{
		class handle_wrapper
		{
		public:
			handle_wrapper(HANDLE handle, std::string &err);
			~handle_wrapper();

			operator HANDLE() const;

		private:
			std::string &_err;
			HANDLE _handle;
		};

		struct ata_command
		{
			uint32_t feature;
			uint32_t count;
			uint64_t lba;
			uint32_t device;
			uint32_t cmd;
		};

		using ata_status = uint32_t;

		enum ata_status_mask
		{
			ata_mask_error = 0x1,
			ata_mask_busy = 0x1 << 7,
			ata_mask_device_fault = 0x1 << 5,
			ata_mask_deferred_write_error = 0x1 << 4,
		};

		enum ata_flags
		{
			ata_drdy_required,
			ata_data_in,
			ata_48bit_command,
			ata_use_dma,
			ata_no_multiple,
		};
	}

	enum disk_layout
	{
		layout_raw,
		layout_mbr,
		layout_gpt
	};

	// physical disk drive
	class disk
	{
	public:
		explicit disk(int num);
		~disk();

	public: // properties
		bool exists() const;
		int number() const;
		disk_size_t size() const;
		const std::string &vendor() const;
		const partition_set &parts() const;
		disk_layout layout() const;

	public: // operations
		bool delete_layout();
		bool format();

	private:
		detail::ata_status exec_ata_cmd(const detail::ata_command &cmd, uint32_t transfer_length, void *buf, detail::ata_flags flags);
		void identify();

	private:
		int _num;
		bool _exists;
		disk_size_t _size;
		std::string _err;
		std::string _filename;
		std::string _vendor;
		std::string _model;
		partition_set _parts;
	};

	bool operator==(const disk &disk1, const disk &disk2);
	bool operator!=(const disk &disk1, const disk &disk2);
	bool operator<(const disk &disk1, const disk &disk2);

	using disk_set = std::vector<disk>;

	// logical drive
	class volume
	{
	public:
		explicit volume(char drive_letter);
		explicit volume(const std::string &name);
//		volume(volume &&);
		~volume();

//		volume &operator=(volume &&);

		disk_size_t size() const;
		disk_size_t freespace() const;
		const std::string &label() const;

		const disk_set &extents() const;

	private:
		//volume(const volume &);
		//volume &operator=(const volume &);

	private:
		std::string _filename;
		std::string _label;
		disk_set _extents;
	};

	using volume_set = std::vector<volume>;
	using disk_volume_map = std::map<disk, volume_set>;

	enum constants
	{
		kwin32_max_phy_disks = 16,
		kwin32_max_parts = 128,
	};

	disk_volume_map get_disk_volume_map(int max_disks = kwin32_max_phy_disks);
	std::string get_win32_error_string(DWORD code = GetLastError(), DWORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
}
