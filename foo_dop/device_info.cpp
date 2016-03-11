#include "main.h"

void g_get_plist_cfobject(const char * ptr, cfobject::object_t::ptr_t & p_out)
{
	XMLPlistParser(ptr).run_cfobject(p_out);
}

#include <ntddscsi.h>

#define SPT_SENSE_LENGTH 32
#define SPTWB_DATA_LENGTH 255

#define SCSI_INQUIRY_CMD      0x12
#define SCSI_INQUIRY_SBUFSIZE SPT_SENSE_LENGTH
#define SCSI_INQUIRY_DBUFSIZE SPTWB_DATA_LENGTH
#define SCSI_INQUIRY_TIMEOUT  2    /* seconds */

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
	SCSI_PASS_THROUGH spt;
	ULONG             Filler;      // realign buffers to double word boundary
	UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
	UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

bool get_ipod_igsc_checkpoint(ipod_device_ptr_ref_t p_ipod, pfc::array_t<t_uint8> & p_out);

void g_get_device_xml(ipod_device_ptr_ref_t p_ipod, pfc::string8 & p_out)
{
	if (p_ipod->mobile)
		throw pfc::exception("Not supported on iPod touch and iPhone");

	pfc::array_t<t_uint8> checkPoint;
	if (get_ipod_igsc_checkpoint(p_ipod, checkPoint))
		p_out.set_string(reinterpret_cast<const char *>(checkPoint.get_ptr()), checkPoint.get_size());
	else
		_g_get_device_xml(p_ipod->drive, p_out);
}

template <size_t buffer>
struct io_ctrl_igsc {
	char v0;
	char v1;
	short v2;
	short v3;
	short v4;
	t_uint8 m_buffer[buffer];
};

bool get_ipod_igsc_checkpoint(ipod_device_ptr_ref_t p_ipod, pfc::array_t<t_uint8> & p_out)
{
	try
	{
		if (p_ipod->mobile)
			throw pfc::exception("Not supported on iPod touch and iPhone");
		if (!p_ipod->driver_symbolic_path.length())
			throw pfc::exception("Could not locate iPod driver");

		win32::handle_ptr_t p_volume;

		unsigned long returned;

		p_volume.set(CreateFile(p_ipod->driver_symbolic_path.get_ptr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL));

		if (!p_volume.is_valid()) throw pfc::exception(pfc::string8() << "CreateFile failed: " << format_win32_error(GetLastError()));

		io_ctrl_igsc<0x1000> data;
		memset(&data, 0, sizeof(data));
		data.v0 = -64;
		data.v1 = 64;
		data.v2 = 2;
		data.v3 = 0;
		data.v4 = 0x1000;

		while (true)
		{
			if (!DeviceIoControl(p_volume, 0x2200A0, &data, sizeof(data), &data, sizeof(data), &returned, NULL))
				break;
			if (returned <= 8) break;
			p_out.append_fromptr(data.m_buffer, returned - 8);
			if (returned < 0x1008) break;
			data.v3++;
		}

		p_volume.release();
	}
	catch (pfc::exception const &) { return false; }
	return p_out.get_size() > 150;
}

struct io_ctrl_batery_data {
	char v0;
	char v1;
	short v2;
	short v3;
	short v4;
	t_uint16 v5;
};

t_uint16 get_ipod_lowlevel_value(ipod_device_ptr_ref_t p_ipod, short value)
{
	if (p_ipod->mobile)
		throw pfc::exception("Not supported on iPod touch and iPhone");
	if (!p_ipod->driver_symbolic_path.length())
		throw pfc::exception("Could not locate iPod driver");

	win32::handle_ptr_t p_volume;

	unsigned long returned;

	p_volume.set(CreateFile(p_ipod->driver_symbolic_path.get_ptr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL));

	if (!p_volume.is_valid()) throw pfc::exception(pfc::string8() << "CreateFile failed: " << format_win32_error(GetLastError()));

	io_ctrl_batery_data data;
	data.v0 = -64;
	data.v1 = 64;
	data.v2 = value;
	data.v3 = 0;
	data.v4 = 2;
	data.v5 = 0;

	if (!DeviceIoControl(p_volume, 0x2200A0, &data, sizeof(data), &data, sizeof(data), &returned, NULL))
		throw exception_win32(GetLastError());

	p_volume.release();
	return data.v5;
}

void get_ipod_battery_status(ipod_device_ptr_ref_t p_ipod, ipod_battery_status & p_out)
{
	t_uint16 data = get_ipod_lowlevel_value(p_ipod, ipod_lowlevel_battery_status);

	p_out.m_charged = (data & 0x80) == 0;
	p_out.m_unk = (data >> 8);
	p_out.m_level = (data & 0x3);
	p_out.m_raw_data = data;
}


void _g_get_device_xml(const char drive, pfc::string8 & p_out)
{
	pfc::array_t<TCHAR> str;
	str.append_fromptr(_T("\\\\.\\"), 4);
	str.append_single(drive);
	str.append_fromptr(_T(":"), 2);

	win32::handle_ptr_t p_volume;
	HANDLE volume = INVALID_HANDLE_VALUE;
	SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
	unsigned long returned, length;

	p_volume.set(CreateFile(str.get_ptr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));

	if (!p_volume.is_valid()) throw pfc::exception(pfc::string8() << "CreateFile failed: " << format_win32_error(GetLastError()));

	memset(&sptwb, 0, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
	sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
	sptwb.spt.PathId = 0;
	sptwb.spt.TargetId = 1;
	sptwb.spt.Lun = 0;
	sptwb.spt.CdbLength = 6;
	sptwb.spt.SenseInfoLength = SCSI_INQUIRY_SBUFSIZE;
	sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
	sptwb.spt.DataTransferLength = SCSI_INQUIRY_DBUFSIZE;
	sptwb.spt.TimeOutValue = SCSI_INQUIRY_TIMEOUT;
	sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
	sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucSenseBuf);
	sptwb.spt.Cdb[0] = SCSI_INQUIRY_CMD;
	sptwb.spt.Cdb[1] = 1;
	sptwb.spt.Cdb[2] = 0xC0;
	sptwb.spt.Cdb[4] = SCSI_INQUIRY_DBUFSIZE;

	length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + sptwb.spt.DataTransferLength;
	if (!DeviceIoControl(p_volume, IOCTL_SCSI_PASS_THROUGH, &sptwb, sizeof(SCSI_PASS_THROUGH), &sptwb, length, &returned, NULL))
		throw exception_win32(GetLastError());

	pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> buf;
	//buf.grow_size(size+sptwb.ucDataBuf[3]);


	if (sptwb.ucDataBuf[1] != 0xC0)
		throw pfc::exception("Error sending command. This is normal for older iPod models.");

	pfc::array_t<t_uint8> pages;

	pfc::dynamic_assert(sptwb.ucDataBuf[3] + 3 < sizeof(sptwb.ucDataBuf));

	pages.append_fromptr(&sptwb.ucDataBuf[4], sptwb.ucDataBuf[3]);


	t_size i, count = pages.get_count();
	buf.prealloc(count * 255);

	for (i = 0; i < count; i++)
	{
		memset(&sptwb, 0, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
		sptwb.spt.Length = sizeof(SCSI_PASS_THROUGH);
		sptwb.spt.PathId = 0;
		sptwb.spt.TargetId = 1;
		sptwb.spt.Lun = 0;
		sptwb.spt.CdbLength = 6;
		sptwb.spt.SenseInfoLength = SCSI_INQUIRY_SBUFSIZE;
		sptwb.spt.DataIn = SCSI_IOCTL_DATA_IN;
		sptwb.spt.DataTransferLength = SCSI_INQUIRY_DBUFSIZE;
		sptwb.spt.TimeOutValue = SCSI_INQUIRY_TIMEOUT;
		sptwb.spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf);
		sptwb.spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucSenseBuf);
		sptwb.spt.Cdb[0] = SCSI_INQUIRY_CMD;
		sptwb.spt.Cdb[1] = 1;
		sptwb.spt.Cdb[2] = pages[i];
		sptwb.spt.Cdb[4] = SCSI_INQUIRY_DBUFSIZE;

		length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, ucDataBuf) + sptwb.spt.DataTransferLength;
		if (!DeviceIoControl(p_volume, IOCTL_SCSI_PASS_THROUGH, &sptwb, sizeof(SCSI_PASS_THROUGH), &sptwb, length, &returned, NULL))
			throw exception_win32(GetLastError());

		buf.append_fromptr(sptwb.ucDataBuf + 4, sptwb.ucDataBuf[3]);
	}

	p_volume.release();

	p_out.set_string((char*)buf.get_ptr(), buf.get_size());
}

void g_get_sysinfo(ipod_device_ptr_ref_t p_ipod, pfc::array_t<t_uint8> & p_out, abort_callback & p_abort)
{
	pfc::string8 drive;
	p_ipod->get_database_path(drive);
	drive << "\\Device\\SysInfo";
	service_ptr_t<file> pfile;
	filesystem::g_open_read(pfile, drive, p_abort);
	t_filesize size = pfile->get_size_ex(p_abort);
	t_size asize = pfc::downcast_guarded<t_size>(size);
	p_out.set_size(asize);
	pfile->read(p_out.get_ptr(), asize, p_abort);
}

void g_get_device_info_from_sysinfo(const t_uint8 * sysinfo, t_size size, ipod_info & info)
{
	const char * ptr_start = (char*)sysinfo;
	const char * ptr = (char*)sysinfo;
	while (t_size(ptr - ptr_start) < size)
	{
		const char * type_start = ptr;
		t_size type_len = 0;
		while (t_size(ptr - ptr_start)<size && *ptr != ':') ptr++;
		type_len = ptr - type_start;
		if (t_size(ptr - ptr_start)<size && *ptr == ':') ptr++;
		else break;
		if (t_size(ptr - ptr_start)<size && *ptr == ' ') ptr++;
		const char * val_start = ptr;
		t_size val_len = 0;
		while (t_size(ptr - ptr_start)<size && *ptr != '\n') ptr++;
		val_len = ptr - val_start;
		if (t_size(ptr - ptr_start)<size && *ptr == '\n') ptr++;

		if (!stricmp_utf8_max(type_start, "pszSerialNumber", type_len))
		{
			info.serial.set_string(val_start, val_len);
		}
		else if (!stricmp_utf8_max(type_start, "boardHwSwInterfaceRev", type_len))
		{
			if (val_len && *val_start == '0')
			{
				val_start++; val_len--;
			}
			if (val_len && *val_start == 'x')
			{
				val_start++; val_len--;
			}
			t_uint32 hwid = strtoul_n(val_start, val_len, 0x10);
			info.board_id = hwid;
			info.board_valid = true;
			/*t_uint32 mhwid = (hwid & 0xffff0000) >> 16;
			if (hwid == 1) // 1g
			else if (hwid == 2) // 2g
			else if (hwid == 3) // 3g
			else if (hwid == 4) // mini 1g*/
		}
		/*else if (!stricmp_utf8_max(type_start, "iPodFamily", type_len))
		{
		p_out.family_id = strtoul_n(val_start, val_len);
		}
		else if (!stricmp_utf8_max(type_start, "updaterFamily", type_len))
		{
		p_out.updater_family_id = strtoul_n(val_start, val_len);
		}
		if (!stricmp_utf8_max(type_start, "visibleBuildID", type_len))
		{
		p_out.firmware . set_string(val_start, val_len);
		}*/

	}
}

void g_get_device_info(const char * xml, ipod_info & info)
{
	XMLPlistParser pxml(xml);
	pxml.run(info);
}
void g_get_artwork_info(const char * xml, device_properties_t & p_out)
{
	XMLPlistParser pxml(xml);
	pxml.run_artwork_v2(p_out);
}

bool g_string_list_has_item(const char * const * p_list, t_size count, const char * item)
{
	t_size i;
	for (i = 0; i<count; i++)
	{
		if (!stricmp_utf8(p_list[i], item)) return true;
	}
	return false;
}
const char * g_model_strings[] =
{ "1G", "2G", "3G", "4G", "4G Photo/Color", "5G", "5.5G", "Classic/6G (2007)", "Classic/6G (2008)", "Classic/6G (2009)", "Mini 1G", "Mini 2G", "Nano 1G", "Nano 2G", "Nano 3G", "Nano 4G", "Nano 5G", "Nano 6G", "Nano 7G", "Shuffle 1G", "Shuffle 2G", "Shuffle 3G", "Shuffle 4G" };

const char * g_5g_model_strings[] =
{ "White","Black","U2" };

const char * g_6g_model_strings[] =
{ "Black","Silver" };

const char * g_6_1g_model_strings[] =
{ "Black","Silver" };

const char * g_6_2g_model_strings[] =
{ "Black","Silver" };

const char * g_mini_1g_model_strings[] =
{ "Silver","Green","Pink","Blue","Gold" };

const char * g_mini_2g_model_strings[] =
{ "Silver","Green","Pink","Blue" };

const char * g_4g_model_strings[] =
{ "White","U2" };

const char * g_nano_1g_model_strings[] =
{ "White","Black" };

const char * g_nano_2g_model_strings[] =
{ "Black","Pink","Green","Blue","Silver","Red" };

const char * g_nano_3g_model_strings[] =
{ "Pink","Black","Red","Green","Blue","Silver" };

const char * g_nano_4g_model_strings[] =
{ "Black","Red","Yellow","Green","Orange","Purple","Pink","Blue","Silver" };

const char * g_nano_5g_model_strings[] =
{ "Silver","Black","Purple","Blue","Green","Yellow","Orange","Red","Pink", };

const char * g_nano_6g_model_strings[] =
{ "Silver","Graphite","Blue","Green","Orange","Pink","Red" };

const char * g_nano_7g_model_strings[] =
{ "Pink","Yellow","Blue","Green","Purple","Silver","Slate","Red" };

const char * g_shuffle_2g_model_strings[] =
{
	"Orange (2006)","Pink (2006)","Green (2006)","Blue (2006)","Silver (2006)",
	"Purple (2007)", "Green (2007)", "Blue (2007)", "Silver (2007)",
	"Purple (2008)", "Blue (2008)", "Green (2008)", "Red (2008)"
};

const char * g_shuffle_3g_model_strings[] =
{ "Black","Silver","Green","Blue","Pink","Aluminium" };

const char * g_shuffle_4g_model_strings[] =
{ "Silver","Pink","Orange","Green","Blue" };

void g_get_model_string(const ipod_models::ipod_model & model, pfc::string8 & p_out)
{
	p_out.reset();
	p_out << "iPod ";
	if (model.model < tabsize(g_model_strings))
		p_out << g_model_strings[model.model];
	if (model.model == ipod_models::ipod_6g)
	{
		if (model.submodel < tabsize(g_6g_model_strings))
			p_out << " " << g_6g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_5g || model.model == ipod_models::ipod_5_5g)
	{
		if (model.submodel < tabsize(g_5g_model_strings))
			p_out << " " << g_5g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_4g || model.model == ipod_models::ipod_4g_color)
	{
		if (model.submodel < tabsize(g_4g_model_strings))
			p_out << " " << g_4g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_mini_1g)
	{
		if (model.submodel < tabsize(g_mini_1g_model_strings))
			p_out << " " << g_mini_1g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_mini_2g)
	{
		if (model.submodel < tabsize(g_mini_2g_model_strings))
			p_out << " " << g_mini_2g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_1g)
	{
		if (model.submodel < tabsize(g_nano_1g_model_strings))
			p_out << " " << g_nano_1g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_2g)
	{
		if (model.submodel < tabsize(g_nano_2g_model_strings))
			p_out << " " << g_nano_2g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_3g)
	{
		if (model.submodel < tabsize(g_nano_3g_model_strings))
			p_out << " " << g_nano_3g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_shuffle_2g)
	{
		if (model.submodel < tabsize(g_shuffle_2g_model_strings))
			p_out << " " << g_shuffle_2g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_shuffle_3g)
	{
		if (model.submodel < tabsize(g_shuffle_3g_model_strings))
			p_out << " " << g_shuffle_3g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_shuffle_4g)
	{
		if (model.submodel < tabsize(g_shuffle_4g_model_strings))
			p_out << " " << g_shuffle_4g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_4g)
	{
		if (model.submodel < tabsize(g_nano_4g_model_strings))
			p_out << " " << g_nano_4g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_5g)
	{
		if (model.submodel < tabsize(g_nano_5g_model_strings))
			p_out << " " << g_nano_5g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_6g)
	{
		if (model.submodel < tabsize(g_nano_6g_model_strings))
			p_out << " " << g_nano_6g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_nano_7g)
	{
		if (model.submodel < tabsize(g_nano_7g_model_strings))
			p_out << " " << g_nano_7g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_6_1g)
	{
		if (model.submodel < tabsize(g_6_1g_model_strings))
			p_out << " " << g_6_1g_model_strings[model.submodel];
	}
	else if (model.model == ipod_models::ipod_6_2g)
	{
		if (model.submodel < tabsize(g_6_2g_model_strings))
			p_out << " " << g_6_2g_model_strings[model.submodel];
	}
}

bool ipod_info::get_model(ipod_models::ipod_model & p_out)
{
	const char * ipod_shuffle_4g_silver_codes[] = { "DCMJ" }; //silver
	const char * ipod_shuffle_4g_pink_codes[] = { "DCMK" }; //pink
	const char * ipod_shuffle_4g_orange_codes[] = { "DFDM" }; //orange
	const char * ipod_shuffle_4g_green_codes[] = { "DFDN" }; //green
	const char * ipod_shuffle_4g_blue_codes[] = { "DFDP" }; //blue

	const char * ipod_shuffle_3g_black_codes[] = { "890", "891", "6FC", "6FQ" };
	const char * ipod_shuffle_3g_silver_codes[] = { "0NJ", "4NZ", "6FB", "6FP" };
	const char * ipod_shuffle_3g_green_codes[] = { "ALB","A1U" };
	const char * ipod_shuffle_3g_blue_codes[] = { "ALD","A7B" };
	const char * ipod_shuffle_3g_pink_codes[] = { "ALG","A7D" };
	const char * ipod_shuffle_3g_aluminium_codes[] = { "A1L" };

	const char * ipod_shuffle_27g_red_codes[] = { "439","3W6" };
	const char * ipod_shuffle_27g_green_codes[] = { "438","3FM" };
	const char * ipod_shuffle_27g_blue_codes[] = { "437","3FL" };
	const char * ipod_shuffle_27g_purple_codes[] = { "436","3FK" };

	const char * ipod_shuffle_25g_silver_codes[] = { "YX7","YXH", "1ZK" };
	const char * ipod_shuffle_25g_blue_codes[] = { "YX8","YXJ", "1ZM" };
	const char * ipod_shuffle_25g_green_codes[] = { "YX9","YXK", "1ZP" };
	const char * ipod_shuffle_25g_purple_codes[] = { "YXA","YXL", "1ZR" };

	const char * ipod_6g_black_codes[] = { "YMV","YMX" };
	const char * ipod_6g_silver_codes[] = { "Y5N","YMU" };

	const char * ipod_6_1g_black_codes[] = { "2C7" };
	const char * ipod_6_1g_silver_codes[] = { "2C5" };

	const char * ipod_6_2g_black_codes[] = { "9ZU" };
	const char * ipod_6_2g_silver_codes[] = { "9ZS" };

	const char * ipod_nano_3g_black_codes[] = { "YXX" };
	const char * ipod_nano_3g_red_codes[] = { "YXV" };
	const char * ipod_nano_3g_green_codes[] = { "YXT" };
	const char * ipod_nano_3g_blue_codes[] = { "YXR" };
	const char * ipod_nano_3g_silver_codes[] = { "Y0P","Y0R" };
	const char * ipod_nano_3g_pink_codes[] = { "13F" };

	const char * ipod_nano_4g_black_codes[] = { "3R0","4S5","5BF" };
	const char * ipod_nano_4g_red_codes[] = { "37S","3QZ","5BE" };
	const char * ipod_nano_4g_yellow_codes[] = { "37Q","3QY","4S4","5BD" };
	const char * ipod_nano_4g_green_codes[] = { "37P","3QX","4S0","5BC" };
	const char * ipod_nano_4g_orange_codes[] = { "37L","3QW","4RY","5BB" };
	const char * ipod_nano_4g_purple_codes[] = { "37K","3QU","4RS","5BA" };
	const char * ipod_nano_4g_pink_codes[] = { "37H","3QT","4RM","5B9" };
	const char * ipod_nano_4g_blue_codes[] = { "37G","3QS","4RL","5B8" };
	const char * ipod_nano_4g_silver_codes[] = { "1P1","2ME","4M7","5B7" };

	const char * ipod_nano_5g_silver_codes[] = { "71V","72Q" };
	const char * ipod_nano_5g_black_codes[] = { "71Y","72R" };
	const char * ipod_nano_5g_purple_codes[] = { "721","72S" };
	const char * ipod_nano_5g_blue_codes[] = { "726","72X" };
	const char * ipod_nano_5g_green_codes[] = { "72A","734" };
	const char * ipod_nano_5g_yellow_codes[] = { "72D","738" };
	const char * ipod_nano_5g_orange_codes[] = { "72F","739" };
	const char * ipod_nano_5g_red_codes[] = { "72K","73A" };
	const char * ipod_nano_5g_pink_codes[] = { "72L","73B" };

	const char * ipod_nano_6g_silver_codes[] = { "DCMN","DCMP" }; //Silver
	const char * ipod_nano_6g_graphite_codes[] = { "DDVX","DDW4" }; //Graphite
	const char * ipod_nano_6g_blue_codes[] = { "DDVY","DDW5" }; //Blue
	const char * ipod_nano_6g_green_codes[] = { "DDW0","DDW6" }; //Green
	const char * ipod_nano_6g_orange_codes[] = { "DDW1","DDW7" }; //Orange
	const char * ipod_nano_6g_pink_codes[] = { "DDW2","DDW8" }; //Pink
	const char * ipod_nano_6g_red_codes[] = { "DDW3","DDW9" }; //RED

	const char * ipod_nano_7g_pink_codes[] = { "F0GD","F0GM" }; //Pink
	const char * ipod_nano_7g_yellow_codes[] = { "F0GF","F0GN" }; //Yellow
	const char * ipod_nano_7g_blue_codes[] = { "F0GG","F0GP" }; //Blue
	const char * ipod_nano_7g_green_codes[] = { "F0GH","F0GQ" }; //Green
	const char * ipod_nano_7g_purple_codes[] = { "F0GJ","F0GR" }; //Purple
	const char * ipod_nano_7g_silver_codes[] = { "F0GK","F0GT" }; //Silver
	const char * ipod_nano_7g_slate_codes[] = { "F0GL","F0GV" }; //Slate
	const char * ipod_nano_7g_red_codes[] = { "F4LN","F4LP" }; //RED


	const char * ipod_5g_black_codes[] = { "TXK","TXM","TXL","TXN","UK8","V9M","V9N","V9R","V9S","WEE","WEF","WEJ","WEK","WUA","WUC" };
	const char * ipod_5g_u2_codes[] = { "V9V","W9G","WEM" };

	const char * ipod_nano_1g_black_codes[] = { "TK2,TK3",   "TJT,TJU,UPR" };

	const char * ipod_nano_2g_black_codes[] = { "VQT","VQU" };
	const char * ipod_nano_2g_pink_codes[] = { "VQK","VQL" };
	const char * ipod_nano_2g_green_codes[] = { "VQH","VQJ" };
	const char * ipod_nano_2g_blue_codes[] = { "V8W","V8X" };
	const char * ipod_nano_2g_silver_codes[] = { "V8T","V8U","VQ5","VQ6" };
	const char * ipod_nano_2g_red_codes[] = { "WL2","WL3","X9A","X9B" };

	const char * ipod_shuffle_2g_orange_codes[] = { "XR1","XR3" };
	const char * ipod_shuffle_2g_green_codes[] = { "XQY","XR0" };
	const char * ipod_shuffle_2g_blue_codes[] = { "XQV","XQX" };
	const char * ipod_shuffle_2g_pink_codes[] = { "XQS","XQU" };
	const char * ipod_shuffle_2g_silver_codes[] = { "VTE","VTF","YX6","YXG","1ZH" };

	const char * ipod_mini_1g_green_codes[] = { "QKJ","QKN" };
	const char * ipod_mini_1g_pink_codes[] = { "QKK","QKP" };
	const char * ipod_mini_1g_blue_codes[] = { "QKL","QKQ" };
	const char * ipod_mini_1g_gold_codes[] = { "QKM","QKR" };

	const char * ipod_mini_2g_green_codes[] = { "S4J","S4K","S47","S48" };
	const char * ipod_mini_2g_pink_codes[] = { "S4G","S4H","S45","S46" };
	const char * ipod_mini_2g_blue_codes[] = { "S4E","S4F","S43","S44" };

	const char * ipod_4g_color_u2_codes[] = { "TM2" };
	const char * ipod_4g_u2_codes[] = { "S2X" };

	p_out.submodel = 0;

	t_size serial_len = serial.get_length();
	const char * p_code = serial.get_ptr();
	t_size model_len = serial_len >= 12 ? 4 : 3;
	p_code += (serial_len > model_len ? serial_len - model_len : serial_len);

	if (family_valid)
		switch (family_id)
		{
			case 1: //1g, 2g
				p_out.model = ipod_models::ipod_1g;
				return true;
			case 2: //3g
				p_out.model = ipod_models::ipod_3g;
				return true;
			case 3: //mini 1g, 2g
				if (updater_family_id == 4)
				{
					p_out.model = ipod_models::ipod_mini_1g;
					if (g_string_list_has_item(ipod_mini_1g_gold_codes, tabsize(ipod_mini_1g_gold_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_1g_gold;
					else if (g_string_list_has_item(ipod_mini_1g_pink_codes, tabsize(ipod_mini_1g_pink_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_1g_pink;
					else if (g_string_list_has_item(ipod_mini_1g_green_codes, tabsize(ipod_mini_1g_green_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_1g_green;
					else if (g_string_list_has_item(ipod_mini_1g_blue_codes, tabsize(ipod_mini_1g_blue_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_1g_blue;
				}
				else if (updater_family_id == 7)
				{
					p_out.model = ipod_models::ipod_mini_2g;
					if (g_string_list_has_item(ipod_mini_2g_pink_codes, tabsize(ipod_mini_2g_pink_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_2g_pink;
					else if (g_string_list_has_item(ipod_mini_2g_green_codes, tabsize(ipod_mini_2g_green_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_2g_green;
					else if (g_string_list_has_item(ipod_mini_2g_blue_codes, tabsize(ipod_mini_2g_blue_codes), p_code))
						p_out.submodel = ipod_models::ipod_mini_2g_blue;
				}
				return true;
			case 4: //4g
				p_out.model = ipod_models::ipod_4g;
				if (g_string_list_has_item(ipod_4g_u2_codes, tabsize(ipod_4g_u2_codes), p_code))
					p_out.submodel = ipod_models::ipod_4g_u2;
				return true;
			case 5: //4g color
				p_out.model = ipod_models::ipod_4g_color;
				if (g_string_list_has_item(ipod_4g_color_u2_codes, tabsize(ipod_4g_color_u2_codes), p_code))
					p_out.submodel = ipod_models::ipod_4g_color_u2;
				return true;
			case 6: //5g
				p_out.model = updater_family_id == 25 ? ipod_models::ipod_5_5g : ipod_models::ipod_5g;
				if (g_string_list_has_item(ipod_5g_black_codes, tabsize(ipod_5g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_5g_black;
				else if (g_string_list_has_item(ipod_5g_u2_codes, tabsize(ipod_5g_u2_codes), p_code))
					p_out.submodel = ipod_models::ipod_5g_u2;
				return true;
			case 7: //nano 1
				p_out.model = ipod_models::ipod_nano_1g;
				if (g_string_list_has_item(ipod_nano_1g_black_codes, tabsize(ipod_nano_1g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_1g_black;
				return true;
			case 9: //nano 2
				p_out.model = ipod_models::ipod_nano_2g;
				if (g_string_list_has_item(ipod_nano_2g_black_codes, tabsize(ipod_nano_2g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_black;
				else if (g_string_list_has_item(ipod_nano_2g_pink_codes, tabsize(ipod_nano_2g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_pink;
				else if (g_string_list_has_item(ipod_nano_2g_green_codes, tabsize(ipod_nano_2g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_green;
				else if (g_string_list_has_item(ipod_nano_2g_blue_codes, tabsize(ipod_nano_2g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_blue;
				else if (g_string_list_has_item(ipod_nano_2g_silver_codes, tabsize(ipod_nano_2g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_silver;
				else if (g_string_list_has_item(ipod_nano_2g_red_codes, tabsize(ipod_nano_2g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_2g_red;
				else
					p_out.submodel = -1;
				return true;
			case 11: //classic
				if (updater_family_id == 35)
				{
					p_out.model = ipod_models::ipod_6_2g;
					if (g_string_list_has_item(ipod_6_2g_silver_codes, tabsize(ipod_6_2g_silver_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_silver;
					else if (g_string_list_has_item(ipod_6_2g_black_codes, tabsize(ipod_6_2g_black_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_black;
					else
						p_out.submodel = -1;
				}
				else if (updater_family_id == 33)
				{
					p_out.model = ipod_models::ipod_6_1g;
					if (g_string_list_has_item(ipod_6_1g_silver_codes, tabsize(ipod_6_1g_silver_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_silver;
					else if (g_string_list_has_item(ipod_6_1g_black_codes, tabsize(ipod_6_1g_black_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_black;
					else
						p_out.submodel = -1;
				}
				else
				{
					p_out.model = ipod_models::ipod_6g;
					if (g_string_list_has_item(ipod_6g_silver_codes, tabsize(ipod_6g_silver_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_silver;
					else if (g_string_list_has_item(ipod_6g_black_codes, tabsize(ipod_6g_black_codes), p_code))
						p_out.submodel = ipod_models::ipod_6xg_black;
					else
						p_out.submodel = -1;
				}
				return true;
			case 12: //nano 3
				p_out.model = ipod_models::ipod_nano_3g;
				if (g_string_list_has_item(ipod_nano_3g_black_codes, tabsize(ipod_nano_3g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_black;
				else if (g_string_list_has_item(ipod_nano_3g_red_codes, tabsize(ipod_nano_3g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_red;
				else if (g_string_list_has_item(ipod_nano_3g_green_codes, tabsize(ipod_nano_3g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_green;
				else if (g_string_list_has_item(ipod_nano_3g_blue_codes, tabsize(ipod_nano_3g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_blue;
				else if (g_string_list_has_item(ipod_nano_3g_silver_codes, tabsize(ipod_nano_3g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_silver;
				else if (g_string_list_has_item(ipod_nano_3g_pink_codes, tabsize(ipod_nano_3g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_3g_pink;
				else
					p_out.submodel = -1;
				return true;
			case 15: //nano 4
				p_out.model = ipod_models::ipod_nano_4g;
				if (g_string_list_has_item(ipod_nano_4g_black_codes, tabsize(ipod_nano_4g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_black;
				else if (g_string_list_has_item(ipod_nano_4g_red_codes, tabsize(ipod_nano_4g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_red;
				else if (g_string_list_has_item(ipod_nano_4g_yellow_codes, tabsize(ipod_nano_4g_yellow_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_yellow;
				else if (g_string_list_has_item(ipod_nano_4g_green_codes, tabsize(ipod_nano_4g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_green;
				else if (g_string_list_has_item(ipod_nano_4g_orange_codes, tabsize(ipod_nano_4g_orange_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_orange;
				else if (g_string_list_has_item(ipod_nano_4g_purple_codes, tabsize(ipod_nano_4g_purple_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_purple;
				else if (g_string_list_has_item(ipod_nano_4g_pink_codes, tabsize(ipod_nano_4g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_pink;
				else if (g_string_list_has_item(ipod_nano_4g_blue_codes, tabsize(ipod_nano_4g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_blue;
				else if (g_string_list_has_item(ipod_nano_4g_silver_codes, tabsize(ipod_nano_4g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_4g_silver;
				else
					p_out.submodel = -1;
				return true;
			case 16: //nano 5
				p_out.model = ipod_models::ipod_nano_5g;
				if (g_string_list_has_item(ipod_nano_5g_black_codes, tabsize(ipod_nano_5g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_black;
				else if (g_string_list_has_item(ipod_nano_5g_red_codes, tabsize(ipod_nano_5g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_red;
				else if (g_string_list_has_item(ipod_nano_5g_yellow_codes, tabsize(ipod_nano_5g_yellow_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_yellow;
				else if (g_string_list_has_item(ipod_nano_5g_green_codes, tabsize(ipod_nano_5g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_green;
				else if (g_string_list_has_item(ipod_nano_5g_orange_codes, tabsize(ipod_nano_5g_orange_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_orange;
				else if (g_string_list_has_item(ipod_nano_5g_purple_codes, tabsize(ipod_nano_5g_purple_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_purple;
				else if (g_string_list_has_item(ipod_nano_5g_pink_codes, tabsize(ipod_nano_5g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_pink;
				else if (g_string_list_has_item(ipod_nano_5g_blue_codes, tabsize(ipod_nano_5g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_blue;
				else if (g_string_list_has_item(ipod_nano_5g_silver_codes, tabsize(ipod_nano_5g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_5g_silver;
				else
					p_out.submodel = -1;
				return true;
			case 17: // nano 6
				p_out.model = ipod_models::ipod_nano_6g;
				if (g_string_list_has_item(ipod_nano_6g_silver_codes, tabsize(ipod_nano_6g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_silver;
				else if (g_string_list_has_item(ipod_nano_6g_graphite_codes, tabsize(ipod_nano_6g_graphite_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_graphite;
				else if (g_string_list_has_item(ipod_nano_6g_blue_codes, tabsize(ipod_nano_6g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_blue;
				else if (g_string_list_has_item(ipod_nano_6g_green_codes, tabsize(ipod_nano_6g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_green;
				else if (g_string_list_has_item(ipod_nano_6g_orange_codes, tabsize(ipod_nano_6g_orange_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_orange;
				else if (g_string_list_has_item(ipod_nano_6g_pink_codes, tabsize(ipod_nano_6g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_pink;
				else if (g_string_list_has_item(ipod_nano_6g_red_codes, tabsize(ipod_nano_6g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_6g_red;
				else
					p_out.submodel = -1;
				return true;
			case 18: // nano 7
				p_out.model = ipod_models::ipod_nano_7g;
				if (g_string_list_has_item(ipod_nano_7g_silver_codes, tabsize(ipod_nano_7g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_silver;
				else if (g_string_list_has_item(ipod_nano_7g_slate_codes, tabsize(ipod_nano_7g_slate_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_slate;
				else if (g_string_list_has_item(ipod_nano_7g_blue_codes, tabsize(ipod_nano_7g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_blue;
				else if (g_string_list_has_item(ipod_nano_7g_green_codes, tabsize(ipod_nano_7g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_green;
				else if (g_string_list_has_item(ipod_nano_7g_yellow_codes, tabsize(ipod_nano_7g_yellow_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_yellow;
				else if (g_string_list_has_item(ipod_nano_7g_pink_codes, tabsize(ipod_nano_7g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_pink;
				else if (g_string_list_has_item(ipod_nano_7g_red_codes, tabsize(ipod_nano_7g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_red;
				else if (g_string_list_has_item(ipod_nano_7g_purple_codes, tabsize(ipod_nano_7g_purple_codes), p_code))
					p_out.submodel = ipod_models::ipod_nano_7g_purple;
				else
					p_out.submodel = -1;
				return true;
			case 128: //shuffle 1
				p_out.model = ipod_models::ipod_shuffle_1g;
				return true;
			case 130: //shuffle 2
				p_out.model = ipod_models::ipod_shuffle_2g;
				if (g_string_list_has_item(ipod_shuffle_2g_orange_codes, tabsize(ipod_shuffle_2g_orange_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_2g_orange;
				else if (g_string_list_has_item(ipod_shuffle_2g_pink_codes, tabsize(ipod_shuffle_2g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_2g_pink;
				else if (g_string_list_has_item(ipod_shuffle_2g_green_codes, tabsize(ipod_shuffle_2g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_2g_green;
				else if (g_string_list_has_item(ipod_shuffle_2g_blue_codes, tabsize(ipod_shuffle_2g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_2g_blue;
				else if (g_string_list_has_item(ipod_shuffle_2g_silver_codes, tabsize(ipod_shuffle_2g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_2g_silver;

				else if (g_string_list_has_item(ipod_shuffle_25g_purple_codes, tabsize(ipod_shuffle_25g_purple_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_25g_purple;
				else if (g_string_list_has_item(ipod_shuffle_25g_green_codes, tabsize(ipod_shuffle_25g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_25g_green;
				else if (g_string_list_has_item(ipod_shuffle_25g_blue_codes, tabsize(ipod_shuffle_25g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_25g_blue;
				else if (g_string_list_has_item(ipod_shuffle_25g_silver_codes, tabsize(ipod_shuffle_25g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_25g_silver;

				else if (g_string_list_has_item(ipod_shuffle_27g_purple_codes, tabsize(ipod_shuffle_27g_purple_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_27g_purple;
				else if (g_string_list_has_item(ipod_shuffle_27g_green_codes, tabsize(ipod_shuffle_27g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_27g_green;
				else if (g_string_list_has_item(ipod_shuffle_27g_blue_codes, tabsize(ipod_shuffle_27g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_27g_blue;
				else if (g_string_list_has_item(ipod_shuffle_27g_red_codes, tabsize(ipod_shuffle_27g_red_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_27g_red;
				else
					p_out.submodel = -1;
				return true;
			case 132: //shuffle 3
				p_out.model = ipod_models::ipod_shuffle_3g;
				if (g_string_list_has_item(ipod_shuffle_3g_black_codes, tabsize(ipod_shuffle_3g_black_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_black;
				else if (g_string_list_has_item(ipod_shuffle_3g_silver_codes, tabsize(ipod_shuffle_3g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_silver;
				else if (g_string_list_has_item(ipod_shuffle_3g_green_codes, tabsize(ipod_shuffle_3g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_green;
				else if (g_string_list_has_item(ipod_shuffle_3g_blue_codes, tabsize(ipod_shuffle_3g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_blue;
				else if (g_string_list_has_item(ipod_shuffle_3g_pink_codes, tabsize(ipod_shuffle_3g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_pink;
				else if (g_string_list_has_item(ipod_shuffle_3g_aluminium_codes, tabsize(ipod_shuffle_3g_aluminium_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_3g_aluminium;
				else
					p_out.submodel = -1;
				return true;
			case 133: //shuffle 4
				p_out.model = ipod_models::ipod_shuffle_4g;
				if (g_string_list_has_item(ipod_shuffle_4g_silver_codes, tabsize(ipod_shuffle_4g_silver_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_4g_silver;
				else if (g_string_list_has_item(ipod_shuffle_4g_pink_codes, tabsize(ipod_shuffle_4g_pink_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_4g_pink;
				else if (g_string_list_has_item(ipod_shuffle_4g_orange_codes, tabsize(ipod_shuffle_4g_orange_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_4g_orange;
				else if (g_string_list_has_item(ipod_shuffle_4g_green_codes, tabsize(ipod_shuffle_4g_green_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_4g_green;
				else if (g_string_list_has_item(ipod_shuffle_4g_blue_codes, tabsize(ipod_shuffle_4g_blue_codes), p_code))
					p_out.submodel = ipod_models::ipod_shuffle_4g_blue;
				else
					p_out.submodel = -1;
				return true;
		}
	else if (board_valid)
	{
		t_uint32 hwid = (board_id & 0xffff0000) >> 16;
		if (hwid == 1) // 1g
		{
			p_out.model = ipod_models::ipod_1g;
			return true;
		}
		else if (hwid == 2) // 2g
		{
			p_out.model = ipod_models::ipod_2g;
			return true;
		}
		else if (hwid == 3) // 3g
		{
			p_out.model = ipod_models::ipod_3g;
			return true;
		}
		else if (hwid == 4) // mini 1g
		{
			p_out.model = ipod_models::ipod_mini_1g;
			if (g_string_list_has_item(ipod_mini_1g_gold_codes, tabsize(ipod_mini_1g_gold_codes), p_code))
				p_out.submodel = ipod_models::ipod_mini_1g_gold;
			else if (g_string_list_has_item(ipod_mini_1g_pink_codes, tabsize(ipod_mini_1g_pink_codes), p_code))
				p_out.submodel = ipod_models::ipod_mini_1g_pink;
			else if (g_string_list_has_item(ipod_mini_1g_green_codes, tabsize(ipod_mini_1g_green_codes), p_code))
				p_out.submodel = ipod_models::ipod_mini_1g_green;
			else if (g_string_list_has_item(ipod_mini_1g_blue_codes, tabsize(ipod_mini_1g_blue_codes), p_code))
				p_out.submodel = ipod_models::ipod_mini_1g_blue;
			return true;
		}
	}
	return false;
}
