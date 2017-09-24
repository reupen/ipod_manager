#include "main.h"

#include "browse.h"
#include "item_properties.h"
#include "smart_playlist_editor.h"
#include "smart_playlist_processor.h"

//#define uT(x) pfc::stringcvt::string_os_from_utf8(x).get_ptr()

class initquit_autoreg
{
public:
	virtual void on_init() 
	{
	};
	virtual void on_quit() 
	{
	};
	initquit_autoreg();
	~initquit_autoreg();
private:
};

class initquit_multiplexer  : public initquit
{
public:
	void register_callback(initquit_autoreg * ptr)
	{
		m_callbacks.add_item(ptr);
	}
	void deregister_callback(initquit_autoreg * ptr)
	{
		m_callbacks.remove_item(ptr);
	}
private:
	void on_init() 
	{
		t_size i, count = m_callbacks.get_count();
		for (i=0; i<count; i++)
		{
			m_callbacks[i]->on_init();
		}
	};
	void on_quit() 
	{
		t_size i, count = m_callbacks.get_count();
		for (i=0; i<count; i++)
		{
			m_callbacks[i]->on_quit();
		}
	};
	pfc::ptr_list_t<initquit_autoreg> m_callbacks;
};

initquit_factory_t<initquit_multiplexer> g_initquit_multiplexer;

initquit_autoreg::initquit_autoreg()
{
	g_initquit_multiplexer.get_static_instance().register_callback(this);
}

initquit_autoreg::~initquit_autoreg()
{
	g_initquit_multiplexer.get_static_instance().deregister_callback(this);
}

class ipod_browse_dialog : public pfc::refcounted_object_root, initquit_autoreg
{
public:
	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	BOOL DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

	ipod::tasks::load_database_t m_library;
	ipod::tasks::drive_scanner_t m_drive_scanner;

	in_ipod_sync m_sync;

	ipod_browse_dialog(t_browser * pbrowser)
		: m_library(pbrowser->m_library), m_drive_scanner(pbrowser->m_drive_scanner),
		m_wnd(NULL), m_imagelist(NULL), m_debug(pbrowser->m_debug)
	{
		m_this=this;
	};
	pfc::refcounted_object_ptr_t<ipod_browse_dialog> m_this;

	void on_quit()
	{
		if (m_wnd)
			SendMessage(m_wnd, WM_CLOSE, NULL, NULL);
	}

	HWND m_wnd;
private:

	class ipod_tree_entry_t : public pfc::refcounted_object_root
	{
	public:
		typedef ipod_tree_entry_t self_t;
		typedef pfc::refcounted_object_ptr_t<self_t> ptr_t;
		enum entry_type_t
		{
			type_library,
			type_folder,
			type_playlist
		};
		entry_type_t m_type;
		pfc::rcptr_t< itunesdb::t_playlist > m_playlist;
		pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > m_tracks;
		metadb_handle_list_t<pfc::alloc_fast_aggressive> m_handles;

		ipod_tree_entry_t() : m_type(type_library) {};
	private:
	};	
	
	void refresh_song_list(const ipod_tree_entry_t * p_selection);

	class tree_builder_t : public tree_builder_base_t
	{
	public:
		tree_builder_t( const ipod::tasks::load_database_t & p_library, const pfc::rcptr_t< itunesdb::t_playlist> & root_playlist,
			pfc::list_t<ipod_tree_entry_t::ptr_t> & p_nodes_receiver, 
			const pfc::list_base_t< pfc::rcptr_t< itunesdb::t_playlist> > & playlists = pfc::list_t< pfc::rcptr_t<itunesdb::t_playlist> >())
			: tree_builder_base_t(p_library, root_playlist, playlists), m_nodes_receiver(p_nodes_receiver) {};

		HTREEITEM insert_item_in_tree(HWND wnd_tree, const pfc::rcptr_t<itunesdb::t_playlist> & p_playlist, HTREEITEM ti_parent)
		{
			ipod_tree_entry_t::ptr_t node = new ipod_tree_entry_t;
			node->m_playlist = p_playlist;
			m_nodes_receiver.add_item(node);
			HTREEITEM ti = NULL;
			if (node->m_playlist->folder_flag)
			{
				ti = uih::tree_view_insert_item_simple(wnd_tree, p_playlist->name.is_empty() ? "<Unnanmed>" : p_playlist->name, (LPARAM)node.get_ptr(), TVIS_EXPANDED, ti_parent, TVI_LAST, true, 2);
				node->m_type = ipod_tree_entry_t::type_folder;
			}
			else
			{
				ti = uih::tree_view_insert_item_simple(wnd_tree, p_playlist->name.is_empty() ? "<Unnanmed>" : p_playlist->name, (LPARAM)node.get_ptr(), TVIS_EXPANDED, ti_parent, TVI_LAST, true, ti_parent == TVI_ROOT ? 0 : 1);
				node->m_type = ti_parent == TVI_ROOT ? ipod_tree_entry_t::type_library : ipod_tree_entry_t::type_playlist;
				g_playlist_get_tracks(node->m_playlist, m_library, node->m_tracks, node->m_handles);
			}
			return ti;
		}
	private:
		pfc::list_t<ipod_tree_entry_t::ptr_t> & m_nodes_receiver;
	};

	void on_size(unsigned cx, unsigned cy);
	void on_size();

	ipod_tree_entry_t * get_active_node();

	pfc::list_t<ipod_tree_entry_t::ptr_t> m_tree_entries;
	HIMAGELIST m_imagelist;
	bool m_debug;
};

class ipod_browse_writer : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_browse_writer);

	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_browse_writer");
		bool b_started = false;
		try 
		{
			t_uint32 steps[] = {30,10};
			m_process.set_steps(steps, tabsize(steps));
			DOP_TRACK_ACTION;
			initialise();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			m_process.checkpoint();
			b_started = true;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
		}
		catch (exception_aborted &) 
		{
			if (!b_started)
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
			fbh::show_info_box_threadsafe("Error",e.what());
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
private:
	ipod_browse_writer(ipod_browse_dialog * pbrowser) : ipod_write_action_v2_t("Save Changes to iPod")
	{
		m_drive_scanner = pbrowser->m_drive_scanner;
		m_library = pbrowser->m_library;
	};
	//t_browser * m_browser;////
};


class rename_param
{
public:
	modal_dialog_scope m_scope;
	pfc::string8 * m_text;
};

static BOOL CALLBACK g_RenameProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			rename_param * ptr = (rename_param *)lp;
			ptr->m_scope.initialize(FindOwningPopup(wnd));
			//uSetWindowText(wnd,uStringPrintf("Rename playlist: \"%s\"",ptr->m_text->get_ptr()));
			uSetDlgItemText(wnd,IDC_EDIT,ptr->m_text->get_ptr());
		}
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				rename_param * ptr = (rename_param *)GetWindowLong(wnd,DWL_USER);
				uGetDlgItemText(wnd,IDC_EDIT,*ptr->m_text);
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

bool g_rename(pfc::string8 * text,HWND parent)
{
	rename_param param;
	param.m_text = text;
	return !!uDialogBox(IDD_RENAME_PLAYLIST,parent,g_RenameProc,(LPARAM)(&param));
}


void t_browser::on_exit() 
{
	if (!m_process.get_abort().is_aborting() && !m_failed)
	{
		sync.release();
		pfc::refcounted_object_ptr_t<ipod_browse_dialog> p_test = new ipod_browse_dialog(this);
		HWND wnd = uCreateDialog(IDD_BROWSE, core_api::get_main_window(), ipod_browse_dialog::g_DialogProc, (LPARAM)p_test.get_ptr());
		ShowWindow(wnd, SW_SHOWNORMAL);
		////m_this = this;
	}
}

void t_browser::on_run()
{
	m_failed = false;
	try {
		t_uint32 steps[] = {1, 10};
		m_process.set_steps(steps, tabsize(steps));
		m_drive_scanner.run(m_process,m_process.get_abort());
		m_process.advance_progresstep();
		m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
		m_library.update_smart_playlists();
		m_process.advance_progresstep();
	}
	catch (exception_aborted) {}
	catch (const pfc::exception & e) 
	{
		fbh::show_info_box_threadsafe("Error",e.what());
		m_failed = true;
	}
}


BOOL CALLBACK ipod_browse_dialog::g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	ipod_browse_dialog * p_this = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(wnd, DWL_USER, lp);
		p_this = reinterpret_cast<ipod_browse_dialog*>(lp);
		break;
	default:
		p_this = reinterpret_cast<ipod_browse_dialog*>(GetWindowLongPtr(wnd, DWL_USER));
		break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

void ipod_browse_dialog::on_size(unsigned cx, unsigned cy)
{
	HWND wnd_list = GetDlgItem(m_wnd, IDC_IPOD_TREE);
	HWND wnd_lv = GetDlgItem(m_wnd, IDC_SONGS);
	HWND wnd_close = GetDlgItem(m_wnd, IDCANCEL);
	HWND wnd_ok = GetDlgItem(m_wnd, IDOK);
	HDWP dwp = BeginDeferWindowPos(4);
	RECT rc_list, rc_close, rc_ok;
	GetWindowRect(wnd_list, &rc_list);
	GetWindowRect(wnd_close, &rc_close);
	GetWindowRect(wnd_ok, &rc_ok);
	unsigned cx_list = rc_list.right - rc_list.left;
	unsigned cy_close = rc_close.bottom - rc_close.top;
	unsigned cx_close = rc_close.right - rc_close.left;
	unsigned cx_ok = rc_ok.right - rc_ok.left;
	const int outter_spacing = 11, inner_spacing = 7;
	dwp = DeferWindowPos(dwp, wnd_lv, NULL, cx_list+inner_spacing+outter_spacing, outter_spacing, cx-cx_list-inner_spacing-outter_spacing*2, cy-cy_close-inner_spacing-outter_spacing*3-1, SWP_NOZORDER);
	dwp = DeferWindowPos(dwp, wnd_list, NULL, outter_spacing, outter_spacing, cx_list, cy-cy_close-inner_spacing-outter_spacing*3-1, SWP_NOZORDER);
	dwp = DeferWindowPos(dwp, wnd_close, NULL, cx-cx_close-outter_spacing, cy-cy_close-outter_spacing, cx_close, cy_close, SWP_NOZORDER);
	dwp = DeferWindowPos(dwp, wnd_ok, NULL, cx-cx_close*2-outter_spacing-inner_spacing, cy-cy_close-outter_spacing, cx_close, cy_close, SWP_NOZORDER);
	RedrawWindow(m_wnd, NULL, NULL, RDW_INVALIDATE);
	EndDeferWindowPos(dwp);
	RedrawWindow(m_wnd, NULL, NULL, RDW_UPDATENOW);
}

void ipod_browse_dialog::on_size()
{
	RECT rc;
	GetClientRect(m_wnd, &rc);
	on_size (RECT_CX(rc), RECT_CY(rc));
}

void g_playlist_get_tracks (const pfc::rcptr_t<itunesdb::t_playlist> & p_playlist, const ipod::tasks::load_database_t & p_library,  pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > & p_out, metadb_handle_list_t<pfc::alloc_fast_aggressive> & p_handles)
{
	p_out.remove_all();
	p_handles.remove_all();
		t_size count_tracks = p_library.m_tracks.get_count(), count = p_playlist->items.get_count(), i;
		p_out.prealloc(count);
		p_handles.prealloc(count);

		mmh::Permutation permutation(count_tracks);

		mmh::sort_get_permutation(p_library.m_tracks.get_ptr(), permutation, ipod::tasks::load_database_t::g_compare_track_id, false);
		pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, const mmh::Permutation & > sorted_array(p_library.m_tracks, permutation);

		mmh::InversePermutation permutation_inverse(permutation);

		mmh::Permutation permutation_position(count);

		if (p_playlist->sort_direction) permutation_position.reset_reverse();

		//mmh::sort_get_permutation(p_playlist->items.get_ptr(), permutation_position, t_playlist_entry::g_compare_position, false);

		for (i=0;i<count;i++)
		{
			t_uint32 index;
			if (pfc::bsearch_t(count_tracks,sorted_array,ipod::tasks::load_database_t::g_compare_track_id_with_id,p_playlist->items[permutation_position[i]].track_id,index))
			{
				p_out.add_item(p_library.m_tracks[permutation_inverse[index]]);
				p_handles.add_item(p_library.m_handles[permutation_inverse[index]]);
			}
		}
}

void ipod_browse_dialog::refresh_song_list(const ipod_tree_entry_t * p_selection)
{
	HWND wnd_lv = GetDlgItem(m_wnd, IDC_SONGS);
	ListView_DeleteAllItems(wnd_lv);
	if (p_selection)
	{
		SendMessage(wnd_lv, WM_SETREDRAW, FALSE, 0);

		t_size i, count=p_selection->m_tracks.get_count();

		for (i=0;i<count;i++)
		{
			{
				itunesdb::t_track & track = *p_selection->m_tracks[i];
				t_size ci = 0;
				if (m_debug) uih::list_view_insert_item_text(wnd_lv, i, ci++, pfc::format_int(track.pid), false, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.artist, m_debug, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.title, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.album, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.genre, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.composer, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, pfc::string8() << track.year, true, i);
				if (m_debug) uih::list_view_insert_item_text(wnd_lv, i, ci++, pfc::format_int(track.mhii_id), true, i);
				if (m_debug) uih::list_view_insert_item_text(wnd_lv, i, ci++, pfc::format_hexdump_lowercase(track.artwork_source_sha1, track.artwork_source_sha1_valid ? 20 : 0, ""), true, i);
				if (m_debug) uih::list_view_insert_item_text(wnd_lv, i, ci++, pfc::format_int(track.album_id), true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.sort_artist, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.sort_title, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.sort_album, true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.sort_album_artist, true, i);
				std::wstring formatted_date_released;
				if (track.datereleased) g_format_date(filetime_time_from_appletime(track.datereleased, false), formatted_date_released);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, formatted_date_released.data(), true, i);
				t_uint32 gapless = track.gapless_heuristic_info;
				pfc::string8 temp;
				if (gapless == 0x1)
					temp << "accurate";
				else if (gapless == 0x2000003)
					temp = "estimated";
				else if (gapless)
					temp << "unknown";
				uih::list_view_insert_item_text(wnd_lv, i, ci++, temp, true, i);
				if (track.encoder_delay)
					uih::list_view_insert_item_text(wnd_lv, i, ci, pfc::string8() << track.encoder_delay, true, i);
				ci++;
				if (track.encoder_padding)
					uih::list_view_insert_item_text(wnd_lv, i, ci, pfc::string8() << track.encoder_padding, true, i);
				ci++;
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.artwork_flag == 0x1 && track.artwork_count ? "yes" : "", true, i);
				uih::list_view_insert_item_text(wnd_lv, i, ci++, track.original_path_valid ? "yes" : "", true, i);
			}
		}
		SendMessage(wnd_lv, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(wnd_lv,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
	}
}

ipod_browse_dialog::ipod_tree_entry_t * ipod_browse_dialog::get_active_node()
{
	ipod_tree_entry_t * ret = NULL;
	HWND wnd_tree = GetDlgItem(m_wnd, IDC_IPOD_TREE);
	HTREEITEM treeitem = TreeView_GetSelection(wnd_tree);
	if (treeitem)
	{
		TVITEMEX tvi;
		memset(&tvi, 0, sizeof(tvi));
		tvi.mask = TVIF_HANDLE|TVIF_PARAM;
		tvi.hItem = treeitem;
		TreeView_GetItem(wnd_tree, &tvi);
		if (tvi.lParam)
			ret = (ipod_tree_entry_t*)tvi.lParam;
	}
	return ret;
}

BOOL ipod_browse_dialog::DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_wnd = wnd;
			modeless_dialog_manager::g_add(wnd);
			HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
			HWND wnd_tree = GetDlgItem(wnd, IDC_IPOD_TREE);
			TreeView_SetItemHeight(wnd_tree, TreeView_GetItemHeight(wnd_tree)+4);
			uih::tree_view_set_explorer_theme(wnd_tree);

			SendMessage(wnd_list, LB_SETITEMHEIGHT, 0, SendMessage(wnd_list, LB_GETITEMHEIGHT, 0, 0)+3);

			HWND wnd_lv = GetDlgItem(wnd, IDC_SONGS);
			ListView_SetExtendedListViewStyleEx(wnd_lv, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
			uih::list_view_set_explorer_theme(wnd_lv);
			

			LVCOLUMN lvc;
			memset(&lvc, 0, sizeof(LVCOLUMN));
			lvc.mask = LVCF_TEXT|LVCF_WIDTH;

			t_size ci = 0;
			if (m_debug) uih::list_view_insert_column_text(wnd_lv, ci++, _T("PID"), 150);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Artist"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Title"), 200);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Album"), 150);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Genre"), 90);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Composer"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Year"), 100);
			if (m_debug) uih::list_view_insert_column_text(wnd_lv, ci++, _T("Artwork cache ID"), 95);
			if (m_debug) uih::list_view_insert_column_text(wnd_lv, ci++, _T("Artwork hash"), 253);
			if (m_debug) uih::list_view_insert_column_text(wnd_lv, ci++, _T("Album ID"), 55);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Sort artist"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Sort title"), 200);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Sort album"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Sort album artist"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Release date"), 100);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Gapless data"), 75);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Encoder delay"), 80);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Encoder padding"), 95);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("Artwork"), 70);
			uih::list_view_insert_column_text(wnd_lv, ci++, _T("In DopDB"), 70);
			//uih::list_view_insert_column_text(wnd_lv, 8, _T("Debug"), 200);

			{
				HICON ico_ipod=NULL, ico_playlist=NULL, ico_folder=NULL;
				ico_ipod = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_IPODFLAT), IMAGE_ICON, 16, 16, NULL);
				ico_playlist = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_PLAYLIST), IMAGE_ICON, 16, 16, NULL);
				ico_folder = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_FOLDER), IMAGE_ICON, 16, 16, NULL);
				
				m_imagelist = ImageList_Create(16, 16, ILC_COLOR32, 3, 0);
				ImageList_AddIcon(m_imagelist, ico_ipod);
				ImageList_AddIcon(m_imagelist, ico_playlist);
				ImageList_AddIcon(m_imagelist, ico_folder);

				DestroyIcon(ico_ipod);
				DestroyIcon(ico_playlist);
				DestroyIcon(ico_folder);
			}
			TreeView_SetImageList(wnd_tree, m_imagelist, TVSIL_NORMAL);

			tree_builder_t (m_library, m_library.m_library_playlist, m_tree_entries, m_library.m_playlists).run(wnd_tree);

			on_size();
		}
		break;
	case WM_CLOSE:
		DestroyWindow(wnd);
		m_this.release();
		return 0;
	case WM_DESTROY:
		modeless_dialog_manager::g_remove(wnd);
		SetWindowLongPtr(wnd, DWL_USER, NULL);
		m_wnd = NULL;
		m_tree_entries.remove_all();
		break;
	case WM_NCDESTROY:
		TreeView_SetImageList(GetDlgItem(wnd, IDC_IPOD_TREE), NULL, TVSIL_NORMAL);
		if (m_imagelist)
		{
			ImageList_Destroy(m_imagelist);
			m_imagelist=NULL;
		}
		////m_this.release();
		break;
	case WM_CONTEXTMENU:
		if ((HWND)wp == GetDlgItem(wnd, IDC_IPOD_TREE))
		{
			if (GetFocus() != (HWND)wp)
				SetFocus((HWND)wp);
			POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

			HTREEITEM treeitem_context = 0;

			HWND wnd_tree = (HWND)wp;
			
			TVHITTESTINFO ti;
			memset(&ti,0,sizeof(ti));
			
			if (pt.x != -1 && pt.y != -1)
			{
				ti.pt = pt;
				ScreenToClient(wnd_tree,&ti.pt);
				uSendMessage(wnd_tree,TVM_HITTEST,0,(long)&ti);
				if (ti.hItem && (ti.flags & TVHT_ONITEM))
				{
					SendMessage(wnd_tree,TVM_SELECTITEM,TVGN_CARET,(long)ti.hItem);
					treeitem_context = ti.hItem;
				}
			}
			else
			{
				treeitem_context = TreeView_GetSelection(wnd_tree);

				RECT rc;
				if (treeitem_context && TreeView_GetItemRect(wnd_tree, treeitem_context, &rc, TRUE))
				{
					MapWindowPoints(wnd_tree, HWND_DESKTOP, (LPPOINT)&rc, 2);

					pt.x = rc.left;
					pt.y = rc.top + (rc.bottom - rc.top) / 2;

				}
				else
				{
					GetMessagePos(&pt);
				}
			}

			ipod_tree_entry_t::ptr_t p_node;
			if (treeitem_context)
			{
				TVITEMEX tvi;
				memset(&tvi, 0, sizeof(tvi));
				tvi.mask = TVIF_HANDLE|TVIF_PARAM;
				tvi.hItem = treeitem_context;
				TreeView_GetItem(wnd_tree, &tvi);
				if (tvi.lParam)
					p_node = (ipod_tree_entry_t*)tvi.lParam;
			}

			if (p_node.is_valid())
			{
				enum {ID_NEW_SMART = 1, ID_EDIT_SMART, ID_REMOVE, ID_NEW_FOLDER};
				HMENU menu = CreatePopupMenu();
				if (p_node.is_valid())
					AppendMenu(menu, MF_STRING, ID_NEW_SMART, _T("New Smart Playlist..."));
				if (p_node.is_valid() && p_node->m_playlist->smart_data_valid && p_node->m_type != ipod_tree_entry_t::type_folder)
					AppendMenu(menu, MF_STRING, ID_EDIT_SMART, _T("Edit Smart Playlist..."));
				if (p_node.is_valid() && p_node->m_type != ipod_tree_entry_t::type_library)
					AppendMenu(menu, MF_STRING, ID_REMOVE, _T("Remove"));

				if (m_drive_scanner.m_ipods.get_count() && m_drive_scanner.m_ipods[0]->supports_playlist_folders() &&
					p_node.is_valid() && (p_node->m_type == ipod_tree_entry_t::type_library || p_node->m_type == ipod_tree_entry_t::type_folder))
					AppendMenu(menu, MF_STRING, ID_NEW_FOLDER, _T("New Folder..."));

				menu_helpers::win32_auto_mnemonics(menu);

				int cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,0,wnd,0);
				DestroyMenu(menu);
				if (cmd == ID_NEW_SMART)
				{
					pfc::string8 name = "New Smart Playlist";
					if (g_rename(&name, wnd))
					{ 
						if (name.is_empty())
							name = "New Smart Playlist";
						//t_uint64 pid = m_library.get_new_playlist_pid();
						t_smart_playlist_editor editor;
						itunesdb::t_smart_playlist_rule rule;
						rule.field = itunesdb::smart_playlist_fields::artist;
						rule.action = (1<<24)|(1<<0);
						editor.m_rules.rules.add_item(rule);
						editor.init(m_library);
						if (uDialogBox(IDD_SMART, wnd, &t_smart_playlist_editor::g_DialogProc, (LPARAM)&editor))
						{
							t_uint64 parentid = NULL;
							if (p_node->m_type == ipod_tree_entry_t::type_folder)
								parentid = p_node->m_playlist->id;
							t_size index = m_library.add_playlist(name, NULL, 0, parentid);
							m_library.m_playlists[index]->smart_data_valid = true;
							m_library.m_playlists[index]->smart_rules_valid = true;
							m_library.m_playlists[index]->smart_playlist_data = editor.m_data;
							m_library.m_playlists[index]->smart_playlist_rules = editor.m_rules;
							m_library.m_playlists[index]->sort_order = editor.m_sort_order;
							m_library.m_playlists[index]->sort_direction = editor.m_sort_direction?1:0;

							ipod::smart_playlist::generator_t(m_library).run(*m_library.m_playlists[index]);
							
							if (index == m_library.m_playlists.get_count()-1)
							{
								HTREEITEM ti_parent = treeitem_context;
								if (p_node->m_type == ipod_tree_entry_t::type_playlist)
									ti_parent = TreeView_GetParent(wnd_tree, ti_parent);
								pfc::list_t< pfc::rcptr_t< itunesdb::t_playlist > > playlists;
								playlists.add_item(m_library.m_playlists[index]);
								tree_builder_t (m_library, pfc::rcptr_t< itunesdb::t_playlist >(), m_tree_entries, playlists).run(wnd_tree, ti_parent);
							}
							//if (ListBox_GetCurSel(wnd_list) == index+1)
							//{
							//	SendMessage(wnd, WM_COMMAND, IDC_LIST|(LBN_SELCHANGE<<16), (LPARAM)wnd_list);
							//}
						}
					}
				}
				else if (cmd == ID_NEW_FOLDER)
				{
					pfc::string8 name = "New Folder";
					if (g_rename(&name, wnd))
					{ 
						if (name.is_empty())
							name = "New Folder";
						{
							t_size index = m_library.add_playlist(name, NULL, 0);

							if (index == m_library.m_playlists.get_count()-1)
							{
								m_library.m_playlists[index]->folder_flag = 1;
								if (p_node->m_type == ipod_tree_entry_t::type_folder)
									m_library.m_playlists[index]->parentid = p_node->m_playlist->id;

								m_library.m_playlists[index]->smart_data_valid = true;
								m_library.m_playlists[index]->smart_rules_valid = true;
								m_library.m_playlists[index]->smart_playlist_rules.rule_operator = 1;

								HTREEITEM ti_parent = treeitem_context;
								pfc::list_t< pfc::rcptr_t< itunesdb::t_playlist > > playlists;
								playlists.add_item(m_library.m_playlists[index]);
								tree_builder_t (m_library, pfc::rcptr_t< itunesdb::t_playlist >(), m_tree_entries, playlists).run(wnd_tree, ti_parent);
							}
							//if (ListBox_GetCurSel(wnd_list) == index+1)
							//{
							//	SendMessage(wnd, WM_COMMAND, IDC_LIST|(LBN_SELCHANGE<<16), (LPARAM)wnd_list);
							//}
						}
					}
				}
				else if (cmd == ID_EDIT_SMART)
				{
					t_smart_playlist_editor editor(m_library, p_node->m_playlist->smart_playlist_data, p_node->m_playlist->smart_playlist_rules, p_node->m_playlist->sort_order, p_node->m_playlist->sort_direction!=0, p_node->m_playlist->id);
					if (uDialogBox(IDD_SMART, wnd, &t_smart_playlist_editor::g_DialogProc, (LPARAM)&editor))
					{
						p_node->m_playlist->smart_playlist_data = editor.m_data;
						p_node->m_playlist->smart_playlist_rules = editor.m_rules;
						p_node->m_playlist->sort_order = editor.m_sort_order;
						p_node->m_playlist->sort_direction = editor.m_sort_direction?1:0;
						ipod::smart_playlist::generator_t(m_library).run(*p_node->m_playlist);
						g_playlist_get_tracks(p_node->m_playlist, m_library, p_node->m_tracks, p_node->m_handles);
						refresh_song_list(get_active_node());
						//m_library.m_playlists[sel-1].items.remove_all();
					}
				}
				else if (cmd == ID_REMOVE)
				{
					t_size i, count=m_library.m_playlists.get_count();
					for (i=0; i<count; i++)
					{
						if (m_library.m_playlists[i] == p_node->m_playlist)
						{
							m_library.remove_playlist(i);
							m_library.update_smart_playlists();
							TreeView_DeleteItem(wnd_tree, treeitem_context);
							m_tree_entries.remove_item(p_node);
							refresh_song_list(get_active_node());
							break;
						}
					}
				}
			}


		}
		else if ((HWND)wp == GetDlgItem(wnd, IDC_SONGS))
		{
			POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
			int sel = ListView_GetNextItem((HWND)wp,-1,LVNI_SELECTED);
			int sub = 0;
			bool hit = false;
			if (sel != -1)
			{
				if (pt.x == -1 && pt.y == -1)
				{
					RECT rc;
					ListView_GetItemRect((HWND)wp, sel, &rc, LVIR_BOUNDS);
					pt.x = 0;
					pt.y = rc.bottom;
					MapWindowPoints((HWND)wp, HWND_DESKTOP, (LPPOINT)&pt, 1);
					hit = true;
				}
				else
				{
					LVHITTESTINFO hti;
					memset(&hti, 0, sizeof(hti));
					hti.pt = pt;
					ScreenToClient((HWND)wp, &hti.pt);
					hit = ((ListView_HitTest((HWND)wp, &hti) != -1 && (hti.flags & LVHT_ONITEM)));
					if (hit)
					{
						sel = hti.iItem;
						sub = hti.iSubItem;
					}
				}
			}

			ipod_tree_entry_t * p_selection = get_active_node();

			if (hit && p_selection)
			{
				if (sel >= 0 && (t_size)sel < p_selection->m_tracks.get_count())
				{


					HMENU menu = CreatePopupMenu();
					AppendMenu(menu, MF_STRING, 1, _T("Item Details"));
					AppendMenu(menu, MF_STRING, 2, _T("Open File Location"));
					{
						MENUITEMINFO mi;
						memset(&mi, 0, sizeof(mi));
						mi.cbSize = sizeof(MENUITEMINFO);
						mi.fMask = MIIM_STATE;
						mi.fState = MFS_DEFAULT;
						SetMenuItemInfo(menu, 1, FALSE, &mi);
					}

					menu_helpers::win32_auto_mnemonics(menu);

					int cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,0,wnd,0);
					DestroyMenu(menu);
					if (cmd == 1)
					{
						item_info_dialog_v2_t::g_run(wnd, p_selection->m_tracks[sel]);
					}
					else if (cmd == 2)
					{
						/*
						pfc::string8 temp = m_library.m_tracks[lvi.lParam]->location, path;
						temp.replace_byte(':', '\\');
						path.add_byte(m_drive_scanner.m_ipods[0]->drive);
						path << ":" << temp;
						//SHOpenFolderAndSelectItems*/

						metadb_handle_list temp;
						temp.add_item(p_selection->m_handles[sel]);
						standard_commands::context_file_open_directory(temp);

					}
				}
			}
			return 0;
		}
		break;
	case WM_COMMAND:
		switch (wp)
		{
		case IDCANCEL:
			SendMessage(wnd, WM_CLOSE, 0, 0);
			return 0;
		case IDOK:
			{
				m_sync.release();
				ipod_browse_writer::g_run(core_api::get_main_window(),this);
				SendMessage(wnd, WM_CLOSE, 0, 0);
				//run(core_api::get_main_window());
			}
			return 0;
		}
		break;
	case WM_SIZE:
		{
			on_size(LOWORD(lp), HIWORD(lp));
		}
		return 0;
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = LPNMHDR(lp);
			switch (lpnm->idFrom)
			{
				case IDC_SONGS:
				{
					switch (lpnm->code)
					{
					case NM_DBLCLK:
						{
							ipod_tree_entry_t * p_selection = get_active_node();
							LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lp;
							if (lpnmitem->iItem != -1 && p_selection && lpnmitem->iItem>=0 && (t_size)lpnmitem->iItem < p_selection->m_tracks.get_count())
							{
								item_info_dialog_v2_t::g_run(wnd, p_selection->m_tracks[lpnmitem->iItem]);
							}
						}
						break;
					};
				}
				break;
			case IDC_IPOD_TREE:
				{
					if (lpnm->code == TVN_SELCHANGED)
					{
						LPNMTREEVIEW param = (LPNMTREEVIEW)lpnm;
						ipod_tree_entry_t * p_selection = (ipod_tree_entry_t*)param->itemNew.lParam;

						if (param->action == TVC_BYMOUSE || param->action == TVC_BYKEYBOARD)
						{
							refresh_song_list(p_selection);
						}
					}
					/*else if (hdr->code == NM_DBLCLK)
					{
						HTREEITEM treeitem_context = TreeView_GetSelection(m_wnd_tree);
						if (treeitem_context)
						{
							TVITEMEX tvi;
							memset(&tvi, 0, sizeof(tvi));
							tvi.mask = TVIF_HANDLE|TVIF_PARAM;
							tvi.hItem = treeitem_context;
							TreeView_GetItem(m_wnd_tree, &tvi);
							node_t * p_selection = (node_t*)tvi.lParam;
							if (p_selection)
								send_node_to_autosend_playlist_read_cache(p_selection);
						}
					}*/
				}
				break;
			}
		}
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(wnd, &ps);
			if (dc)
			{
				RECT rc_client, rc_button;
				GetClientRect(wnd, &rc_client);
				RECT rc_fill = rc_client;
				HWND m_wnd_button = GetDlgItem(wnd, IDCANCEL);
				if (m_wnd_button)
				{
					GetWindowRect(m_wnd_button, &rc_button);
					rc_fill.bottom -= RECT_CY(rc_button)+7;
					rc_fill.bottom -= 11+1;
				}

				FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_WINDOW));

				if (m_wnd_button)
				{
					rc_fill.top=rc_fill.bottom;
					rc_fill.bottom+=1;
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DLIGHT));
				}
				rc_fill.top=rc_fill.bottom;
				rc_fill.bottom=rc_client.bottom;
				FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DFACE));

				EndPaint(wnd, &ps);
			}
		}
		return 0;
	}
	return FALSE;
}



