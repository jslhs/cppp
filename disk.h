#pragma once

#include <map>
#include <vector>
#include <string>
#include <cstdint>

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <ntddscsi.h>

namespace utility
{
	using disk_size_t = uint64_t;
	using sector_size_t = uint32_t;

	struct partition
	{
		int number;
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

		enum ata_protocols
		{
			ata_proto_hard_reset,
			ata_proto_srst,
			ata_proto_reserved,
			ata_proto_non_data,
			ata_proto_pio_data_in,
			ata_proto_pio_data_out,
			ata_proto_pio_dma,
			ata_proto_pio_dma_queued,
			ata_proto_device_diagnostic,
			ata_proto_device_reset,
			ata_proto_udma_data_in,
			ata_proto_udma_data_out,
			ata_proto_fpdma,
			ata_return_response_info = 15
		};

		enum ata_status_mask
		{
			ata_mask_error = 0x1,
			ata_mask_busy = 0x1 << 7,
			ata_mask_device_fault = 0x1 << 5,
			ata_mask_deferred_write_error = 0x1 << 4,
		};

		enum ata_flags
		{
			ata_drdy_required =			ATA_FLAGS_DRDY_REQUIRED,
			ata_data_in =				ATA_FLAGS_DATA_IN,
			ata_data_out =				ATA_FLAGS_DATA_OUT,
			ata_48bit_command =			ATA_FLAGS_48BIT_COMMAND,
			ata_use_dma =				ATA_FLAGS_USE_DMA,
			ata_no_multiple =			ATA_FLAGS_NO_MULTIPLE,
		};

		bool exec_ata_cmd(HANDLE hdev, const ata_command &cmd, void *buf, uint32_t transfer_length, int flags, int timeout_seconds = 1);
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

		// logical sector size, bytes per sector
		sector_size_t sector_size() const;

		// physical sector size, bytes per sector
		sector_size_t phy_sector_size() const;

		const std::string& model() const;
		const std::string& serial_number() const;
		const std::string& firmware_rev() const;
		const partition_set& parts() const;
		disk_layout layout() const;

	public: // operations
		bool delete_layout();
		bool format();

	private:
		void identify();

	private:
		int _num;
		bool _exists;
		sector_size_t _log_bytes_per_sector; // logical bytes/sector
		sector_size_t _phy_bytes_per_sector; // physical bytes/sector
		disk_size_t _size;
		std::string _err;
		std::string _filename;
		std::string _serial;
		std::string _model;
		std::string _firm_rev;
		partition_set _parts;
	};

	bool operator==(const disk& disk1, const disk& disk2);
	bool operator!=(const disk& disk1, const disk& disk2);
	bool operator<(const disk& disk1, const disk& disk2);

	using disk_set = std::vector<disk>;

	// logical drive
	class volume
	{
	public:
		explicit volume(char drive_letter);
		explicit volume(const std::string &root_path);
		~volume();

		disk_size_t size() const;
		disk_size_t free_space() const;
		const std::string& label() const;
		const std::string& root_path() const;
		const std::string& fs_name() const;

		const disk_set& extents() const;

	private:
		void init();

	private:
		disk_size_t _size;
		disk_size_t _free_space;
		std::string _filename;
		std::string _fs_name;
		std::string _root_path;
		std::string _label;
		std::string _err;
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
