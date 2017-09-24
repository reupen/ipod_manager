#include "main.h"

#include "actions_base.h"

volatile bool g_mobile_devices_enabled = false;

ipod_action_manager::track_action::track_action(const ipod_device_ptr_t & p_device, ipod_action_t & p_action) : m_device(p_device), m_action(p_action) {m_device->m_action_manager.register_action(&m_action);}
ipod_action_manager::track_action::~track_action() {m_device->m_action_manager.deregister_action(&m_action);}

void ipod_action_manager::suspend_all()
{
	insync (m_sync);
	for (t_size i = 0, count = m_actions.get_count(); i<count; i++)
		m_actions[i]->m_process.get_suspend().suspend();
}
void ipod_action_manager::resume_all()
{
	insync (m_sync);
	for (t_size i = 0, count = m_actions.get_count(); i<count; i++)
		m_actions[i]->m_process.get_suspend().resume();
}
void ipod_action_manager::abort_all()
{
	insync (m_sync);
	for (t_size i = 0, count = m_actions.get_count(); i<count; i++)
		m_actions[i]->m_process.get_abort_impl().abort();
}

t_drive_manager g_drive_manager;

class t_volume
{
public:
	pfc::string_simple_t<WCHAR> volume_name;
	pfc::string_simple_t<WCHAR> disk_device_id;
	pfc::string_simple_t<WCHAR> driver_symbolic_path;
	t_ipod_model model;
	bool shuffle;
	DEVINST instance;
	t_volume() {};
	t_volume(const WCHAR * vol, const WCHAR * devid, t_ipod_model & p_model, bool & p_shuffle, DEVINST inst, const WCHAR * symbpath)
		: volume_name(vol), disk_device_id(devid), model(p_model), shuffle(p_shuffle), instance(inst), driver_symbolic_path(symbpath) {};
};

int compare_wchar(unsigned c1,unsigned c2)
{
	if (c1==c2) return 0;
	c1 = uCharLower(c1);
	c2 = uCharLower(c2);
	if (c1<c2) return -1;
	else if (c1>c2) return 1;
	else return 0;
}

int wcsicmp_partial(const WCHAR * p1,const WCHAR * p2,t_size num)
{
	for(;num;)
	{
		if (*p2==0) return 0;
		int rv = compare_wchar(*p1,*p2);
		if (rv) return rv;
		p1++;p2++;
		num--;
	}
	return 0;
}

int wcsicmp_ex(const wchar_t * p1,t_size p1_bytes,const wchar_t * p2,t_size p2_bytes)
{
	p1_bytes = pfc::wcslen_max(p1,p1_bytes);
	p2_bytes = pfc::wcslen_max(p2,p2_bytes);
	for(;;)
	{
		if (p1_bytes == 0 && p2_bytes == 0) return 0;
		else if (p1_bytes == 0) return -1;
		else if (p2_bytes == 0) return 1;
		else
		{
			int rv = compare_wchar(*p1,*p2);
			if (rv) return rv;
			p1 ++;
			p2 ++;
			p1_bytes--;
			p2_bytes--;
		}
	}
}

template <typename t_char>
unsigned t_strtoul_n(const t_char * p_val, t_size p_val_length)
{
	unsigned rv = 0;
	const t_char * ptr = p_val;
	/*while (ptr - p_val < p_val_length && *ptr == '0')
	{
		ptr++;
	}*/
	while (t_size(ptr - p_val) < p_val_length && *ptr)
	{
		if (*ptr >= '0' && *ptr <='9')
		{
			rv *= 16;
			rv += *ptr - '0';
		}
		else if (*ptr >= 'a' && *ptr <= 'f')
		{
			rv *= 16;
			rv += *ptr - 'a' + 10;
		}
		else if (*ptr >= 'A' && *ptr <= 'F')
		{
			rv *= 16;
			rv += *ptr - 'A' + 10;
		}
		else break;
		ptr++;
	}
	return rv;
}

void ipod_device_t::get_capacity_information (drive_space_info_t & p_info)
	{
		if (mobile)
		{
			if (mobile_device.is_valid())
			{
				in_mobile_device_api_handle_sync lockedAPI(mobile_device->m_api);
				if (lockedAPI.is_valid())
				{
				afc_dictionary * p_dict = NULL;
				afc_error_t err =  lockedAPI->AFCDeviceInfoOpen(mobile_device->m_pafc, &p_dict);
				
				//console::formatter() << "AFCDeviceInfoOpen called, Ret: " << err;
				
				_check_afc_ret(err, "AFCDeviceInfoOpen");

				bool b_freefound = false, b_totalfound = false, b_clusterfound = false;

				char *key = NULL, *val = NULL;
				while ((lockedAPI->AFCKeyValueRead(p_dict, &key, &val) == MDERR_OK) && key && val)
				{
					if (!strcmp(key, "FSTotalBytes"))
					{
						p_info.m_capacity = mmh::strtoul64_n(val, pfc_infinite);
						b_totalfound = true;
					}
					else if (!strcmp(key, "FSFreeBytes"))
					{
						p_info.m_freespace = mmh::strtoul64_n(val, pfc_infinite);
						b_freefound = true;
					}
					else if (!strcmp(key, "FSBlockSize"))
					{
						p_info.m_blocksize = mmh::strtoul_n(val, pfc_infinite);
						b_clusterfound = true;
					}
				}
				lockedAPI->AFCKeyValueClose(p_dict);
				if (!b_totalfound)
					throw pfc::exception("AFCDeviceInfo: FSTotalBytes not found");
				if (!b_freefound)
					throw pfc::exception("AFCDeviceInfo: FSFreeBytes not found");
				if (!b_clusterfound)
					throw pfc::exception("AFCDeviceInfo: FSBlockSize not found");

#if 0
				cfobject::object_t::ptr_t disk_usage;

				CFTypeRef ref = NULL;
				ref = api->AMDeviceCopyValue(p_ipod->mobile_device->m_device, mobile_device->m_api->_CFSTR("com.apple.disk_usage"), NULL);
				g_get_CFType_object(api, ref, disk_usage);
				if (ref)
				{
					api->CFRelease(ref);
					ref = NULL;
				}

				if (disk_usage.is_valid())
				{
					t_size i, count = disk_usage->m_dictionary.get_count();
					for (i=0; i<count; i++)
					{
						if (disk_usage->m_dictionary[i].m_key.is_valid() && disk_usage->m_dictionary[i].m_value.is_valid())
						{
							if (!_wcsicmp(disk_usage->m_dictionary[i].m_key->m_string.get_ptr(), L"TotalDataCapacity"))
							{
								batteryLevel =  (disk_usage->m_dictionary[i].m_value->m_integer);
							}
						}
					}
				}
#endif
				}
			}
			else
				throw pfc::exception_bug_check();
		}
		else
		{
			pfc::array_t<WCHAR> str;
			str.append_single(drive);
			str.append_fromptr(L":\\", 3);
			ULARGE_INTEGER ufree = {0}, utotal = {0};
			GetDiskFreeSpaceEx (str.get_ptr(), &ufree, &utotal, NULL);
			p_info.m_freespace = ufree.QuadPart;
			p_info.m_capacity = utotal.QuadPart;


			DWORD sectorspercluster=NULL, bytespersector=NULL;
			GetDiskFreeSpace(str.get_ptr(), &sectorspercluster, &bytespersector, NULL, NULL);

			p_info.m_blocksize = bytespersector*sectorspercluster;
		}
	}

bool g_check_devid_is_ipod(const WCHAR * devid, t_ipod_model & model, bool & shuffle)
{
	shuffle = false;
	model = ipod_unrecognised;
	if (!wcsicmp_partial(devid, L"USB\\"))
	{
		devid += 4;
		shuffle = !wcsicmp_partial(devid, L"VID_05AC&PID_13");
		bool ipod = !wcsicmp_partial(devid, L"VID_05AC&PID_12");
		if (ipod || shuffle)
		{
			devid += 13;
			if (wcslen(devid) >=4)
			{
				unsigned pid = t_strtoul_n(devid, 4);
				if (pid == 0x1201)
					model = ipod_3g;
				else if (pid == 0x1203)
					model = ipod_4g;
				else if (pid == 0x1204)
					model = ipod_4g_colour;
				else if (pid == 0x1205)
					model = ipod_mini;
				else if (pid == 0x1209)
					model = ipod_5g;
				else if (pid == 0x120a)
					model = ipod_nano;
				else if (pid == 0x1260)
					model = ipod_nano_2g;
				else if (pid == 0x1261)
					model = ipod_6g;
				else if (pid == 0x1262)
					model = ipod_nano_3g;
				else if (pid == 0x1263)
					model = ipod_nano_4g;
				else if (pid == 0x1265)
					model = ipod_nano_5g;
				else if (pid == 0x1266)
					model = ipod_nano_6g;
				else if (pid == 0x1267)
					model = ipod_nano_7g;
				else if (pid == 0x1300)
					model = ipod_shuffle;
				else if (pid == 0x1301)
					model = ipod_shuffle_2g;
				else if (pid == 0x1302)
					model = ipod_shuffle_3g;
				else if (pid == 0x1303)
					model = ipod_shuffle_4g;
				else if (pid == 0x1291)
					model = ipod_touch;
				else if (pid == 0x1290)
					model = ipod_iphone;
				else if (pid == 0x1292)
					model = ipod_iphone_3g;
				else if (pid == 0x1293)
					model = ipod_touch_2g;
				else if (pid == 0x1294)
					model = ipod_iphone_3g_s;
				else if (pid == 0x1297)
					model = ipod_iphone_4;
				else if (pid == 0x1299)
					model = ipod_touch_3g;
				else if (pid == 0x129a)
					model = ipad;
				else if (pid == 0x129c)
					model = ipod_iphone_4_verizon;
				else if (pid == 0x129e)
					model = ipod_touch_4g;
				else if (pid == 0x129f)
					model = ipod_ipad_2_1;
				else if (pid == 0x12a2)
					model = ipod_ipad_2_2;
				else if (pid == 0x12a3)
					model = ipod_ipad_2_3;
			}
		}
		return  ipod || shuffle;
	}
	else if (!wcsicmp_partial(devid, L"1394\\"))
	{
		devid += 5;
		model = ipod_unknown_1394;
		return !wcsicmp_partial(devid, L"Apple_Computer__Inc.&iPod");
	}
	else return false;
}

class t_volumes : public pfc::list_t<t_volume>
{
public:
	bool have(const WCHAR * ptr, t_size len)
	{
		t_size dummy;
		return find(ptr, len, dummy);
	}
	bool find(const WCHAR * ptr, t_size len, t_size & index)
	{
		if (!len || !*ptr) return false;
		t_size i, count = get_count();
		for (i=0; i<count; i++)
		{
			if (!wcscmp(ptr, get_item(i).volume_name))
			{
				index = i;
				return true;
			}
		}
		return false;
	}
};

bool g_check_devid_is_mobile_device(const WCHAR * devid)
{
	if (!wcsicmp_partial(devid, L"USB\\"))
	{
		devid += 4;
		bool ipod = !wcsicmp_partial(devid, L"VID_05AC&PID_12"), mobile=false;
		if (ipod)
		{
			devid += 13;
			if (wcslen(devid) >=4)
			{
				unsigned pid = t_strtoul_n(devid, 4);
				if (pid >= 0x1290 && pid <= 0x12af)
					mobile=true;
			}
		}
		return mobile;
	}
	else return false;
}

static const GUID GUID_DEVINTERFACE_USB_HOST_CONTROLLER = {0x3ABF6F2D, 0x71C4, 0x462A, {0x8A, 0x92, 0x1E, 0x68, 0x61, 0x0E6, 0x0AF, 0x27} };
static const GUID InterfaceClassGuid = {0x0F0B32BE3, 0x6678, 0x4879, {0x92, 0x30, 0x0E4, 0x38, 0x45, 0x0D8, 0x5, 0x0EE} };

static const GUID GUID_DEVINTERFACE_USB_DEVICE =
{ 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };

static const GUID GUID_DEVINTERFACE_USBAPPL_DEVICE =
{ 0xb3146650, 0xf6c0, 0x4c73, {0xaa, 0x5f, 0xbe, 0x3a, 0x1e, 0xe7, 0xae, 0xba} };

void enum_mobile_devices(pfc::list_t<device_instance_info_t> & p_out)
{
	HDEVINFO di = SetupDiGetClassDevs(&InterfaceClassGuid, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (di != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA did;
		memset(&did, 0, sizeof(did));
		did.cbSize = sizeof(did);

		DWORD i;
		for (i=0; SetupDiEnumDeviceInfo(di, i, &did); i++)
		{
			//if (did.ClassGuid == GUID_DEVCLASS_USB)
			{
				ULONG DevDiskLen=0;
				pfc::array_t<WCHAR> DevDisk;
				if (CR_SUCCESS == CM_Get_Device_ID_Size(&DevDiskLen, did.DevInst, NULL))
				{
					DevDisk.set_size(DevDiskLen+1);
					DevDisk.fill_null();
					if (CR_SUCCESS == CM_Get_Device_ID(did.DevInst, DevDisk.get_ptr(), DevDisk.get_size(), NULL))
					{
						if (g_check_devid_is_mobile_device(DevDisk.get_ptr()))
						{
							device_instance_info_t temp;
							temp.m_handle = did.DevInst;
							temp.m_path = DevDisk.get_ptr();
							p_out.add_item(temp);

							console::formatter() << "iPod manager: USB AMD enumerator: Found " << Tu(DevDisk.get_ptr());
						}
					}
				}
			}
		}

		SetupDiDestroyDeviceInfoList(di);
	}
	if (!p_out.get_count())
		console::formatter() << "iPod manager: USB AMD enumerator: No devices found!";
}

bool get_mobile_device_instance_by_serial(const char * serial, device_instance_info_t & p_out)
{
	pfc::list_t<device_instance_info_t> instances;
	enum_mobile_devices(instances);
	t_size i, count = instances.get_count();
	for (i=0; i<count; i++)
	{
		pfc::string8 temp = pfc::stringcvt::string_utf8_from_wide(instances[i].m_path);
		t_size pos = pfc_infinite;
		if ((pos = temp.find_last('\\')) != pfc_infinite)
			if (!stricmp_utf8_max(serial, &temp[pos+1], strlen(serial)))
			{
				p_out = instances[i];
				return true;
			}
	}
	return false;
}

void build_volumes_v2(t_volumes & volumes)
{
	HDEVINFO di = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (di != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA did;
		memset(&did, 0, sizeof(did));
		did.cbSize = sizeof(did);

		DWORD i;
		for (i=0; SetupDiEnumDeviceInfo(di, i, &did); i++)
		{
			if (did.ClassGuid == GUID_DEVCLASS_DISKDRIVE)
			{
				ULONG DevLen = 0, DevDiskLen=0;
				pfc::array_t<WCHAR> Dev, DevDisk, DevRoot;
				DEVINST pParent = NULL, ppParent = NULL;
				CM_Get_Parent_Ex(&pParent, did.DevInst, NULL, NULL);
				CM_Get_Parent_Ex(&ppParent, pParent, NULL, NULL);
				CM_Get_Device_ID_Size(&DevLen, pParent, NULL);
				CM_Get_Device_ID_Size(&DevDiskLen, did.DevInst, NULL);
				Dev.set_size(DevLen+1);
				Dev.fill_null();
				DevDisk.set_size(DevDiskLen+1);
				DevDisk.fill_null();
				CM_Get_Device_ID(pParent, Dev.get_ptr(), Dev.get_size(), NULL);
				CM_Get_Device_ID(did.DevInst, DevDisk.get_ptr(), DevDisk.get_size(), NULL);

				{
					ULONG len = 0;
					CM_Get_Device_ID_Size(&len, ppParent, NULL);
					DevRoot.set_size(len+1);
					DevRoot.fill_null();
					CM_Get_Device_ID(ppParent, DevRoot.get_ptr(), len, NULL);
				}
				bool b_shuffle;
				t_ipod_model model;
				if (g_check_devid_is_ipod(Dev.get_ptr(), model, b_shuffle))
				{
					pfc::array_t<WCHAR> DriverSymbolicPath;
					if (!wcsncmp(Dev.get_ptr(), DevRoot.get_ptr(), 7))
					{
						ULONG len=0;

						CM_Get_Device_Interface_List_Size(&len, (LPGUID)&GUID_DEVINTERFACE_USBAPPL_DEVICE, DevRoot.get_ptr(), CM_GET_DEVICE_INTERFACE_LIST_PRESENT);

						DriverSymbolicPath.set_size(len+1);
						DriverSymbolicPath.fill_null();
						CM_Get_Device_Interface_List((LPGUID)&GUID_DEVINTERFACE_USBAPPL_DEVICE, DevRoot.get_ptr(), DriverSymbolicPath.get_ptr(), len, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
						//console::formatter() << pfc::stringcvt::string_utf8_from_os(buff.get_ptr());
					}
					else
					{
						DriverSymbolicPath.set_size(1);
						DriverSymbolicPath.fill_null();
					}

					{
						ULONG DevRemovalListSize = NULL, DevBusListSize = NULL;
						pfc::array_t<WCHAR> DevRemovalList, DevBusList;
						CM_Get_Device_ID_List_Size_Ex(&DevRemovalListSize, DevDisk.get_ptr(), CM_GETIDLIST_FILTER_REMOVALRELATIONS, NULL);
						CM_Get_Device_ID_List_Size_Ex(&DevBusListSize, DevDisk.get_ptr(), CM_GETIDLIST_FILTER_BUSRELATIONS, NULL);
						DevRemovalList.set_size(DevRemovalListSize);
						DevBusList.set_size(DevBusListSize);
						CM_Get_Device_ID_List_Ex(DevDisk.get_ptr(), DevRemovalList.get_ptr(), DevRemovalListSize, CM_GETIDLIST_FILTER_REMOVALRELATIONS, NULL);
						CM_Get_Device_ID_List_Ex(DevDisk.get_ptr(), DevBusList.get_ptr(), DevBusListSize, CM_GETIDLIST_FILTER_BUSRELATIONS, NULL);
						WCHAR * ptr = DevRemovalList.get_ptr(), *pvolume=NULL;
						{
							t_size ptrlen= NULL;
							while (ptr && (ptrlen = wcslen(ptr)))
							{
								if (!wcsicmp_partial(ptr, L"STORAGE\\"))
								{
									pvolume = ptr;
									break;
								}
								ptr+=ptrlen;
								ptr++;
							}
						}

						if (!pvolume)
						{
							ptr = DevBusList.get_ptr();
							t_size ptrlen= NULL;
							while (ptr && (ptrlen = wcslen(ptr)))
							{
								if (!wcsicmp_partial(ptr, L"STORAGE\\"))
								{
									pvolume = ptr;
									break;
								}
								ptr+=ptrlen;
								ptr++;
							}
						}

						if (pvolume)
						{
							SP_DEVINFO_DATA pdid;
							memset(&pdid, 0, sizeof(pdid));
							pdid.cbSize = sizeof(pdid);
							HDEVINFO pdi = SetupDiGetClassDevs(&GUID_DEVINTERFACE_VOLUME, NULL, NULL, DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);;
							SetupDiOpenDeviceInfo(pdi, pvolume, NULL, NULL, &pdid);
							{
								{
									DWORD j;
									SP_DEVICE_INTERFACE_DATA dia;
									memset(&dia, 0, sizeof(dia));
									dia.cbSize = sizeof(dia);
									for (j=0; SetupDiEnumDeviceInterfaces(pdi, &pdid, &GUID_DEVINTERFACE_VOLUME, j, &dia); j++)
									{
										DWORD required_size = 0;

										pfc::array_t<t_uint8> data;

										SetupDiGetDeviceInterfaceDetail(pdi, &dia, NULL, NULL, &required_size, &pdid);
										data.set_size(required_size);
										data.fill_null();
										if (required_size >= sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA))
										{
											SP_DEVICE_INTERFACE_DETAIL_DATA * didd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)data.get_ptr();
											didd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
											if (SetupDiGetDeviceInterfaceDetail(pdi, &dia, didd, required_size, NULL, &pdid))
											{

												pfc::array_t<WCHAR> path;
												t_size len = wcslen(didd->DevicePath);
												path.append_fromptr(didd->DevicePath, len);
												path.grow_size (len + sizeof(WCHAR)*2);
												path[len] = '\\';
												path[len+1] = 0;

												WCHAR volumename[129];
												memset(&volumename, 0, sizeof(volumename));
												if (GetVolumeNameForVolumeMountPoint(path.get_ptr(), volumename, 128))
												{
													volumes.add_item(t_volume(volumename, Dev.get_ptr(), model, b_shuffle, pParent, DriverSymbolicPath.get_ptr()));
												}
											}
										}
									}
								}
							}
							SetupDiDestroyDeviceInfoList(pdi);
						}
					}

				}
			}
		}

		SetupDiDestroyDeviceInfoList(di);
	}
}








class dop_message_window : public ui_helpers::container_window
{
	long ref_count;
public:
	dop_message_window() : m_abort(NULL) {};
	void set_abort (abort_callback & p_abort) {m_abort = &p_abort;}
	void reset_abort () {m_abort = NULL;}
	virtual class_data & get_class_data() const 
	{
		__implement_get_class_data_ex(_T("dop_message_window"), _T(""), false, 0, 0, 0, 0);
	}

	void g_scan_drives (DWORD mask)
	{
	//pfc::hires_timer timer;
	//timer.start();
	//profiler(scandrives);
#if 0
		{
			TCHAR volume[128], mount[512];
			memset(volume, 0, sizeof(volume));
			HANDLE vol = FindFirstVolume(volume, tabsize(volume)-1);
			if (vol != INVALID_HANDLE_VALUE)
			{
				do
				{
					console::formatter() << "Volume: " << pfc::stringcvt::string_utf8_from_wide(volume, 128);
					memset(mount, 0, sizeof(mount));
					HANDLE hmount = FindFirstVolumeMountPoint(volume, mount, tabsize(mount)-1);
					if (hmount != INVALID_HANDLE_VALUE)
					{
						do
						{
							console::formatter() << "mountpoint: " << pfc::stringcvt::string_utf8_from_wide(mount, tabsize(mount));
							memset(mount, 0, sizeof(mount));
						}
						while (FindNextVolumeMountPoint(hmount, mount, tabsize(mount)-1) || GetLastError() != ERROR_NO_MORE_FILES);
						FindVolumeMountPointClose(hmount);

					}
					memset(volume, 0, sizeof(volume));
				}
				while (FindNextVolume(vol, volume, tabsize(volume)-1) || GetLastError() != ERROR_NO_MORE_FILES);
				FindVolumeClose(vol);
			}
		}
#endif
		if (mask)
		{
			t_volumes volumes;
			build_volumes_v2(volumes);

			t_size i =0;
			for (i=0; i<32; i++)
			{
				if (mask & 1<<i) 
				{
					pfc::string8 drive; drive.add_byte('A'+i);
					pfc::array_t<WCHAR> path, itunesdb_path;
					path.append_single('A'+i);
					path.append_fromptr(L":\\", 3);
					{
						WCHAR volumename[129];
						memset(volumename, 0, sizeof(volumename));
						if (GetDriveType(path.get_ptr()) == DRIVE_REMOVABLE && GetVolumeNameForVolumeMountPoint(path.get_ptr(), volumename, 128))
						{
							t_size indexvol;
							//if (volumes.find(volumename, 128, indexvol))
							{
								//FIXME WTF
								if (volumes.find(volumename, 128, indexvol) /*&& g_check_devid_is_ipod(volumes[indexvol].disk_device_id)*/)
								{
									ipod_device_ptr_t temp = new ipod_device_t('A' + i, volumes[indexvol].model, volumes[indexvol].shuffle, volumes[indexvol].disk_device_id.get_ptr(), volumes[indexvol].volume_name.get_ptr(), volumes[indexvol].driver_symbolic_path.get_ptr(), volumes[indexvol].instance, device_properties_t());

									pfc::string8 pl;
									try {
										g_get_device_xml(temp, pl);
#if 0//_DEBUG
										{
											file::ptr f;
											abort_callback_dummy noabort;
											filesystem::g_open_read(f, "i:\\nano6g.plist", noabort);
											pfc::array_staticsize_t<char> d(pfc::downcast_guarded<t_uint32>(f->get_size_ex(noabort)));
											f->read(d.get_ptr(), d.get_size(), noabort);
											pl.set_string(d.get_ptr(), d.get_size());
										}
#endif
										g_get_artwork_info(pl, temp->m_device_properties);
									} 
									catch (const pfc::exception & ex) 
									{
										console::formatter() << "iPod manager: Failed to get iPod checkpoint data - " << ex.what() << ". Artwork functionality will be unavailable.";
										temp->m_device_properties.m_artwork_formats.remove_all();
									};
									//console::formatter() << "New iPod detected. Drive: " << drive << " Device Instance ID: " << pfc::stringcvt::string_utf8_from_wide(volumes[indexvol].disk_device_id);
									if (m_abort)
										g_drive_manager.add_drive(temp, *m_abort);
									else
										g_drive_manager.add_drive(temp, abort_callback_dummy());
								}
								//else
								//	console::formatter() << "New drive detected. Drive: " << drive << " Device Instance ID: " << pfc::stringcvt::string_utf8_from_wide(volumes[indexvol].disk_device_id);// << " Volume ID: " << pfc::stringcvt::string_utf8_from_wide(volumes[indexvol].volume_name);
							}
						}
						//else
						//	console::formatter() << "Drive is not expected type or GetVolumeNameForVolumeMountPoint failed. Drive: " << drive;
					}
				}

			}
		}
		//console::formatter() << "old:" << timer.query();
	}

	virtual LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_CREATE:
			g_scan_drives(GetLogicalDrives());
			if (g_mobile_devices_enabled)
				g_mobile_device_api.initialise();
			g_drive_manager.m_event_initialised.set_state(true);
			break;
		case WM_USER:
			PostQuitMessage(0);
			return 0;
		case WM_DESTROY:
			if (g_mobile_devices_enabled)
				g_mobile_device_api.deinitialise();
			g_drive_manager.set_wnd(NULL);
			break;
		case WM_DEVICECHANGE:
			if (wp == DBT_DEVICEARRIVAL)
			{
				PDEV_BROADCAST_HDR lpdb = PDEV_BROADCAST_HDR(lp);
				//console::formatter() << "DBT_DEVICEARRIVAL: " << "type: " << (t_uint32)lpdb->dbch_devicetype;
				if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
					//console::formatter() << "volume details: mask: " << pfc::format_hex(lpdbv->dbcv_unitmask, 8) << " flags: " << lpdbv->dbcv_flags;

					if (lpdbv -> dbcv_flags ==0)
					{
						g_scan_drives(lpdbv->dbcv_unitmask);
					}
				}
			}
			else if (wp == DBT_DEVICEREMOVECOMPLETE)
			{
				PDEV_BROADCAST_HDR lpdb = PDEV_BROADCAST_HDR(lp);
				//console::formatter() << "DBT_DEVICEREMOVECOMPLETE: " << "type: " << (t_uint32)lpdb->dbch_devicetype;
				if (lpdb -> dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
					//console::formatter() << "volume details: mask: " << pfc::format_hex(lpdbv->dbcv_unitmask, 8) << " flags: " << lpdbv->dbcv_flags;
					if (lpdbv -> dbcv_flags ==0 )
					{
						g_drive_manager.remove_drives(lpdbv->dbcv_unitmask);
					}
				}
			}
			break;

		}
		return DefWindowProc(wnd, msg, wp, lp);
	}
	abort_callback * m_abort;
} g_dop_message_window;

class t_thread
{
public:
	void create()
	{
		if (!m_thread)
			m_thread = CreateThread(NULL, NULL, &g_threadproc, (LPVOID)this, NULL, NULL);
	}
	void on_destroy()
	{
		if (m_thread)
		{
			m_abort.abort();
			WaitForSingleObject(m_thread,INFINITE);
			CloseHandle(m_thread);
			m_thread = NULL;
		}
	}
	void release()
	{
		if (m_thread)
		{
			CloseHandle(m_thread);
			m_thread = NULL;
		}
	}
	virtual DWORD on_thread()=0;
	HANDLE get_handle() {return m_thread;}
	t_thread() : m_thread(NULL) {};
protected:
	abort_callback_impl m_abort;
private:
	static DWORD CALLBACK g_threadproc(LPVOID lpThreadParameter)
	{
		t_thread * p_this = reinterpret_cast<t_thread*>(lpThreadParameter);
		return p_this->on_thread();
	}
	HANDLE m_thread;
};

class t_thread_impl : public t_thread
{
	virtual DWORD on_thread()
	{
		TRACK_CALL_TEXT("iPod manager device watcher thread");
		MSG msg;

		g_dop_message_window.set_abort(m_abort);
		HWND wnd_msgwnd = g_dop_message_window.create(0/*HWND_MESSAGE*/);
		g_drive_manager.set_wnd(wnd_msgwnd);
		BOOL bRet;
		
		m_event_window_created.set_state(true);

		if (wnd_msgwnd)
		{
			// Main message loop:
			while ((bRet = GetMessage(&msg, NULL, 0, 0)) !=0)
			{
				if (bRet == -1)
				{
				}
				else
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			g_dop_message_window.destroy();
		}

		g_dop_message_window.reset_abort();

		return (int) msg.wParam;
	}
public:
	win32_event m_event_window_created;
} g_message_thread;

bool g_Gdiplus_initialised = false;

volatile bool g_InitQuit_Initialised = false, g_InitQuit_Deinitialised = false;
class initquit_impl : public initquit
{
public:
	virtual void on_init() 
	{
		if (settings::encoder_imported == false)
		{
			if (!settings::conversion_command.is_empty())
			{
				settings::active_encoder = settings::encoder_list.add_item(settings::conversion_preset_t("Imported settings", settings::conversion_command, settings::conversion_parameters, settings::conversion_extension, settings::conversion_preset_t::bps_16, true));
				g_sort_converstion_encoders();
			}
			settings::encoder_imported = true;
		}
		{
			g_mobile_devices_enabled = settings::mobile_devices_enabled;
		}

		g_Gdiplus_initialised =
			Gdiplus::Ok == Gdiplus::GdiplusStartup(&m_Gdiplus_token, &Gdiplus::GdiplusStartupInput(), NULL);
		g_message_thread.m_event_window_created.create(true, false);
		g_message_thread.create();
		g_InitQuit_Initialised = true;
	}
	virtual void on_quit() 
	{
		g_InitQuit_Deinitialised = true;
		if (g_message_thread.get_handle() && g_message_thread.m_event_window_created.is_valid())
			g_message_thread.m_event_window_created.wait_for(-1);
		PostMessage(g_drive_manager.get_wnd(), WM_USER, 0, 0);
		g_message_thread.on_destroy();
		g_drive_manager.m_event_initialised.release();
		g_drive_manager.remove_all_drives();
		if (g_Gdiplus_initialised)
			Gdiplus::GdiplusShutdown(m_Gdiplus_token);
	}
	ULONG_PTR m_Gdiplus_token;

	initquit_impl() : m_Gdiplus_token(NULL) {};
};

initquit_factory_t<initquit_impl> g_initquit;

