#pragma once

#include "ipod_manager.h"
#include "ipod_scanner.h"
#include "lock.h"
#include "prepare.h"
#include "writer.h"

#define DOP_IPOD_ACTION_ENTRY(classname) \
	typedef classname self_t; \
	typedef pfc::refcounted_object_ptr_t<self_t> ptr_t; \
\
	static ptr_t instantiate() {return new self_t;} \
	static void g_run(HWND wnd) {instantiate()->run(wnd);}

#define DOP_IPOD_ACTION_ENTRY_PARAM(classname) \
	typedef classname self_t; \
	typedef pfc::refcounted_object_ptr_t<self_t> ptr_t; \
\
	template <typename t_class> \
	static ptr_t instantiate(const t_class & p_data) {return new self_t(p_data);} \
	template <typename t_class> \
	static void g_run(HWND wnd, const t_class & p_data) {instantiate(p_data)->run(wnd);}

#define DOP_IPOD_ACTION_ENTRY_PARAM_3(classname) \
	typedef classname self_t; \
	typedef pfc::refcounted_object_ptr_t<self_t> ptr_t; \
\
	template <typename t_class1, typename t_class2, typename t_class3> \
	static ptr_t instantiate(const t_class1 & p_param1, const t_class2 & p_param2, const t_class3 & p_param3) {return new self_t(p_param1, p_param2, p_param3);} \
	template <typename t_class1, typename t_class2, typename t_class3> \
	static void g_run(HWND wnd, const t_class1 & p_param1, const t_class2 & p_param2, const t_class3 & p_param3) {instantiate(p_param1, p_param2, p_param3)->run(wnd);}

#define DOP_TRACK_ACTION \
	ipod_action_manager::track_action actiontracker(m_drive_scanner.m_ipods[0], *this)

class ipod_action_base_t : public pfc::refcounted_object_root
{
protected:
	class threaded_process_v2_impl_t : public threaded_process_v2_t
	{
	public:
		virtual void on_run() {m_this->__on_run();}
		virtual void on_init() {m_this->__on_init();}
		virtual void on_exit() {m_this->__on_exit();}

		threaded_process_v2_impl_t(ipod_action_base_t * p_this, const char * title, t_size flags) : m_this(p_this), threaded_process_v2_t(title, flags) {};

		ipod_action_base_t * m_this;
	};

	threaded_process_v2_impl_t m_process;

	void __on_run() {on_run();};
	void __on_init() {on_init();};
	void __on_exit() {on_exit();m_self.release();};

	virtual void on_run() = 0;
	virtual void on_init() {};
	virtual void on_exit() {};
public:
	typedef ipod_action_base_t self_t;
	typedef pfc::refcounted_object_ptr_t<self_t> ptr_t;

	ipod::tasks::drive_scanner_t m_drive_scanner;

	void run(HWND wnd)
	{
		{
			m_self = this;
			m_process.run(wnd);
		}
	}

	ipod_action_base_t(const char * title, t_size flags = threaded_process_v2_t::flag_show_text|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_show_button) : m_process(this, title, flags) {};

private:
	ptr_t m_self;

	friend class ipod_action_manager;
};

class ipod_action_v2_t : public ipod_action_base_t
{
protected:
public:
	typedef ipod_action_v2_t self_t;
	typedef pfc::refcounted_object_ptr_t<self_t> ptr_t;

	ipod::tasks::load_database_t m_library;
	t_field_mappings m_mappings;

	bool check_available() {return sync.check_eating();};

	void run(HWND wnd)
	{
		if (check_available())
		{
			ipod_action_base_t::run(wnd);
		}
	}

	ipod_action_v2_t(const char * title, t_size flags = threaded_process_v2_t::flag_show_text|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_show_button, bool b_writing = false) 
		: ipod_action_base_t(title, flags), m_library(b_writing) {};

protected:
	in_ipod_sync sync;
};

class ipod_write_action_v2_t : public ipod_action_v2_t
{
public:
	ipod::tasks::database_writer_t m_writer;
	ipod::tasks::preparer_t m_preparer;

	void initialise()
	{
		{
			if (!m_drive_scanner.m_ipods.get_count()) throw pfc::exception_bug_check();
			ipod_device_ptr_t p_ipod = m_drive_scanner.m_ipods[0];

			m_writer.m_iPhoneCalc.initialise();

			if ( (p_ipod->model != ipod_3g 
				&& p_ipod->model != ipod_4g
				&& p_ipod->model != ipod_mini
				&& p_ipod->model != ipod_shuffle
				&& p_ipod->model != ipod_unknown_1394)
				&& !p_ipod->m_device_properties.m_Initialised )
				throw pfc::exception("Failed to query device properties. Write operations are disabled.");
			if (p_ipod->m_device_properties.m_db_version > 5)
				throw pfc::exception(pfc::string8() << "DB Version " << p_ipod->m_device_properties.m_db_version << " is not supported. Write operations are disabled.");
			if (p_ipod->m_device_properties.m_db_version == 5 && !m_writer.m_iPhoneCalc.is_valid_gen())
			{
				if (!p_ipod->mobile || !m_writer.m_iPhoneCalc.is_valid())
					throw pfc::exception(pfc::string8() << "DB Version 5 requires iPhoneCalc.dll. Write operations are disabled.");
			}
			if (p_ipod->m_device_properties.m_ShadowDBVersion > 2)
				throw pfc::exception(pfc::string8() << "Shadow DB Version " << p_ipod->m_device_properties.m_ShadowDBVersion << " is not supported. Write operations are disabled.");
			if (p_ipod->m_device_properties.m_SQLiteDB && !p_ipod->m_device_properties.m_SQLMusicLibraryUserVersion)
				throw pfc::exception("Failed to query SQL post process commands. Check iPod driver is installed. Write operations are disabled.");
#if 0
			try 
			{
			m_writer.m_database_sign_library = new database_sign_library_t;
			}
			catch (pfc::exception const & ex)
			{
#ifdef NDEBUG
				if (m_drive_scanner.m_ipods[0]->m_device_properties.m_db_version == 4 || m_drive_scanner.m_ipods[0]->m_device_properties.m_SQLiteDB)
#endif
					throw;
			}
#endif
		}
	}

	bool prepare(threaded_process_v2_t & p_status, abort_callback & p_abort)
	{
		return m_preparer.run(m_drive_scanner.m_ipods[0], m_library, m_writer, p_status, p_abort);
	}
	ipod_write_action_v2_t(const char * title , t_size flags = threaded_process_v2_t::flag_show_text|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_show_button) : ipod_action_v2_t(title, flags, true) {};
};

