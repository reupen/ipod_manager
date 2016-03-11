#include "main.h"

#if 0
void g_close_explorer_windows_for_drive(char drive)
{
	coinitialise_scope coinit(COINIT_MULTITHREADED);
	pfc::array_t<WCHAR> path;
	path.append_single(drive);
	path.append_fromptr(L":\\", 3);

	mmh::comptr_t<IRunningObjectTable> prot;

	HRESULT hr = GetRunningObjectTable(0, prot);

	if (SUCCEEDED(hr))

	{

		mmh::comptr_t<IMoniker> pmkFile;

		hr = CreateFileMoniker(path.get_ptr(), pmkFile);

		if (SUCCEEDED(hr))

		{

			mmh::comptr_t<IEnumMoniker> penumMk;

			hr = prot->EnumRunning(penumMk);

			if (SUCCEEDED(hr))

			{

				hr = E_FAIL;

				ULONG celt;

				mmh::comptr_t<IMoniker> pmk;

				while ((penumMk->Next(1, pmk, &celt) == S_OK))

				{

					LPOLESTR ppszDisplaynamefull;
					mmh::comptr_t<IBindCtx> pbcfull;  
					if (SUCCEEDED(CreateBindCtx( 0, pbcfull )))
					{
						if(SUCCEEDED(pmk->GetDisplayName(pbcfull,NULL,
							&ppszDisplaynamefull)))
						{
							console::formatter() << pfc::stringcvt::string_utf8_from_os(ppszDisplaynamefull);
						}
					}

					DWORD dwType;

					if (SUCCEEDED(pmk->IsSystemMoniker(&dwType)) &&

						(dwType == MKSYS_FILEMONIKER))

					{

						// Is this a moniker prefix?

						mmh::comptr_t<IMoniker> pmkPrefix;

						if (SUCCEEDED(pmkFile->CommonPrefixWith(pmk, pmkPrefix)))

						{

							//if (S_OK == pmkFile->IsEqual(pmkPrefix))

							{

								// Get the IFileIsInUse instance

								mmh::comptr_t<IUnknown> punk;

								if (prot->GetObject(pmk, punk) == S_OK)

								{

									mmh::comptr_t<IFileIsInUse> pfiu = punk;
									if (pfiu.is_valid())
									{
										hr = pfiu->CloseFile();
									}

								}

							}

						}

					}


				}

			}

		}

	}
}
#endif
// {0C6FBCEE-8EEC-4f87-8FE4-25C23D902136}
const GUID guid_playlist_sync_to_ipod = 
{ 0xc6fbcee, 0x8eec, 0x4f87, { 0x8f, 0xe4, 0x25, 0xc2, 0x3d, 0x90, 0x21, 0x36 } };


static BOOL CALLBACK g_sync_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		//uSetWindowLong(wnd,DWL_USER,lp);
		{
			SendDlgItemMessage(wnd, IDC_MEDIA_LIBRARY, BM_SETCHECK, settings::sync_library ? BST_CHECKED : BST_UNCHECKED, 0);
			//rename_param * ptr = (rename_param *)lp;
			Button_SetCheck(GetDlgItem(wnd, IDC_EJECT_WHEN_DONE), settings::sync_eject_when_done ? BST_CHECKED : NULL);

			HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
			uih::SetListViewWindowExplorerTheme(wnd_list);
			ListView_SetExtendedListViewStyleEx(wnd_list, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES);

			LVCOLUMN lvc;
			memset(&lvc, 0, sizeof(LVCOLUMN));
			lvc.mask = LVCF_TEXT|LVCF_WIDTH;
			lvc.pszText = _T("Name");
			lvc.cx = 500;

			ListView_InsertColumn(wnd_list, 0, &lvc);
			static_api_ptr_t<playlist_manager_v3> api;

			t_size i, count=api->get_playlist_count();

			LVITEM lvi;
			memset(&lvi, 0, sizeof(LVITEM));
			lvi.mask=LVIF_TEXT;

			for (i=0;i<count;i++)
			{
				lvi.iItem = i;
				pfc::string8 name;
				api->playlist_get_name(i, name);
				pfc::stringcvt::string_os_from_utf8 wide(name);
				lvi.pszText = const_cast<TCHAR*>(wide.get_ptr());
				//lvi.stateMask = LVIS_STATEIMAGEMASK;
				//lvi.state = INDEXTOSTATEIMAGEMASK((settings::sync_playlists.have_item(name))?2:1);
				int index = ListView_InsertItem(wnd_list, &lvi);

				if (!settings::sync_playlists_imported)
				{
					if (settings::sync_playlists.have_item(name))
						api->playlist_set_property_int<t_uint8>(i, guid_playlist_sync_to_ipod, 1);
				}

				t_uint8 b_val = false;

				if (api->playlist_get_property_int(i, guid_playlist_sync_to_ipod, b_val) && b_val)
					ListView_SetCheckState(wnd_list, index, TRUE);
			}
			settings::sync_playlists_imported = true;
		}
		return 1;
	case WM_CONTEXTMENU:
		if ((HWND)wp == GetDlgItem(wnd, IDC_LIST))
		{
			HMENU menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING, 1, _T("Select All"));
			AppendMenu(menu, MF_STRING, 2, _T("Deselect All"));
			int cmd = TrackPopupMenuEx(menu, TPM_RETURNCMD|TPM_RIGHTBUTTON, GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wnd, NULL);
			DestroyMenu(menu);

			if (cmd==1||cmd==2)
			{
				t_size n, count = ListView_GetItemCount(HWND(wp));
				for (n=0; n<count; n++)
					ListView_SetCheckState((HWND)wp, n, cmd==1)
			}
			return 0;
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
				bool b_media_library = SendMessage(GetDlgItem(wnd, IDC_MEDIA_LIBRARY), BM_GETCHECK, 0, 0) == BST_CHECKED;
				settings::sync_library = b_media_library;
				static_api_ptr_t<library_manager> library_api;
				metadb_handle_list_t<pfc::alloc_fast_aggressive> items;
				if (b_media_library)
					library_api->get_all_items(items);

				int i, count = ListView_GetItemCount(wnd_list);
				static_api_ptr_t<playlist_manager_v3> p_manager;
				pfc::list_t<ipod_sync::t_playlist> playlists;
				settings::sync_playlists.remove_all();
				if (count>0)
				{
					for (i=0; i<count; i++)
					{
						if (i >=0 && (t_size)i < p_manager->get_playlist_count())
						{
							if (ListView_GetCheckState(wnd_list, i))
							{
								ipod_sync::t_playlist playlist;
								p_manager->playlist_get_name(i, playlist.name);
								p_manager->playlist_get_all_items(i, playlist.items);
								p_manager->playlist_set_property_int<bool>(i, guid_playlist_sync_to_ipod, true);
								playlists.add_item(playlist);
							}
							else
								p_manager->playlist_remove_property(i, guid_playlist_sync_to_ipod);
						}
					}
				}
				ipod_sync::g_run(core_api::get_main_window(), items, playlists, true) ;
				//rename_param * ptr = (rename_param *)GetWindowLong(wnd,DWL_USER);
				EndDialog(wnd,1);
			}
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		case IDC_EJECT_WHEN_DONE:
			settings::sync_eject_when_done = Button_GetCheck((HWND)lp) == BST_CHECKED;
			break;
		}
		break;
	case WM_ERASEBKGND:
		SetWindowLongPtr(wnd, DWL_MSGRESULT, TRUE);
		return TRUE;
	case WM_PAINT:
		uih::HandleModernBackgroundPaint(wnd, GetDlgItem(wnd, IDOK));
		return TRUE;
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
		SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
		return (BOOL)GetSysColorBrush(COLOR_WINDOW);
	case WM_CLOSE:
		EndDialog(wnd,0);
		break;
	}
	return 0;
}

static UINT g_EjectTimerId = NULL;
void CALLBACK g_EjectTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	ipod_eject_t::g_run(core_api::get_main_window());
	KillTimer(NULL, g_EjectTimerId);
	g_EjectTimerId = NULL;
}

void g_EjectIPodDelayed()
{
	if (g_EjectTimerId == NULL)
		g_EjectTimerId = SetTimer(NULL, NULL, 2000, &g_EjectTimerProc);
}

void ipod_sync::on_exit()
	{
		file_move_helper::g_on_deleted(m_remover.m_deleted_items);
		if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
		{
			if (m_remover.m_errors.get_count() || m_adder.m_errors.get_count())
			{
				m_adder.m_errors.add_items(m_remover.m_errors);
				results_viewer::g_run(L"Warnings - Synchronise iPod", m_adder.m_errors);
			}
		}
		if (!m_failed)
		{
			if (settings::sync_eject_when_done)
			{
				g_EjectIPodDelayed();
			}
		}
	}


void g_run_sync()
{
	uDialogBox(IDD_SYNC, core_api::get_main_window(), g_sync_proc);
}


	BOOL CALLBACK t_main_thread_sync_confirm::g_dlg_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSetWindowLong(wnd,DWL_USER,lp);
			{
				t_main_thread_sync_confirm * ptr = (t_main_thread_sync_confirm *)lp;
				HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
				uih::SetListViewWindowExplorerTheme(wnd_list);
				ListView_SetExtendedListViewStyleEx(wnd_list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

				uih::ListView_InsertColumnText(wnd_list, 0, L"Artist", 150);
				uih::ListView_InsertColumnText(wnd_list, 1, L"Title", 250);
				uih::ListView_InsertColumnText(wnd_list, 2, L"Action", 100);
				static_api_ptr_t<playlist_manager> api;

				t_size i, count=ptr->m_item_actions.m_new_tracks.get_count(), count_nodups = count;
				bit_array_bittable mask(count);
				mmh::permutation_t permuation(count);
				mmh::g_sort_get_permutation_qsort_v2(ptr->m_item_actions.m_new_tracks.get_ptr(), permuation, pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>, true);
				for (t_size k=0; k+1 < count; k++)
				{
					if (ptr->m_item_actions.m_new_tracks[permuation[k]] == ptr->m_item_actions.m_new_tracks[permuation[k+1]])
					{
						mask.set(permuation[k+1], true);
						count_nodups--;
					}
				}
				{
					t_size j=0;
					for (i=0;i<count;i++)
					{
						if (!mask.get(i))
						{
							metadb_info_container::ptr p_info;
							if (ptr->m_item_actions.m_new_tracks[i]->get_async_info_ref(p_info))
							{
								pfc::string8 temp;
								g_print_meta(p_info->info(), "ARTIST", temp);
								uih::ListView_InsertItemText(wnd_list, j, 0, temp);
								g_print_meta(p_info->info(), "TITLE", temp);
								uih::ListView_InsertItemText(wnd_list, j, 1, temp, true);
							}
							else
								uih::ListView_InsertItemText(wnd_list, j, 0, pfc::string_filename(ptr->m_item_actions.m_new_tracks[i]->get_path()));
							uih::ListView_InsertItemText(wnd_list, j, 2, "Add", true);
							++j;
						}
					}
				}
				count = ptr->m_library.m_tracks.get_count();

				t_size count_remove = 0;
				{
					for (i=0;i<count;i++)
					{
						if (ptr->m_item_actions.m_tracks_to_remove[i])
						{
							t_size index = ListView_GetItemCount(wnd_list);
							uih::ListView_InsertItemText(wnd_list, index, 0, ptr->m_library.m_tracks[i]->artist);
							uih::ListView_InsertItemText(wnd_list, index, 1, ptr->m_library.m_tracks[i]->title, true);
							uih::ListView_InsertItemText(wnd_list, index, 2, "Remove", true);
							count_remove++;
						}
					}
				}

				uSetWindowText(GetDlgItem(wnd, IDC_TOTALADD), pfc::string8() << "Summary: " << count_nodups << " tracks to add, " << count_remove << " tracks to remove.");
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				EndDialog(wnd,1);
				break;
			case IDCANCEL:
				EndDialog(wnd,0);
				break;
			}
			break;
		case WM_ERASEBKGND:
			SetWindowLongPtr(wnd, DWL_MSGRESULT, TRUE);
			return TRUE;
		case WM_PAINT:
			uih::HandleModernBackgroundPaint(wnd, GetDlgItem(wnd, IDOK));
			return TRUE;
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
			SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		case WM_CLOSE:
			EndDialog(wnd,0);
			break;
		}
		return 0;
	}
