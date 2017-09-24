#ifndef _DOP_GAPLESS_H_
#define _DOP_GAPLESS_H_

#include "actions_base.h"

class ipod_scan_gapless : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_scan_gapless);

	ipod::tasks::gapless_scanner_t m_scanner;
	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_scan_gapless");
		bool b_started = false, b_need_to_update_database=false;
		m_failed = false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, m_items.get_count(), 3};
			m_process.set_steps(steps, tabsize(steps));
			initialise();
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			t_uint32 index = m_library.m_tracks.get_count()-1;
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;
			//g_load_info(m_wnd, m_items, m_process);
			m_process.advance_progresstep();
			m_scanner.run(m_drive_scanner.m_ipods[0], m_items, m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
		}
		catch (exception_aborted) 
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
	virtual void on_exit();
private:
	ipod_scan_gapless(const pfc::list_base_const_t<metadb_handle_ptr> & items)
		: m_failed(false), m_items(items), ipod_write_action_v2_t("Scan Files on iPod for Gapless Data")
	{};
	bool m_failed;
	metadb_handle_list m_items;
};

#endif _DOP_GAPLESS_H_