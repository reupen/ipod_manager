#pragma once

#include "cfobject.h"
#include "mobile_device.h"

struct mobile_device_functionlist
{
	MDMP(__CFStringMakeConstantString);
	MDMP(CFStringGetLength);
	MDMP(CFStringGetCharacters);
	MDMP(CFStringGetTypeID);
	MDMP(CFGetTypeID);
	MDMP(CFRelease);
	MDMP(CFDictionaryGetTypeID);
	MDMP(CFDictionaryGetKeysAndValues);
	MDMP(CFDictionaryGetCount);
	MDMP(CFBooleanGetTypeID);
	MDMP(CFBooleanGetValue);
	MDMP(CFNumberGetTypeID);
	MDMP(CFNumberGetValue);
	MDMP(CFNumberIsFloatType);
	MDMP(CFArrayGetTypeID);
	MDMP(CFArrayGetCount);
	MDMP(CFArrayGetValueAtIndex);
	MDMP(CFDataGetTypeID);
	MDMP(CFDataGetLength);
	MDMP(CFDataGetBytes);

	MDMP(AMDeviceNotificationSubscribe);
	MDMP(AMDeviceConnect);
	MDMP(AMDeviceIsPaired);
	MDMP(AMDeviceValidatePairing);
	MDMP(AMDeviceStartSession);
	MDMP(AMDeviceStartService);
	MDMP(AMDeviceSecureStartService);
	MDMP(AMDServiceConnectionInvalidate);
	MDMP(AMDeviceStopSession);
	MDMP(AMDevicePair);
	MDMP(AMDeviceRetain);
	MDMP(AMDeviceRelease);
	MDMP(AMDeviceNotificationUnsubscribe);
	MDMP(AMDeviceDisconnect);
	MDMP(AMDSecureShutdownNotificationProxy);
	//MDMP(AMDeviceNotificationGetThreadHandle);
	MDMP(AMDSecurePostNotification);
	MDMP(AMDSecureListenForNotifications);
	MDMP(AMDListenForNotifications);
	MDMP(AMDSecureObserveNotification);
	MDMP(AMDeviceCopyValue);

	MDMP(AMSInitialize);
	MDMP(AMSCleanup);

	MDMP(AFCConnectionOpen);
	MDMP(AFCDeviceInfoOpen);
	MDMP(AFCDirectoryOpen);
	MDMP(AFCDirectoryRead);
	MDMP(AFCDirectoryClose);
	MDMP(AFCConnectionClose);
	MDMP(AFCFileRefOpen);
	MDMP(AFCFileRefRead);
	MDMP(AFCFileRefWrite);
	MDMP(AFCFileRefClose);
	MDMP(AFCFileRefSetFileSize);
	MDMP(AFCFileRefSeek);
	MDMP(AFCFileRefTell);
	MDMP(AFCFileInfoOpen);
	MDMP(AFCKeyValueRead);
	MDMP(AFCKeyValueClose);
	MDMP(AFCRemovePath);
	MDMP(AFCRenamePath);
	MDMP(AFCDirectoryCreate);
	MDMP(AFCFileRefLock);
	MDMP(AFCFileRefUnlock);
	//MDMP(AFCConnectionSetIOTimeout);
#ifdef _DEBUG
	MDMP(AMDeviceSetValue);
	MDMP(AMDSetLogLevel);
#endif

	//MDMP(kCFBooleanTrue);
	//MDMP(kCFBooleanFalse);
	MDMP(ucol_open_4_0);
	MDMP(ucol_close_4_0);
	MDMP(ucol_getSortKey_4_0);
	MDMP(ucol_setAttribute_4_0);
	MDMP(ucol_getBound_4_0);
	MDMP(utrans_openU_4_0);
	MDMP(utrans_close_4_0);
	MDMP(utrans_transUChars_4_0);
};

class syslog_relay : mmh::Thread
{
public:
	DWORD on_thread();

	void initialise(SOCKET s);
	void deinitialise();

	void get_syslog (pfc::string8 & p_out);

	void register_wnd (HWND wnd);
	void deregister_wnd (HWND wnd);

	syslog_relay();
private:
	SOCKET m_socket;

	critical_section m_sync;
	pfc::string8 m_buffer;

	pfc::list_t<HWND> m_wnd_notify_list;
};

typedef pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> icu_sort_key;
class icu_context
{
	class language
	{
	public:
		pfc::array_t<icu_sort_key> m_HeaderSortKeys;
		icu_sort_key m_FirstCharacterAfterLanguageSortKey;
		t_size m_HeaderBase;

		language() : m_HeaderBase(0) {};
	};
public:
	pfc::array_t<language> m_SortSections;
	t_size m_HeaderCount;
	UCollator * m_coll;
	UTransliterator * m_trans;

	icu_context() : m_trans(NULL), m_coll(NULL), m_HeaderCount(0) {};
};

class mobile_device_handle : public pfc::refcounted_object_root
{
public:
	typedef mobile_device_handle self;
	typedef pfc::refcounted_object_ptr_t<self> ptr;

	syslog_relay m_syslog_relay;


	cfobject::object_t::ptr_t m_device_info, m_SQLMusicLibraryPostProcessCommands;

	pfc::refcounted_object_ptr_t<class mobile_device_api_handle> m_api;

	critical_section m_device_sync; //used to sync lockdown connections

	void connect_lockdown();
	void disconnect_lockdown();

	/** must be connected to lockdownd */
	bool lockdown_copy_value(const char * domain, const char * subdomain, cfobject::object_t::ptr_t & ptr);

	void sync_get_iPhoneSortKey(const wchar_t * string, t_size string_length, pfc::array_t<t_uint8> & p_out);
	t_int64 sync_get_iPhoneSortSection(const t_uint8 * SortKey, t_size SortKeyLen);

	void post_notification(CFStringRef notification, CFStringRef userinfo);
	void do_before_sync();
	void do_after_sync();

	void initialise_international();
	void deinitialise_international();

	void deinitialise();

	mobile_device_handle();

	~mobile_device_handle();

	am_device * m_device;
	afc_connection * m_pafc;
	SOCKET m_afc_service;
	service_connection_ref m_notification_proxy;
	service_connection_ref m_send_notification_proxy;
	pfc::string8 mobile_serial;
	icu_context m_icu_context;

private:
	afc_file_ref m_sync_lock;
	bool m_sync_lock_locked;
};

class mobile_device_list : public pfc::list_t< pfc::refcounted_object_ptr_t<class mobile_device_handle> >
{
public:
	void remove_by_am_device(const am_device * p_amd);
	bool find_by_am_device(const am_device * p_amd, t_size & index);
	bool find_by_serial(const char * serial, t_size & index);
};

class mobile_device_api
{
public:
	mobile_device_functionlist m_functions;
	critical_section m_handles_sync, m_devices_sync;

	mobile_device_api() 
		: m_library_mobiledevice(NULL), m_library_corefoundation(NULL), m_initialised(false), m_wsa_initialised(false),
		m_ams_initialised(false), m_am_device_notification(NULL), m_library_cfnetwork(NULL), m_library_icuin(NULL)
	{
		memset (&m_functions, 0, sizeof (m_functions));
	}

	void initialise();

	void deinitialise();

	bool create_handle(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & p_out);
	static bool g_create_handle(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & p_out);
	static void g_create_handle_throw_io(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & p_out);

	void register_handle(class mobile_device_api_handle * ptr) {insync (m_handles_sync); m_handles.add_item(ptr);}
	void deregister_handle(class mobile_device_api_handle * ptr) {insync (m_handles_sync); m_handles.remove_item(ptr);}

	bool is_initialised() {return m_initialised;}

	void register_notification();

	mobile_device_list m_devices;

	void icu_trans_chars(UTransliterator * trans, const wchar_t * string, t_size string_length, pfc::array_t<wchar_t> & p_out);
	void icu_get_sort_key(UCollator * coll, const wchar_t * string, t_size string_length, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out);
	void icu_get_sort_key_bound(UCollator * coll, const wchar_t * string, t_size string_length, bool b_bound, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out);

private:
	static void __cdecl mobile_device_api::g_NotificationCallback(CFStringRef str, unsigned int user_data);
	static void __cdecl g_on_AMD_notification(struct am_device_notification_callback_info *info, void * p_data);
	void on_AMD_notification(struct am_device_notification_callback_info *info);
	void on_mobile_device_connected (am_device * dev);

	void __initialise();

	HINSTANCE m_library_mobiledevice, m_library_corefoundation, m_library_cfnetwork, m_library_icuin;
	volatile bool m_initialised;
	bool m_wsa_initialised, m_ams_initialised;
	am_device_notification * m_am_device_notification;

	pfc::ptr_list_t<class mobile_device_api_handle> m_handles;
};

extern mobile_device_api g_mobile_device_api;

class mobile_device_api_handle : public pfc::refcounted_object_root
{
public:
	typedef mobile_device_api_handle type;
	typedef pfc::refcounted_object_ptr_t<type> ptr;

	bool is_valid() const {return m_valid;}
	void invalidate() {insync(m_sync); m_valid=false;}

	mobile_device_api_handle(mobile_device_api & p_api) : m_api(p_api), m_valid(true) {m_api.register_handle(this);}
	~mobile_device_api_handle() {if (m_valid) m_api.deregister_handle(this);}

private:
	volatile bool m_valid;
	critical_section m_sync;

	mobile_device_api & m_api;

	friend class in_mobile_device_api_handle_sync;
};

class in_mobile_device_api_handle_sync
{
public:
	in_mobile_device_api_handle_sync(pfc::refcounted_object_ptr_t<class mobile_device_api_handle> & ptr)
		: m_ptr(ptr) {if (m_ptr.is_valid()) m_ptr->m_sync.enter();}
	
	~in_mobile_device_api_handle_sync() {if (m_ptr.is_valid()) m_ptr->m_sync.leave(); m_ptr.release();}

	bool is_valid() const {return m_ptr.is_valid() && m_ptr->is_valid();};
	void ensure_valid() {if (!is_valid()) throw pfc::exception("AMDS APIs unavailable");}
	void ensure_valid_io() {if (!is_valid()) throw exception_io("AMDS APIs unavailable");}

	const mobile_device_functionlist * operator -> () const {return &m_ptr->m_api.m_functions;}

	void icu_trans_chars(UTransliterator * trans, const wchar_t * string, t_size string_length, pfc::array_t<wchar_t> & p_out) {return m_ptr->m_api.icu_trans_chars(trans, string, string_length, p_out);}
	void icu_get_sort_key(UCollator * coll, const wchar_t * string, t_size string_length, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out) {return m_ptr->m_api.icu_get_sort_key(coll, string, string_length, p_out);}
	void icu_get_sort_key_bound(UCollator * coll, const wchar_t * string, t_size string_length, bool b_bound, pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & p_out) {return m_ptr->m_api.icu_get_sort_key_bound(coll, string, string_length, b_bound, p_out);}

	//const mobile_device_functionlist & api() const {return m_ptr->m_api.m_functions;}
private:
	pfc::refcounted_object_ptr_t<class mobile_device_api_handle> m_ptr;
};

class NOVTABLE file_mobile_device : public file
{
public:
	virtual void set_checkpoint(const class checkpoint_base * p_checkpoint)=0;
	FB2K_MAKE_SERVICE_INTERFACE(file_mobile_device, file);
};

bool g_get_CFType_object (const in_mobile_device_api_handle_sync & api, CFTypeRef ref, cfobject::object_t::ptr_t & p_out);
void g_get_sql_commands (cfobject::object_t::ptr_t const & cfobj, pfc::string_list_impl & p_out, t_size & p_version);
void _check_afc_ret (unsigned code, const char * function, const char * path = NULL);
