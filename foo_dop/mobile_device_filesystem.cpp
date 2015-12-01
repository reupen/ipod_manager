#include "main.h"

extern volatile bool g_InitQuit_Initialised, g_InitQuit_Deinitialised, g_mobile_devices_enabled;

void _check_afc_ret (unsigned code, const char * function, const char * path)
{
	switch (code)
	{
	case 0:
		break;
	case 8:
		throw exception_io_not_found();
	case 0xa:
		throw exception_io_denied();
	case 0x10:
		throw exception_io_already_exists();
	case 0x12:
		throw exception_io_device_full();
	case 0xc:
		message_window_t::g_run_threadsafe("Error - iPod manager", "Connection to device filesystem service lost.\r\nPlease visit http://yuo.be/r/afcto", OIC_ERROR);
	default:
		throw exception_io(pfc::string8() << "I/O Error: " << function << " returned: " << code << (path ? " Path was: " :  "") << ( path ? path : "" ) );
	};

}

//#define LOG_AFC_FILE
#ifdef LOG_AFC_FILE
#define DECLARE_AFC_TIMER pfc::hires_timer timer; timer.start()
#define AFC_FORMAT_TIMER " , Time: " << pfc::format_float(timer.query_reset())
#else 
#define DECLARE_AFC_TIMER
#endif

// {A3089296-1BBD-42e2-9E7F-8EA6ABA94F8E}
const GUID file_mobile_device::class_guid = 
{ 0xa3089296, 0x1bbd, 0x42e2, { 0x9e, 0x7f, 0x8e, 0xa6, 0xab, 0xa9, 0x4f, 0x8e } };

class file_afc : public file_mobile_device
{
public:
	virtual void set_checkpoint(const checkpoint_base * p_checkpoint)
	{
		m_checkpoint = p_checkpoint;
	}
	void checkpoint(abort_callback & p_abort)
	{
		p_abort.check();
		if (m_checkpoint) m_checkpoint->checkpoint();
	}

	virtual t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		if (m_mode == 3)
			throw exception_io("Cannot read in write new mode");
		t_size ret = 0;
		const t_size buffsize = 8*1024*1024;
		t_size position = 0, remaining=p_bytes;
		t_uint8 * const ptr = reinterpret_cast<t_uint8*>(p_buffer);
		while (remaining)
		{
			DECLARE_AFC_TIMER;
			checkpoint(p_abort);
			t_size toread = min(remaining, buffsize);
			t_size read = toread;
			afc_error_t err = lockedAPI->AFCFileRefRead(m_device->m_pafc, m_handle, ptr+position, &read);
			position += toread;
			remaining -= toread;
			ret += read;

#ifdef LOG_AFC_FILE
			console::formatter() << "AFCFileRefRead called. Handle: " << m_handle << ", Requested bytes: " << p_bytes << ", Read: " << ret << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
			_check_afc_ret(err, "AFCFileRefRead");
			if (read != toread) break;
		}
		return ret;
	}
	virtual void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		if (m_mode == 1)
			throw exception_io("Cannot write in read mode");
		if (p_bytes > 0)
		{
			const t_size buffsize = 8*1024*1024;
			t_size position = 0, remaining=p_bytes;
			const t_uint8 * const ptr = reinterpret_cast<const t_uint8*>(p_buffer);
			while (remaining)
			{
				DECLARE_AFC_TIMER;
				checkpoint(p_abort);
				t_size towrite = min(remaining, buffsize);
				afc_error_t err = lockedAPI->AFCFileRefWrite(m_device->m_pafc, m_handle, ptr+position, towrite);
				position += towrite;
				remaining -= towrite;
#ifdef LOG_AFC_FILE
				console::formatter() << "AFCFileRefWrite called. Handle: " << m_handle << ", Requested bytes: " << p_bytes << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
				_check_afc_ret(err, "AFCFileRefWrite");
			}
		}
	}
	virtual t_filesize get_size(abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		DECLARE_AFC_TIMER;
		checkpoint(p_abort);
		afc_dictionary *info = NULL;
		afc_error_t err = lockedAPI->AFCFileInfoOpen(m_device->m_pafc, m_path.get_ptr(), &info);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileInfoOpen called. Path: " << m_path.get_ptr() << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileInfoOpen", m_path);

		t_filesize ret = filesize_invalid;
		bool b_found = false;
		char *key = NULL, *val = NULL;
		while ((lockedAPI->AFCKeyValueRead(info, &key, &val) == MDERR_OK) && key && val)
		{
			if (!strcmp(key, "st_size"))
			{
				ret = strtoul64_n(val, pfc_infinite);
				b_found = true;
			}
		}
		lockedAPI->AFCKeyValueClose(info);
		return ret;
	}
	virtual t_filetimestamp get_timestamp(abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		checkpoint(p_abort);
		afc_dictionary *info = NULL;
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileInfoOpen(m_device->m_pafc, m_path.get_ptr(), &info);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileInfoOpen called. Path: " << m_path.get_ptr() << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileInfoOpen", m_path);

		t_filesize ret = filesize_invalid;
		bool b_found = false;
		char *key = NULL, *val = NULL;
		while ((lockedAPI->AFCKeyValueRead(info, &key, &val) == MDERR_OK) && key && val)
		{
			if (!strcmp(key, "st_mtime"))
			{
				ret = strtoul64_n(val, pfc_infinite)/100 + 116444736000000000;
				b_found = true;
			}
		}
		lockedAPI->AFCKeyValueClose(info);
		return ret;
	}
	virtual t_filesize get_position(abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		checkpoint(p_abort);
		t_sfilesize ret = NULL;
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileRefTell(m_device->m_pafc, m_handle, &ret);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileRefTell called. Handle: " << m_handle << ", Position: " << ret << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileRefTell");
		return pfc::downcast_guarded<t_filesize> (ret);
	}
	virtual void resize(t_filesize p_size,abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		checkpoint(p_abort);
		if (m_mode == 1)
			throw exception_io("Cannot write in read mode");
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileRefSetFileSize(m_device->m_pafc, m_handle, p_size);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileRefSetFileSize called. Handle: " << m_handle << ", Position: " << p_size << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileRefSetFileSize");
	}
	virtual void seek(t_filesize p_position,abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		checkpoint(p_abort);
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileRefSeek(m_device->m_pafc, m_handle, p_position, NULL);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileRefSeek called. Handle: " << m_handle << ", Position: " << p_position << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileRefSeek");
	}
	virtual bool can_seek()
	{
		return true;
	}
	virtual bool get_content_type(pfc::string_base & p_out)
	{
		return false;
	}
	virtual void reopen(abort_callback & p_abort)
	{
		in_mobile_device_api_handle_sync lockedAPI (m_api);
		lockedAPI.ensure_valid_io();

		checkpoint(p_abort);
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileRefSeek(m_device->m_pafc, m_handle, 0, NULL);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileRefSeek called. Handle: " << m_handle << ", Position: (reopen), Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileRefSeek");
	}
	virtual bool is_remote()
	{
		return false;
	}
	static void g_create(filesystem::t_open_mode open_mode, const mobile_device_handle::ptr & p_device, const char * path, file::ptr & p_out)
	{
		//insync (g_mobile_device_api_sync);
		mobile_device_api_handle::ptr api;
		if (!mobile_device_api::g_create_handle(api))
			throw exception_io("AMD APIs unavailable");

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		unsigned long long mode = 0;
		if (open_mode == filesystem::open_mode_read)
			mode = 1;
		else if (open_mode == filesystem::open_mode_write_existing)
			mode = 2;
		else if (open_mode == filesystem::open_mode_write_new)
			mode = 4;
		else
			throw pfc::exception_bug_check();

		service_ptr_t<file_afc> temp = new service_impl_t<file_afc>;
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileRefOpen(p_device->m_pafc, path, mode, &temp->m_handle);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileRefOpen called. Handle: " << temp->m_handle << ", Path: " << path << ", Mode: " << mode << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileRefOpen", path);
		temp->m_device = p_device;
		temp->m_path = path;
		temp->m_api = api;
		temp->m_mode = mode;
		p_out = temp.get_ptr();
	}
	file_afc() : m_handle(NULL), m_mode(NULL), m_checkpoint(NULL) {};
	~file_afc() 
	{
		if (m_handle && m_api.is_valid())
		{
			in_mobile_device_api_handle_sync lockedAPI (m_api);
			if (lockedAPI.is_valid())
			{
		DECLARE_AFC_TIMER;
				afc_error_t err = lockedAPI->AFCFileRefClose(m_device->m_pafc, m_handle);
#ifdef LOG_AFC_FILE
			console::formatter() << "AFCFileRefClose called. Handle: " << m_handle << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
				m_handle = NULL;
			}
		}
	}
private:
	afc_file_ref m_handle;
	mobile_device_handle::ptr m_device;
	pfc::string8 m_path;
	mobile_device_api_handle::ptr m_api;
	t_uint64 m_mode;
	//critical_section m_sync;
	const checkpoint_base * m_checkpoint;
};

class filesystem_afc : public filesystem
{
public:
	static void g_replace_slashes(const char * p_path, pfc::string_base & p_out)
	{
		pfc::string8 temp = p_path;
		temp.replace_char('\\', '/');
		p_out = temp; //meh
	}
	bool g_is_our_path(const char * p_path)
	{
		return !stricmp_utf8_max(p_path, "applemobiledevice://", 20);
	}
	virtual bool get_canonical_path(const char * p_path,pfc::string_base & p_out)
	{
		if (g_is_our_path(p_path))
		{
			g_replace_slashes(p_path, p_out);
			return true;
		}
		return false;
	}
	virtual bool is_our_path(const char * p_path)
	{
		return g_is_our_path(p_path);
	}
	virtual bool get_display_path(const char * p_path,pfc::string_base & p_out)
	{
		return false;
	}

	static void split_path_and_device(const char * p_path, mobile_device_handle::ptr & p_out, pfc::string8 & p_device_path, abort_callback & p_abort)
	{
		p_out.release();

		if (g_InitQuit_Initialised && !g_mobile_devices_enabled)
			throw exception_io("Apple Mobile Device Support is disabled");

		if (!g_InitQuit_Initialised && core_api::is_main_thread())
			throw exception_io("Apple Mobile Device Support is not initialised (main thread I/O)");

		if (g_InitQuit_Deinitialised)
			throw exception_io("Apple Mobile Device Support is not initialised");

		HANDLE handles[] = {p_abort.get_abort_event(), g_drive_manager.m_event_initialised.get()};
		DWORD waitRet = WaitForMultipleObjects(2, handles, FALSE, 5000);
		p_abort.check();

		if (waitRet != WAIT_OBJECT_0 + 1)
			throw exception_io("Apple Mobile Device Support is not initialised");

		if (!g_mobile_device_api.is_initialised())
			throw exception_io("Apple Mobile Device Support is not initialised");

		const char * ptr = p_path;
		t_size len = strlen(p_path);
		if (len <=21)
			throw exception_io_no_handler_for_path();
		ptr += 20;
		const char * serial = ptr;
		while (*ptr && *ptr != ':') ptr++;
		t_size index;
		{
			t_size counter = 0;
			bool b_break = false;
			do 
			{
				{
					insync(g_mobile_device_api.m_devices_sync);
					if (b_break = g_mobile_device_api.m_devices.find_by_serial(pfc::string8(serial, ptr-serial), index))
					{
						if (*ptr == ':') ptr++;
						p_out = g_mobile_device_api.m_devices[index];
					}
				}
				if (!b_break) 
				{
					p_abort.check();
				//	if (counter > 2 && !g_drive_manager.have_pending_mobile_device(serial, ptr-serial)) b_break = true;
					if (counter > 2 && !g_drive_manager.get_pending_mobile_device_connection()) b_break = true;
					else if (counter == 50) b_break = true;
					else if (counter <= 2) Sleep(10);
					else Sleep(66);
					counter++;
				}
			} 
			while (!b_break);

			if (!p_out.is_valid())
				throw exception_io("Device not available");
		}
		g_replace_slashes(ptr, p_device_path);
	}

	virtual void open(service_ptr_t<file> & p_out,const char * p_path, t_open_mode p_mode,abort_callback & p_abort)
	{
		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, p_abort);
		file_afc::g_create(p_mode, handle, path, p_out);
		if (p_mode == filesystem::open_mode_read && p_out.is_valid())
		{
			file_cached::g_create(p_out, p_out, p_abort, 256*1024);
		}
	}
	virtual void remove(const char * p_path,abort_callback & p_abort)
	{
		mobile_device_api_handle::ptr api;
		if (!mobile_device_api::g_create_handle(api))
			throw exception_io("AMD APIs unavailable");

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, p_abort);
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCRemovePath(handle->m_pafc, path);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCRemovePath called. Path: " << path << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCRemovePath", path);
	}
	virtual void move(const char * p_src,const char * p_dst,abort_callback & p_abort)
	{
		mobile_device_api_handle::ptr api;
		if (!mobile_device_api::g_create_handle(api))
			throw exception_io("AMD APIs unavailable");

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle1, handle2;
		pfc::string8 path1, path2;
		split_path_and_device(p_src, handle1, path1, p_abort);
		split_path_and_device(p_dst, handle2, path2, p_abort);
		if (handle1->m_device != handle2->m_device)
			throw exception_io("Objects are on different devices");
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCRenamePath(handle1->m_pafc, path1, path2);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCRenamePath called. From: " << path1 << ", To: " << path2 << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCRenamePath", pfc::string8() << path1 << " to " << path2);
	}

	virtual bool is_remote(const char * p_src)
	{
		return false;
	}
#if 0
	static void g_get_stats_list (const char * p_path, pfc::list_t< stats_t> & p_out)
	{
		if (stricmp_utf8_max(p_path, "applemobiledevice://", 20))
			throw pfc::exception("Only for applemobiledevice:// paths");

		mobile_device_api_handle::ptr api;
		mobile_device_api::g_create_handle_throw_io(api);

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, abort_callback_dummy());
		afc_dictionary *info = NULL;
		afc_error_t err = lockedAPI->AFCFileInfoOpen(handle->m_pafc, path, &info);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileInfoOpen called. Path: " << path << ", Ret: " << err;
#endif
		_check_afc_ret(err, "AFCFileInfoOpen", path);

		char *key = NULL, *val = NULL;
		while ( (lockedAPI->AFCKeyValueRead(info, &key, &val) == MDERR_OK) && key && val)
		{
			stats_t temp;
			temp.name = key;
			temp.value = val;
			p_out.add_item(temp);
		}
		lockedAPI->AFCKeyValueClose(info);
	}
#endif
	void get_stats_ex(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,bool& is_dir,abort_callback & p_abort)
	{
		p_is_writeable = true;
		is_dir = false;

		mobile_device_api_handle::ptr api;
		mobile_device_api::g_create_handle_throw_io(api);

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, p_abort);
		afc_dictionary *info = NULL;
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCFileInfoOpen(handle->m_pafc, path, &info);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCFileInfoOpen called. Path: " << path << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCFileInfoOpen", path);

		char *key = NULL, *val = NULL;
		while ( (lockedAPI->AFCKeyValueRead(info, &key, &val) == MDERR_OK) && key && val)
		{
			if (!strcmp(key, "st_size"))
			{
				p_stats.m_size = strtoul64_n(val, pfc_infinite);
			}
			else if (!strcmp(key, "st_mtime"))
			{
				p_stats.m_timestamp = strtoul64_n(val, pfc_infinite)/100 + 116444736000000000;
			}
			else if (!strcmp(key, "st_ifmt"))
			{
				is_dir = !strcmp(val, "S_IFDIR");
			}
		}
		lockedAPI->AFCKeyValueClose(info);
	}

	virtual void get_stats(const char * p_path,t_filestats & p_stats,bool & p_is_writeable,abort_callback & p_abort)
	{
		bool b_dummy;
		get_stats_ex(p_path, p_stats, p_is_writeable, b_dummy, p_abort);
	}
	virtual void create_directory(const char * p_path,abort_callback & p_abort)
	{

		mobile_device_api_handle::ptr api;
		mobile_device_api::g_create_handle_throw_io(api);

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, p_abort);
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCDirectoryCreate(handle->m_pafc, path);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCDirectoryCreate called. Path: " << path << ", Ret: " << err;
#endif
		_check_afc_ret(err, "AFCDirectoryCreate", path);
	}


	virtual void list_directory(const char * p_path,directory_callback & p_out,abort_callback & p_abort)
	{
		mobile_device_api_handle::ptr api;
		mobile_device_api::g_create_handle_throw_io(api);

		in_mobile_device_api_handle_sync lockedAPI (api);
		lockedAPI.ensure_valid_io();

		mobile_device_handle::ptr handle;
		pfc::string8 path;
		split_path_and_device(p_path, handle, path, p_abort);
		afc_directory * dir = NULL;
		DECLARE_AFC_TIMER;
		afc_error_t err = lockedAPI->AFCDirectoryOpen(handle->m_pafc, path, &dir);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCDirectoryOpen called. Handle: " << (t_size)dir << ", Path: " << path << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCDirectoryOpen", path);

		try
		{
			char * folder = NULL;
			DECLARE_AFC_TIMER;
			while (((err = lockedAPI->AFCDirectoryRead(handle->m_pafc, dir, &folder)) == MDERR_OK) && folder)
			{
#ifdef LOG_AFC_FILE
				console::formatter() << "AFCDirectoryRead called. Handle: " << (t_size)dir << ", Path: " << path << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
				p_abort.check();
				if ((strcmp(folder, ".") && strcmp(folder, "..")))
				{
					bool is_dir=false, is_writeable=false;
					t_filestats stats = filestats_invalid;
					pfc::string8 path_child; path_child<< p_path ;
					if (!path_child.get_length() || path_child.get_ptr()[path_child.get_length()-1] != '/')
						path_child << "/";
					path_child << folder;
					get_stats_ex(path_child, stats, is_writeable, is_dir, p_abort);
					p_out.on_entry(this, p_abort, path_child, is_dir, stats);
				}
			}
#ifdef LOG_AFC_FILE
			console::formatter() << "AFCDirectoryRead called. Handle: " << (t_size)dir << ", Path: " << path << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		} catch (const pfc::exception &)
		{
			err = lockedAPI->AFCDirectoryClose(handle->m_pafc, dir);
			throw;
		}
		err = lockedAPI->AFCDirectoryClose(handle->m_pafc, dir);
#ifdef LOG_AFC_FILE
		console::formatter() << "AFCDirectoryClose called. Handle: " << (t_size)dir << ", Ret: " << err << AFC_FORMAT_TIMER;
#endif
		_check_afc_ret(err, "AFCDirectoryClose", path);
	}

	virtual bool supports_content_types()
	{
		return false;
	}
};

service_factory_single_t<filesystem_afc> g_filesystem_afc;
