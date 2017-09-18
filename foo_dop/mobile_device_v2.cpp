#include "main.h"

namespace library_path_helpers
{
	namespace folders {
		void get_path_by_registry_value(const wchar_t * p_key, const wchar_t * p_value, pfc::array_t<WCHAR> & p_out, bool b_dir = true)
		{
			HKEY key = NULL;
			DWORD err = RegOpenKeyEx (HKEY_LOCAL_MACHINE, p_key, NULL, KEY_READ, &key);
			if (err !=ERROR_SUCCESS)
				throw exception_win32(err);
			pfc::array_t<WCHAR> buffer;
			buffer.set_size(MAX_PATH + 1);
			buffer.fill_null();
			DWORD size = buffer.get_size() * sizeof(WCHAR);
			DWORD type = NULL;
			err = RegQueryValueEx (key, p_value, NULL, &type,  (LPBYTE)buffer.get_ptr(), &size);
			RegCloseKey (key);
			if (err !=ERROR_SUCCESS)
				throw exception_win32(err);
			if (type != REG_SZ)
				throw pfc::exception("Expected REG_SZ value type");

			t_size wstrlen = wcsnlen(buffer.get_ptr(), size);
			p_out.append_fromptr(buffer.get_ptr(), wstrlen );

			if (b_dir)
			{
				if (wstrlen && p_out[wstrlen-1] != '\\')
					p_out.append_single('\\');
			}
		}
		void get_appsupport_path(pfc::array_t<WCHAR> & p_out)
		{
			get_path_by_registry_value(L"SOFTWARE\\Apple Inc.\\Apple Application Support", L"InstallDir", p_out);
		}
		void get_mobiledevicesupport_path(pfc::array_t<WCHAR> & p_out)
		{
			get_path_by_registry_value(L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support", L"InstallDir", p_out);
		}
		void get_itunesmobiledevice_path(pfc::array_t<WCHAR> & p_out)
		{
			get_path_by_registry_value(L"SOFTWARE\\Apple Inc.\\Apple Mobile Device Support\\Shared", L"iTunesMobileDeviceDLL", p_out, false);
		}
		void get_qtsystem_path(pfc::array_t<WCHAR> & p_out)
		{
			get_path_by_registry_value(L"SOFTWARE\\Apple Computer, Inc.\\Quicktime", L"QTSysDir", p_out);
#if 0
			HKEY key = NULL;
			DWORD err = RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"SOFTWARE\\Apple Computer, Inc.\\Quicktime", NULL, KEY_READ, &key);
			if (err !=ERROR_SUCCESS)
				throw exception_win32(err);
			pfc::array_t<WCHAR> buffer;
			buffer.set_size(MAX_PATH + 1);
			buffer.fill_null();
			DWORD size = buffer.get_size() * sizeof(WCHAR);
			DWORD type = NULL;
			err = RegQueryValueEx (key, L"QTSysDir", NULL, &type,  (LPBYTE)buffer.get_ptr(), &size);
			RegCloseKey (key);
			if (err !=ERROR_SUCCESS)
				throw exception_win32(err);
			if (type != REG_SZ)
				throw pfc::exception("Expected REG_SZ value type");

			t_size wstrlen = wcsnlen(buffer.get_ptr(), size);
			p_out.append_fromptr(buffer.get_ptr(), wstrlen );
			if (wstrlen && p_out[wstrlen-1] != '\\')
				p_out.append_single('\\');
#endif
		}
		void get_commonfiles_path(pfc::array_t<WCHAR> & p_out)
		{
			t_size commonsize = GetEnvironmentVariable (L"CommonProgramFiles", NULL, NULL);
			p_out.set_size(commonsize);
			p_out.fill_null();
			GetEnvironmentVariable (L"CommonProgramFiles", p_out.get_ptr(), p_out.get_size());
			t_size wstrlen = wcsnlen(p_out.get_ptr(), commonsize);
			p_out.set_size(wstrlen);
			if (wstrlen && p_out[wstrlen-1] != '\\')
				p_out.append_single('\\');
		}
	}
	namespace libraries {
		void get_qtmlclient_path(pfc::array_t<wchar_t> & p_out)
		{
			const wchar_t * p_qtmlclient = L"QTMLClient.dll";
			folders::get_qtsystem_path(p_out);
			p_out.append_fromptr(p_qtmlclient, wcslen(p_qtmlclient)+1);
		}
		void get_corefoundation_path(pfc::array_t<wchar_t> & p_corefoundation, pfc::array_t<wchar_t> & p_cfnetwork, pfc::array_t<wchar_t> & p_icuin)
		{
			const wchar_t * p_dll = L"CoreFoundation.dll", * p_cfnetworkstr = L"CFNetwork.dll", *p_icuinstr=L"icuin40.dll";
			folders::get_appsupport_path(p_corefoundation);
			p_cfnetwork.append_fromptr(p_corefoundation.get_ptr(), p_corefoundation.get_size());
			p_icuin.append_fromptr(p_corefoundation.get_ptr(), p_corefoundation.get_size());

			p_corefoundation.append_fromptr(p_dll, wcslen(p_dll)+1);
			p_cfnetwork.append_fromptr(p_cfnetworkstr, wcslen(p_cfnetworkstr)+1);
			p_icuin.append_fromptr(p_icuinstr, wcslen(p_icuinstr)+1);
		}
		void get_itunesmobiledevice_path(pfc::array_t<wchar_t> & p_out)
		{
			folders::get_itunesmobiledevice_path(p_out);
			p_out.append_single(0);
		}
	}
}


#define MDGPA2(x, y) m_functions.##x = (p_##x##_t)GetProcAddress(y, #x)
#define SafeMDGPA2(x, y) if (!(MDGPA2(x,y))) throw pfc::exception(pfc::string8() << "Failed to locate function " << #x)


mobile_device_handle::~mobile_device_handle()
{
	deinitialise();
}

mobile_device_handle::mobile_device_handle()
: m_pafc(NULL), m_device(NULL), m_afc_service(INVALID_SOCKET), m_notification_proxy(nullptr),
m_send_notification_proxy(nullptr), m_sync_lock(NULL), m_sync_lock_locked(false)
{};

int memcmp_sized(const void * p1, t_size p1_size, const void * p2, t_size p2_size)
{
	int ret = memcmp(p1, p2, min(p1_size, p2_size));
	if (ret == 0) ret = pfc::compare_t(p1_size, p2_size);
	return ret;
}

template <typename T1, typename T2>
int memcmp_sized(const T1 & p1, const T2 & p2)
{
	return memcmp_sized(p1.get_ptr(), pfc::array_size_t(p1), p2.get_ptr(), pfc::array_size_t(p2));
}

void mobile_device_handle::sync_get_iPhoneSortKey(const wchar_t * p_string, t_size string_length, pfc::array_t<t_uint8> & p_out)
{
	if (m_icu_context.m_coll == NULL) throw pfc::exception("ICU error");
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	if (lockedAPI.is_valid())
	{
		icu_sort_key p_sort_key;
		if (m_icu_context.m_trans)
		{
			pfc::array_t<wchar_t> transstr;
			lockedAPI.icu_trans_chars(m_icu_context.m_trans, p_string, string_length, transstr);
			lockedAPI.icu_get_sort_key(m_icu_context.m_coll, transstr.get_ptr(), transstr.get_size(), p_sort_key);
		}
		else
			lockedAPI.icu_get_sort_key(m_icu_context.m_coll, p_string, string_length, p_sort_key);
		t_size i = 0, count = m_icu_context.m_SortSections.get_count();
		for (; i<count; i++)
		{
			t_size SectionHeaderCount = m_icu_context.m_SortSections[i].m_HeaderSortKeys.get_count();
			if (SectionHeaderCount 
				&& memcmp_sized(m_icu_context.m_SortSections[i].m_HeaderSortKeys[0], p_sort_key) <= 0
				&& memcmp_sized(m_icu_context.m_SortSections[i].m_FirstCharacterAfterLanguageSortKey, p_sort_key) >= 0) 
				break;
		}
		p_out.append_single(0x30 + i);
		p_out.append(p_sort_key);
	}
}

t_int64 mobile_device_handle::sync_get_iPhoneSortSection(const t_uint8 * SortKey, t_size SortKeyLen)
{
	t_size counter = 0;
	if (m_icu_context.m_coll == NULL) throw pfc::exception("ICU error");
	if (SortKeyLen < 2) throw pfc::exception_invalid_params();
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	if (lockedAPI.is_valid())
	{
		UErrorCode status = 0;
		t_size languageIndex = *SortKey - 0x30;
		t_size boundedlength = lockedAPI->ucol_getBound_4_0(SortKey+1, SortKeyLen-1, 0, 1, 0, 0, &status);
		if (U_FAILURE(status)) throw pfc::exception("ICU error");
		t_size SortSectionCount = m_icu_context.m_SortSections.get_count();
		counter = m_icu_context.m_HeaderCount;
		if (languageIndex < SortSectionCount)
		{
			t_size SectionHeaderCount = m_icu_context.m_SortSections[languageIndex].m_HeaderSortKeys.get_count();
			for (t_size j = 0; j<SectionHeaderCount; j++)
			{
				if (memcmp_sized(m_icu_context.m_SortSections[languageIndex].m_HeaderSortKeys[j].get_ptr(), m_icu_context.m_SortSections[languageIndex].m_HeaderSortKeys[j].get_size(), SortKey+1, boundedlength) <= 0)
					counter = j + m_icu_context.m_SortSections[languageIndex].m_HeaderBase;
				else break;
			}
			//if (counter) --counter;	
		}
	}
	return counter;
}

void mobile_device_handle::initialise_international()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();

	cfobject::object_t::ptr_t International_Locale, International_SortSections, International_SectionHeaders, International_NameTransform;
	connect_lockdown();

	const char * defaultSortSections = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"><plist version=\"1.0\"><dict><key>SectionHeaders</key><array><dict><key>LastCharacter</key><string>Z</string><key>Headers</key><array><string>A</string><string>B</string><string>C</string><string>D</string><string>E</string><string>F</string><string>G</string><string>H</string><string>I</string><string>J</string><string>K</string><string>L</string><string>M</string><string>N</string><string>O</string><string>P</string><string>Q</string><string>R</string><string>S</string><string>T</string><string>U</string><string>V</string><string>W</string><string>X</string><string>Y</string><string>Z</string></array><key>FirstCharacterAfterLanguage</key><string>ʒ</string></dict></array><key>SectionIndices</key><array><string>A</string><string>B</string><string>C</string><string>D</string><string>E</string><string>F</string><string>G</string><string>H</string><string>I</string><string>J</string><string>K</string><string>L</string><string>M</string><string>N</string><string>O</string><string>P</string><string>Q</string><string>R</string><string>S</string><string>T</string><string>U</string><string>V</string><string>W</string><string>X</string><string>Y</string><string>Z</string></array></dict></plist>";

	if (!lockdown_copy_value("com.apple.international", "SortSections", International_SortSections))
	{
		try 
		{
			PlistParser plist(defaultSortSections, strlen(defaultSortSections));
			International_SortSections = plist.m_root_object;
		} 
		catch (pfc::exception const &) 
		{
		};
	}

	bool b_continue = lockdown_copy_value("com.apple.international", "Locale", International_Locale) 
		&& International_SortSections.is_valid() //lockdown_copy_value("com.apple.international", "SortSections", International_SortSections)
		&& International_SortSections->m_dictionary.get_child(L"SectionHeaders", International_SectionHeaders);
	disconnect_lockdown();

	if (b_continue)
	{
		International_SortSections->m_dictionary.get_child(L"NameTransform", International_NameTransform);

		UErrorCode status = 0;
		m_icu_context.m_coll = lockedAPI->ucol_open_4_0(pfc::stringcvt::string_utf8_from_wide(International_Locale->m_string.get_ptr()),&status);

		if (U_SUCCESS(status))
		{
			if (International_NameTransform.is_valid() && !International_NameTransform->m_string.is_empty())
				m_icu_context.m_trans = lockedAPI->utrans_openU_4_0(International_NameTransform->m_string, -1, 0, 0, 0, 0, &status);

			lockedAPI->ucol_setAttribute_4_0(m_icu_context.m_coll, 7, 17, &status);
			t_size section_count = International_SectionHeaders->m_array.get_count();
			m_icu_context.m_SortSections.set_count(section_count);
			t_size rollingCount = 0;
			for (t_size i = 0; i<section_count; i++)
			{
				cfobject::object_t::ptr_t Headers, FirstCharacterAfterLanguage;
				if (International_SectionHeaders->m_array[i].is_valid() 
					&& International_SectionHeaders->m_array[i]->m_dictionary.get_child(L"Headers", Headers) 
					&& International_SectionHeaders->m_array[i]->m_dictionary.get_child(L"FirstCharacterAfterLanguage", FirstCharacterAfterLanguage))
				{
					lockedAPI.icu_get_sort_key_bound(m_icu_context.m_coll, FirstCharacterAfterLanguage->m_string.get_ptr(), -1, true, m_icu_context.m_SortSections[i].m_FirstCharacterAfterLanguageSortKey);
					t_size HeaderCount = Headers->m_array.get_count();
					m_icu_context.m_SortSections[i].m_HeaderSortKeys.set_size(HeaderCount);
					for (t_size j=0; j<HeaderCount; j++)
					{
						lockedAPI.icu_get_sort_key_bound(m_icu_context.m_coll, Headers->m_array[j]->m_string.get_ptr(), -1, true, m_icu_context.m_SortSections[i].m_HeaderSortKeys[j]);
					}
					m_icu_context.m_SortSections[i].m_HeaderBase = rollingCount;
					rollingCount += HeaderCount;
				}
			}
			m_icu_context.m_HeaderCount = rollingCount;
		}
		//icu_get_sort_key_bound(coll, &last_char, 1, true, last_char_key);
		//m_functions.ucol_close_4_0(coll); 
	}
}

void mobile_device_handle::deinitialise_international()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();

	if (m_icu_context.m_trans)
	{
		lockedAPI->utrans_close_4_0(m_icu_context.m_trans);
		m_icu_context.m_trans = NULL;
	}
	if (m_icu_context.m_coll)
	{
		lockedAPI->ucol_close_4_0(m_icu_context.m_coll);
		m_icu_context.m_coll = NULL;
	}
	
	m_icu_context.m_HeaderCount = 0;
	m_icu_context.m_SortSections.set_size(0);
}

void mobile_device_handle::do_before_sync()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();

	try 
	{
		post_notification(lockedAPI->__CFStringMakeConstantString("com.apple.itunes-mobdev.syncWillStart"), NULL);
		Sleep(20);
		int amret = MDERR_OK;
		amret = lockedAPI->AFCFileRefOpen(m_pafc, "/com.apple.itunes.lock_sync", 2, &m_sync_lock);
		if (MDERR_OK != amret)
		{
			m_sync_lock = NULL;
			_check_afc_ret(amret, "AFCFileRefOpen", "/com.apple.itunes.lock_sync");
			//throw exception_io(pfc::string8() << "Error opening lock file - AFCFileRefOpen returned: " << amret);
		}

		t_size attempts = 100;
		while (attempts)
		{
			amret = lockedAPI->AFCFileRefLock(m_pafc, m_sync_lock, true);
			if (MDERR_OK == amret)
			{
				m_sync_lock_locked = true;
				break;
			}
			else if (amret != 19)
			{
				_check_afc_ret(amret, "AFCFileRefLock");
			}
			//else
			//	console::formatter() << "iPod manager: Apple Mobile Device: Warning - could not lock device. Retrying in 66ms...";
			post_notification(lockedAPI->__CFStringMakeConstantString("com.apple.itunes-mobdev.syncLockRequest"), NULL);
			Sleep(66);
			attempts--;
		}
		if (!m_sync_lock_locked)
		{
			throw pfc::exception("Could not lock device for syncing! Please wait and retry.");
		}

		initialise_international();

		post_notification(lockedAPI->__CFStringMakeConstantString("com.apple.itunes-mobdev.syncDidStart"), NULL);

		afc_file_ref file = NULL;
		if (MDERR_OK == lockedAPI->AFCFileRefOpen(m_pafc, "/com.apple.itunes.syncing", 3, &file))
			lockedAPI->AFCFileRefClose(m_pafc, file);
	}
	catch (pfc::exception const &)
	{
		if (m_sync_lock)
		{
			lockedAPI->AFCFileRefClose(m_pafc, m_sync_lock);
			m_sync_lock = NULL;
		}

		try {post_notification(lockedAPI->__CFStringMakeConstantString("com.apple.itunes-mobdev.syncFailedToStart"), NULL);} catch (pfc::exception const &) {};
		try {deinitialise_international();} catch (pfc::exception const &) {};

		throw;
	}
}
void mobile_device_handle::do_after_sync()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();

	lockedAPI->AFCRemovePath(m_pafc, "/com.apple.itunes.syncing");
	if (m_sync_lock)
	{
		if (m_sync_lock_locked)
			lockedAPI->AFCFileRefUnlock(m_pafc, m_sync_lock);
		m_sync_lock_locked=false;
		lockedAPI->AFCFileRefClose(m_pafc, m_sync_lock);
	}
	m_sync_lock= NULL;
	post_notification(lockedAPI->__CFStringMakeConstantString("com.apple.itunes-mobdev.syncDidFinish"), NULL);

	deinitialise_international();
}

void mobile_device_handle::post_notification(CFStringRef notification, CFStringRef userinfo)
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();
	{
		if (m_send_notification_proxy == nullptr)
			throw pfc::exception("Invalid service connection");
		mach_error_t err = lockedAPI->AMDSecurePostNotification(m_send_notification_proxy, notification, userinfo);
		if (err != MDERR_OK)
			throw pfc::exception(pfc::string8() << "error: AMDSecurePostNotification returned " << pfc::format_hex(err,8));
	}
}

void mobile_device_handle::connect_lockdown()
{
	mach_error_t amret = MDERR_OK;
	bool b_dead = false;

	in_mobile_device_api_handle_sync lockedAPI(m_api);
	lockedAPI.ensure_valid();

	{
	try 
	{
		amret = lockedAPI->AMDeviceConnect(m_device);
		if (amret != MDERR_OK)
			throw pfc::exception(pfc::string8() << "Apple Mobile Device: Could not connect! Error " << pfc::format_hex(amret, 8));

		b_dead = true;
		bool b_need_pairing = false;

		int ispaired = lockedAPI->AMDeviceIsPaired(m_device);

		if (!ispaired)
		{
			console::formatter() << "iPod manager: Apple Mobile Device: Mobile Device not paired. Trying to pair device...";
			b_need_pairing = true;
		}

		if (ispaired)
		{
			amret = lockedAPI->AMDeviceValidatePairing(m_device);
			if (amret != MDERR_OK)
			{
				console::formatter() << "iPod manager: Apple Mobile Device: Failed to validate Mobile Device pairing! Error: " << pfc::format_hex(amret, 8) << ". Trying to re-pair device...";
				b_need_pairing=true;
			}
		}

		if (b_need_pairing)
		{
			amret = lockedAPI->AMDevicePair(m_device);
			if (amret != MDERR_OK)
			{
				if (amret == AMDError::kAMDPasswordProtectedError)
					throw pfc::exception("Failed to pair device - device is passcode locked. Unlock the device and retry.");
				else
					throw pfc::exception(pfc::string8() << "Apple Mobile Device: Failed to pair device. Error " << pfc::format_hex(amret, 8));
			}
		}

		amret = lockedAPI->AMDeviceStartSession(m_device);
		if (amret != MDERR_OK)
		{

			if ( (amret == AMDError::kAMDMissingPairRecordError || amret == AMDError::kAMDInvalidPairRecordError) )
			{
				console::formatter() << "iPod manager: Apple Mobile Device: Trying to pair the device...";
				amret = lockedAPI->AMDevicePair(m_device);
				if (amret != MDERR_OK)
				{
					throw pfc::exception(pfc::string8() << "Apple Mobile Device: Failed to pair device. Error " << pfc::format_hex(amret, 8));
				}
				else
				{
					amret = lockedAPI->AMDeviceStartSession(m_device);
					if (amret != MDERR_OK)
						throw pfc::exception(pfc::string8() << "Apple Mobile Device: error: AMDeviceStartSession returned " << pfc::format_hex(amret, 8));
				}
			}
			else throw pfc::exception(pfc::string8() << "Apple Mobile Device: error: AMDeviceStartSession returned " << pfc::format_hex(amret, 8));

		}
	} 
	catch (pfc::exception const &)
	{
		if (b_dead)
		{
			amret = lockedAPI->AMDeviceDisconnect(m_device);
			if (amret != MDERR_OK)
				console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceDisconnect returned " << pfc::format_hex(amret, 8);
		}
		throw;
	}
	}

}


void mobile_device_handle::disconnect_lockdown()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	if (lockedAPI.is_valid())
	{
		mach_error_t amret = lockedAPI->AMDeviceStopSession(m_device);
		if (amret != MDERR_OK && !(amret == AMDError::kAMDNotConnectedError || amret == AMDError::kAMDSessionActiveError || amret == AMDError::kAMDSessionInactiveError))
			console::formatter() << "iPod manager: Error: AMDeviceStopSession returned " << pfc::format_hex(amret, 8);
		amret = lockedAPI->AMDeviceDisconnect(m_device);
		if (amret != MDERR_OK && !(amret == AMDError::kAMDNotConnectedError))
			console::formatter() << "iPod manager: Error: AMDeviceDisconnect returned " << pfc::format_hex(amret, 8);
	}
}

bool mobile_device_handle::lockdown_copy_value(const char * domain, const char * subdomain, cfobject::object_t::ptr_t & ptr)
{
	ptr.release();
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	if (lockedAPI.is_valid())
	{
		CFTypeRef ref = NULL;
		ref = lockedAPI->AMDeviceCopyValue(m_device, lockedAPI->__CFStringMakeConstantString(domain), lockedAPI->__CFStringMakeConstantString(subdomain));
		g_get_CFType_object(lockedAPI, ref, ptr);
		if (ref)
		{
			lockedAPI->CFRelease(ref);
		}
	}
	return ptr.is_valid();
}

void mobile_device_handle::deinitialise()
{
	in_mobile_device_api_handle_sync lockedAPI(m_api);
	if (lockedAPI.is_valid())
	{
		if (m_pafc)
		{
			afc_error_t afret = lockedAPI->AFCConnectionClose(m_pafc);
			if (afret != MDERR_OK)
				console::formatter() << "iPod manager: Error: AFCConnectionClose returned " << pfc::format_hex(afret,8);
			m_pafc = NULL;
			m_afc_service = INVALID_SOCKET;
		}
		if (m_notification_proxy != nullptr)
		{
			mach_error_t amret = lockedAPI->AMDSecureShutdownNotificationProxy(m_notification_proxy);
			if (amret != MDERR_OK)
				console::formatter() << "iPod manager: Error: AMDSecureShutdownNotificationProxy returned " << pfc::format_hex(amret,8);
			Sleep(50); //Stoopid Apple library, try and let its thread terminate (!!)
			lockedAPI->CFRelease(m_notification_proxy);
			m_notification_proxy = nullptr;
		}
		if (m_send_notification_proxy != nullptr)
		{
			mach_error_t amret = lockedAPI->AMDSecureShutdownNotificationProxy(m_send_notification_proxy);
			if (amret != MDERR_OK)
				console::formatter() << "iPod manager: Error: AMDSecureShutdownNotificationProxy returned " << pfc::format_hex(amret,8);
			lockedAPI->CFRelease(m_send_notification_proxy);
			m_send_notification_proxy = nullptr;
		}
		m_syslog_relay.deinitialise();
		if (m_device)
		{
			lockedAPI->AMDeviceRelease(m_device);
			m_device = NULL;
		}
	}
	m_api.release();
}

void mobile_device_list::remove_by_am_device(const am_device * p_amd)
{
	t_size i = get_count();
	for (; i; i--)
	{
		if (get_item(i-1)->m_device == p_amd)
			remove_by_idx(i-1);
	}
}
bool mobile_device_list::find_by_am_device(const am_device * p_amd, t_size & index)
{
	t_size i = get_count();
	for (; i; i--)
	{
		if (get_item(i-1)->m_device == p_amd)
		{
			index = i-1;
			return true;
		}
	}
	return false;
}
bool mobile_device_list::find_by_serial(const char * serial, t_size & index)
{
	t_size i = get_count();
	for (; i; i--)
	{
		if (!_stricmp(get_item(i-1)->mobile_serial, serial))
		{
			index = i-1;
			return true;
		}
	}
	return false;
}


mobile_device_api g_mobile_device_api;

bool mobile_device_api::g_create_handle(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & p_out)
{
	return g_mobile_device_api.create_handle(p_out);
}

void mobile_device_api::g_create_handle_throw_io(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & p_out)
{
	if (!g_create_handle(p_out)) throw exception_io("AMD APIs unavailable");
}


void mobile_device_api::register_notification()
{
	mach_error_t err = m_functions.AMDeviceNotificationSubscribe(g_on_AMD_notification, 0, 0, (unsigned)this, &m_am_device_notification);
	Sleep(20); //hack fix for Apple lib issue on x86 OS
	if (err != MDERR_OK)
		console::formatter() << "iPod manager: Error: AMDeviceNotificationSubscribe returned " << pfc::format_hex(err, 8);
	else
		console::formatter() << "iPod manager: Listening for Apple mobile devices.";
}

void mobile_device_api::initialise()
{
	try 
	{
		__initialise();
	}
	catch (pfc::exception const & ex) 
	{
		deinitialise();
		fbh::show_info_box_threadsafe("Error - iPod manager", ex.what());
	}
}

void mobile_device_api::__initialise()
{
	pfc::array_t<wchar_t> pathQTMLClient, pathiTunesMobileDevice, pathCFNetwork, path_icuin;

	bool b_CoreFoundation = true;

	try 
	{
		library_path_helpers::libraries::get_corefoundation_path(pathQTMLClient, pathCFNetwork, path_icuin);
	}
	catch (const pfc::exception & ex)
	{
		b_CoreFoundation = false;
		throw pfc::exception(pfc::string8() << "Please install or reinstall Apple Application Support.\n\n(Error: Failed to query registry for AAS directory - " << ex.what() << ")");
#if 0
		try 
		{
			library_path_helpers::libraries::get_qtmlclient_path(pathQTMLClient);
		} 
		catch (const pfc::exception & ex)
		{
			throw pfc::exception(pfc::string8() << "Failed to query registry for Quicktime system directory - " << ex.what());
		}
#endif
	}

	try 
	{
		library_path_helpers::libraries::get_itunesmobiledevice_path(pathiTunesMobileDevice);
	}
	catch (const pfc::exception & ex)
	{
		throw pfc::exception(pfc::string8() << "Please install or reinstall Apple Mobile Device Support.\n\n(Error: Failed to query registry for AMDS directory - " << ex.what() << ")");
	}

	DWORD wsaErr = 0;
	WSADATA wsadata;
	memset(&wsadata, 0, sizeof(wsadata));
//	if (!(m_wsa_initialised = (wsaErr == WSAStartup(MAKEWORD(2,2), &wsadata))))
//	{
//		throw pfc::exception(pfc::string8() << "Windows Sockets initialisation failed. Error code: " << (unsigned)wsaErr);
//	}

	SetErrorMode(0);
	//if (b_CoreFoundation)
		m_library_cfnetwork = LoadLibraryEx(pathCFNetwork.get_ptr(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	if (!(m_library_corefoundation = LoadLibraryEx(pathQTMLClient.get_ptr(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
		throw pfc::exception(pfc::string8() << "Failed to load " << ( b_CoreFoundation ? "CoreFoundation" : "QTMLClient" ) << ".dll - " << format_win32_error(GetLastError()));

	if (!(m_library_mobiledevice = LoadLibraryEx(pathiTunesMobileDevice.get_ptr(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
		throw pfc::exception(pfc::string8() << "Failed to load iTunesMobileDevice.dll - " << format_win32_error(GetLastError()));

	if (!(m_library_icuin = LoadLibraryEx(path_icuin.get_ptr(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
		throw pfc::exception(pfc::string8() << "Failed to load icuin40.dll - " << format_win32_error(GetLastError()));

	if (m_library_cfnetwork)
	{
		FreeLibrary(m_library_cfnetwork);
		m_library_cfnetwork = NULL;
	}

	SafeMDGPA2(__CFStringMakeConstantString, m_library_corefoundation);
	SafeMDGPA2(CFStringGetLength, m_library_corefoundation);
	SafeMDGPA2(CFStringGetCharacters, m_library_corefoundation);

	SafeMDGPA2(CFRelease, m_library_corefoundation);
	SafeMDGPA2(CFDictionaryGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFDictionaryGetKeysAndValues, m_library_corefoundation);
	SafeMDGPA2(CFDictionaryGetCount, m_library_corefoundation);
	SafeMDGPA2(CFStringGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFBooleanGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFNumberGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFBooleanGetValue, m_library_corefoundation);
	SafeMDGPA2(CFNumberGetValue, m_library_corefoundation);
	SafeMDGPA2(CFGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFNumberIsFloatType, m_library_corefoundation);
	SafeMDGPA2(CFArrayGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFArrayGetCount, m_library_corefoundation);
	SafeMDGPA2(CFArrayGetValueAtIndex, m_library_corefoundation);
	SafeMDGPA2(CFDataGetTypeID, m_library_corefoundation);
	SafeMDGPA2(CFDataGetLength, m_library_corefoundation);
	SafeMDGPA2(CFDataGetBytes, m_library_corefoundation);

	SafeMDGPA2(AMDeviceNotificationSubscribe, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceNotificationUnsubscribe, m_library_mobiledevice);
	//SafeMDGPA2(AMDeviceNotificationGetThreadHandle, m_library_mobiledevice);
	SafeMDGPA2(AMDSecureShutdownNotificationProxy, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceConnect, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceIsPaired, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceValidatePairing, m_library_mobiledevice);
	SafeMDGPA2(AMDevicePair, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceStartSession, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceStartService, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceSecureStartService, m_library_mobiledevice);
	SafeMDGPA2(AMDServiceConnectionInvalidate, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceStopSession, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceDisconnect, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceRetain, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceRelease, m_library_mobiledevice);
	SafeMDGPA2(AFCConnectionOpen, m_library_mobiledevice);
	SafeMDGPA2(AFCConnectionClose, m_library_mobiledevice);
	SafeMDGPA2(AFCDeviceInfoOpen, m_library_mobiledevice);
	SafeMDGPA2(AFCDirectoryCreate, m_library_mobiledevice);
	SafeMDGPA2(AFCDirectoryOpen, m_library_mobiledevice);
	SafeMDGPA2(AFCDirectoryRead, m_library_mobiledevice);
	SafeMDGPA2(AFCDirectoryClose, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefOpen, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefRead, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefWrite, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefClose, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefSeek, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefSetFileSize, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefTell, m_library_mobiledevice);
	SafeMDGPA2(AFCFileInfoOpen, m_library_mobiledevice);
	SafeMDGPA2(AFCKeyValueRead, m_library_mobiledevice);
	SafeMDGPA2(AFCKeyValueClose, m_library_mobiledevice);
	SafeMDGPA2(AFCRemovePath, m_library_mobiledevice);
	SafeMDGPA2(AFCRenamePath, m_library_mobiledevice);
	SafeMDGPA2(AMDSecurePostNotification, m_library_mobiledevice);
	SafeMDGPA2(AMSInitialize, m_library_mobiledevice);
	SafeMDGPA2(AMSCleanup, m_library_mobiledevice);
	SafeMDGPA2(AMDSecureListenForNotifications, m_library_mobiledevice);
	SafeMDGPA2(AMDListenForNotifications, m_library_mobiledevice);
	SafeMDGPA2(AMDSecureObserveNotification, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefLock, m_library_mobiledevice);
	SafeMDGPA2(AFCFileRefUnlock, m_library_mobiledevice);
	SafeMDGPA2(AMDeviceCopyValue, m_library_mobiledevice);
#ifdef _DEBUG
	SafeMDGPA2(AMDeviceSetValue, m_library_mobiledevice);
	SafeMDGPA2(AMDSetLogLevel, m_library_mobiledevice);
#endif

	SafeMDGPA2(ucol_getSortKey_4_0, m_library_icuin);
	SafeMDGPA2(ucol_close_4_0, m_library_icuin);
	SafeMDGPA2(ucol_open_4_0, m_library_icuin);
	SafeMDGPA2(ucol_setAttribute_4_0, m_library_icuin);
	SafeMDGPA2(ucol_getBound_4_0, m_library_icuin);
	SafeMDGPA2(utrans_openU_4_0, m_library_icuin);
	SafeMDGPA2(utrans_close_4_0, m_library_icuin);
	SafeMDGPA2(utrans_transUChars_4_0, m_library_icuin);

#if _DEBUG
	m_functions.AMDSetLogLevel(10);
#endif

	if (MDERR_OK == m_functions.AMSInitialize())
		m_ams_initialised = true;

	m_initialised = true;

	register_notification();
}

void mobile_device_api::icu_trans_chars(UTransliterator * trans, const wchar_t * string, t_size string_length, pfc::array_t<wchar_t> & p_out)
{
	UErrorCode status = 0;
	p_out.set_size(string_length*10 + 1);
	memcpy(p_out.get_ptr(), string, string_length);
	int textLength = string_length, limit = textLength;
	m_functions.utrans_transUChars_4_0(trans, p_out.get_ptr(), &textLength, p_out.get_size(), 0, &limit, &status);
	p_out.set_size(limit);
}

void mobile_device_api::icu_get_sort_key(UCollator * coll, const wchar_t * string, t_size string_length, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out)
{
	int status = 0;
	p_out.prealloc(512);
	t_size requiredSize = m_functions.ucol_getSortKey_4_0(coll, string, string_length, p_out.get_ptr(), p_out.get_size());
	if (requiredSize > p_out.get_size())
	{
		p_out.set_size(requiredSize);
		m_functions.ucol_getSortKey_4_0(coll, string, string_length, p_out.get_ptr(), p_out.get_size());
	}
	else p_out.set_size(requiredSize);
}

void mobile_device_api::icu_get_sort_key_bound(UCollator * coll, const wchar_t * string, t_size string_length, bool b_bound, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out)
{
	int status = 0;
	icu_get_sort_key(coll, string, string_length, p_out);
	if ( b_bound )
	{
		t_size requiredSize = m_functions.ucol_getBound_4_0(p_out.get_ptr(), p_out.get_size(), 0, 1, 0, 0, &status);
		p_out.set_size(requiredSize);
	}
}

bool mobile_device_api::create_handle(pfc::refcounted_object_ptr_t<mobile_device_api_handle> & p_out)
{
	insync (m_handles_sync);
	if (is_initialised())
	{
		p_out = new mobile_device_api_handle(*this);
		return true;
	}
	return false;
}

void mobile_device_api::deinitialise()
{
	g_drive_manager.remove_all_mobile_devices();

	{
		insync(m_devices_sync);
		{
			t_size i, count = m_devices.get_count();
			for (i=0; i<count; i++)
				m_devices[i]->deinitialise();
			m_devices.remove_all();
		}
	}

	mach_error_t err = 0;

	if (m_am_device_notification && m_library_mobiledevice)
	{
		//HANDLE hThread = m_functions.AMDeviceNotificationGetThreadHandle(m_am_device_notification);
		err = m_functions.AMDeviceNotificationUnsubscribe(m_am_device_notification);//am_device_notification *
	}
	m_am_device_notification = NULL;
	

	m_initialised = false;
	{
		insync (m_handles_sync);
		for (t_size i = 0, count = m_handles.get_count(); i<count; i++)
			m_handles[i]->invalidate();
		m_handles.remove_all();
	}

	if (m_ams_initialised)
	{
		err = m_functions.AMSCleanup();
		m_ams_initialised = false;
	}
	if (m_library_icuin)
	{
		FreeLibrary(m_library_icuin);
		m_library_icuin = NULL;
	}
	if (m_library_mobiledevice)
	{
		FreeLibrary(m_library_mobiledevice);
		m_library_mobiledevice = NULL;
	}
	if (m_library_corefoundation)
	{
		FreeLibrary(m_library_corefoundation);
		m_library_corefoundation = NULL;
	}
	if (m_library_cfnetwork)
	{
		FreeLibrary(m_library_cfnetwork);
		m_library_cfnetwork = NULL;
	}
	//if (m_wsa_initialised)
	//	WSACleanup();
	//m_wsa_initialised = false;
}

void __cdecl mobile_device_api::g_NotificationCallback(CFStringRef str, unsigned int user_data)
{
	ipod_device_ptr_t p_ipod;
	if (g_drive_manager.find_by_mobile_device_handle((am_device*)user_data, p_ipod))
	{

		mobile_device_api_handle::ptr api;
		if (mobile_device_api::g_create_handle(api))
		{
			in_mobile_device_api_handle_sync lockedAPI (api);

			if (lockedAPI.is_valid())
			{
				t_size len = lockedAPI->CFStringGetLength(str);
				pfc::array_t<WCHAR> wstr;
				wstr.fill_null();
				wstr.set_size(len+1);
				lockedAPI->CFStringGetCharacters(str, CFRangeMake(0, len), (unsigned short *)wstr.get_ptr());
				wstr[len] = 0; //Bubble wrap
				if (!_wcsicmp(wstr.get_ptr(), L"com.apple.itunes-client.syncCancelRequest"))
				{
					console::formatter() << "iPod manager: Apple Mobile Device: Received request to cancel sync";
					p_ipod->m_action_manager.abort_all();
				}
				else if (!_wcsicmp(wstr.get_ptr(), L"com.apple.itunes-client.syncSuspendRequest"))
				{
					console::formatter() << "iPod manager: Apple Mobile Device: Received request to suspend sync";
					p_ipod->m_action_manager.suspend_all();
				}
				else if (!_wcsicmp(wstr.get_ptr(), L"com.apple.itunes-client.syncResumeRequest"))
				{
					console::formatter() << "iPod manager: Apple Mobile Device: Received request to resume sync";
					p_ipod->m_action_manager.resume_all();
				}
			}
		}
	}
}

void __cdecl mobile_device_api::g_on_AMD_notification(struct am_device_notification_callback_info *info, void * p_data)
{
	mobile_device_api * p_this = reinterpret_cast<mobile_device_api*>(p_data);
	if (p_this) p_this->on_AMD_notification(info);
}

void mobile_device_api::on_AMD_notification(struct am_device_notification_callback_info *info)
{
	/*if (info->msg == 3)
	{
	DWORD ret = WaitForSingleObject((HANDLE)info->data->unknown2, 10000);
	if (ret == WAIT_FAILED)
	{
	ret = GetLastError();
	}
	}*/


	{
		if (info->msg == ADNCI_MSG_CONNECTED)
		{
			console::formatter() << "iPod manager: Apple Mobile Device: New device detected.";
			on_mobile_device_connected(info->dev);
		}
		else if (info->msg == ADNCI_MSG_DISCONNECTED)
		{
			t_size index;
			insync(m_devices_sync);
			if (m_devices.find_by_am_device(info->dev, index))
			{
				g_drive_manager.remove_by_mobile_device(m_devices[index]);
				m_devices.remove_by_idx(index);
				console::formatter() <<"iPod manager: Disconnected from Apple Mobile Device.";

			}
		}
	}
}

bool g_check_devid_is_ipod(const WCHAR * devid, t_ipod_model & model, bool & shuffle);

void mobile_device_api::on_mobile_device_connected (am_device * dev)
{
	mobile_device_api_handle::ptr api;
	if (!mobile_device_api::g_create_handle(api))
		return;

	in_mobile_device_api_handle_sync lockedAPI(api);
	if (lockedAPI.is_valid())
	{
	//	g_drive_manager.add_pending_mobile_device(dev->serial);
		g_drive_manager.set_pending_mobile_device_connection(true);

		mobile_device_handle::ptr device = new mobile_device_handle;
		lockedAPI->AMDeviceRetain(dev);

		device->m_device = dev;
		device->m_api = api;

		try 
		{
			device->connect_lockdown();

			pfc::string8 serial;

			cfobject::object_t::ptr_t lockdownValues;
			CFTypeRef ref = lockedAPI->AMDeviceCopyValue(dev, NULL, NULL);
			g_get_CFType_object(lockedAPI, ref, lockdownValues);
			if (ref)
			{
				lockedAPI->CFRelease(ref);
				ref = NULL;
			}
			else
			{
				device->disconnect_lockdown();
				throw pfc::exception("Error querying lockdown values.");
			}

			if (!lockdownValues.is_valid() || !lockdownValues->m_dictionary.get_child(L"UniqueDeviceID", serial))
			{
				device->disconnect_lockdown();
				throw pfc::exception("Error querying UDID.");
			}
#if 0
		cfobject::object_t::ptr_t International_Locale, International_SortSections, International_SectionHeaders;
		bool b_continue = device->lockdown_copy_value("com.apple.international", "Locale", International_Locale) 
			&& device->lockdown_copy_value("com.apple.international", "SortSections", International_SortSections) 
			&& International_SortSections->m_dictionary.get_child(L"SectionHeaders", International_SectionHeaders);

		if (b_continue)
		{
			UErrorCode status = 0;
			device->m_icu_context.m_coll = lockedAPI->ucol_open_4_0(pfc::stringcvt::string_utf8_from_wide(International_Locale->m_string.get_ptr()),&status);

			if (U_SUCCESS(status))
			{
				lockedAPI->ucol_setAttribute_4_0(device->m_icu_context.m_coll, 7, 17, &status);
				t_size section_count = International_SectionHeaders->m_array.get_count();
				device->m_icu_context.m_SortSections.set_count(section_count);
				t_size rollingCount = 0;
				for (t_size i = 0; i<section_count; i++)
				{
					cfobject::object_t::ptr_t Headers, FirstCharacterAfterLanguage;
					if (International_SectionHeaders->m_array[i].is_valid() 
						&& International_SectionHeaders->m_array[i]->m_dictionary.get_child(L"Headers", Headers) 
						&& International_SectionHeaders->m_array[i]->m_dictionary.get_child(L"FirstCharacterAfterLanguage", FirstCharacterAfterLanguage))
					{
						lockedAPI.icu_get_sort_key_bound(device->m_icu_context.m_coll, FirstCharacterAfterLanguage->m_string.get_ptr(), -1, true, device->m_icu_context.m_SortSections[i].m_FirstCharacterAfterLanguageSortKey);
						t_size HeaderCount = Headers->m_array.get_count();
						device->m_icu_context.m_SortSections[i].m_HeaderSortKeys.set_size(HeaderCount);
						for (t_size j = 0; j<HeaderCount; j++)
						{
							lockedAPI.icu_get_sort_key_bound(device->m_icu_context.m_coll, Headers->m_array[j]->m_string.get_ptr(), -1, true, device->m_icu_context.m_SortSections[i].m_HeaderSortKeys[j]);
						}
						device->m_icu_context.m_SortSections[i].m_HeaderBase = rollingCount;
						rollingCount += HeaderCount;
					}
				}
				device->m_icu_context.m_HeaderCount = rollingCount;
			}
			pfc::array_t<t_uint8> buffer;
			device->sync_get_iPhoneSortKey(L"1 ty", buffer);
			console::formatter() << pfc::format_hexdump(buffer.get_ptr(), buffer.get_size());
			console::formatter() << device->sync_get_iPhoneSortSection(buffer.get_ptr() ,buffer.get_size());
			//icu_get_sort_key_bound(coll, &last_char, 1, true, last_char_key);
			//m_functions.ucol_close_4_0(coll); 
		}
#endif
			device->mobile_serial = serial;

			mach_error_t amret = MDERR_OK;
			afc_error_t afcret = 0;

			// Start AFC service
			amret = lockedAPI->AMDeviceStartService(dev, lockedAPI->__CFStringMakeConstantString(AMSVC_AFC), &device->m_afc_service);
			if (amret != MDERR_OK)
				console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceStartService (AFC) returned " << pfc::format_hex(amret, 8);
			else
			{
				// Open an AFC Connection
				afcret = lockedAPI->AFCConnectionOpen(device->m_afc_service, 0, &device->m_pafc);
				if (afcret != MDERR_OK)
					console::formatter() << "iPod manager: Apple Mobile Device: error: AFCConnectionOpen returned " << pfc::format_hex(afcret, 8);

#if 0
				if (lockedAPI->AFCConnectionSetIOTimeout != NULL)
				{
					//default is 60 secs, we increase to 120
					lockedAPI->AFCConnectionSetIOTimeout(device->m_pafc, 120, 0);
				}
#endif
			}

			if (amret !=MDERR_OK || afcret != MDERR_OK) 
			{
				//g_drive_manager.remove_pending_mobile_device(dev->serial);
				g_drive_manager.set_pending_mobile_device_connection(false);
				device->disconnect_lockdown();
			}
			else
			{

#if 1
				amret = lockedAPI->AMDeviceSecureStartService(dev, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.notification_proxy"), NULL, &device->m_send_notification_proxy);
				if (amret != MDERR_OK)
					console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceSecureStartService (Send Proxy) returned " << pfc::format_hex(amret, 8);
				if (device->m_send_notification_proxy == nullptr)
					console::formatter() << "iPod manager: Apple Mobile Device: error: Send Proxy Create: Invalid connection";
#endif

				SOCKET syslog_relay;
				amret = lockedAPI->AMDeviceStartService(dev, lockedAPI->__CFStringMakeConstantString("com.apple.syslog_relay"), &syslog_relay);
				if (amret != MDERR_OK)
					console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceSecureStartService (syslog relay) returned " << pfc::format_hex(amret, 8);
				if (syslog_relay == INVALID_SOCKET)
					console::formatter() << "iPod manager: Apple Mobile Device: error: syslog relay: Invalid socket";

				device->m_syslog_relay.initialise(syslog_relay);

#if 0
				amret = lockedAPI->AMDeviceSecureStartService(dev, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.notification_proxy"), NULL, &device->m_notification_proxy);
				if (amret != MDERR_OK)
					console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceStartService (Recv Proxy) returned " << pfc::format_hex(amret, 8);
				if (device->m_notification_proxy == nullptr)
					console::formatter() << "iPod manager: Apple Mobile Device: error: Recv Proxy Create: Invalid connection";

				if (device->m_notification_proxy)
				{
					amret = lockedAPI->AMDSecureObserveNotification(device->m_notification_proxy, lockedAPI->__CFStringMakeConstantString("com.apple.itunes-client.syncCancelRequest"));
					amret = lockedAPI->AMDSecureObserveNotification(device->m_notification_proxy, lockedAPI->__CFStringMakeConstantString("com.apple.itunes-client.syncSuspendRequest"));
					amret = lockedAPI->AMDSecureObserveNotification(device->m_notification_proxy, lockedAPI->__CFStringMakeConstantString("com.apple.itunes-client.syncResumeRequest"));
					amret = lockedAPI->AMDSecureListenForNotifications(device->m_notification_proxy, mobile_device_api::g_NotificationCallback, (unsigned)dev);
				}

#endif

				CFTypeRef ref = NULL;
				ref = lockedAPI->AMDeviceCopyValue(dev, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.iTunes"), NULL);
				g_get_CFType_object(lockedAPI, ref, device->m_device_info);
				if (ref)
				{
#if 0
					try 
					{
						pfc::string8 msg;
						g_print_CFTypeRef(api, ref, msg);

						msg <<"\r\n--------\r\n";

						CFTypeRef ref2 = api ->AMDeviceCopyValue(dev, NULL, NULL);
						if (ref2)
						{
							g_print_CFTypeRef(api, ref2, msg);
							api->CFRelease(ref2);
						}
						popup_message::g_show(msg, "Test Results");
					}
					catch (const pfc::exception & ex)
					{
						popup_message::g_show(ex.what(), "Test Error");
					}
#endif

					lockedAPI->CFRelease(ref);
					ref = NULL;
				}

				ref = lockedAPI->AMDeviceCopyValue(dev, lockedAPI->__CFStringMakeConstantString("com.apple.mobile.iTunes.SQLMusicLibraryPostProcessCommands"), NULL);
				if (ref)
				{
					g_get_CFType_object(lockedAPI, ref, device->m_SQLMusicLibraryPostProcessCommands);
					lockedAPI->CFRelease(ref);
					ref = NULL;
				}

				{
					insync(m_devices_sync);
					m_devices.add_item(device);
				}
				console::formatter() <<"iPod manager: Connected to Apple Mobile Device.";
				device->disconnect_lockdown();

				//g_test_copyvalue(lockedAPI, dev);

				device_instance_info_t devinst;
				bool b_device_found = get_mobile_device_instance_by_serial(serial, devinst); //wi-fi sync hack has no USB device

				{
					ipod_device_ptr_t temp = new ipod_device_t;
					if (b_device_found)
					{
						temp->device_instance_path = devinst.m_path;
						temp->instance = devinst.m_handle;
						bool dummy = false;
						g_check_devid_is_ipod(devinst.m_path, temp->model, dummy);
					}
					else
					{
						temp->device_instance_path = L"<unknown device instance path>";
						console::formatter() << "iPod manager: Warning: Could not locate a USB device for the connected Apple Mobile Device.";
					}
					temp->mobile = true;
					temp->mobile_serial = serial;

					temp->mobile_device = device;
					if (device->m_device_info.is_valid())
					{
						temp->m_device_properties.m_Initialised = true;
						g_get_checkpoint_device_info(device->m_device_info, temp->m_device_properties);
					}
					if (device->m_SQLMusicLibraryPostProcessCommands.is_valid())
					{
						g_get_sql_commands(device->m_SQLMusicLibraryPostProcessCommands, temp->m_device_properties.m_SQLMusicLibraryPostProcessCommands, temp->m_device_properties.m_SQLMusicLibraryUserVersion);
						console::formatter() << "iPod manager: " << temp->m_device_properties.m_SQLMusicLibraryPostProcessCommands.get_count() << " post process SQL commands, version " << temp->m_device_properties.m_SQLMusicLibraryUserVersion;
					}
					g_drive_manager.add_drive(temp, abort_callback_dummy());
					//g_drive_manager.remove_pending_mobile_device(dev->serial);
					g_drive_manager.set_pending_mobile_device_connection(false);

					//test
					//amret = api->AMDeviceStopSession(dev);
					//amret = api->AMDeviceDisconnect(dev);
				}
				//else 
				//{
				//	throw pfc::exception(pfc::string8() << "Could not locate matching USB device for Apple Mobile Device. Please reinstall Apple Mobile Device Support.");
				//}
			}
		}
		catch(pfc::exception const & ex) 
		{
			//g_drive_manager.remove_pending_mobile_device(dev->serial);
			g_drive_manager.set_pending_mobile_device_connection(false);
			fbh::show_info_box_threadsafe("Error - iPod manager", ex.what(), OIC_ERROR);
		}
	}
#if 0
	else console::formatter() << "iPod manager: Apple Mobile Device: error: AMDeviceConnect returned " << amret;
}
#endif
}

t_uint32 AFCInternalError::ToExternalError(t_uint32 internal_error) {return internal_error & 0x1FFF;}