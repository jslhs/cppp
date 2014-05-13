#include "disk.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <bitset>
#include <memory>
#include <winerror.h>
#pragma comment(lib, "Ws2_32")

namespace utility
{
	namespace detail
	{
		inline std::string trim(const std::string &s)
		{
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){return std::isspace(c); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){return std::isspace(c); }).base();
			return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
		}

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

		bool exec_ata_cmd(HANDLE hdev, const ata_command &cmd, void *buf, uint32_t transfer_length, int flags, int timeout_seconds)
		{
			ata_status status = 0;
			DWORD bytes_returned;
			BOOL rv;

			auto cached_buf = (char *)VirtualAlloc(nullptr, transfer_length, MEM_COMMIT, PAGE_READWRITE);
			if (!cached_buf) return false;

			ATA_PASS_THROUGH_DIRECT param{}, out{};
			param.Length = sizeof(ATA_PASS_THROUGH_DIRECT);
			param.AtaFlags = flags;
			param.DataBuffer = cached_buf;
			param.DataTransferLength = transfer_length;
			param.TimeOutValue = timeout_seconds;
			auto &taskfile = param.CurrentTaskFile;
			taskfile[0] = cmd.feature & 0xff;
			taskfile[1] = cmd.count & 0xff;
			taskfile[2] = cmd.lba & 0xff;
			taskfile[3] = (cmd.lba >> 8) & 0xff;
			taskfile[4] = (cmd.lba >> 16) & 0xff;
			taskfile[5] = cmd.device & 0xff;
			taskfile[6] = cmd.cmd & 0xff;

			rv = DeviceIoControl(hdev
				, IOCTL_ATA_PASS_THROUGH_DIRECT
				, &param
				, sizeof(param)
				, &out
				, sizeof(out)
				, &bytes_returned
				, nullptr);
			status = out.CurrentTaskFile[6];
			auto err = GetLastError();

			if (err == ERROR_NOT_SUPPORTED)
			{
				//SCSI_PASS_THROUGH_DIRECT sp{}, sp_out{};
				auto bufsize = sizeof(SCSI_PASS_THROUGH_DIRECT) + 32;
				std::unique_ptr<char[]> buf(new char[bufsize]);
				SCSI_PASS_THROUGH_DIRECT &sp = *reinterpret_cast<SCSI_PASS_THROUGH_DIRECT *>(buf.get());
				sp.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
				sp.DataBuffer = cached_buf;
				sp.DataTransferLength = transfer_length;
				sp.TimeOutValue = timeout_seconds;
				sp.CdbLength = 12;
				sp.SenseInfoLength = 32;
				sp.SenseInfoOffset = sp.Length;
				sp.Cdb[0] = 0xa1; // ATA PASS-THROUGH(12)

				char *sense = (char *)buf.get() + sp.Length;
				
				int proto = ata_proto_reserved;
				int dir = 0;
				bool use_dma = ((flags & ata_use_dma) == ata_use_dma);
				if (flags & ata_data_in)
				{
					proto = use_dma ? ata_proto_udma_data_in : ata_proto_pio_data_in;
					sp.DataIn = SCSI_IOCTL_DATA_IN;
					dir = 1;
				}
				else if (flags & ata_data_out)
				{
					proto = use_dma ? ata_proto_udma_data_out : ata_proto_pio_data_out;
					sp.DataIn = SCSI_IOCTL_DATA_OUT;
				}
				else
				{
					sp.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
				}

				sp.Cdb[1] = proto << 1;
				sp.Cdb[2] = (0x1 << 5) | (dir << 3) | (0x1 << 2) | 0x02; // transfer length in SECTOR_COUNT field
				sp.Cdb[3] = cmd.feature & 0xff;
				sp.Cdb[4] = cmd.count & 0xff;
				sp.Cdb[5] = cmd.lba & 0xff;
				sp.Cdb[6] = (cmd.lba >> 8) & 0xff;
				sp.Cdb[7] = (cmd.lba >> 16) & 0xff;
				sp.Cdb[8] = cmd.device & 0xff;
				sp.Cdb[9] = cmd.cmd & 0xff;

				rv = DeviceIoControl(hdev
					, IOCTL_SCSI_PASS_THROUGH_DIRECT
					, buf.get()
					, bufsize
					, buf.get()
					, bufsize
					, &bytes_returned
					, nullptr);
				err = GetLastError();
				if (err || sp.ScsiStatus)
				{
					if (sense[1] != 0x1)
					{
						status = 1;
					}
					else
					{
						status = sense[21];
						err = 0;
					}
				}
			}

			memcpy_s(buf, transfer_length, cached_buf, transfer_length);

			rv = VirtualFree(cached_buf, 0, MEM_RELEASE);
			SetLastError(err);

			return !(status & ata_mask_error);
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
			, GENERIC_READ | GENERIC_WRITE
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
			_log_bytes_per_sector = geo.Geometry.BytesPerSector;
		}

		int bufsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + kwin32_max_parts * sizeof(PARTITION_INFORMATION_EX);
		std::unique_ptr<char[]> buf(new char[bufsize]);
		DRIVE_LAYOUT_INFORMATION_EX *di = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX *>(buf.get());
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
			for (DWORD i = 0; i < di->PartitionCount; i++)
			{
				auto &part = di->PartitionEntry[i];
				partition p;
				p.number = part.PartitionNumber;
				p.starting_offset = part.StartingOffset.QuadPart;
				p.length = part.PartitionLength.QuadPart;
				p.unused = (part.PartitionStyle == PARTITION_STYLE_MBR) 
					&& (part.Mbr.PartitionType == PARTITION_ENTRY_UNUSED);
				_parts.push_back(p);
			}
		}

		STORAGE_PROPERTY_QUERY q{};
		q.PropertyId = StorageDeviceProperty;
		q.QueryType = PropertyStandardQuery;

		STORAGE_DESCRIPTOR_HEADER hdr{};
		rv = DeviceIoControl(handle_wrapper(h, _err)
			, IOCTL_STORAGE_QUERY_PROPERTY
			, &q
			, sizeof(q)
			, &hdr
			, sizeof(hdr)
			, &bytes_returned
			, nullptr);

		if (rv)
		{
			std::unique_ptr<char[]> buf(new char[hdr.Size]);
			rv = DeviceIoControl(handle_wrapper(h, _err)
				, IOCTL_STORAGE_QUERY_PROPERTY
				, &q
				, sizeof(q)
				, buf.get()
				, hdr.Size
				, &bytes_returned
				, nullptr);

			STORAGE_DEVICE_DESCRIPTOR *sdd = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR *>(buf.get());

		}

		// ATA IDENTIFY DEVICE APPROACH
		ata_command cmd{};
		cmd.cmd = 0xec; // IDENTIFY DEVICE
		cmd.count = 1;
		uint16_t data[256];

		const int word_serial_start = 10;
		const int word_serial_count = 10;
		const int word_firm_start = 23;
		const int word_firm_count = 4;
		const int word_model_start = 27;
		const int word_model_count = 20;

		typedef std::pair<int, int> range;
		range ranges[] = {
			{ word_serial_start, word_serial_start + word_serial_count }, // serial number
			{ word_firm_start, word_firm_start + word_firm_count }, // firmware revision
			{ word_model_start, word_model_start + word_model_count }, // model number
		}; 
		
		auto s = exec_ata_cmd(handle_wrapper(h, _err), cmd, data, sizeof(data), ata_data_in);
	
		for (auto r : ranges)
		{
			for (int i = r.first; i < r.second; i++)
				data[i] = ntohs(data[i]);
		}

		_serial = trim(std::string((char *)&data[word_serial_start], word_serial_count * 2));
		_firm_rev = trim(std::string((char *)&data[word_firm_start], word_firm_count * 2));
		_model = trim(std::string((char *)&data[word_model_start], word_model_count * 2));

		auto word106 = std::bitset<16>(data[106]);
		_log_bytes_per_sector = word106.test(12) ? 
			(*reinterpret_cast<uint32_t *>(&data[107])) : 256 * 2;
		_phy_bytes_per_sector = _log_bytes_per_sector << (data[106] & 0xf);

		disk_size_t max_lba = *reinterpret_cast<disk_size_t *>(&data[100]);
	}

	int disk::number() const
	{
		return _num;
	}

	disk_size_t disk::size() const
	{
		return _size;
	}

	// logical sector size, bytes per sector
	sector_size_t disk::sector_size() const
	{
		return _log_bytes_per_sector;
	}

	// physical sector size, bytes per sector
	sector_size_t disk::phy_sector_size() const
	{
		return _phy_bytes_per_sector;
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
