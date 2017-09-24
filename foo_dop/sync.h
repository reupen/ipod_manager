#ifndef _SYNC_DOP_H_
#define _SYNC_DOP_H_

#include "actions_base.h"

void g_EjectIPodDelayed();

class ipod_send_playlist : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_send_playlist);

	ipod::tasks::check_files_in_library_t m_checker;
	ipod_add_files m_adder;
	t_field_mappings m_mappings;
	t_scan_items m_item_scanner;

	class t_playlist
	{
	public:
		pfc::string8 name;
		metadb_handle_list items;
	};

	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_send_playlist");
		bool b_started = false, b_need_to_update_database=false;
		m_failed = false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, 5, 40, 40, 1, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			if (!prepare(m_process,m_process.get_abort()))
				m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;
			m_process.advance_progresstep();
			metadb_handle_list handles;
			t_size k, count_playlists=m_playlists.get_count();
			for (k=0; k<count_playlists; k++)
			{
				//m_playlists[k].items.remove_duplicates();
				handles.add_items(m_playlists[k].items);
			}
			//m_items.remove_duplicates();
			g_load_info(m_process.get_wnd(), handles, m_process);
			m_checker.run(m_drive_scanner.m_ipods[0], handles, m_library, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_item_scanner.run(m_drive_scanner.m_ipods[0], handles, m_checker, m_library, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_adder.run(m_drive_scanner.m_ipods[0], m_item_scanner.m_new_tracks, m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_process.update_progress_subpart_helper(0,1);
			m_process.update_text("Building playlists");
			t_size i, count,j=0,base=0;
			for (k=0; k<count_playlists; k++)
			{
				count = m_playlists[k].items.get_count();
				pfc::list_t<t_uint32, pfc::alloc_fast_aggressive> items;
				items.prealloc(count);
				for (i=0;i<count;i++)
				{
					if (m_checker.m_result[i+base].have)
						items.add_item(m_checker.m_result[i+base].index);
					else 
					{
						if (j >= m_adder.m_results.get_count())
							throw pfc::exception_bug_check();
						if (m_adder.m_results[j].b_added)
						{
							items.add_item(m_adder.m_results[j].index);
						}
						j++;
					}
				}
				m_library.add_playlist(m_playlists[k].name, items.get_ptr(), items.get_count());
				base += count;
			}
			m_process.advance_progresstep();
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();

			/*if (m_adder.m_error_list.get_count())
			{
				pfc::string8_fast_aggressive buffer;
				t_size i, count = m_adder.m_error_list.get_count();
				for (i=0; i<count; i++)
				{
					buffer << m_adder.m_error_list[i];
					if (i+1 <count) buffer << "\n";
				}
				fbh::show_info_box_threadsafe("Errors occurred updating your iPod", buffer, OIC_WARNING);
			}*/
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
#if 0
			try 
			{
				abort_callback_impl p_dummy_abort;
				t_size i, count = m_adder.m_added_items.get_count();
				for (i=0; i<count; i++)
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
				for (i=0; i<count; i++)
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
	void on_exit()
	{
		if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
		{
			if (m_adder.m_errors.get_count())
			{
				results_viewer::g_run(L"Warnings - Send Playlists to iPod", m_adder.m_errors);
			}
		}
	}
	ipod_send_playlist(const pfc::list_base_const_t<t_playlist> & playlists)
		: m_failed(false), ipod_write_action_v2_t("Send Playlists to iPod")
	{
		m_playlists.add_items(playlists);
	}
	bool m_failed;

	pfc::list_t<t_playlist> m_playlists;

	//pfc::string8 m_playlist;
	//metadb_handle_list m_items;
};

class t_main_thread_sync_confirm : public main_thread_callback
{
public:
	t_main_thread_sync_confirm(const ipod::tasks::load_database_t & p_library,const t_scan_items & p_actions,HWND p_parent_window) 
		: m_library(p_library), m_parent_window(p_parent_window), m_exitcode(0), m_item_actions(p_actions)
	{
		m_signal.create(true, false);
	};

	static BOOL CALLBACK g_dlg_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	virtual void callback_run()
	{
		m_exitcode = uDialogBox(IDD_SYNC_CONFIRM, m_parent_window, g_dlg_proc, (LPARAM)this);
		m_signal.set_state(true);
	}
	win32_event m_signal;
	const ipod::tasks::load_database_t & m_library;
	const t_scan_items & m_item_actions;
	HWND m_parent_window;
	int m_exitcode;
};

class ipod_update_playback_data
{
public:
	void run (ipod_device_ptr_ref_t p_ipod, const ipod::tasks::check_files_in_library_t & p_checker, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort)
	{
		titleformat_object::ptr to_rating, to_playcount, to_lastplayed;
		static_api_ptr_t<titleformat_compiler>()->compile_force(to_rating, "[%rating%]");
		static_api_ptr_t<titleformat_compiler>()->compile_force(to_playcount, "[%play_count%]");
		static_api_ptr_t<titleformat_compiler>()->compile_force(to_lastplayed, "[%last_played_timestamp%]");

		t_size i, count = items.get_count();
		for (i=0; i<count; i++)
		{
			if (p_checker.m_result[i].have && items[i]->is_info_loaded_async() && p_mappings.m_media_library.bsearch_by_pointer(items[i]) != pfc_infinite)
			{
				{
					itunesdb::t_track & p_track = *p_library.m_tracks[p_checker.m_result[i].index];

					pfc::string8 temp;
					items[i]->format_title(NULL, temp, to_rating, NULL);
					p_track.rating = mmh::strtoul_n(temp.get_ptr(), pfc_infinite)*20;

					items[i]->format_title(NULL, temp, to_playcount, NULL);
					if (temp.length())
						p_track.playcount = mmh::strtoul_n(temp.get_ptr(), pfc_infinite);

					items[i]->format_title(NULL, temp, to_lastplayed, NULL);
					if (temp.length())
						p_track.lastplayedtime = apple_time_from_filetime(mmh::strtoul64_n(temp.get_ptr(), pfc_infinite) );
				}
			}
		}
	}
	ipod_update_playback_data()
	{
		core_api::ensure_main_thread();
	};
private:
	pfc::string8 m_artwork_script;
};


class ipod_sync : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM_3(ipod_sync);

	ipod::tasks::check_files_in_library_t m_checker;
	ipod_file_remover m_remover;
	ipod_add_files m_adder;
	ipod_update_playback_data m_update_playback_data;
	t_field_mappings m_mappings;
	t_scan_items m_item_scanner;

	class t_playlist
	{
	public:
		pfc::string8 name;
		metadb_handle_list items;
	};

	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_sync");
		bool b_started = false, b_need_to_update_database=false;
		m_failed = false;
		try 
		{
			t_uint32 steps[] = {1, 5, 30, 1, 10, 50, 1, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			if (!prepare(m_process,m_process.get_abort()))
				m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_process.advance_progresstep();

			metadb_handle_list handles;
			t_size k, count_playlists=m_playlists.get_count();
			for (k=0; k<count_playlists; k++)
			{
				//m_playlists[k].items.remove_duplicates();
				handles.add_items(m_playlists[k].items);
			}
			handles.add_items(m_items);
			//m_items.remove_duplicates();
			g_load_info(m_process.get_wnd(), handles, m_process);

			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;

			bool b_stopped = false;

			m_checker.run(m_drive_scanner.m_ipods[0], handles, m_library, m_process,m_process.get_abort());
			m_item_scanner.run(m_drive_scanner.m_ipods[0], handles, m_checker, m_library, m_process,m_process.get_abort());
			
			m_update_playback_data.run(m_drive_scanner.m_ipods[0], m_checker, handles, m_library, m_mappings, m_process,m_process.get_abort());

			if (!m_mappings.quiet_sync)
			{
				m_process.update_text("Waiting");
				service_ptr_t<t_main_thread_sync_confirm> p_info_loader = new service_impl_t<t_main_thread_sync_confirm>
					(m_library, m_item_scanner, m_process.get_wnd());
				static_api_ptr_t<main_thread_callback_manager> p_main_thread;
				p_main_thread->add_callback(p_info_loader);
				if (!p_info_loader->m_signal.wait_for(-1))
					throw pfc::exception("User feedback timeout!");

				if (!p_info_loader->m_exitcode)
					b_stopped = true;
			}

			if (b_stopped)
			{
				m_process.advance_progresstep(4);
				m_failed = true;
			}
			else
			{
				m_process.advance_progresstep();
				m_remover.run(m_drive_scanner.m_ipods[0], m_item_scanner.m_tracks_to_remove.get_ptr(), m_library, m_process,m_process.get_abort());
				t_size i, count,j=0,base=0;
				//count = m_item_scanner.m_tracks_to_remove.get_size();
				t_size count_items = m_checker.m_result.get_size();
				for (i=0; i<count_items; i++)
				{
					t_size index = m_checker.m_result[i].index;
					for (k=0; k < index; k++)
						if (m_item_scanner.m_tracks_to_remove[k]) m_checker.m_result[i].index--;
				}
				m_process.advance_progresstep();
				m_adder.run(m_drive_scanner.m_ipods[0], m_item_scanner.m_new_tracks, m_library, m_mappings, m_process,m_process.get_abort());
				m_process.advance_progresstep();
				m_process.update_progress_subpart_helper(0,1);
				m_process.update_text("Building playlists");

				//Make all names unique
				for (t_size i = 0, count = m_playlists.get_count(); i<count; i++)
				{
					pfc::string8 original_name = m_playlists[i].name;
					pfc::string8 fixed_name = original_name;
					t_size k = 0;
					for (t_size j = i+1; j<count; j++)
					{
						if (!stricmp_utf8(fixed_name, m_playlists[j].name))
							fixed_name = original_name << " (" << (++k) << ")";
					}
					m_playlists[i].name = fixed_name;
				}

				pfc::array_t<bool> mask_remove_playlists;
				if (m_remove_playlists)
				{
					t_size u = m_library.m_playlists.get_count();
					mask_remove_playlists.set_size(u);
					for (; u; u--)
					{
						bool is_keep_type = (
								m_library.m_playlists[u-1]->folder_flag || m_library.m_playlists[u-1]->smart_rules_valid ||
								m_library.m_playlists[u-1]->smart_data_valid || m_library.m_playlists[u-1]->podcast_flag
							);
						mask_remove_playlists[u-1] = !is_keep_type;
						if (!is_keep_type)
							m_library.m_playlists[u-1]->items.remove_all();
					}
					for (t_size i = 0, count = m_playlists.get_count(); i<count; i++)
					{
					}
				}
				for (k=0; k<count_playlists; k++)
				{
					count = m_playlists[k].items.get_count();
					pfc::list_t<t_uint32, pfc::alloc_fast_aggressive> items;
					items.prealloc(count);
					for (i=0;i<count;i++)
					{
						if (m_checker.m_result[i+base].have)
							items.add_item(m_checker.m_result[i+base].index);
						else 
						{
							if (j >= m_adder.m_results.get_count())
								throw pfc::exception_bug_check();
							if (m_adder.m_results[j].b_added)
							{
								items.add_item(m_adder.m_results[j].index);
							}
							j++;
						}
					}
					bool b_found = false;
					for (t_size j=0, jcount = m_library.m_playlists.get_count(); j<jcount; j++)
					{
						if (mask_remove_playlists[j] && !stricmp_utf8(m_playlists[k].name, m_library.m_playlists[j]->name))
						{
							m_library.set_up_playlist(m_library.m_playlists[j], items.get_ptr(), items.get_count());
							b_found = true;
							mask_remove_playlists[j] = false;
							break;
						}
					}
					if (!b_found)
					{
						m_library.add_playlist(m_playlists[k].name, items.get_ptr(), items.get_count());
						mask_remove_playlists.append_single(false);
					}
					base += count;
				}
				if (m_remove_playlists)
					m_library.m_playlists.remove_mask(mask_remove_playlists.get_ptr());

				m_process.advance_progresstep();
			}
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();

			/*if (m_remover.m_error_list.get_count() || m_adder.m_error_list.get_count())
			{
				pfc::string8_fast_aggressive buffer;
				t_size i, count = m_remover.m_error_list.get_count();
				for (i=0; i<count; i++)
				{
					buffer << m_remover.m_error_list[i];
					//if (i+1 <count) 
						buffer << "\n";
				}
				count = m_adder.m_error_list.get_count();
				for (i=0; i<count; i++)
				{
					buffer << m_adder.m_error_list[i];
					if (i+1 <count) buffer << "\n";
				}
				fbh::show_info_box_threadsafe("Errors occurred updating your iPod", buffer, popup_message::icon_error);
			}*/
		}
		catch (exception_aborted) 
		{
			m_failed = true;
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
#if 0
			try 
			{
				abort_callback_impl p_dummy_abort;
				t_size i, count = m_adder.m_added_items.get_count();
				for (i=0; i<count; i++)
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
				for (i=0; i<count; i++)
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
	virtual void on_exit();
private:
	ipod_sync(const pfc::list_base_const_t<metadb_handle_ptr> & items, const pfc::list_base_const_t<t_playlist> & playlists,
		bool b_remove_playlists = true)
		: m_failed(false), m_items(items), m_remove_playlists(b_remove_playlists), ipod_write_action_v2_t("Synchronise iPod")
	{
		m_playlists.add_items(playlists);
	};
	bool m_failed;
	bool m_remove_playlists;
	metadb_handle_list m_items;
	pfc::list_t<t_playlist> m_playlists;
};

#if 0
class t_main_thread_select_playlists : public main_thread_callback
{
public:
	t_main_thread_select_playlists(const ipod::tasks::load_database_t & p_library,HWND p_parent_window) 
		: m_library(p_library), m_parent_window(p_parent_window), m_exitcode(0)
	{
		m_signal.create(true, false);
	};

	pfc::array_t<bool> m_checked;

	static BOOL CALLBACK g_select_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSetWindowLong(wnd,DWL_USER,lp);
			{
				t_main_thread_select_playlists * ptr = (t_main_thread_select_playlists *)lp;
				HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
				uih::list_view_set_explorer_theme(wnd_list);
				ListView_SetExtendedListViewStyleEx(wnd_list, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

				LVCOLUMN lvc;
				memset(&lvc, 0, sizeof(LVCOLUMN));
				lvc.mask = LVCF_TEXT|LVCF_WIDTH;
				lvc.pszText = _T("Playlist");
				lvc.cx = 500;

				ListView_InsertColumn(wnd_list, 0, &lvc);
				static_api_ptr_t<playlist_manager> api;

				LVITEM lvi;
				memset(&lvi, 0, sizeof(LVITEM));
				lvi.mask=LVIF_TEXT;
				t_size i, count=ptr->m_library.m_playlists.get_size();
				for (i=0;i<count;i++)
				{
					lvi.iItem = i;
					pfc::stringcvt::string_os_from_utf8 wide(ptr->m_library.m_playlists[i]->name);
					lvi.pszText = const_cast<TCHAR*>(wide.get_ptr());
					ListView_InsertItem(wnd_list, &lvi);
				}
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				{
					t_main_thread_select_playlists * ptr = (t_main_thread_select_playlists *)GetWindowLongPtr(wnd,DWL_USER);
					HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
					t_size i, count = ptr->m_checked.get_size();
					for (i=0; i<count; i++)
						ptr->m_checked[i] = ListView_GetCheckState(wnd_list, i) ? true : false;
					EndDialog(wnd,1);
				}
				break;
			case IDCANCEL:
				EndDialog(wnd,0);
				break;
			}
			break;
		case WM_CLOSE:
			EndDialog(wnd,0);
			break;
		}
		return 0;
	}
	virtual void callback_run()
	{
		m_checked.set_size(m_library.m_playlists.get_size());
		m_checked.fill_null();
		m_exitcode = uDialogBox(IDD_REMOVE_PLAYLISTS1, m_parent_window, g_select_proc, (LPARAM)this);
		m_signal.set_state(true);
	}
	win32_event m_signal;
	const ipod::tasks::load_database_t & m_library;
	HWND m_parent_window;
	bool m_show_errors;
	int m_exitcode;
};

class ipod_remove_playlists : public ipod_write_action_v2_t
{
public:
	virtual void on_run()
	{
		bool b_started = false, b_need_to_update_database=false;
		m_failed = false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, 1, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();

			m_process.update_text("Waiting for user feedback...");
			service_ptr_t<t_main_thread_select_playlists> p_info_loader = new service_impl_t<t_main_thread_select_playlists>
				(m_library, get_wnd());
			static_api_ptr_t<main_thread_callback_manager> p_main_thread;
			p_main_thread->add_callback(p_info_loader);
			if (!p_info_loader->m_signal.wait_for(-1))
				throw pfc::exception("User feedback timeout!");

			if (p_info_loader->m_exitcode)
			{

				pfc::array_t<itunesdb::t_playlist> newplaylists;

				t_size i = m_library.m_playlists.get_size();
				for (; i; i--)
				{
					if (p_info_loader->m_checked[i-1])
						m_library.m_playlists.remove_by_idx(i-1);//newplaylists.append_single(m_library.m_playlists[i]);
				}

				//m_library.m_playlists = newplaylists;

				m_process.advance_progresstep();
				m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
				m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
				b_started = true; b_need_to_update_database = true;
				m_process.advance_progresstep();
				m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
				m_process.advance_progresstep();
			}

		}
		catch (exception_aborted) 
		{
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
			m_failed = true;
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
	ipod_remove_playlists()
		: m_failed(false), ipod_write_action_v2_t("Remove Playlists from iPod", 5, flag_show_text|flag_show_progress_window|flag_show_button)
	{};
	bool m_failed;
};
#endif

void g_run_sync();
//void g_close_explorer_windows_for_drive(char drive);

class ipod_eject_t : public ipod_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_eject_t);

	void on_run() 
	{
		TRACK_CALL_TEXT("ipod_eject_t");
		try 
		{
			//t_uint32 steps[] = {1};
			//m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			if (m_drive_scanner.m_ipods.get_count())
			{
				//if (!m_drive_scanner.m_ipods[0]->mobile)
				//	g_close_explorer_windows_for_drive(m_drive_scanner.m_ipods[0]->drive);
				PNP_VETO_TYPE veto_type = PNP_VetoTypeUnknown;
				pfc::array_t<WCHAR> buff;
				buff.set_size(MAX_PATH+1);
				DEVINST dev = m_drive_scanner.m_ipods[0]->instance;
				if (dev == NULL)
					throw pfc::exception("Failed to get device instance handle.");
				buff.fill_null();//CR_REMOVE_VETOED; PNP_VetoOutstandingOpen
				CONFIGRET ret = CM_Request_Device_Eject(dev, &veto_type, buff.get_ptr(), MAX_PATH, NULL);
				if (ret == CR_SUCCESS)
					fbh::show_info_box_threadsafe("Eject iPod", "You may now safely remove your iPod.");
				else
					throw pfc::exception("Failed to eject iPod. Close any applications using the device and try again.");
			}
			//advance_progresstep();
		}
		catch (exception_aborted) 
		{
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
	}

private:
	ipod_eject_t() : ipod_action_v2_t("Eject iPod", threaded_process_v2_t::flag_position_bottom_right|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_progress_marquee) {};
};

class ipod_device_eject_t : public ipod_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_device_eject_t);
	void on_run() 
	{
		TRACK_CALL_TEXT("ipod_device_eject_t");
		try 
		{
			if (m_ipod.is_valid())
			{
				PNP_VETO_TYPE veto_type = PNP_VetoTypeUnknown;
				pfc::array_t<WCHAR> buff;
				buff.set_size(MAX_PATH+1);
				DEVINST dev = m_ipod->instance;
				if (dev == NULL)
					throw pfc::exception("Failed to get device instance handle.");
				buff.fill_null();//CR_REMOVE_VETOED; PNP_VetoOutstandingOpen
				CONFIGRET ret = CM_Request_Device_Eject(dev, &veto_type, buff.get_ptr(), MAX_PATH, NULL);
				if (ret == CR_SUCCESS)
					fbh::show_info_box_threadsafe("Eject iPod", "You may now safely remove your iPod.");
				else
					throw pfc::exception("Failed to eject iPod. Close any applications using the device and try again.");
			}
		}
		catch (exception_aborted const &) 
		{
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
	}

	ipod_device_ptr_t m_ipod;

	ipod_device_eject_t(ipod_device_ptr_cref_t p_device) 
		: ipod_action_v2_t("Eject iPod", threaded_process_v2_t::flag_position_bottom_right|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_progress_marquee),
		m_ipod(p_device)
	{};
};

#endif