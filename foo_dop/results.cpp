#include "stdafx.h"

#include "resource.h"
#include "results.h"

namespace results_viewer
{
	class t_results_data
	{
	public:
		pfc::list_t<result_t> m_results;
		const TCHAR * m_title;
		t_results_data(const TCHAR * p_title, const pfc::list_base_const_t<result_t> & p_results)
			: m_title(p_title)
		{
			m_results.add_items(p_results);
			HWND wnd;
			if (!(wnd = CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_RESULTS), core_api::get_main_window(), &g_DialogProc, (LPARAM)this)))
			throw exception_win32(GetLastError());
			ShowWindow(wnd, SW_SHOWNORMAL);
		};

		static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg,WPARAM wp,LPARAM lp)
		{
			switch(msg)
			{
			case WM_INITDIALOG:
				SetWindowLongPtr(wnd,DWL_USER,lp);
				if (lp)
					return reinterpret_cast<t_results_data*>(lp)->on_message(wnd,msg,wp,lp);
				break;
			default:
				{
					t_results_data * p_this = reinterpret_cast<t_results_data*>(GetWindowLongPtr(wnd,DWL_USER));
					if (p_this)
						return p_this->on_message(wnd,msg,wp,lp);

					if (msg==WM_DESTROY && p_this)
					{
						SetWindowLongPtr(wnd,DWL_USER,NULL);
						delete p_this;
					}
				}
				break;
			}
			return FALSE;
		}

		BOOL on_message(HWND wnd, UINT msg,WPARAM wp,LPARAM lp);
	};
	BOOL t_results_data::on_message(HWND wnd, UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				t_results_data * p_data = this;

				//modeless_dialog_manager::g_add(wnd);
				SetWindowText(wnd, p_data->m_title);
				HWND wnd_lv = GetDlgItem(wnd, IDC_LIST);
				uih::list_view_set_explorer_theme(wnd_lv);

				LVCOLUMN lvc;
				memset(&lvc, 0, sizeof(LVCOLUMN));
				lvc.mask = LVCF_TEXT|LVCF_WIDTH;

				uih::list_view_insert_column_text(wnd_lv, 0, _T("Source file"), 225);
				uih::list_view_insert_column_text(wnd_lv, 1, _T("File on iPod"), 225);
				uih::list_view_insert_column_text(wnd_lv, 2, _T("Result"), 225);

				SendMessage(wnd_lv, WM_SETREDRAW, FALSE, 0);

				LVITEM lvi;
				memset(&lvi, 0, sizeof(LVITEM));
				lvi.mask=LVIF_TEXT;
				t_size i, count=p_data->m_results.get_count();
				for (i=0;i<count;i++)
				{
					pfc::string8 temp, temp2;
					if (p_data->m_results[i].m_source_handle.is_valid())
						filesystem::g_get_display_path(p_data->m_results[i].m_source_handle->get_path(), temp);
					uih::list_view_insert_item_text(wnd_lv, i, 0, temp, false);
					if (p_data->m_results[i].m_handle.is_valid())
						filesystem::g_get_display_path(p_data->m_results[i].m_handle->get_path(), temp2);
					uih::list_view_insert_item_text(wnd_lv, i, 1, temp2, true);
					uih::list_view_insert_item_text(wnd_lv, i, 2, p_data->m_results[i].m_message.get_ptr(), true);
				}
				SendMessage(wnd_lv, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(wnd_lv,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			}
			break;
		case WM_SIZE:
			{
				RECT rc_button;
				GetWindowRect(GetDlgItem(wnd, IDCANCEL), &rc_button);
				unsigned cy_button = RECT_CY(rc_button);
				unsigned cx_button = RECT_CX(rc_button);

				HDWP dwp = BeginDeferWindowPos(2);
				DeferWindowPos(dwp, GetDlgItem(wnd, IDC_LIST), NULL, 11, 11, LOWORD(lp)-11-11, HIWORD(lp)-11*4-cy_button, SWP_NOZORDER);
				DeferWindowPos(dwp, GetDlgItem(wnd, IDCANCEL), NULL, LOWORD(lp)-11-cx_button, HIWORD(lp)-11-cy_button, cx_button, cy_button, SWP_NOZORDER);
				RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE);
				EndDeferWindowPos(dwp);
				RedrawWindow(wnd, NULL, NULL, RDW_UPDATENOW);
			}
			break;
		case WM_ERASEBKGND:
			SetWindowLongPtr(wnd, DWL_MSGRESULT, TRUE);
			return TRUE;
		case WM_PAINT:
			uih::handle_modern_background_paint(wnd, GetDlgItem(wnd, IDCANCEL));
			return TRUE;
		case WM_CONTEXTMENU:
			{
				if ((HWND)wp == GetDlgItem(wnd, IDC_LIST))
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

					if (hit)
					{
						if (sel >= 0 && (t_size)sel < m_results.get_count())
						{


							HMENU menu = CreatePopupMenu();
							if (m_results[sel].m_source_handle.is_valid())
								AppendMenu(menu, MF_STRING, 1, _T("Open source file location"));
							if (m_results[sel].m_handle.is_valid())
								AppendMenu(menu, MF_STRING, 2, _T("Open iPod file location"));

							if (false)
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
								metadb_handle_list temp;
								temp.add_item(m_results[sel].m_source_handle);
								standard_commands::context_file_open_directory(temp);
							}
							else if (cmd == 2)
							{
								metadb_handle_list temp;
								temp.add_item(m_results[sel].m_handle);
								standard_commands::context_file_open_directory(temp);

							}
						}
					}
					return 0;
				}
			}
			break;
		case WM_COMMAND:
			switch (wp)
			{
			case IDCANCEL:
				DestroyWindow(wnd);
				return 0;
			}
			break;
		case WM_CLOSE:
			DestroyWindow(wnd);
			return 0;
		case WM_NCDESTROY:
			//modeless_dialog_manager::g_remove(wnd);
			break;
		}

		return FALSE;
	}
	void g_run(const TCHAR * p_title, const pfc::list_base_const_t<result_t> & p_results)
	{
		t_results_data * data = new t_results_data(p_title, p_results);
		//ShowWindow(uCreateDialog(IDD_RESULTS, core_api::get_main_window(), g_DialogProc, (LPARAM)&data), SW_SHOWNORMAL);
	}
};
