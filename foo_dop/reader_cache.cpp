#include "stdafx.h"

#include "ipod_manager.h"
#include "writer.h"

namespace ipod
{
	namespace tasks
	{
		void load_database_t::load_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, bool b_CheckIfFilesChanged, threaded_process_v2_t & p_status, abort_callback & p_abort)
		{
			pfc::string8 base;
			p_ipod->get_root_path(base);

			metadb_handle_list handlestoread(m_handles);

			pfc::string8 cachePath = base;
			cachePath << "metadata_cache.fpl";

			//We want to hint from main thread to avoid annoyances
			static_api_ptr_t<main_thread_callback_manager> p_main_thread;
			p_status.update_text("Loading metadata cache");
			service_ptr_t<t_main_thread_load_cache_v2_t> p_cache_loader = new service_impl_t<t_main_thread_load_cache_v2_t>
				(base);

			p_cache_loader->callback_run();
			//p_main_thread->add_callback(p_cache_loader);
			if (!p_cache_loader->m_signal.wait_for(-1))
				throw pfc::exception("Cache read timeout!");
			if (!p_cache_loader->m_ret)
				throw pfc::exception(pfc::string8() << "Error reading metadata cache: " << p_cache_loader->m_error);

			p_status.update_text("Checking files for changes");
			//pfc::hires_timer timer2;
			//timer2.start();
			t_size n = handlestoread.get_count(), count = n;
			for (; n; n--)
			{
				if (p_ipod->mobile || !b_CheckIfFilesChanged)
				{
					if (handlestoread[n - 1]->is_info_loaded_async())
						handlestoread.remove_by_idx(n - 1);
					m_tracks[n - 1]->m_runtime_filestats.m_timestamp = filetimestamp_invalid;
					if (m_tracks[n - 1]->original_timestamp_valid)
						m_tracks[n - 1]->m_runtime_filestats.m_timestamp = m_tracks[n - 1]->original_timestamp;
					else if (m_tracks[n - 1]->lastmodifiedtime)
						m_tracks[n - 1]->m_runtime_filestats.m_timestamp = filetime_time_from_appletime(m_tracks[n - 1]->lastmodifiedtime);
					m_tracks[n - 1]->m_runtime_filestats.m_size = m_tracks[n - 1]->file_size_32;
				}
				else
				{
					if (true)
					{
						if (n % 20 == 0)
							p_abort.check();
						t_filestats stats = handlestoread[n - 1]->get_filestats();
						const char * path = handlestoread[n - 1]->get_path();
						if (!stricmp_utf8_max(path, "file://", 7))
						{
							path += 7;
							win32::handle_ptr_t p_file =
								CreateFile(pfc::stringcvt::string_os_from_utf8(path), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
							if (p_file.is_valid())
							{
								t_filestats newstats = filestats_invalid;
								GetFileSizeEx(p_file, (PLARGE_INTEGER)&newstats.m_size);
								GetFileTime(p_file, NULL, NULL, (LPFILETIME)&newstats.m_timestamp);
								m_tracks[n - 1]->m_runtime_filestats = newstats;
								if (handlestoread[n - 1]->is_info_loaded_async())
								{
									t_uint64 hour = 60 * 60;
									hour *= 10000000;
									if (stats.m_size == newstats.m_size && (stats.m_timestamp == newstats.m_timestamp || _abs64(newstats.m_timestamp - stats.m_timestamp) == hour) && newstats.m_size != filesize_invalid && newstats.m_timestamp != filetimestamp_invalid)
										handlestoread.remove_by_idx(n - 1);
								}
							}
						}
					}
				}
				p_status.update_progress_subpart_helper(count - n + 1, count);
			}
			//console::formatter() << "info checked in: " << pfc::format_time_ex(timer2.query(),6);

			if (handlestoread.get_count())
			{
				p_status.update_text("Loading file info");
				//static_api_ptr_t<main_thread_callback_manager> p_main_thread;

				service_ptr_t<t_main_thread_scan_file_info> p_info_loader = new service_impl_t<t_main_thread_scan_file_info>
					(handlestoread, metadb_io::load_info_force, wnd);
				p_main_thread->add_callback(p_info_loader);
				if (!p_info_loader->m_signal.wait_for(-1))
					throw pfc::exception("File info reading timeout!");
				if (p_info_loader->m_ret == metadb_io::load_info_aborted)
					throw pfc::exception("File info read was aborted");
			}

			p_abort.check();

		}

		void load_database_t::save_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status, abort_callback & p_abort) const
		{
			//pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(1);
			//progress_details[0].m_label = "Database:";
			//progress_details[0].m_value = "Metadata cache";
			p_status.update_text("Saving metadata cache");

			pfc::string8 base;
			p_ipod->get_root_path(base);

			pfc::string8 path = base; path << "metadata_cache.fpl";
			try
			{
				playlist_loader::g_save_playlist(path, m_handles, abort_callback_dummy());
			}
			catch (const pfc::exception & ex)
			{
				try { filesystem::g_remove(path, abort_callback_dummy()); }
				catch (pfc::exception const &) {};
				console::formatter() << "iPod manager: Error saving metadata_cache.fpl to iPod: " << ex.what();
				//throw;
			}
			p_status.checkpoint();
		}

		void load_database_t::refresh_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, bool b_CheckIfFilesChanged, threaded_process_v2_t & p_status, abort_callback & p_abort)
		{
			p_status.checkpoint();
			load_cache(wnd, p_ipod, b_CheckIfFilesChanged, p_status, p_abort);
			p_status.checkpoint();
			if (!m_writing)
				save_cache(wnd, p_ipod, p_status, p_abort);
			p_status.checkpoint();
		}

	}
}