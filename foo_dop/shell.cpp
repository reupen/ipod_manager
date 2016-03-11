#include "main.h"

bool is_char_dir_sep (char c) {return c == '/' || c == '\\';}

class delete_task : public threaded_process_v2_t
{
public:
	delete_task(HWND wnd, const pfc::list_t<shell_window::item> & p_files)
		: m_wnd_shell_window(wnd), threaded_process_v2_t("Delete Files", threaded_process_v2_t::flag_show_progress_window), m_files(p_files) {};

private:
	void on_run()
	{
		for (t_size i = 0, count = m_files.get_count(); i<count; i++)
		{
			if (!m_files[i].m_is_virtual && !m_files[i].m_is_dir)
			{
				try {filesystem::g_remove(m_files[i].m_path, get_abort());}
				catch (pfc::exception const & ex) {m_error_log << "Error deleting \"" << m_files[i].m_path << "\" - " << ex.what() << "\r\n"; };
			}
		}
		PostMessage(m_wnd_shell_window, shell_window::MSG_REFRESH, NULL, NULL);
	}
	void on_exit() 
	{
		if (!m_error_log.is_empty()) popup_message::g_show(m_error_log, "Errors - Copy Files"); 
		delete this;
	}
	HWND m_wnd_shell_window;
	pfc::list_t<shell_window::item> m_files;
	pfc::string8 m_error_log;
};

	class directory_callback_impl_copy_no_overwrite : public directory_callback
	{
	public:
		directory_callback_impl_copy_no_overwrite(const char * p_target)
		{
			m_target = p_target;
			m_target.fix_dir_separator('\\');
		}

		bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats) {
			const char * fn = url + pfc::scan_filename(url);
			t_size truncat = m_target.length();
			m_target += fn;
			if (is_subdirectory) {
				try {
					filesystem::g_create_directory(m_target,p_abort);
				} catch(exception_io_already_exists) {}
				m_target += "\\";
				owner->list_directory(url,*this,p_abort);
			} else {
				try {
				if (filesystem::g_exists(m_target, p_abort))
					throw exception_io_already_exists();
				filesystem::g_copy(url,m_target,p_abort);
				} catch (pfc::exception const & ex) {m_error_log << "Error copying \"" << url << "\" - " << ex.what() << "\r\n";};
			}
			m_target.truncate(truncat);
			return true;
		}
		pfc::string8_fastalloc m_error_log;
	private:
		pfc::string8_fastalloc m_target;
	};


class copy_to_task : public threaded_process_v2_t
{
public:
	copy_to_task(HWND wnd, const char * p_dest, const pfc::list_t<shell_window::item> & p_files) 
		: m_files(p_files), m_destination(p_dest), m_wnd_shell_window(wnd), 
		threaded_process_v2_t("Copy Files", threaded_process_v2_t::flag_block_app_close|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_show_button|threaded_process_v2_t::flag_progress_marquee) {};

private:
	void on_run()
	{
		t_size len = m_destination.get_length();
		if (len && !is_char_dir_sep(m_destination[len-1])) m_destination.add_byte('\\');

		for (t_size i = 0, count = m_files.get_count(); i<count; i++)
		{
			if (!m_files[i].m_is_virtual)
			{
				try 
				{
					
					if (m_files[i].m_is_dir)
					{
						pfc::string8 outDir; outDir << m_destination << pfc::string_filename_ext(m_files[i].m_path);
						try {filesystem::g_create_directory(outDir, get_abort());} catch (exception_io_already_exists const &) {};
						directory_callback_impl_copy_no_overwrite dirListCopy(outDir);
						filesystem::g_list_directory(m_files[i].m_path,dirListCopy,get_abort());
						m_error_log << dirListCopy.m_error_log;
					}
					else
					{
						pfc::string8 dest; dest << m_destination << pfc::string_filename_ext(m_files[i].m_path);
						if (filesystem::g_exists(dest, get_abort()))
							throw exception_io_already_exists();
						filesystem::g_copy(m_files[i].m_path, dest, get_abort());
					}
				}
				catch (pfc::exception const & ex) {m_error_log << "Error copying \"" << m_files[i].m_path << "\" - " << ex.what() << "\r\n"; };
			}
		}
		//PostMessage(m_wnd_shell_window, shell_window::MSG_REFRESH, NULL, NULL);
	}
	void on_exit() 
	{
#if 0
		LPITEMIDLIST piidl = NULL;
		//memset(&iidl, 0, sizeof(iidl));
		DWORD flag = NULL;
		SHILCreateFromPath (pfc::stringcvt::string_wide_from_utf8(pfc::string8() << m_destination) , &piidl, &flag);
		SHOpenFolderAndSelectItems(piidl, 0, NULL, NULL);
		ILFree(piidl);
#endif

		if (!m_error_log.is_empty()) popup_message::g_show(m_error_log, "Errors - Copy Files"); 
		delete this;
	}
	HWND m_wnd_shell_window;
	pfc::list_t<shell_window::item> m_files;
	pfc::string8 m_destination;
	pfc::string8 m_error_log;
};

void shell_window::on_size(unsigned cx, unsigned cy)
{
	SetWindowPos(m_wnd_items_view, NULL, 0, 0, cx, cy, SWP_NOZORDER);
}

//typedef HRESULT (WINAPI * SHGetStockIconInfoProc)(UINT , UINT , SHSTOCKICONINFO *);

t_size g_path_find_next_dir_sep (const pfc::string8 & path, t_size start)
{
	t_size c1 = path.find_first('/', start);
	t_size c2 = path.find_first('\\', start);
	return min (c1, c2);
}
t_size g_path_find_previous_dir_sep (const pfc::string8 & path, t_size start)
{
	t_size c1 = path.find_last('/', start);
	t_size c2 = path.find_last('\\', start);
	return max (c1+1, c2+1)-1;
}
bool g_path_is_root(const pfc::string8 & path)
{
	t_size index = path.find_first(':');
	if (index != pfc_infinite)
	{
		while (is_char_dir_sep(path[index])) index++;
		index = g_path_find_next_dir_sep (path, index);
		if (index != pfc_infinite)
		{
			if (index+1 < path.get_length())
				return false;
		}
	}
	return true;
}

bool g_path_get_level_up(const pfc::string8 & path, pfc::string8 & p_out)
{
	t_size length = path.get_length();
	if (length == 0) return false;

	t_size index = length - 1;
	while (is_char_dir_sep(path[index]))
	{
		if (index == 0) return false;
		index--;
	}
	if (path[index] == ':') return false;
	index = g_path_find_previous_dir_sep(path, index);
	if (index == pfc_infinite) return false;
	p_out.set_string(path, index+1);
	return true;
}

void shell_window::disable ()
{
	ListView_DeleteAllItems(m_wnd_items_view);
	m_items.remove_all();
}

void shell_window::enable ()
{
	m_directory_reader.start(get_wnd(), m_directory);
}

void shell_window::refresh (const pfc::string8 & p_path)
{
	ListView_DeleteAllItems(m_wnd_items_view);
	m_items.remove_all();
	m_directory = p_path;
	m_directory_reader.start(get_wnd(), p_path);
}


void shell_window::populate ()
{
	if (!m_directory_reader.m_error.is_empty())
	{
		message_window_t::g_run(get_wnd(), "Error - File System Explorer", m_directory_reader.m_error);
		return;
	}
	ui_helpers::DisableRedrawScope drd(m_wnd_items_view);

	t_size j = -1;

	pfc::string8 parent;
	if (g_path_get_level_up(m_directory_reader.m_directory, parent))
	{
		SHFILEINFO shi;
		memset(&shi, 0, sizeof(shi));
		int image = I_IMAGENONE;
		if (SUCCEEDED(SHGetFileInfo(L"_", FILE_ATTRIBUTE_DIRECTORY, &shi, sizeof (shi), SHGFI_USEFILEATTRIBUTES|SHGFI_SYSICONINDEX)))
			image = shi.iIcon;
		uih::ListView_InsertItemText(m_wnd_items_view, ++j, 0, "..", false, NULL, image);
		m_items.add_item(item(parent, true, true));
	}

	directory_callback_simple & dirContents = m_directory_reader.m_data;

	pfc::list_t<t_list_view::t_item_insert> items;
	for (t_size i = 0, count = dirContents.get_count(); i<count; i++)
	{
		pfc::stringcvt::string_wide_from_utf8 filename(pfc::string_filename_ext(dirContents[i]).get_ptr());
		SHFILEINFO shi;
		memset(&shi, 0, sizeof(shi));
		int image = I_IMAGENONE;
		if (SUCCEEDED(SHGetFileInfo(filename, dirContents.get_item(i).m_is_dir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL, &shi, sizeof (shi), SHGFI_USEFILEATTRIBUTES|SHGFI_SYSICONINDEX)))
			image = shi.iIcon;
		uih::ListView_InsertItemText(m_wnd_items_view, ++j, 0, filename, false, NULL, image);
		if (dirContents.get_item(i).m_stats.m_timestamp != filetimestamp_invalid)
		{
			std::basic_string<TCHAR> str;
			uih::FormatDate(dirContents.get_item(i).m_stats.m_timestamp, str, true);
			uih::ListView_InsertItemText(m_wnd_items_view, j, 1, str.data(), true);
		}
		if (!dirContents.get_item(i).m_is_dir)
			uih::ListView_InsertItemText(m_wnd_items_view, j, 2, mmh::format_file_size(dirContents.get_item(i).m_stats.m_size), true);
		m_items.add_item(item(dirContents[i], dirContents.get_item(i).m_is_dir));
	}
}

LRESULT shell_window::on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg)
	{
		case WM_NCCREATE:
			modeless_dialog_manager::g_add(wnd);
			m_this = this;
			break;
		case WM_NCDESTROY:
			modeless_dialog_manager::g_remove(wnd);
			m_this.release();
			break;
		case WM_CREATE:
			{
				mmh::comptr_t<IImageList> pIL;
				HIMAGELIST il = NULL;
				if (SUCCEEDED(SHGetImageList(SHIL_SMALL, IID_IImageList, pIL)))
				{
					il = (HIMAGELIST)pIL.get_ptr();					
				}
					//ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 2, 0);
#if 0
				HINSTANCE inst = LoadLibrary(L"Shell32.dll");
				if (inst)
				{
					SHGetStockIconInfoProc p_SHGetStockIconInfo = (SHGetStockIconInfoProc)GetProcAddress(inst, "SHGetStockIconInfo");
					if (p_SHGetStockIconInfo)
					{
						SHSTOCKICONINFO ssii, ssii2;
						memset(&ssii, 0, sizeof(ssii));
						ssii.cbSize=sizeof(ssii);
						ssii2 = ssii;
						HICON ico_ipod=NULL, ico_playlist=NULL;
						if (SUCCEEDED(p_SHGetStockIconInfo(SIID_FOLDER, SHGSI_ICON|SHGSI_SMALLICON, &ssii))) 
						{
							ImageList_AddIcon(m_imagelist, ssii.hIcon);
							DestroyIcon(ssii.hIcon);
						};
						if (SUCCEEDED(p_SHGetStockIconInfo(SIID_DOCNOASSOC, SHGSI_ICON|SHGSI_SMALLICON, &ssii2))) 
						{
							ImageList_AddIcon(m_imagelist, ssii2.hIcon);
							DestroyIcon(ssii2.hIcon);
						};
						
					}
					FreeLibrary(inst);
				}
#endif

				m_wnd_items_view = CreateWindowEx(WS_EX_CONTROLPARENT, WC_LISTVIEW, L"", LVS_NOSORTHEADER | LVS_SHAREIMAGELISTS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD , 0, 0, 0, 0, wnd, (HMENU)1001, core_api::get_my_instance(), NULL);

				ListView_SetImageList(m_wnd_items_view, il, LVSIL_SMALL);
				ListView_SetExtendedListViewStyleEx(m_wnd_items_view, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
				uih::SetListViewWindowExplorerTheme(m_wnd_items_view);

				uih::ListView_InsertColumnText(m_wnd_items_view, 0, L"Name", 300);
				uih::ListView_InsertColumnText(m_wnd_items_view, 1, L"Date modified", 150);
				uih::ListView_InsertColumnText(m_wnd_items_view, 2, L"Size", 100);

				pfc::list_t<ipod_device_ptr_t> ipods;
				g_drive_manager.m_event_initialised.wait_for(5);
				g_drive_manager.get_drives(ipods);
				if (ipods.get_count())
				{
					pfc::string8 rootPath;
					ipods[0]->get_root_path(rootPath);

					refresh(rootPath);
					
					ui_helpers::popup_container_window::on_size();
					ShowWindow(m_wnd_items_view, SW_SHOWNORMAL);
				}
				else
				{
					message_window_t::g_run(core_api::get_main_window(), "Error - File System Explorer", "No device found!");
					return -1;
				}
			}
			break;
		case WM_DESTROY:
			m_directory_reader.abort();
			m_directory_reader.wait_for_and_release_thread();
			//m_items_view.destroy();
			break;
		case MSG_ON_DIRECTORY_THREAD_END:
			m_directory_reader.wait_for_and_release_thread();
			populate();
			break;
		case MSG_REFRESH:
			enable();
			break;
		case WM_PAINT:
			{
				ui_helpers::PaintScope ps(wnd);
				FillRect(ps->hdc, &ps->rcPaint, GetSysColorBrush(COLOR_WINDOW));
			}
			return 0;
		case WM_COMMAND:
			{
				switch (LOWORD(wp))
				{
				case IDOK:
					{
						t_size index = ListView_GetNextItem(m_wnd_items_view, -1, LVNI_SELECTED|LVNI_FOCUSED);
						if (index != -1 && index < m_items.get_count() && m_items[index].m_is_dir)
							refresh(pfc::string8(m_items[index].m_path));
						return 0;
					};
				case IDCANCEL:
					SendMessage(wnd, WM_CLOSE, NULL, NULL);
					return 0;
				};
			}
		case WM_NOTIFY:
			{
				LPNMHDR lpnm = (LPNMHDR)lp;
				switch (lpnm->idFrom)
				{
				case 1001:
					switch (lpnm->code)
					{
					case NM_DBLCLK:
						{
							LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lp;
							if (lpnmia->iItem != -1 && lpnmia->iItem >=0 && (t_size)lpnmia->iItem < m_items.get_count() && m_items[lpnmia->iItem].m_is_dir)
								refresh(pfc::string8(m_items[lpnmia->iItem].m_path)); //copy it as it gets nuked
						};
						break;
					case LVN_KEYDOWN:
						{
							LPNMLVKEYDOWN  lpnmk = (LPNMLVKEYDOWN )lp;
							if (lpnmk->wVKey == VK_F5)
								refresh(pfc::string8(m_directory));
						};
						break;
					};
					break;
				};
			}
			break;
		case WM_CONTEXTMENU:
			if ((HWND)wp == m_wnd_items_view)
			{
				enum {id_delete = 1, id_copy_to};

				POINT pt = {GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};

				pfc::array_t<bool> selection_mask;
				t_size selection_count = ListView_GetSelectedCount(m_wnd_items_view);
				if (selection_count && m_items.get_count() == ListView_GetItemCount(m_wnd_items_view))
				{
					selection_mask.set_size(m_items.get_count());
					selection_mask.fill_null();

					int index_focused = ListView_GetNextItem(m_wnd_items_view, -1, LVNI_FOCUSED);
					int index_selected = ListView_GetNextItem(m_wnd_items_view, -1, LVNI_SELECTED);

					t_size index = -1;

					t_size selection_non_virtual_count = 0;

					while ( (index = ListView_GetNextItem(m_wnd_items_view, index, LVNI_SELECTED)) != -1)
					{
						if (index<m_items.get_count() && !m_items[index].m_is_virtual)
						{
							selection_mask[index] = true;
							selection_non_virtual_count++;
						}
					}

					if (pt.x == -1 && pt.y == -1)
					{
						index = selection_mask[index_focused] ? index_focused : index_selected;
						RECT rc;
						ListView_GetItemRect(m_wnd_items_view, index, &rc, LVIR_BOUNDS);
						pt.x = rc.left + GetSystemMetrics(SM_CXSMICON);
						pt.y = rc.top + RECT_CY(rc)/2;
						ClientToScreen(m_wnd_items_view, &pt);
					}
					else 
					{
						/*POINT pt_client;
						pt_client.x = pt.x;
						pt_client.y = pt.y;
						ScreenToClient(m_wnd_items_view, &pt_client);
						LVHITTESTINFO lvht;
						memset(&lvht, 0, sizeof(lvht));
						lvht.pt.x = pt_client.x;
						lvht.pt.y = pt_client.y;
						index = ListView_HitTest(m_wnd_items_view, &lvht);*/
						index = index_selected;
						//if (index != -1)
						{
							//for (t_size i = 0, count = m_items.get_count(); i<count; i++)
							//	ListView_SetItemState(m_wnd_items_view, i, i == index ? (LVIS_SELECTED|LVIS_FOCUSED) : NULL, LVIS_SELECTED|LVIS_FOCUSED);
						}
					}
					if (selection_non_virtual_count && index != -1)
					{
						mmh::ui::menu menu;
						menu.append_command(L"Copy to..", id_copy_to);
						if (GetKeyState(VK_SHIFT) & KF_UP)
							menu.append_command(L"Delete", id_delete);
						switch (menu.run(wnd, pt))
						{
						case id_delete:
							{
								pfc::list_t<item> files;
								for (t_size i = 0, count = m_items.get_count(); i<count; i++)
									if (selection_mask[i] && !m_items[i].m_is_virtual) files.add_item(m_items[i]);
								delete_task * p_delete_task = new delete_task(wnd, files);
								p_delete_task->run(wnd);
								disable();
							}
							break;
						case id_copy_to:
							{
								pfc::string8 outDir;
								if (uBrowseForFolder(wnd, "Select Folder - Copy To", outDir))
								{
									pfc::list_t<item> files;
									for (t_size i = 0, count = m_items.get_count(); i<count; i++)
										if (selection_mask[i] && !m_items[i].m_is_virtual) files.add_item(m_items[i]);
									copy_to_task * p_copy_to_task = new copy_to_task(wnd, outDir, files);
									p_copy_to_task->run(wnd);
								}
							}
							break;
						};
					}
				}
				return 0;
			}
			break;
	};
	return DefWindowProc(wnd, msg, wp, lp);
}