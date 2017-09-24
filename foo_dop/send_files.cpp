#include "main.h"

#include "send_files.h"

void ipod_send_files::on_run()
{
	TRACK_CALL_TEXT("ipod_send_files");


	bool b_started = false, b_need_to_update_database = false;
	m_failed = false;
	try
	{
		t_uint32 steps[] = { 1, 10, 30, 5, m_items.get_count() * 3, 3 };
		m_process.set_steps(steps, tabsize(steps));
		m_drive_scanner.run(m_process, m_process.get_abort());

		DOP_TRACK_ACTION;

		initialise();
		m_process.advance_progresstep();
		if (!prepare(m_process, m_process.get_abort()))
			m_library.run(m_drive_scanner.m_ipods[0], m_process, m_process.get_abort(), true);
		//m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], m_process,m_process.get_abort()); //for 6G artwork
		m_process.advance_progresstep();
		m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process, m_process.get_abort());
		m_process.advance_progresstep();

		//We don't want to let these be aborted
		m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy()/*get_abort()*/);
		b_started = true;
		b_need_to_update_database = true;
		g_load_info(m_process.get_wnd(), m_items, m_process);
		m_checker.run(m_drive_scanner.m_ipods[0], m_items, m_library, m_process, m_process.get_abort());
		m_process.advance_progresstep();
		m_process.update_progress_subpart_helper(0, 1);
		//m_items.sort_by_path_quick();
		{
			t_size i, count = m_items.get_count();
			metadb_handle_list handles;
			for (i = 0; i<count; i++)
			{
				if (!m_checker.m_result[i].have)
					handles.add_item(m_items[i]);
				else
					m_adder.m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), m_items[i], "File skipped: Already on iPod"));
			}
			m_adder.run(m_drive_scanner.m_ipods[0], handles, m_library, m_field_mappings, m_process, m_process.get_abort());
		}
		m_process.advance_progresstep();
		//if (m_adder.m_added_items.get_count())
		m_library.update_smart_playlists();
		m_process.checkpoint();
		b_need_to_update_database = false;
		m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process, m_process.get_abort());
		m_process.advance_progresstep();
	}
	catch (exception_aborted)
	{
		if (b_need_to_update_database)
		{
			try
			{
				m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process, abort_callback_dummy());
			}
			catch (const pfc::exception & e)
			{
				fbh::show_info_box_threadsafe("Error", e.what());
			}
		}
#if 0
		try
		{
			abort_callback_impl p_dummy_abort;
			t_size i, count = m_adder.m_added_items.get_count();
			for (i = 0; i<count; i++)
			{
				filesystem::g_remove(m_adder.m_added_items[i], p_dummy_abort);
			}
		}
		catch (const pfc::exception &) {}
#endif
	}
	catch (const pfc::exception & e)
	{
		try
		{
			abort_callback_impl p_dummy_abort;
			t_size i, count = m_adder.m_added_items.get_count();
			for (i = 0; i<count; i++)
			{
				filesystem::g_remove(m_adder.m_added_items[i], p_dummy_abort);
			}
		}
		catch (const pfc::exception &) {}
		fbh::show_info_box_threadsafe("Error", e.what());
		m_failed = true;
	}
	if (m_drive_scanner.m_ipods.get_count() && b_started)
		m_drive_scanner.m_ipods[0]->do_after_sync();
}

void ipod_send_files::on_exit()
{
	if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
	{

		if (m_adder.m_errors.get_count())
		{
			results_viewer::g_run(L"Warnings - Send Files to iPod", m_adder.m_errors);
		}

	}
}
