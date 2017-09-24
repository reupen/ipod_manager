#ifndef _DOP_REMOVE_H_
#define _DOP_REMOVE_H_

#include "file_remover.h"

class ipod_remove_files : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_remove_files);

	ipod_file_remover m_remover;
	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_remove_files");
		bool b_started = false, b_need_to_update_database=false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, 20, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			//m_library.hint_foobar2000(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;
			m_process.advance_progresstep();
			m_process.update_progress_subpart_helper(0,1);
			m_remover.run(m_drive_scanner.m_ipods[0], m_items, m_library, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_process.checkpoint();
			b_need_to_update_database = false;
			if (m_remover.m_deleted_items.get_count())
			{
				m_library.update_smart_playlists();
				m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			}
			m_process.advance_progresstep();

			/*if (m_remover.m_error_list.get_count())
			{
				pfc::string8_fast_aggressive buffer;
				t_size i, count = m_remover.m_error_list.get_count();
				for (i=0; i<count; i++)
				{
					buffer << m_remover.m_error_list[i];
					if (i+1 <count) buffer << "\n";
				}
				fbh::show_info_box_threadsafe("Errors occurred removing files from your iPod", buffer, OIC_WARNING);
			}*/
		}
		catch (exception_aborted &) 
		{
			if (b_need_to_update_database)
			{
				try 
				{
					m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,abort_callback_dummy());
				}
				catch (const pfc::exception & e) 
				{
					fbh::show_info_box_threadsafe("Error", e.what());
				}
			}
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
			m_failed = true;
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
	virtual void on_exit()
	{
		file_move_helper::g_on_deleted(m_remover.m_deleted_items);
		if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
		{
			if (m_remover.m_errors.get_count())
			{
				results_viewer::g_run(L"Warnings - Remove Files from iPod", m_remover.m_errors);
			}
		}
	}
private:
	ipod_remove_files(const pfc::list_base_const_t<metadb_handle_ptr> & items)
		: m_failed(false), m_items(items), ipod_write_action_v2_t("Remove Files from iPod")
	{};
	bool m_failed;
	metadb_handle_list m_items;
};





#endif //_DOP_REMOVE_H_