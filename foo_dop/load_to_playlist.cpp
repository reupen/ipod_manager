#include "main.h"


void ipod_load_library_v2_t::on_exit() 
{
	if (!m_process.get_abort().is_aborting() && !m_failed)
	{
		static_api_ptr_t<playlist_manager> playlist_api;
		{
			t_size index = playlist_api->find_or_create_playlist(settings::devices_panel_autosend_playlist,pfc_infinite);
			playlist_api->playlist_clear(index);
			playlist_api->playlist_add_items(index, m_handles, bit_array_val(false));

			playlist_api->set_active_playlist(index);
		}
	}
}




void ipod_load_library_v2_t::on_run(/*abort_callback & p_abort*/)
{
	TRACK_CALL_TEXT("ipod_load_library_v2_t");
	//abort_callback_impl p_abort;
	m_failed = false;
	try {
		t_uint32 steps[] = {1, 10, 30};
		m_process.set_steps(steps, tabsize(steps));
		m_drive_scanner.run(m_process, m_process.get_abort());
		m_process.advance_progresstep();
		m_library.run(m_drive_scanner.m_ipods[0], m_process, m_process.get_abort());
		m_process.advance_progresstep();
		m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], false, m_process, m_process.get_abort());
		m_library.update_smart_playlists();
		m_handles = m_library.m_handles;
		if (m_mappings.sort_ipod_library_playlist)
		{
			service_ptr_t<titleformat_object> to;
			static_api_ptr_t<titleformat_compiler>()->compile_safe(to, m_mappings.ipod_library_sort_script);
			mmh::fb2k::g_sort_metadb_handle_list_by_format_v2(m_handles, to, NULL);
		}
		m_process.advance_progresstep();
	}
	catch (const exception_aborted &) {}
	catch (const pfc::exception & e) 
	{
		message_window_t::g_run_threadsafe(NULL, "Error", e.what());
		//message_window_t::g_run_threadsafe("Error", e.what());
		m_failed = true;
	}
}
