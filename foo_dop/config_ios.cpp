#include "stdafx.h"

#include "config_ios.h"
#include "resource.h"

#if 0
class directory_callback_full_impl : public directory_callback {
public:
	struct t_entry {
		pfc::string_simple m_path;
		t_filestats m_stats;
		bool m_subdir;
		t_entry(const char * p_path, bool b_subdir, const t_filestats & p_stats) : m_path(p_path), m_subdir(b_subdir), m_stats(p_stats) {}
	};
	pfc::list_t<pfc::rcptr_t<t_entry> > m_data;
	bool m_recur;

	static int sortfunc(const pfc::rcptr_t<const t_entry> & p1, const pfc::rcptr_t<const t_entry> & p2) { return stricmp_utf8(p1->m_path, p2->m_path); }

	bool on_entry(filesystem * owner, abort_callback & p_abort, const char * url, bool is_subdirectory, const t_filestats & p_stats);

	directory_callback_full_impl(bool p_recur) : m_recur(p_recur) {}
	t_size get_count() { return m_data.get_count(); }
	const t_entry & operator[](t_size n) const { return *m_data[n]; }
	const t_entry & get_item(t_size n) const { return *m_data[n]; }
	const t_filestats & get_item_stats(t_size n) const { return m_data[n]->m_stats; }
	void sort() { m_data.sort_t(sortfunc); }
};

bool directory_callback_full_impl::on_entry(filesystem * owner, abort_callback & p_abort, const char * url, bool is_subdirectory, const t_filestats & p_stats)
{
	p_abort.check_e();
	m_data.add_item(pfc::rcnew_t<t_entry>(url, is_subdirectory, p_stats));
	if (is_subdirectory)
	{
		if (m_recur)
		{
			try
			{
				owner->list_directory(url, *this, p_abort);
			}
			catch (exception_io const &) {}
		}
	}
	return true;
}
#endif


#if 0//_DEBUG
void g_print_CFTypeRef(mobile_device_api_t::ptr & api, CFTypeRef ref, pfc::string8 & p_out)
{
	if (ref)
	{
		CFTypeID type = api->CFGetTypeID(ref);
		if (type == api->CFStringGetTypeID())
		{
			CFStringRef str = (CFStringRef)ref;
			t_size len = api->CFStringGetLength(str);
			pfc::array_t<WCHAR> wstr;
			wstr.fill_null();
			wstr.set_size(len + 1);
			api->CFStringGetCharacters(str, CFRangeMake(0, len), (unsigned short *)wstr.get_ptr());
			wstr[len] = 0; //Bubble wrap
			p_out << "\"" << pfc::stringcvt::string_utf8_from_wide(wstr.get_ptr(), len) << "\"";
		}
		else if (type == api->CFNumberGetTypeID())
		{
			if (api->CFNumberIsFloatType((CFNumberRef)ref))
			{
				double buff = NULL;
				api->CFNumberGetValue((CFNumberRef)ref, kCFNumberDoubleType, &buff);
				p_out << buff;
			}
			else
			{
				long long buff = NULL;
				api->CFNumberGetValue((CFNumberRef)ref, kCFNumberLongLongType, &buff);
				p_out << buff;
			}
		}
		else if (type == api->CFBooleanGetTypeID())
		{
			p_out << api->CFBooleanGetValue((CFBooleanRef)ref) ? "True" : "False";
		}
		else if (type == api->CFDictionaryGetTypeID())
		{
			CFDictionaryRef dict = (CFDictionaryRef)ref;
			t_size i, count = api->CFDictionaryGetCount(dict);
			pfc::array_t<CFTypeRef> keys, values;
			keys.set_size(count);
			values.set_size(count);
			api->CFDictionaryGetKeysAndValues(dict, keys.get_ptr(), values.get_ptr());
			p_out << "{\r\n";
			for (i = 0; i<count; i++)
			{
				g_print_CFTypeRef(api, keys[i], p_out);
				p_out << ": ";
				g_print_CFTypeRef(api, values[i], p_out);
				p_out << "\r\n";
			}
			p_out << "}\r\n";
		}
		else if (type == api->CFArrayGetTypeID())
		{
			CFArrayRef arr = (CFArrayRef)ref;
			t_size i, count = api->CFArrayGetCount(arr);
			p_out << "{\r\n";
			for (i = 0; i<count; i++)
			{
				CFTypeRef child = api->CFArrayGetValueAtIndex(arr, i);
				g_print_CFTypeRef(api, child, p_out);
				p_out << "\r\n";
			}
			p_out << "}\r\n";
		}
		else if (type == api->CFDataGetTypeID())
		{
			CFDataRef data = (CFDataRef)ref;
			t_size len = api->CFDataGetLength(data);

			pfc::array_t <t_uint8> bytes;
			bytes.set_size(len);
			api->CFDataGetBytes(data, CFRangeMake(0, len), bytes.get_ptr());

			p_out << "{\r\n" << pfc::format_hexdump(bytes.get_ptr(), len) << "\r\n}\r\n";
		}
		else p_out << "<unsupported data type>";
	}
}
#endif

BOOL t_config_tab3::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialising = true;
			m_wnd_enabled = GetDlgItem(wnd, IDC_ENABLED);
			Button_SetCheck(m_wnd_enabled, settings::mobile_devices_enabled ? BST_CHECKED : BST_UNCHECKED);
			m_initialising = false;
		}
		break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDC_ENABLED | (BN_CLICKED << 16) :
					settings::mobile_devices_enabled = Button_GetCheck(m_wnd_enabled) == BST_CHECKED;
					break;
#if 0
				case IDC_DIR:
				{
					string_utf8_from_window dir(wnd, IDC_PATH);
					pfc::string8 dir_can, output;
					try
					{
						filesystem::g_get_canonical_path(dir, dir_can);
						directory_callback_full_impl list(false);
						filesystem::g_list_directory(dir_can, list, abort_callback_dummy());
						t_size i, count = list.get_count();
						for (i = 0; i<count; i++)
						{
							if (list[i].m_subdir)
								output << "<dir> ";
							output << list[i].m_path;
							//output << "     " << list[i].m_stats.m_size;
							//if (list[i].m_stats.m_timestamp != filetimestamp_invalid)
							//	output << "     " << format_filetimestamp(list[i].m_stats.m_timestamp);
							output << "\r\n";
						}
					}
					catch (const pfc::exception & ex)
					{
						output << "\r\nError: " << ex.what();
					}
					uSendDlgItemMessageText(wnd, IDC_OUTPUT, WM_SETTEXT, NULL, output.get_ptr());
				}
				break;
				case IDC_STATS:
				{
					pfc::string8 output;
					try
					{
						pfc::list_t<stats_t> stats;
						g_get_stats_list(string_utf8_from_window(wnd, IDC_PATH), stats);
						t_size i, count = stats.get_count();
						for (i = 0; i<count; i++)
							output << stats[i].name << "=" << stats[i].value << "\r\n";
					}
					catch (const pfc::exception & ex)
					{
						output << "\r\nError: " << ex.what();
					}
					uSendDlgItemMessageText(wnd, IDC_OUTPUT, WM_SETTEXT, NULL, output.get_ptr());
				}
				break;
				case IDC_TEST:
				{
					//console::formatter() << pfc::format_hex(IOCTL_SCSI_PASS_THROUGH);
					mobile_device_api_t::ptr api = g_get_mobile_device_api();
					mobile_device_api_t::device_handle_t::ptr handle;
					if (api.is_valid() && api->is_initialised())
					{
						{
							insync(api->m_devices_sync);
							if (api->m_devices.get_count())
								handle = api->m_devices[0];
						}
						if (handle.is_valid())
						{
							try
							{
								pfc::string8 msg;
								CFTypeRef ref = NULL;
								msg << "\r\n1:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, api->_CFSTR("com.apple.mobile.iTunes"), NULL);
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n2:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, NULL, NULL);
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n3:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, api->_CFSTR("com.apple.mobile.battery"), NULL);
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n4:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, api->_CFSTR("com.apple.disk_usage"), NULL);
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n5:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, api->_CFSTR("com.apple.mobile.sync_data_class"), NULL);
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n6:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, NULL, api->_CFSTR("DeviceClass"));
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								msg << "\r\n7:\r\n";
								ref = api->AMDeviceCopyValue(api->m_devices[0]->m_device, NULL, api->_CFSTR("DeviceName"));
								msg << "Ref: " << pfc::format_hex((t_uint64)ref, 8) << "\r\n";
								if (ref)
								{
									g_print_CFTypeRef(api, ref, msg);
									api->CFRelease(ref);
									ref = NULL;
								}
								popup_message::g_show(msg, "Test Results");
								//NULL
								//com.apple.mobile.iTunes
								//com.apple.mobile.battery
								//com.apple.disk_usage
								//com.apple.mobile.sync_data_class
								//DeviceClass
								//DeviceName
							}
							catch (const pfc::exception & ex)
							{
								popup_message::g_show(ex.what(), "Test Error");
							}
						}
					}
				}
				break;
#endif
			}
			break;
		case WM_DESTROY:
			m_wnd_enabled = NULL;
			break;
	}
	return FALSE;
}

BOOL t_config_tab3::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	t_config_tab3 * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_tab3*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_tab3*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);
	return FALSE;
}
