#ifndef _DOP_WRITER_H_
#define _DOP_WRITER_H_

#include "iPhoneCalc.h"
#include "reader.h"

template<typename t_receiver>
class process_locations_notify_impl : public process_locations_notify
{
public:
	virtual void on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items)
	{
		if (m_receiver != NULL) 
		{
			m_receiver->on_completion(m_taskid,p_items);
		}
	}
	virtual void on_aborted()
	{
		if (m_receiver != NULL)
		{
			m_receiver->on_aborted(m_taskid);
		}
	}
	void setup(t_receiver * p_receiver, unsigned p_task_id) {m_receiver = p_receiver; m_taskid = p_task_id;}
private:
	t_receiver * m_receiver;
	unsigned m_taskid;
};

template<typename t_receiver>
process_locations_notify_ptr process_locations_notify_create(t_receiver * p_receiver,unsigned p_taskid) {
	service_ptr_t<process_locations_notify_impl<t_receiver> > instance = new service_impl_t<process_locations_notify_impl<t_receiver> >();
	instance->setup(p_receiver,p_taskid);
	return instance;
}

class playlist_loader_callback_dop : public playlist_loader_callback {
public:

	metadb_handle_list m_handles;
	pfc::list_t<file_info_impl> m_infos;
	pfc::list_t<t_filestats> m_stats;
	pfc::list_t<bool> m_freshes;

	// WTF is browse info?
	virtual bool want_browse_info(const metadb_handle_ptr & p_item, t_entry_type p_type, t_filetimestamp ts) { return false; }
	virtual void on_browse_info(const metadb_handle_ptr & p_item, t_entry_type p_type, const file_info & info, t_filetimestamp ts) {}
	virtual bool is_path_wanted(const char * path, t_entry_type type) { return true; }

	void on_progress(const char * path) {}

	void on_entry(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh)
	{
		/*m_handles.add_item(ptr);
		m_infos.add_item(file_info_impl());
		m_stats.add_item(p_stats);
		m_freshes.add_item(p_fresh);*/
	}
	bool want_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,bool p_fresh) {return true;}
	void on_entry_info(const metadb_handle_ptr & ptr,t_entry_type type,const t_filestats & p_stats,const file_info & p_info,bool p_fresh) 
	{
		m_handles.add_item(ptr);
		m_infos.add_item(p_info);
		m_stats.add_item(p_stats);
		m_freshes.add_item(p_fresh);
	}

	void hint_metadb()
	{
		static_api_ptr_t<metadb_io_v3> api;
		pfc::list_t<const file_info*> infos;
		t_size i, count = m_infos.get_count();
		infos.set_count(count);
		for (i=0; i<count; i++)
			infos[i]=&m_infos[i];
		api->hint_multi_async(m_handles, infos, m_stats, pfc::bit_array_table(m_freshes.get_ptr(), m_freshes.get_count()));
	}

	void handle_create(metadb_handle_ptr & p_out,const playable_location & p_location) {m_api->handle_create(p_out,p_location);}

private:

	//abort_callback & m_abort;
	static_api_ptr_t<metadb> m_api;
};

class t_main_thread_load_cache_v2_t : public main_thread_callback
{
public:
	t_main_thread_load_cache_v2_t(const char * p_ipod) 
		: m_path(p_ipod), m_ret(false)
	{
		m_signal.create(true, false);
		m_playlist_loader_callback = new service_impl_t<playlist_loader_callback_dop>;
	};

	abort_callback_impl m_abort;

	virtual void callback_run()
	{
		pfc::string8 path; path << m_path << "metadata_cache.fpl";
		m_ret = true;
		try
		{
			playlist_loader::g_load_playlist(path, m_playlist_loader_callback, m_abort);
			m_playlist_loader_callback->hint_metadb();
		}
		catch (const exception_io_not_found &) {}
		catch (const exception_io_data & ex) 
		{
			console::formatter() << "iPod manager: Warning: Error reading metadata_cache.fpl from iPod: " << ex.what() << " Cache will be regenerated.";
		}
		catch (const pfc::exception & ex) 
		{
			m_ret=false;
			m_error = ex.what();
			console::formatter() << "iPod manager: Error reading metadata_cache.fpl from iPod: " << ex.what();
		}
		m_signal.set_state(true);
	}
	pfc::string8 m_path;
	win32_event m_signal;
	bool m_ret;
	pfc::string8 m_error;
	service_ptr_t<playlist_loader_callback_dop> m_playlist_loader_callback;
};

class t_main_thread_scan_file_info : public main_thread_callback
{
public:
	t_main_thread_scan_file_info(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,
		metadb_io::t_load_info_type p_type,HWND p_parent_window, t_uint32 flags= metadb_io_v2::op_flag_no_errors|metadb_io_v2::op_flag_delay_ui|metadb_io_v2::op_flag_background) 
		: m_list(p_list), m_type(p_type), m_parent_window(p_parent_window), m_flags(flags)
	{
		m_signal.create(true, false);
	};

	void on_task_completion(unsigned taskid, unsigned p_code)
	{
		//console::formatter() << "info loaded in: " << pfc::format_time_ex(timer.query(),6);
		//if (taskid ==0)
		{
			m_ret = (metadb_io::t_load_info_state)p_code;
			m_signal.set_state(true);
		}
	}
	virtual void callback_run()
	{
		//load info
		static_api_ptr_t<metadb_io_v2> api;
		api->load_info_async(m_list, m_type, m_parent_window, m_flags, completion_notify_create(this,0));
	}
	//pfc::hires_timer timer, timer2;
	//pfc::string8 m_path;
	metadb_io::t_load_info_state m_ret;
	win32_event m_signal;
	const pfc::list_base_const_t<metadb_handle_ptr> & m_list;
	metadb_io::t_load_info_type m_type;
	HWND m_parent_window;
	t_uint32 m_flags;
	//bool m_load_cache;
};

void ipod_write_dopdb(const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort);
void ipod_write_shuffledb(ipod_device_ptr_ref_t p_ipod, const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort);
void ipod_write_shadowdb_v2(ipod_device_ptr_ref_t p_ipod, const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort);

namespace ipod
{
	namespace tasks
	{

		class database_writer_t
		{
		public:
			//database_sign_library_t::ptr_t m_database_sign_library;
			iPhoneCalc m_iPhoneCalc;

			void run(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort);

			void write_itunesdb		(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void write_artworkdb	(ipod_device_ptr_ref_t p_ipod, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void write_sqlitedb		(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort);
			pfc::array_staticsize_t<t_uint8> calculate_cbk(ipod_device_ptr_ref_t p_ipod, const char* locations_itdb_path);
			//construct from main thread only
			database_writer_t() 
			{
				core_api::ensure_main_thread();
				m_use_ipod_sorting = settings::use_ipod_sorting;
			};
		private:
			bool m_use_ipod_sorting;
		};
	}
}

namespace ipod
{
	namespace tasks
	{

		class check_files_in_library_t
		{
		public:
			static int g_compare_filesize (const t_filestats & item1, const t_filestats & item2)
			{return pfc::compare_t<t_filesize>(item1.m_size, item2.m_size);}
			static int g_compare_filestats_with_filesize (const t_filestats & item1, const t_filesize & item2)
			{return pfc::compare_t<t_filesize>(item1.m_size, item2);}
			static bool g_compare_meta(const metadb_handle_ptr & item1, const metadb_handle_ptr & item2);
			void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort);
			struct t_result
			{
				bool have;
				t_uint32 index;
				t_result() : have(false), index(0) {};
			};
			pfc::array_t<t_result> m_result;
			pfc::list_t<t_filestats> m_stats;
		};

	}
}


#endif //_DOP_WRITER_H_