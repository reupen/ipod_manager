#include "stdafx.h"

#include "browse.h"
#include "load_to_playlist.h"
#include "maintenance.h"
#include "resource.h"
#include "shell.h"
#include "sync.h"


namespace mainmenu_groups_dop
{
	// {BBA2F214-DCD4-4af2-A060-FA0A01852164}
	const GUID file_ipod_popup = 
	{ 0xbba2f214, 0xdcd4, 0x4af2, { 0xa0, 0x60, 0xfa, 0xa, 0x1, 0x85, 0x21, 0x64 } };

	// {C9419AF0-87EE-4c85-AF64-BB841BD7F376}
	const GUID ipod_read_part = 
	{ 0xc9419af0, 0x87ee, 0x4c85, { 0xaf, 0x64, 0xbb, 0x84, 0x1b, 0xd7, 0xf3, 0x76 } };

	// {8B2AC2AF-DAB9-43a2-8F0A-6A4215940E6D}
	const GUID ipod_write_part = 
	{ 0x8b2ac2af, 0xdab9, 0x43a2, { 0x8f, 0xa, 0x6a, 0x42, 0x15, 0x94, 0xe, 0x6d } };

	// {870EBB73-6F25-4333-B82A-36BB849BBE07}
	const GUID ipod_write2_part = 
	{ 0x870ebb73, 0x6f25, 0x4333, { 0xb8, 0x2a, 0x36, 0xbb, 0x84, 0x9b, 0xbe, 0x7 } };

	// {E4E08914-50E6-43a3-964A-E9F6B2B5A3E2}
	const GUID ipod_mount_part = 
	{ 0xe4e08914, 0x50e6, 0x43a3, { 0x96, 0x4a, 0xe9, 0xf6, 0xb2, 0xb5, 0xa3, 0xe2 } };

	// {03F8BE3E-7996-4b11-8013-82E54A45C82E}
	const GUID ipod_properties_part = 
	{ 0x3f8be3e, 0x7996, 0x4b11, { 0x80, 0x13, 0x82, 0xe5, 0x4a, 0x45, 0xc8, 0x2e } };

}

mainmenu_group_popup_factory g_mainmenu_group_ipod_popup(mainmenu_groups_dop::file_ipod_popup, mainmenu_groups::file, mainmenu_commands::sort_priority_dontcare, "iPod");

mainmenu_group_factory g_mainmenu_group_ipod_read_part(mainmenu_groups_dop::ipod_read_part, mainmenu_groups_dop::file_ipod_popup, mainmenu_commands::sort_priority_base);
mainmenu_group_factory g_mainmenu_group_ipod_write_part(mainmenu_groups_dop::ipod_write_part, mainmenu_groups_dop::file_ipod_popup, mainmenu_commands::sort_priority_base+1);
mainmenu_group_factory g_mainmenu_group_ipod_write2_part(mainmenu_groups_dop::ipod_write2_part, mainmenu_groups_dop::file_ipod_popup, mainmenu_commands::sort_priority_base+2);
mainmenu_group_factory g_mainmenu_group_ipod_mount_part(mainmenu_groups_dop::ipod_mount_part, mainmenu_groups_dop::file_ipod_popup, mainmenu_commands::sort_priority_base+4);
mainmenu_group_factory g_mainmenu_group_ipod_properties_part(mainmenu_groups_dop::ipod_properties_part, mainmenu_groups_dop::file_ipod_popup, mainmenu_commands::sort_priority_base+3);

void g_show_ipod_devices_popup();

class NOVTABLE mainmenu_command_t
{
public:
	virtual const GUID & get_guid() const = 0;
	virtual void get_name(pfc::string_base & p_out)const = 0;
	virtual bool get_description(pfc::string_base & p_out) const = 0;
	virtual bool get_display(pfc::string_base & p_text, t_uint32 & p_flags) const
	{
		p_flags = 0;
		get_name(p_text);
		return true;
	}
	virtual void execute(service_ptr_t<service_base> p_callback) const = 0;
};

class mainmenu_popup_ipod_devices_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "iPod devices";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Shows connected iPods."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		g_show_ipod_devices_popup();
	}
}; 

class mainmenu_load_library_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Load library";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Loads your iPod music library to a playlist."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		ipod_load_library_v2_t::g_run(core_api::get_main_window());
	}
}; 

class mainmenu_rewrite_database_soft_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Rewrite database";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Loads database, and writes it back."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		ipod_rewrite_library_soft_t::g_run(core_api::get_main_window());
	}
}; 

class mainmenu_recover_orphaned_files_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Recover orphaned tracks";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Locates orphaned tracks on your iPod and adds them to the database."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		ipod_recover_orphaned_files::g_run(core_api::get_main_window());
	}
}; 

class mainmenu_sync_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Synchronise...";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Synchronises your iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		g_run_sync();
	}
}; 

class mainmenu_browse_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Manage contents";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Browse and manage your iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		t_browser::g_run(core_api::get_main_window());
	}
}; 

class mainmenu_filesystem_explorer_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "File system explorer";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "View the device file system."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		//t_browser::g_run(core_api::get_main_window());
		shell_window::ptr shell = new shell_window;
		if (shell->create(core_api::get_main_window(), NULL, ui_helpers::window_position_t(core_api::get_main_window(),0,0,600,400)))
		{
			ShowWindow(shell->get_wnd(), SW_SHOWNORMAL);
		}
	}
}; 


class mainmenu_eject_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Eject";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Eject your iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		//ipod_hash_t * p_tst = new ipod_hash_t;
		//p_tst->run(core_api::get_main_window());
		ipod_eject_t::g_run(core_api::get_main_window());
	}
}; 

#ifdef _DEBUG
void _check_hresult (HRESULT hr);
namespace photodb
{
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
void g_check_gdiplus_ret(Gdiplus::Status ret, const char * func);
}
using namespace photodb;
// #define TESTCOMMAND 1
#endif
#if TESTCOMMAND	

class mainmenu_mount_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Test command.";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "xxx."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		try {
			abort_callback_impl noabort;

			file::ptr f, w;
			filesystem::g_open_read(f, "i:\\iTunesCDB", noabort);
			pfc::array_staticsize_t<t_uint8> d(pfc::downcast_guarded<t_uint32>(f->get_size_ex(noabort)));
			if (d.get_size() > 244)
			{
				f->read(d.get_ptr(), d.get_size(), noabort);
				pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> decompressed_data;
				zlib_stream zs;
				zs.decompress_singlerun(d.get_ptr()+244, d.get_size(), decompressed_data);
				filesystem::g_open_write_new(w, "i:\\iTunesCDB - decompressed", noabort);
				w->write(d.get_ptr(), 244, noabort);
				w->write(decompressed_data.get_ptr(), decompressed_data.get_size(), noabort);
			}
		} catch (pfc::exception const & ex) {
			console::formatter() << ex.what();
		}

		//ipod_mount_t::g_run(core_api::get_main_window());
	}
}; 

#endif

void g_get_mobile_device_properties_string (const ipod_device_ptr_t & p_ipod, pfc::string8 & p_out)
{
	cfobject::object_t::ptr_t main_device_info, battery_info;

	if (p_ipod->mobile_device.is_valid() && p_ipod->mobile_device->m_api.is_valid())
	{
		in_mobile_device_api_handle_sync lockedAPI(p_ipod->mobile_device->m_api);

		if (lockedAPI.is_valid())
		{
			{
				insync (p_ipod->mobile_device->m_device_sync);
				p_ipod->mobile_device->connect_lockdown();

				CFTypeRef ref = NULL;
				ref = lockedAPI->AMDeviceCopyValue(p_ipod->mobile_device->m_device, NULL, NULL);
				g_get_CFType_object(lockedAPI, ref, main_device_info);
				if (ref)
				{
					lockedAPI->CFRelease(ref);
					ref = NULL;
				}
				ref = lockedAPI->AMDeviceCopyValue(p_ipod->mobile_device->m_device, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.battery"), NULL);
				g_get_CFType_object(lockedAPI, ref, battery_info);
				if (ref)
				{
					lockedAPI->CFRelease(ref);
					ref = NULL;
				}
				p_ipod->mobile_device->disconnect_lockdown();
			}


			class string8_valid_t
			{
			public:
				pfc::string8 m_value;
				bool m_valid;

				string8_valid_t() : m_valid(false) {};
			};

			pfc::string8 temp, serialNumber, firmwareVersion, firmwareBuild, bootLoaderVersion, productType, modelNumber, wifiMacAddress;

			string8_valid_t baseBandVersion, baseBandVersionBL, IMEI, phoneNumber, bluetoothMACAddress;

			bool b_value = false;

			if (main_device_info.is_valid())
			{
				pfc::string8 productName;

				if (main_device_info->m_dictionary.get_child(L"DeviceName", temp))
					p_out << "Name: " << temp << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"ProductType", productType))
				{
					if (!stricmp_utf8(productType, "iPod1,1"))
						productName = "iPod touch 1G";
					else if (!stricmp_utf8(productType, "iPod2,1"))
						productName = "iPod touch 2G";
					else if (!stricmp_utf8(productType, "iPod3,1"))
						productName = "iPod touch 3G";
					else if (!stricmp_utf8(productType, "iPod4,1"))
						productName = "iPod touch 4G";
					else if (!stricmp_utf8(productType, "iPhone1,1"))
						productName = "iPhone";
					else if (!stricmp_utf8(productType, "iPhone1,2"))
						productName = "iPhone 3G";
					else if (!stricmp_utf8(productType, "iPhone2,1"))
						productName = "iPhone 3GS";
					else if (!stricmp_utf8(productType, "iPhone3,1"))
						productName = "iPhone 4 GSM";
					else if (!stricmp_utf8(productType, "iPhone3,2"))
						productName = "iPhone 4 (Unknown Variant)"; //cdma china?
					else if (!stricmp_utf8(productType, "iPhone3,3"))
						productName = "iPhone 4 CDMA";
					else if (!stricmp_utf8(productType, "iPhone4,1"))
						productName = "iPhone (2011)"; //gsm?
					else if (!stricmp_utf8(productType, "iPhone4,2"))
						productName = "iPhone (2011)"; //cdma?
					else if (!stricmp_utf8(productType, "iPad1,1"))
						productName = "iPad";
					else if (!stricmp_utf8(productType, "iPad2,1"))
						productName = "iPad 2 (Wi-Fi)";
					else if (!stricmp_utf8(productType, "iPad2,2"))
						productName = "iPad 2 (Wi-Fi + 3G GSM)";
					else if (!stricmp_utf8(productType, "iPad2,3"))
						productName = "iPad 2 (Wi-Fi + 3G CDMA)";
					p_out << "Model: " << productName << "\r\n";
				}
				if (main_device_info->m_dictionary.get_child(L"ModelNumber", modelNumber))
					p_out << "Model Number: " << modelNumber << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"HardwareModel", temp))
					p_out << "Hardware Model Number: " << temp << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"CPUArchitecture", temp))
					p_out << "CPU Architecture: " << temp << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"ProductionSOC", b_value))
					p_out << "Production SOC: " << (b_value ? "Yes" : "No") << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"SerialNumber", serialNumber))
					p_out << "Serial Number: " << serialNumber << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"MLBSerialNumber", temp))
					p_out << "Main Logic Board Serial Number: " << temp << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"InternationalMobileEquipmentIdentity", IMEI.m_value))
					p_out << "IMEI: " << IMEI.m_value << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"PhoneNumber", phoneNumber.m_value))
					p_out << "Phone Number: " << phoneNumber.m_value << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"WiFiAddress", wifiMacAddress))
					p_out << "WiFi MAC Address: " << wifiMacAddress << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"BluetoothAddress", bluetoothMACAddress.m_value))
					p_out << "Bluetooth MAC Address: " << bluetoothMACAddress.m_value << "\r\n";
				
				p_out << "\r\n";

				if (main_device_info->m_dictionary.get_child(L"ProductVersion", firmwareVersion))
					p_out << "Software Version: " << firmwareVersion << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"BuildVersion", firmwareBuild))
					p_out << "Software Build: " << firmwareBuild << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"FirmwareVersion", bootLoaderVersion))
					p_out << "Firmware Version: " << bootLoaderVersion << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"BasebandBootloaderVersion", baseBandVersionBL.m_value))
					p_out << "Baseband Bootloader Version: " << baseBandVersionBL.m_value << "\r\n";
				if (main_device_info->m_dictionary.get_child(L"BasebandVersion", baseBandVersion.m_value))
					p_out << "Baseband Version: " << baseBandVersion.m_value << "\r\n";
			}

			t_size batteryLevel = pfc_infinite;
			bool isCharging = false;

			if (battery_info.is_valid())
			{
				cfobject::object_t::ptr_t BatteryCurrentCapacity, BatteryIsCharging;
				if (battery_info->m_dictionary.get_child(L"BatteryIsCharging", BatteryIsCharging))
					isCharging = BatteryIsCharging->get_bool();
				if (battery_info->m_dictionary.get_child(L"BatteryCurrentCapacity", BatteryCurrentCapacity))
				{
					batteryLevel = BatteryCurrentCapacity->get_flat_uint32();
					
					p_out << "\r\n" "Battery Status: " << (isCharging ? "Charging" : (batteryLevel == 100 ? "Charged" : "Not charging")) << "\r\n";
					p_out << "Battery Current Level: " << batteryLevel << "%";
				}
			}



		}
	}
}

class ipod_get_properties_t : public ipod_action_base_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_get_properties_t);

	void on_run() 
	{
		TRACK_CALL_TEXT("ipod_get_properties_t");
		try
		{
			ipod::tasks::drive_scanner_t & drives = m_drive_scanner;
			drives.run();
			pfc::string8 str, msg;
			/*if (drives.m_ipods[0]->mobile)
			{
			drives.m_ipods[0]->device_instance_path
			}
			else*/
			{
				ipod_info info;
				drive_space_info_t drive_space_info;

				if (drives.m_ipods[0]->mobile)
				{
					HWND wnd = g_drive_manager.get_wnd();
					//if (wnd)
					//	PostMessage(wnd, MSG_GET_PROPERTIES, NULL, NULL);
					g_get_mobile_device_properties_string(drives.m_ipods[0], msg);

				}
				else
				{
					try
					{
						g_get_device_xml(drives.m_ipods[0], str);
						{
							try
							{
								//device_properties_t aw;
								//g_get_artwork_info(str, aw);
								g_get_device_info(str, info);
								ipod_models::ipod_model model;
								bool mod_valid = info.get_model(model);
								pfc::string8 mod;
								if (!mod_valid)
								{
									t_size sc = info.serial.get_length();
									mod << "Unknown. Key: (" << info.family_id << "," << info.updater_family_id << "," << info.serial.get_ptr() + (sc > 4 ? sc-4 : 0) << ")\r\n";
								}else
									g_get_model_string(model, mod);
								msg << "Model: " << mod << "\r\n"
									<< "Serial Number: " << info.serial << "\r\n"
									<< "Software Version: " << info.firmware;

								try
								{
									if (info.battery_poll_interval)
									{
										/*try {
										t_uint16 limit =
											get_ipod_lowlevel_value(drives.m_ipods[0], ipod_lowlevel_volume_limit);
										} catch (pfc::exception & ex) {console::formatter() << ex.what();}*/
										ipod_battery_status battery_status;
										get_ipod_battery_status(drives.m_ipods[0], battery_status);
										msg << "\r\n""\r\n"
											<< "Battery Status: ";
										if (battery_status.m_level == 3)
											msg << "Charging";
										else if (battery_status.m_level == 2)
											msg << "Charged";
										else if (battery_status.m_level == 1)
											msg << "Not charging";
										else
											msg << "Unknown";
										msg << "\r\n"
											<< "Raw Battery Data: " << battery_status.m_charged << " / "
											<< battery_status.m_level << " / "
											<< battery_status.m_unk; //<< " ("
										//<< pfc::format_hex(battery_status.m_raw_data, 4) << ")";
									}
								}
								catch (const pfc::exception &) {};
							}
							catch (const pfc::exception & ex)
							{
								msg
									<< "Error parsing XML:" << ex.what() << "\r\n" << str << "\r\n\r\n";
							}
						}
					}
					catch (const pfc::exception & ex) 
					{
						try 
						{
							msg << "Failed to send SCSI Inquiry command: " << ex.what() << "\r\n";
							//if (ex.get_code() != ERROR_ACCESS_DENIED)
							{
								msg << "Trying to read SysInfo. Model may not be identified if SCSI Inquiry failed due to denied access.\r\n\r\n";
								abort_callback_impl p_abort;
								pfc::array_t<t_uint8> sysinfo;
								g_get_sysinfo(drives.m_ipods[0], sysinfo, p_abort);
								g_get_device_info_from_sysinfo(sysinfo.get_ptr(), sysinfo.get_size(), info);

								ipod_models::ipod_model model;
								bool mod_valid = info.get_model(model);
								pfc::string8 mod;
								if (!mod_valid)
								{
									t_size sc = info.serial.get_length();
									mod << "Unknown.";
									if (info.board_valid)
										msg << " Key (" << info.board_id << "," << info.serial.get_ptr() + (sc > 3 ? sc-3 : sc) << ")";
								}
								else
									g_get_model_string(model, mod);
								msg << "Model: " << mod << "\r\n"
									<< "Serial Number: " << info.serial;
							}
							//<< "Firmware version: " << info.firmware << "\r\n";
						}
						catch (const pfc::exception & ex2) 
						{
							msg << "Failed to read SysInfo: " << ex2.what();
						};
					};
				}
				{
					{
						try {
							drives.m_ipods[0]->get_capacity_information(drive_space_info);
							msg << "\r\n""\r\n" "Drive Total Capacity: " << mmh::FileSizeFormatter(drive_space_info.m_capacity);
							msg << "\r\n" "Drive Free Space: " << mmh::FileSizeFormatter(drive_space_info.m_freespace);
						}
						catch (const pfc::exception & ex2) 
						{
							msg << "Failed to read query disk drive capacity information: " << ex2.what();
						};
					}
				fbh::show_info_box_threadsafe("iPod Device Information", msg);
				}

			}

		}
		catch (exception_aborted) 
		{
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
	}

private:
	ipod_get_properties_t() : ipod_action_base_t("Load iPod Properties", threaded_process_v2_t::flag_position_bottom_right|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_progress_marquee) {};
};


class ipod_get_properties_raw_t : public ipod_action_base_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_get_properties_raw_t);

	void on_run() 
	{
		TRACK_CALL_TEXT("ipod_get_properties_raw_t");
		try
		{
			ipod::tasks::drive_scanner_t & drives = m_drive_scanner;
			drives.run();
			pfc::string8 str, msg;

			{
				ipod_info info;
				drive_space_info_t drive_space_info;

				if (drives.m_ipods[0]->mobile)
				{
					g_get_mobile_device_properties_string(drives.m_ipods[0], msg);
					{
						cfobject::object_t::ptr_t main_device_info, checkpoint_data, international, sqlpp;

						if (drives.m_ipods[0]->mobile_device.is_valid() && drives.m_ipods[0]->mobile_device->m_api.is_valid())
						{
							in_mobile_device_api_handle_sync lockedAPI(drives.m_ipods[0]->mobile_device->m_api);

							if (lockedAPI.is_valid())
							{
								insync (drives.m_ipods[0]->mobile_device->m_device_sync);
								drives.m_ipods[0]->mobile_device->connect_lockdown();

								CFTypeRef ref = NULL;
								ref = lockedAPI->AMDeviceCopyValue(drives.m_ipods[0]->mobile_device->m_device, NULL, NULL);
								g_get_CFType_object(lockedAPI, ref, main_device_info);
								if (ref)
								{
									lockedAPI->CFRelease(ref);
									ref = NULL;
								}
								ref = lockedAPI->AMDeviceCopyValue(drives.m_ipods[0]->mobile_device->m_device, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.iTunes"), NULL);
								g_get_CFType_object(lockedAPI, ref, checkpoint_data);
								if (ref)
								{
									lockedAPI->CFRelease(ref);
									ref = NULL;
								}
#ifdef _DEBUG
								ref = lockedAPI->AMDeviceCopyValue(drives.m_ipods[0]->mobile_device->m_device, lockedAPI->__CFStringMakeConstantString("com.apple.international"), lockedAPI->__CFStringMakeConstantString("SortSections"));
								g_get_CFType_object(lockedAPI, ref, international);
								if (ref)
								{
									lockedAPI->CFRelease(ref);
									ref = NULL;
								}
								ref = lockedAPI->AMDeviceCopyValue(drives.m_ipods[0]->mobile_device->m_device, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.sync_data_class"), NULL);
								g_get_CFType_object(lockedAPI, ref, sqlpp);
								if (ref)
								{
									lockedAPI->CFRelease(ref);
									ref = NULL;
								}
#endif

								drives.m_ipods[0]->mobile_device->disconnect_lockdown();

								if (main_device_info.is_valid())
								{
									pfc::string8 xml;
									cfobject::g_export_object_to_xml(main_device_info, xml);
									popup_message::g_show(xml, "Lockdown values");
								}
								if (checkpoint_data.is_valid())
								{
									pfc::string8 xml;
									cfobject::g_export_object_to_xml(checkpoint_data, xml);
									popup_message::g_show(xml, "Checkpoint data");
								}
#ifdef _DEBUG
								if (international.is_valid())
								{
									pfc::string8 xml;
									cfobject::g_export_object_to_xml(international, xml);
									popup_message::g_show(xml, "International");
								}
								if (sqlpp.is_valid())
								{
									pfc::string8 xml;
									cfobject::g_export_object_to_xml(sqlpp, xml);
									popup_message::g_show(xml, "SQLPP");
								}
#endif
							}
						}
					}
				}
				else
				{
					//try
					{
						g_get_device_xml(drives.m_ipods[0], str);
						popup_message::g_show(str, "Checkpoint data");
					}
#if 0
					catch (const exception_win32 & ex) 
					{
						if (ex.get_code() != ERROR_ACCESS_DENIED)
						{
							abort_callback_impl p_abort;
							pfc::array_t<t_uint8> sysinfo;
							g_get_sysinfo(drives.m_ipods[0], sysinfo, p_abort);
							sysinfo.append_single(0);
							fbh::show_info_box_threadsafe("SysInfo data", sysinfo.get_ptr());
						}
						else throw;
					};
#endif
				}
			}

		}
		catch (exception_aborted) 
		{
		}
		catch (const pfc::exception & ex) 
		{
			fbh::show_info_box_threadsafe("Error - Load Raw Properties", ex.what(), OIC_WARNING);
		}
	}

private:
	ipod_get_properties_raw_t() : ipod_action_base_t("Load iPod Properties", threaded_process_v2_t::flag_position_bottom_right|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_progress_marquee) {};
};

class mainmenu_properties_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Properties";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Displays some information about the connected iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		ipod_get_properties_t::g_run(core_api::get_main_window());
	}
}; 

// {EB5557ED-8CBB-47d4-B177-53E5B13CBCED}
static const GUID g_guid_cfg_syslog_position = 
{ 0xeb5557ed, 0x8cbb, 0x47d4, { 0xb1, 0x77, 0x53, 0xe5, 0xb1, 0x3c, 0xbc, 0xed } };

cfg_struct_t<ui_helpers::window_position_t> cfg_syslog_position(g_guid_cfg_syslog_position, ui_helpers::window_position_t(0, 0, 800, 600));

class syslog_viewer_t : public ui_helpers::container_window_autorelease_t
{
	class_data & get_class_data() const 
	{
		__implement_get_class_data_ex(_T("syslog_viewer_t"), _T("System Log Viewer - iPod manager"), false, 0, WS_POPUP | WS_CLIPSIBLINGS| WS_CLIPCHILDREN  | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, WS_EX_DLGMODALFRAME, 0);
	}
	void on_size(t_size cx, t_size cy) 
	{
		HDWP dwp = BeginDeferWindowPos(2);
		int m_font_height = uGetFontHeight(m_font);
		if (m_wnd_edit)
			dwp = DeferWindowPos(dwp, m_wnd_edit, NULL, 11*2, 11, cx-11*2-11*2, cy-m_font_height-10-11*2-11*2, SWP_NOZORDER);	
		if (m_wnd_close)
			dwp = DeferWindowPos(dwp, m_wnd_close, NULL, cx-11*2-73*1, cy-11-m_font_height-10, 73, m_font_height+10, SWP_NOZORDER);
		EndDeferWindowPos(dwp);
	};
	void on_size()
	{
		RECT rc;
		GetClientRect(get_wnd(), &rc);
		on_size(RECT_CX(rc), RECT_CY(rc));
	}
	syslog_viewer_t(ipod_device_ptr_t & p_device) : m_device(p_device), m_timer_active(false) {};
public:
	HWND m_wnd_edit, m_wnd_close;
	bool m_initialising;
	gdi_object_t<HFONT>::ptr_t m_font;
	ipod_device_ptr_t m_device;
	bool m_timer_active;

	static void g_run(ipod_device_ptr_t & p_device)
	{
		{
			syslog_viewer_t * p_test = new syslog_viewer_t(p_device);
			RECT rc;
			cfg_syslog_position.get_value().convert_to_rect(rc);
			MapWindowPoints(core_api::get_main_window(), HWND_DESKTOP, (LPPOINT)&rc, 2);
			POINT pt_topleft = {rc.left, rc.top};
			POINT pt_bottomright = {rc.bottom, rc.right};
			ui_helpers::window_position_t pos(rc);
			{
				HMONITOR mon = MonitorFromPoint(pt_bottomright, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(info));
				info.cbSize=sizeof(info);
				GetMonitorInfo(mon, &info);
				if (pos.x+(int)pos.cx>=info.rcWork.right)
					pos.x=info.rcWork.right-(int)pos.cx;
				if (pos.y+(int)pos.cy>=info.rcWork.bottom)
					pos.y=info.rcWork.bottom-(int)pos.cy;
			}
			{
				HMONITOR mon = MonitorFromPoint(pt_topleft, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(info));
				info.cbSize=sizeof(info);
				GetMonitorInfo(mon, &info);
				if (pos.x<info.rcWork.left)
					pos.x=info.rcWork.left;
				else if (pos.x>=info.rcWork.right)
					pos.x=info.rcWork.right-pos.cx;
				if (pos.y<info.rcWork.top)
					pos.y=info.rcWork.top;
				else if (pos.y>=info.rcWork.bottom)
					pos.y=info.rcWork.bottom-pos.cy;
			}
			HWND wnd = p_test->create(core_api::get_main_window(), NULL, pos);
			if (wnd)
				ShowWindow(wnd, SW_SHOWNORMAL);
			else delete p_test;
		}
	}

	void fix_newlines(const char * p_text, pfc::string8 & p_out)
	{
		p_out.reset();
		//pfc::string8 buffer;
		const char * ptr = p_text, *start = ptr;

		while (*ptr)
		{
			start = ptr;
			while (*ptr && *ptr != '\r' && *ptr!='\n') ptr++;
			if (ptr>start)
				p_out.add_string(start, ptr-start);
			if (*ptr)
			{
				p_out.add_byte('\r');
				p_out.add_byte('\n');
			}
			if (*ptr == '\r') ptr++;
			if (*ptr == '\n') ptr++;
		}
	}

	void intialise()
	{
	}

	void update()
	{
		pfc::string8 syslog, fixedsyslog;
		if (m_device->mobile_device.is_valid())
		{
			m_device->mobile_device->m_syslog_relay.get_syslog(syslog);
		}
		fix_newlines(syslog, fixedsyslog);

		uSetWindowText(m_wnd_edit, fixedsyslog);
		LONG_PTR len = SendMessage(m_wnd_edit, EM_GETLINECOUNT , 0, 0);
		SendMessage(m_wnd_edit, EM_LINESCROLL , 0, len);
	}

	virtual LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch (msg)
		{
		case WM_NCCREATE:
			modeless_dialog_manager::g_add(wnd);
			break;
		case WM_TIMER:
			m_timer_active = false;
			KillTimer(wnd, 1);
			update();
			break;
		case WM_USER+2:
			if (m_timer_active == false)
			{
				 SetTimer(wnd, 1, 100, NULL);
				 m_timer_active = true;
			}
			break;
		case WM_CREATE:
			{
				m_initialising=true;

				m_font = uCreateIconFont();

				if (m_device->mobile_device.is_valid())
					m_device->mobile_device->m_syslog_relay.register_wnd(wnd);

				m_wnd_edit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_READONLY|ES_MULTILINE|WS_VSCROLL|WS_HSCROLL, 0, 0, 0, 0, wnd, (HMENU)1001, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_edit, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				m_wnd_close = CreateWindowEx(0, WC_BUTTON, L"Close", WS_CHILD|WS_VISIBLE|WS_TABSTOP, 0, 0,0,0, wnd, (HMENU)IDCANCEL, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_close, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				on_size();
				update();

				m_initialising=false;
			}
			return 0;
		case WM_DESTROY:
			if (m_device->mobile_device.is_valid())
				m_device->mobile_device->m_syslog_relay.deregister_wnd(wnd);
			m_font.release();
			return 0;
		case WM_NCDESTROY:
			modeless_dialog_manager::g_remove(wnd);
			break;
		case WM_SHOWWINDOW:
			if (wp == TRUE && lp == 0)
				SetFocus(m_wnd_close);
			break;
		case DM_GETDEFID:
			return IDCANCEL|(DC_HASDEFID<<16);
		case WM_SIZE:
			{
				RECT rc;
				GetRelativeRect(get_wnd(), core_api::get_main_window(), &rc);
				cfg_syslog_position.get_value().set_from_rect(rc);
				on_size(LOWORD(lp), HIWORD(lp));
				RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE);
			}
			return 0;
		case WM_MOVE:
			{
				RECT rc;
				GetRelativeRect(get_wnd(), core_api::get_main_window(), &rc);
				cfg_syslog_position.get_value().set_from_rect(rc);
			}
			return 0;
		case WM_COMMAND:
			switch (wp)
			{
			case IDCANCEL:
				SendMessage(wnd, WM_CLOSE, NULL, NULL);
				return 0;
			}
			break;
		case WM_ERASEBKGND:
			return FALSE;
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
			SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
			return (LRESULT)GetSysColorBrush(COLOR_BTNHIGHLIGHT);
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(wnd, &ps);
				if (dc)
				{
					RECT rc_client, rc_button;
					GetClientRect(wnd, &rc_client);
					RECT rc_fill = rc_client;
					if (m_wnd_close)
					{
						GetWindowRect(m_wnd_close, &rc_button);
						rc_fill.bottom -= RECT_CY(rc_button)+11;
						rc_fill.bottom -= 11+1;
					}

					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_WINDOW));

					if (m_wnd_close)
					{
						rc_fill.top=rc_fill.bottom;
						rc_fill.bottom+=1;
						FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DLIGHT));
					}
					rc_fill.top=rc_fill.bottom;
					rc_fill.bottom=rc_client.bottom;
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DFACE));

					EndPaint(wnd, &ps);
				}
			}
			return 0;
		case WM_CLOSE:
			//if (!m_tagging_in_progress)
			{
				destroy();
				delete this;
			}
			return 0;
		}
		return DefWindowProc(wnd, msg, wp, lp);
	}
};

class mainmenu_syslog_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "System log viewer";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Displays the syslog for iPod touch/iPhone devices."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		try
		{

			ipod::tasks::drive_scanner_t drives;
			drives.run();
			
			{
				if (drives.m_ipods[0]->mobile)
				{
					syslog_viewer_t::g_run(drives.m_ipods[0]);
					//pfc::string8 syslog;
					//drives.m_ipods[0]->mobile_device->m_syslog_relay.get_syslog(syslog);
					//popup_message::g_show(syslog, "Syslog - iPod manager");
				}
				else
				{
					fbh::show_info_box_threadsafe("Error - System Log", "Only supported on iPod touch/iPhone.");
				}

			}

		}
		catch (const pfc::exception & ex) 
		{
			fbh::show_info_box_threadsafe("Error - System Log Viewer", ex.what());
		};
	}
	virtual bool get_display(pfc::string_base & p_text,t_uint32 & p_flags) const 
	{
		p_flags = 0;
		get_name(p_text);
		return (GetKeyState(VK_SHIFT) & KF_UP) != 0;
	}
}; 
class mainmenu_debug_properties_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Raw properties";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Displays raw device properties for the connected iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		ipod_get_properties_raw_t::g_run(core_api::get_main_window());
	}
	virtual bool get_display(pfc::string_base & p_text,t_uint32 & p_flags) const 
	{
		p_flags = 0;
		get_name(p_text);
		return (GetKeyState(VK_SHIFT) & KF_UP) != 0;
	}
}; 


#ifdef PHOTO_BROWSER
class mainmenu_browse_photos_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Browse Photos";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Browse your photos on your iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		t_photo_browser::g_run(core_api::get_main_window());
	}
}; 
#endif


class mainmenu_send_playlists_t : public mainmenu_command_t
{
public:
	static const GUID g_guid;

	static BOOL CALLBACK g_select_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			//uSetWindowLong(wnd,DWL_USER,lp);
			{
				//rename_param * ptr = (rename_param *)lp;
				HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
				uih::list_view_set_explorer_theme(wnd_list);
				ListView_SetExtendedListViewStyleEx(wnd_list, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

				LVCOLUMN lvc;
				memset(&lvc, 0, sizeof(LVCOLUMN));
				lvc.mask = LVCF_TEXT|LVCF_WIDTH;
				lvc.pszText = _T("Name");
				lvc.cx = 500;

				ListView_InsertColumn(wnd_list, 0, &lvc);
				static_api_ptr_t<playlist_manager> api;

				LVITEM lvi;
				memset(&lvi, 0, sizeof(LVITEM));
				lvi.mask=LVIF_TEXT;
				t_size i, count=api->get_playlist_count();
				for (i=0;i<count;i++)
				{
					lvi.iItem = i;
					pfc::string8 name;
					api->playlist_get_name(i, name);
					pfc::stringcvt::string_os_from_utf8 wide(name);
					lvi.pszText = const_cast<TCHAR*>(wide.get_ptr());
					ListView_InsertItem(wnd_list, &lvi);
				}
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				{
					HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
					int i, count = ListView_GetItemCount(wnd_list);
					static_api_ptr_t<playlist_manager> p_manager;
					pfc::list_t<ipod_send_playlist::t_playlist> playlists;
					if (count>0)
					{
						for (i=0; i<count; i++)
						{
							if (ListView_GetCheckState(wnd_list, i))
							{
								if (i >=0 && (t_size)i < p_manager->get_playlist_count())
								{
									ipod_send_playlist::t_playlist playlist;
									p_manager->playlist_get_name(i, playlist.name);
									p_manager->playlist_get_all_items(i, playlist.items);

									playlists.add_item(playlist);
								}
							}
						}
						ipod_send_playlist::g_run(core_api::get_main_window(),playlists);
					}
					//rename_param * ptr = (rename_param *)GetWindowLong(wnd,DWL_USER);
					EndDialog(wnd,1);
				}
				break;
			case IDCANCEL:
				EndDialog(wnd,0);
				break;
			}
			break;
		case WM_ERASEBKGND:
			SetWindowLongPtr(wnd, DWL_MSGRESULT, TRUE);
			return TRUE;
		case WM_PAINT:
			uih::handle_modern_background_paint(wnd, GetDlgItem(wnd, IDOK));
			return TRUE;
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
			SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		case WM_CLOSE:
			EndDialog(wnd,0);
			break;
		}
		return 0;
	}
	virtual const GUID & get_guid() const {return g_guid;}
	virtual void get_name(pfc::string_base & p_out)const {p_out = "Send playlists...";};
	virtual bool get_description(pfc::string_base & p_out) const {p_out = "Sends playlists to your iPod."; return true;}
	virtual void execute(service_ptr_t<service_base> p_callback) const
	{
		{
			uDialogBox(IDD_SELECT_PLAYLIST, core_api::get_main_window(), g_select_proc);
		}
	}
}; 



const mainmenu_load_library_t g_mainmenu_load_library;
//const mainmenu_load_library_and_playlists_t g_mainmenu_load_library_and_playlists;
//const mainmenu_rewrite_database_t g_mainmenu_rewrite_database;
const mainmenu_rewrite_database_soft_t g_mainmenu_rewrite_database_soft;
const mainmenu_recover_orphaned_files_t g_mainmenu_recover_orphaned_files;
//const mainmenu_mount_ipod_t g_mainmenu_mount_ipod;
//const mainmenu_unmount_ipod_t g_mainmenu_unmount_ipod;
const mainmenu_send_playlists_t g_mainmenu_send_playlists;
//const mainmenu_remove_playlists_t g_mainmenu_remove_playlists;
const mainmenu_sync_t g_mainmenu_sync;
const mainmenu_browse_t g_mainmenu_browse;
const mainmenu_filesystem_explorer_t g_mainmenu_filesystem_explorer;
//const mainmenu_debug_backup_itunesdb_t g_mainmenu_debug_backup_itunesdb;
//const mainmenu_debug_backup_artworkdb_t g_mainmenu_debug_backup_artworkdb;
#ifdef PHOTO_BROWSER
const mainmenu_browse_photos_t g_mainmenu_browse_photos;
#endif
//const mainmenu_prepare_ipod_t g_mainmenu_prepare_ipod;
const mainmenu_eject_t g_mainmenu_eject;
#ifdef TESTCOMMAND
//const mainmenu_dismount_t g_mainmenu_dismount;
const mainmenu_mount_t g_mainmenu_mount;
#endif
const mainmenu_debug_properties_t g_mainmenu_debug_properties;
const mainmenu_syslog_t g_mainmenu_syslog;
const mainmenu_properties_t g_mainmenu_properties;
const mainmenu_popup_ipod_devices_t g_mainmenu_popup_ipod_devices;

const mainmenu_command_t * const g_mainmenu_commands_read_part[] = {&g_mainmenu_load_library,
#ifdef PHOTO_BROWSER
&g_mainmenu_browse_photos, 
#endif //PHOTO_BROWSER
};
const mainmenu_command_t * const g_mainmenu_commands_write_part[] = {&g_mainmenu_rewrite_database_soft,&g_mainmenu_recover_orphaned_files/*,&g_mainmenu_debug_backup_itunesdb,&g_mainmenu_debug_backup_artworkdb*//*, &g_mainmenu_rewrite_database*/};
const mainmenu_command_t * const g_mainmenu_commands_write2_part[] = {&g_mainmenu_sync, &g_mainmenu_send_playlists/*, &g_mainmenu_remove_playlists*/ ,&g_mainmenu_browse, &g_mainmenu_filesystem_explorer};
const mainmenu_command_t * const g_mainmenu_commands_mount_part[] = {&g_mainmenu_eject
#ifdef TESTCOMMAND
,&g_mainmenu_mount
#endif
,/*&g_mainmenu_dismount*/};
const mainmenu_command_t * const g_mainmenu_commands_properties_part[] = {&g_mainmenu_properties,&g_mainmenu_debug_properties,&g_mainmenu_syslog};

const mainmenu_command_t * const g_mainmenu_commands_view_part[] = {&g_mainmenu_popup_ipod_devices};

class mainmenu_command_list_view_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_view_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_view_part);}
	static GUID get_parent() {return mainmenu_groups::view;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

class mainmenu_command_list_read_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_read_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_read_part);}
	static GUID get_parent() {return mainmenu_groups_dop::ipod_read_part;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

class mainmenu_command_list_write_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_write_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_write_part);}
	static GUID get_parent() {return mainmenu_groups_dop::ipod_write_part;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

class mainmenu_command_list_write2_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_write2_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_write2_part);}
	static GUID get_parent() {return mainmenu_groups_dop::ipod_write2_part;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

class mainmenu_command_list_mount_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_mount_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_mount_part);}
	static GUID get_parent() {return mainmenu_groups_dop::ipod_mount_part;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

class mainmenu_command_list_properties_part_t
{
public:
	static const mainmenu_command_t * const * const get_ptr() {return &g_mainmenu_commands_properties_part[0];}
	static t_size get_size() {return tabsize(g_mainmenu_commands_properties_part);}
	static GUID get_parent() {return mainmenu_groups_dop::ipod_properties_part;}
	static t_uint32 get_sort_priority() {return mainmenu_commands::sort_priority_dontcare;}
};

// {34ECAC4E-A484-489f-BF84-24E8C0139D18}
const GUID mainmenu_filesystem_explorer_t::g_guid = 
{ 0x34ecac4e, 0xa484, 0x489f, { 0xbf, 0x84, 0x24, 0xe8, 0xc0, 0x13, 0x9d, 0x18 } };

// {ACE49732-D788-4858-BA89-2A8764476EC3}
const GUID mainmenu_popup_ipod_devices_t::g_guid = 
{ 0xace49732, 0xd788, 0x4858, { 0xba, 0x89, 0x2a, 0x87, 0x64, 0x47, 0x6e, 0xc3 } };

// {4D1760D2-EC81-4cce-9FBA-6D95CF9A614E}
const GUID mainmenu_recover_orphaned_files_t::g_guid = 
{ 0x4d1760d2, 0xec81, 0x4cce, { 0x9f, 0xba, 0x6d, 0x95, 0xcf, 0x9a, 0x61, 0x4e } };

// {EFFFB72F-6D95-4b6a-8CC7-E16B621B0B9B}
const GUID mainmenu_load_library_t::g_guid = 
{ 0xefffb72f, 0x6d95, 0x4b6a, { 0x8c, 0xc7, 0xe1, 0x6b, 0x62, 0x1b, 0xb, 0x9b } };

// {5E736B1D-C472-482c-860B-FBF9AB497B0B}
const GUID mainmenu_rewrite_database_soft_t::g_guid = 
{ 0x5e736b1d, 0xc472, 0x482c, { 0x86, 0xb, 0xfb, 0xf9, 0xab, 0x49, 0x7b, 0xb } };

// {2AE98870-B79F-4804-A072-1557B47E0597}
const GUID mainmenu_sync_t::g_guid = 
{ 0x2ae98870, 0xb79f, 0x4804, { 0xa0, 0x72, 0x15, 0x57, 0xb4, 0x7e, 0x5, 0x97 } };

// {335F7DB7-831C-468a-B9C4-E9F7E390D973}
const GUID mainmenu_browse_t::g_guid = 
{ 0x335f7db7, 0x831c, 0x468a, { 0xb9, 0xc4, 0xe9, 0xf7, 0xe3, 0x90, 0xd9, 0x73 } };

#ifdef PHOTO_BROWSER
// {27BF8646-2CF3-4cc7-AAF9-353BA9276881}
const GUID mainmenu_browse_photos_t::g_guid = 
{ 0x27bf8646, 0x2cf3, 0x4cc7, { 0xaa, 0xf9, 0x35, 0x3b, 0xa9, 0x27, 0x68, 0x81 } };
#endif

#if 0
// {B0F8D3A4-7C14-4964-91EA-748134BCF621}
const GUID mainmenu_debug_backup_artworkdb_t::g_guid = 
{ 0xb0f8d3a4, 0x7c14, 0x4964, { 0x91, 0xea, 0x74, 0x81, 0x34, 0xbc, 0xf6, 0x21 } };

// {E4CB1FF7-ACB7-4587-911F-682E96DEFC7E}
const GUID mainmenu_debug_backup_itunesdb_t::g_guid = 
{ 0xe4cb1ff7, 0xacb7, 0x4587, { 0x91, 0x1f, 0x68, 0x2e, 0x96, 0xde, 0xfc, 0x7e } };
#endif

#ifdef TESTCOMMAND
// {CC9B5E59-29FB-4fc2-AE65-7AD842F1489C}
const GUID mainmenu_mount_t::g_guid = 
{ 0xcc9b5e59, 0x29fb, 0x4fc2, { 0xae, 0x65, 0x7a, 0xd8, 0x42, 0xf1, 0x48, 0x9c } };
#endif

#if 0
// {3762ECD6-0F35-4877-8460-C60E4BFF1953}
const GUID mainmenu_dismount_t::g_guid = 
{ 0x3762ecd6, 0xf35, 0x4877, { 0x84, 0x60, 0xc6, 0xe, 0x4b, 0xff, 0x19, 0x53 } };

#endif

// {3A542F6C-6075-4fec-9005-00E35132D49B}
const GUID mainmenu_eject_t::g_guid = 
{ 0x3a542f6c, 0x6075, 0x4fec, { 0x90, 0x5, 0x0, 0xe3, 0x51, 0x32, 0xd4, 0x9b } };

// {C56EAE37-2C58-497a-9399-89932198E66F}
const GUID mainmenu_send_playlists_t::g_guid = 
{ 0xc56eae37, 0x2c58, 0x497a, { 0x93, 0x99, 0x89, 0x93, 0x21, 0x98, 0xe6, 0x6f } };

// {61C490C2-C885-47bc-8F6A-F2856B312838}
const GUID mainmenu_properties_t::g_guid = 
{ 0x61c490c2, 0xc885, 0x47bc, { 0x8f, 0x6a, 0xf2, 0x85, 0x6b, 0x31, 0x28, 0x38 } };

// {2DDC7C59-F593-40a1-910D-1F8BB67578E0}
const GUID mainmenu_debug_properties_t::g_guid = 
{ 0x2ddc7c59, 0xf593, 0x40a1, { 0x91, 0xd, 0x1f, 0x8b, 0xb6, 0x75, 0x78, 0xe0 } };

// {201BDB07-83B4-4708-8D7C-D17CC41A18B8}
const GUID mainmenu_syslog_t::g_guid = 
{ 0x201bdb07, 0x83b4, 0x4708, { 0x8d, 0x7c, 0xd1, 0x7c, 0xc4, 0x1a, 0x18, 0xb8 } };

template <typename t_commands>
class mainmenu_commands_t : public mainmenu_commands
{

	virtual t_uint32 get_command_count()
	{
		return t_commands::get_size();
	}
	virtual GUID get_command(t_uint32 p_index)
	{
		if (p_index < t_commands::get_size())
			return t_commands::get_ptr()[p_index]->get_guid();
		return pfc::guid_null;
	}
	virtual void get_name(t_uint32 p_index,pfc::string_base & p_out)
	{
		if (p_index < t_commands::get_size())
			t_commands::get_ptr()[p_index]->get_name(p_out);
	}
	virtual bool get_description(t_uint32 p_index,pfc::string_base & p_out)
	{
		if (p_index < t_commands::get_size())
			return t_commands::get_ptr()[p_index]->get_description(p_out);
		return false;
	}
	virtual bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags)
	{
		if (p_index < t_commands::get_size())
			return t_commands::get_ptr()[p_index]->get_display(p_text, p_flags);
		return false;
	}
	virtual void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback)
	{
		if (p_index < t_commands::get_size())
			t_commands::get_ptr()[p_index]->execute(p_callback);
	}

	virtual GUID get_parent()
	{
		return t_commands::get_parent();
	}
	virtual t_uint32 get_sort_priority() 
	{
		return t_commands::get_sort_priority();
	}
};

mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_read_part_t > > g_mainmenu_commands_ipod_read;
mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_write2_part_t > > g_mainmenu_commands_ipod_write2;
mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_write_part_t > > g_mainmenu_commands_ipod_write;
mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_properties_part_t > > g_mainmenu_commands_ipod_properties;
mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_mount_part_t > > g_mainmenu_commands_ipod_mount;
mainmenu_commands_factory_t<mainmenu_commands_t<mainmenu_command_list_view_part_t > > g_mainmenu_command_view_part;
