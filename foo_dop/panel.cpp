#include "main.h"

class portable_devices_panel_t : public ui_extension::container_ui_extension
{
	class node_device_t : public pfc::refcounted_object_root
	{
	public:
		typedef node_device_t self_t;
		typedef pfc::refcounted_object_ptr_t<self_t> ptr;

		ipod_device_ptr_t m_ipod;
		bool m_isInfoLoaded;
		node_device_t() : m_isInfoLoaded(false) {};
	};
	class node_t : public pfc::refcounted_object_root
	{
	public:
		typedef node_t self_t;
		typedef pfc::refcounted_object_ptr_t<self_t> ptr;

		HTREEITEM m_treeitem;

		node_device_t::ptr m_device;
		metadb_handle_list_t<pfc::alloc_fast_aggressive> m_handles;
		bool m_isDevice;
		pfc::string8 m_name;
		node_t() : m_treeitem(NULL), m_isDevice(false) {};
	};
public:
	void set_clientedge(bool b_val) {m_clientedge = b_val;}
	typedef portable_devices_panel_t self_t;
	typedef service_ptr_t<portable_devices_panel_t> ptr;
	virtual class_data & get_class_data()const 
	{
		__implement_get_class_data(_T("{606E9CDD-45EE-4c3b-9FD5-49381CEBE8AE}"), false);
	}
	static const GUID extension_guid;

	virtual const GUID & get_extension_guid() const
	{
		return extension_guid;
	}

	virtual void get_name(pfc::string_base & out) const
	{
		out = "iPod devices";
	}
	virtual void get_category(pfc::string_base & out) const
	{
		out = "Panels";
	}
	unsigned get_type () const{return ui_extension::type_panel;}

	portable_devices_panel_t()
		: m_wnd_tree(NULL) , m_proc_tree(NULL), m_imagelist(NULL), m_clientedge(false)
		{};

	virtual void on_device_modified(ipod_device_ptr_cref_t p_ipod)
	{
		on_device_removal(p_ipod);
		on_device_arrival(p_ipod);
	}

	class tree_builder_t : public tree_builder_base_t
	{
	public:
		tree_builder_t(const node_device_t::ptr & p_device, const ipod::tasks::load_database_t & p_library, pfc::list_t<node_t::ptr> & p_nodes_receiver, 
			const pfc::rcptr_t< itunesdb::t_playlist> & root_playlist,
			const pfc::list_base_t< pfc::rcptr_t< itunesdb::t_playlist> > & playlists = pfc::list_t< pfc::rcptr_t<itunesdb::t_playlist> >())
			: tree_builder_base_t(p_library, root_playlist, playlists), m_nodes_receiver(p_nodes_receiver), m_device(p_device) {};

		HTREEITEM insert_item_in_tree(HWND wnd_tree, const pfc::rcptr_t<itunesdb::t_playlist> & p_playlist, HTREEITEM ti_parent)
		{
			node_t::ptr node = new node_t;
			node->m_device = m_device;
			node->m_isDevice=ti_parent == TVI_ROOT;
			node->m_name = p_playlist->name;
			m_nodes_receiver.add_item(node);
			HTREEITEM ti = NULL;
			if (p_playlist->folder_flag)
			{
				ti = uTreeView_InsertItemSimple(wnd_tree, p_playlist->name.is_empty() ? "<Unnamed>" : p_playlist->name, (LPARAM)node.get_ptr(), TVIS_EXPANDED, ti_parent, TVI_LAST, true, 2);
				//node->m_type = ipod_tree_entry_t::type_folder;
			}
			else
			{
				ti = uTreeView_InsertItemSimple(wnd_tree, p_playlist->name.is_empty() ? "<Unnamed>" : p_playlist->name, (LPARAM)node.get_ptr(), TVIS_EXPANDED, ti_parent, TVI_LAST, true, ti_parent == TVI_ROOT ? 0 : 1);
				//node->m_type = ti_parent == TVI_ROOT ? ipod_tree_entry_t::type_library : ipod_tree_entry_t::type_playlist;
				pfc::list_t<pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > tracks;
				g_playlist_get_tracks(p_playlist, m_library, tracks, node->m_handles);
			}
			node->m_treeitem = ti;
			return ti;
		}
	private:
		pfc::list_t<node_t::ptr> & m_nodes_receiver;
		node_device_t::ptr m_device;
	};

	virtual void on_device_arrival(ipod_device_ptr_cref_t p_ipod)
	{
		insync (p_ipod->m_database_sync);
		{
			node_device_t::ptr device = new node_device_t;
			device->m_ipod = p_ipod;

			BOOL b_disable_redraw = IsWindowVisible(m_wnd_tree);
			if (b_disable_redraw)
				SendMessage(m_wnd_tree, WM_SETREDRAW, FALSE, NULL);

			tree_builder_t(device, p_ipod->m_database, m_nodes, p_ipod->m_database.m_library_playlist, p_ipod->m_database.m_playlists).run(m_wnd_tree);

			if (b_disable_redraw)
				SendMessage(m_wnd_tree, WM_SETREDRAW, TRUE, NULL);
		}
	}
	virtual void on_device_removal(ipod_device_ptr_cref_t p_ipod)
	{
		t_size i = m_nodes.get_count();
		for (;i;i--)
		{
			if (m_nodes[i-1]->m_device->m_ipod == p_ipod)
			{
				if (m_nodes[i-1]->m_isDevice)
					TreeView_DeleteItem(m_wnd_tree, m_nodes[i-1]->m_treeitem);
				m_nodes.remove_by_idx(i-1);
				//break;
			}
		}
	}
	virtual void on_shutdown()
	{
		if (m_wnd_tree)
		{
			TreeView_DeleteAllItems(m_wnd_tree);
		}
		m_nodes.remove_all();
	}
	LRESULT WINAPI on_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		static bool process_char = true;
		uie::window_ptr p_this;

		switch(msg)
		{
		case WM_KEYDOWN:
			p_this = this;
			if (get_host()->get_keyboard_shortcuts_enabled() && wp != VK_LEFT && wp != VK_RIGHT && g_process_keydown_keyboard_shortcuts(wp)) {break;}
			else if (wp == VK_TAB)
			{
				ui_extension::window::g_on_tab(wnd);
			}
			break;
		case WM_SYSKEYDOWN:
			p_this = this;
			if (get_host()->get_keyboard_shortcuts_enabled() && wp != VK_LEFT && wp != VK_RIGHT && g_process_keydown_keyboard_shortcuts(wp)) {break;}
			break;
		}
		return uCallWindowProc(m_proc_tree,wnd,msg,wp,lp);
	}
	static LRESULT WINAPI g_on_hook(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		self_t * p_this;
		LRESULT rv;
		
		p_this = reinterpret_cast<self_t*>(uGetWindowLong(wnd,GWL_USERDATA));
		
		rv = p_this ? p_this->on_hook(wnd,msg,wp,lp) : uDefWindowProc(wnd, msg, wp, lp);

		return rv;
	}
	static void send_node_to_playlist(node_t * p_selection, const char * p_playlist)
	{
		static_api_ptr_t<playlist_manager_v3> api;
		t_size index = api->find_or_create_playlist_unlocked(p_playlist);
		api->playlist_undo_backup(index);
		api->playlist_remove_items(index, bit_array_true());
		if (settings::sort_ipod_library && p_selection->m_isDevice)
		{
			metadb_handle_list handles(p_selection->m_handles);
			titleformat_object::ptr to;
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(to, settings::ipod_library_sort_script);
			mmh::fb2k::g_sort_metadb_handle_list_by_format_v2(handles, to, NULL);
			api->playlist_insert_items(index, 0, handles, bit_array_false());
		}
		else
			api->playlist_insert_items(index, 0, p_selection->m_handles, bit_array_false());
		api->set_active_playlist(index);
	}
protected:
	class send_files_to_playlist_read_cache_v2 : public ipod_action_base_t
	{
	public:
		send_files_to_playlist_read_cache_v2() : m_InfoLoadSucceeded(false), 
			ipod_action_base_t("Load Playlist from iPod", threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_progress_marquee) {};

		void run(HWND wnd, node_t::ptr const & p_node, const char * p_playlist)
		{
			m_parent_window = wnd;
			m_node = p_node;
			m_playlist = p_playlist;
			m_InfoLoadSucceeded = false;

			if (m_node->m_device->m_isInfoLoaded)
			{
				portable_devices_panel_t::send_node_to_playlist(m_node.get_ptr(), m_playlist);
			}
			else
			{
				//threaded_process::g_run_modeless(this, threaded_process::flag_no_focus|threaded_process::flag_show_delayed, m_parent_window, "Load Playlist from iPod");
				if (sync.check_eating())
					ipod_action_base_t::run(wnd);
			}
		}

		virtual void on_init() 
		{
			m_node->m_device->m_ipod->get_root_path(m_path);
			m_path << "metadata_cache.fpl";
		}
		virtual void on_run()
		{
			service_ptr_t<playlist_loader_callback_dop> cache = new service_impl_t<playlist_loader_callback_dop>;
			bool cache_valid = true;

			try {
				playlist_loader::g_load_playlist(m_path, cache, m_process.get_abort());
				cache->hint_metadb();
			} catch (const exception_io_not_found &) {
				cache_valid=false;
			}
			catch (const pfc::exception & ex) {
				console::formatter() << "iPod manager: Error reading metadata cache: " << ex.what();
			}

			metadb_handle_list handlestoread;

			{
				insync (m_node->m_device->m_ipod->m_database_sync);
				handlestoread = (m_node->m_device->m_ipod->m_database.m_handles);
			}
			t_size n = handlestoread.get_count(), count = n;
			for (; n; n--)
			{
				{
					if (handlestoread[n-1]->is_info_loaded_async())
						handlestoread.remove_by_idx(n-1);
				}
			}
			if (handlestoread.get_count())
			{
				service_ptr_t<t_main_thread_scan_file_info> p_info_loader = new service_impl_t<t_main_thread_scan_file_info>
					(handlestoread, metadb_io::load_info_force, core_api::get_main_window());
				static_api_ptr_t<main_thread_callback_manager>()->add_callback(p_info_loader);
				if (p_info_loader->m_signal.wait_for(-1) && p_info_loader->m_ret != metadb_io::load_info_aborted)
				{
					m_InfoLoadSucceeded = true;
					try
					{
						insync(m_node->m_device->m_ipod->m_database_sync);
						playlist_loader::g_save_playlist(m_path, m_node->m_device->m_ipod->m_database.m_handles, abort_callback_dummy());
					}
					catch (const pfc::exception & ex)
					{
						console::formatter() << "iPod manager: Could not save metadata cache to iPod: " << ex.what();
					}
				}
			}
			else
			{
				m_InfoLoadSucceeded = true;
			}
		}
		virtual void on_exit() 
		{
			m_node->m_device->m_isInfoLoaded = m_InfoLoadSucceeded;
			portable_devices_panel_t::send_node_to_playlist(m_node.get_ptr(), m_playlist);
		}

		node_t::ptr m_node;
		HWND m_parent_window;
		pfc::string8 m_playlist;
		pfc::string8 m_path;
		bool m_InfoLoadSucceeded;
		in_ipod_sync sync;
	};

	void send_node_to_playlist_read_cache(node_t * p_selection, bool b_autosend_playlist = true)
	{
		pfc::refcounted_object_ptr_t<send_files_to_playlist_read_cache_v2> ptr = new send_files_to_playlist_read_cache_v2;
		pfc::string8 playlist;

		if (b_autosend_playlist)
			playlist = settings::devices_panel_autosend_playlist;
		else
			playlist << p_selection->m_name << " - iPod";

		ptr->run(core_api::get_main_window(), p_selection, playlist);
	}
	enum {IDC_TREE=1001};
	typedef struct _SHSTOCKICONINFO
	{
		DWORD cbSize;
		HICON hIcon;
		int   iSysImageIndex;
		int   iIcon;
		WCHAR szPath[MAX_PATH];
	} SHSTOCKICONINFO;

	typedef HRESULT (WINAPI * SHGetStockIconInfoProc)(UINT , UINT , SHSTOCKICONINFO *);

	LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_CREATE:
			{
				{
					HICON ico_ipod=NULL, ico_playlist=NULL, ico_folder=NULL;
					if (ico_ipod = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_IPODFLAT), IMAGE_ICON, 16, 16, NULL))
					{
						if (ico_playlist = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_PLAYLIST), IMAGE_ICON, 16, 16, NULL))
						{
							if (ico_folder = (HICON)LoadImage(core_api::get_my_instance(), MAKEINTRESOURCE(IDI_FOLDER), IMAGE_ICON, 16, 16, NULL))
							{
								if (m_imagelist = ImageList_Create(16, 16, ILC_COLOR32, 3, 0))
								{
									ImageList_AddIcon(m_imagelist, ico_ipod);
									ImageList_AddIcon(m_imagelist, ico_playlist);
									ImageList_AddIcon(m_imagelist, ico_folder);
								}
								DestroyIcon(ico_folder);
							}
							DestroyIcon(ico_playlist);
						}
						DestroyIcon(ico_ipod);
					}
				}

				m_wnd_tree = CreateWindowEx(m_clientedge ? WS_EX_CLIENTEDGE : WS_EX_STATICEDGE, WC_TREEVIEW, _T(""),
					TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | (TVS_NOHSCROLL ) | WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0,
					wnd, HMENU(IDC_TREE), core_api::get_my_instance(), NULL);
				TreeView_SetImageList(m_wnd_tree, m_imagelist, TVSIL_NORMAL);
				TreeView_SetItemHeight(m_wnd_tree, TreeView_GetItemHeight(m_wnd_tree)+3);
				
				SetWindowLongPtr(m_wnd_tree,GWL_USERDATA,(LPARAM)(this));
				m_proc_tree = (WNDPROC)SetWindowLongPtr(m_wnd_tree,GWL_WNDPROC,(LPARAM)(g_on_hook));

				g_set_treeview_window_explorer_theme(m_wnd_tree);
				m_font = uCreateIconFont();
				SendMessage(m_wnd_tree, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(TRUE,0));

				m_ipod_device_notification.m_receiver=this;
				g_drive_manager.register_callback(&m_ipod_device_notification);
			}
			break;
		case WM_SIZE:
			on_size(LOWORD(lp), HIWORD(lp));
			break;
		case WM_NOTIFY:
			{
				LPNMHDR hdr = (LPNMHDR)lp;

				switch (hdr->idFrom) 
				{

				case IDC_TREE:
					{
						if (hdr->code == TVN_SELCHANGED)
						{
							LPNMTREEVIEW param = (LPNMTREEVIEW)hdr;
							node_t * p_selection = (node_t*)param->itemNew.lParam;

							if (settings::devices_panel_autosend && (param->action == TVC_BYMOUSE || param->action == TVC_BYKEYBOARD) )
							{
								if (p_selection)
									send_node_to_playlist_read_cache(p_selection);
							}
						}
					}
					break;
				}

			} 
			break;
		case WM_DESTROY:
			g_drive_manager.deregister_callback(&m_ipod_device_notification);
			m_ipod_device_notification.m_receiver.release();

			DestroyWindow(m_wnd_tree);
			m_wnd_tree=NULL;
			m_font.release();

			m_nodes.remove_all();
			if (m_imagelist)
			{
				ImageList_Destroy(m_imagelist);
				m_imagelist=NULL;
			}
			break;
		case WM_CONTEXTMENU:
			{
				enum { ID_EJECT=1, ID_AUTOSEND=2, ID_SEND=3 };

				HMENU menu = CreatePopupMenu();

				POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

				HWND list = m_wnd_tree;
				
				HTREEITEM treeitem_context = 0;
				
				TVHITTESTINFO ti;
				memset(&ti,0,sizeof(ti));
				
				if (pt.x != -1 && pt.y != -1)
				{
					ti.pt = pt;
					ScreenToClient(list,&ti.pt);
					uSendMessage(list,TVM_HITTEST,0,(long)&ti);
					if (ti.hItem && (ti.flags & TVHT_ONITEM))
					{
						SendMessage(list,TVM_SELECTITEM,TVGN_CARET,(long)ti.hItem);
						treeitem_context = ti.hItem;
					}
				}
				else
				{
					treeitem_context = TreeView_GetSelection(list);

					RECT rc;
					if (treeitem_context && TreeView_GetItemRect(m_wnd_tree, treeitem_context, &rc, TRUE))
					{
						MapWindowPoints(m_wnd_tree, HWND_DESKTOP, (LPPOINT)&rc, 2);

						pt.x = rc.left;
						pt.y = rc.top + (rc.bottom - rc.top) / 2;

					}
					else
					{
						GetMessagePos(&pt);
					}
				}

				node_t::ptr p_node;
				if (treeitem_context)
				{
					TVITEMEX tvi;
					memset(&tvi, 0, sizeof(tvi));
					tvi.mask = TVIF_HANDLE|TVIF_PARAM;
					tvi.hItem = treeitem_context;
					TreeView_GetItem(list, &tvi);
					if (tvi.lParam)
						p_node = (node_t*)tvi.lParam;
				}
				
				if (p_node.is_valid())
				{
					AppendMenu(menu,MF_STRING,ID_AUTOSEND,L"Send to autosend playlist");
					AppendMenu(menu,MF_STRING,ID_SEND,L"Send to playlist");
					if (p_node->m_isDevice)
					{
						AppendMenu(menu,MF_SEPARATOR,NULL,NULL);
						AppendMenu(menu,MF_STRING,ID_EJECT,L"Eject");
					}
				}
				
				int cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,0,get_wnd(),0);
				DestroyMenu(menu);
				
				if (cmd)
				{
					if (cmd == ID_EJECT)
					{
						if (p_node.is_valid() && p_node->m_isDevice && p_node->m_device->m_ipod.is_valid())
						{
							ipod_device_eject_t::g_run(core_api::get_main_window(),p_node->m_device->m_ipod);
						}
					}
					else if (cmd == ID_AUTOSEND || cmd == ID_SEND)
					{
						if (p_node.is_valid())
							send_node_to_playlist_read_cache(p_node.get_ptr(), cmd == ID_AUTOSEND);
					}
				}
			}
			return 0;
		}
		return DefWindowProc(wnd,msg,wp,lp);
	}
private:
	void on_size(unsigned cx, unsigned cy)
	{
		SetWindowPos(m_wnd_tree, NULL, 0, 0, cx, cy, SWP_NOZORDER);
	}

	HWND m_wnd_tree;
	gdi_object_t<HFONT>::ptr_t m_font;

	pfc::list_t<node_t::ptr> m_nodes;

	ipod_device_notification_maintthread_t<portable_devices_panel_t> m_ipod_device_notification;

	WNDPROC m_proc_tree;
	HIMAGELIST m_imagelist;
	bool m_clientedge;
};

// {1E2DC783-1BE1-40b0-9790-971F3B9DA447}
const GUID portable_devices_panel_t::extension_guid = 
{ 0x1e2dc783, 0x1be1, 0x40b0, { 0x97, 0x90, 0x97, 0x1f, 0x3b, 0x9d, 0xa4, 0x47 } };

uie::window_factory<portable_devices_panel_t> g_portable_devices_panel;



//namespace {

class popup_window_t : public ui_helpers::container_window_release_t
{
public:
	popup_window_t()
		: m_wnd_child(NULL), m_wnd_button(NULL) {};
	void run()
	{
		if (get_wnd())
		{
			SetWindowPos(get_wnd(), HWND_TOP, 0, 0, 0, 0,SWP_NOSIZE|SWP_NOMOVE);
		}
		else
		{
			RECT rc;
			GetClientRect(core_api::get_main_window(), &rc);
			MapWindowRect(core_api::get_main_window(), HWND_DESKTOP, &rc);
			int cx = 400;
			int cy = 375;
			HWND wnd;
			if (wnd = create(core_api::get_main_window(),0,ui_helpers::window_position_t((rc.left), (rc.top), cx, cy)))
			{
				ShowWindow(wnd, SW_SHOWNORMAL);
			}
		}
	}
private:
	class window_host_devices : public ui_extension::window_host
	{
	public:

		virtual const GUID & get_host_guid()const
		{
			// {5E47F5EA-A883-412d-BEA6-B3AE494ED382}
			static const GUID ret = 
			{ 0x5e47f5ea, 0xa883, 0x412d, { 0xbe, 0xa6, 0xb3, 0xae, 0x49, 0x4e, 0xd3, 0x82 } };
			return ret;
		}

		virtual void on_size_limit_change(HWND wnd, unsigned flags)
		{
		};
		virtual bool get_keyboard_shortcuts_enabled()const
		{
			return false;
		}

		
		virtual bool override_status_text_create(service_ptr_t<ui_status_text_override> & p_out)
		{
			static_api_ptr_t<ui_control> api;
			return api->override_status_text_create(p_out);
		}

		virtual unsigned is_resize_supported(HWND wnd)const
		{
			return false;
		}

		virtual bool request_resize(HWND wnd, unsigned flags, unsigned width, unsigned height)
		{
			bool rv = false;
			return rv;
		}
		virtual bool is_visible(HWND wnd) const
		{
			bool rv = false;
			return  rv;
		}
		virtual bool is_visibility_modifiable(HWND wnd, bool desired_visibility)const
		{
			bool rv = false;
			return  rv;
		}
		virtual bool set_window_visibility(HWND wnd, bool visibility)
		{
			bool rv = false;
			return rv;
		}

		virtual void relinquish_ownership(HWND wnd)
		{
		}
		
	};
	class_data & get_class_data() const 
	{
		__implement_get_class_data_ex(_T("Dop_Devices_Popup"), _T("iPod Devices"), false, 0, WS_SYSMENU | WS_POPUP | WS_CLIPSIBLINGS| WS_CLIPCHILDREN  | WS_CAPTION | WS_THICKFRAME, WS_EX_DLGMODALFRAME, 0/*CS_VREDRAW*/);
	}
	virtual LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch (msg)
		{
		case WM_NCCREATE:
			modeless_dialog_manager::g_add(wnd);
			register_initquit();
			break;
		case WM_SIZE:
			{
				RedrawWindow(wnd, 0, 0, RDW_INVALIDATE);
				RECT rc;
				GetWindowRect(m_wnd_button, &rc);
				int cy_button = RECT_CY(rc);
				HDWP dwp = BeginDeferWindowPos(2);
				dwp = DeferWindowPos(dwp, m_wnd_child, NULL, 11+11, 11, LOWORD(lp)-11*4, HIWORD(lp)-11*4-1-cy_button, SWP_NOZORDER);
				dwp = DeferWindowPos(dwp, m_wnd_button, NULL, LOWORD(lp)-11-73-11, HIWORD(lp)-11-cy_button, 73, cy_button, SWP_NOZORDER);
				EndDeferWindowPos(dwp);
				RedrawWindow(wnd, 0, 0, RDW_UPDATENOW);
			}
			return 0;
		case WM_CREATE:
			{
				m_font = uCreateIconFont();
				RECT rc;
				GetClientRect(wnd, &rc);
				
				int cy_button = uGetFontHeight(m_font)+10;
				m_wnd_button = CreateWindowEx(0, WC_BUTTON, L"Close", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON|WS_GROUP, RECT_CX(rc)-11*2-73, RECT_CY(rc)-11-cy_button, 73, cy_button, wnd, (HMENU)IDCANCEL, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_button, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(FALSE,0));

				service_ptr_t<service_base> ptr;
				g_portable_devices_panel.instance_create(ptr);
				m_child = static_cast<portable_devices_panel_t *>(ptr.get_ptr());
				m_child->set_clientedge(true);
				m_wnd_child = m_child->create_or_transfer_window(wnd, new service_impl_t<window_host_devices>, ui_helpers::window_position_null/*, ui_helpers::window_position_t(rc)*/);
				ShowWindow(m_wnd_child, SW_SHOWNORMAL);
			}
			return 0;
		case WM_DESTROY:
			return 0;
		case WM_NCDESTROY:
			deregister_initquit();
			modeless_dialog_manager::g_remove(wnd);
			m_font.release();
			break;
		case WM_ERASEBKGND:
			return FALSE;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(wnd, &ps);
				if (dc)
				{
					RECT rc_client, rc_button;
					GetClientRect(wnd, &rc_client);
					RECT rc_fill = rc_client;
					if (m_wnd_button)
					{
						GetWindowRect(m_wnd_button, &rc_button);
						rc_fill.bottom -= RECT_CY(rc_button)+11;
						rc_fill.bottom -= 11+1;
					}

					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_WINDOW));

					if (m_wnd_button)
					{
						rc_fill.top=rc_fill.bottom;
						rc_fill.bottom+=1;
						FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DLIGHT));
					}
					rc_fill.top = rc_fill.bottom;
					rc_fill.bottom = rc_client.bottom;
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DFACE));
					EndPaint(wnd, &ps);
				}
			}
			return 0;
		case WM_COMMAND:
			switch (wp)
			{
			case IDCANCEL:
				SendMessage(wnd, WM_CLOSE, NULL, NULL);
				return 0;
			}
			break;
		case WM_CLOSE:
			destroy();
			m_child.release();
			m_wnd_child = NULL;
			m_wnd_button = NULL;
			return 0;
		}
		return DefWindowProc(wnd, msg, wp, lp);
	}
	service_ptr_t<portable_devices_panel_t> m_child;
	HWND m_wnd_child;
	HWND m_wnd_button;
	gdi_object_t<HFONT>::ptr_t m_font;
} g_popup_window;

void g_show_ipod_devices_popup()
{
	g_popup_window.run();
}

//};