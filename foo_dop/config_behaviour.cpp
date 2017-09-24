#include "stdafx.h"

#include "config_behaviour.h"

BOOL t_config_tab4::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialising = true;
			m_wnd_sort_library = GetDlgItem(wnd, IDC_SORTLIBRARY);
			m_wnd_sort_library_script = GetDlgItem(wnd, IDC_SORTLIBRARYSCRIPT);
			m_wnd_devices_panel_autosend = GetDlgItem(wnd, IDC_AUTOSEND);
			m_wnd_devices_panel_autosend_playlist = GetDlgItem(wnd, IDC_AUTOSENDPLAYLIST);
			m_wnd_quiet_sync = GetDlgItem(wnd, IDC_QUIETSYNC);

			Button_SetCheck(m_wnd_quiet_sync, settings::quiet_sync ? BST_CHECKED : BST_UNCHECKED);

			uSetWindowText(m_wnd_devices_panel_autosend_playlist, settings::devices_panel_autosend_playlist);
			Button_SetCheck(m_wnd_devices_panel_autosend, settings::devices_panel_autosend ? BST_CHECKED : BST_UNCHECKED);

			uSetWindowText(m_wnd_sort_library_script, settings::ipod_library_sort_script);
			Button_SetCheck(m_wnd_sort_library, settings::sort_ipod_library ? BST_CHECKED : BST_UNCHECKED);
			on_sort_library_change();
			m_initialising = false;
		}
		break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDC_SORTLIBRARY | (BN_CLICKED << 16) :
					settings::sort_ipod_library = Button_GetCheck(m_wnd_sort_library) == BST_CHECKED;
					on_sort_library_change();
					break;
				case IDC_SORTLIBRARYSCRIPT | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::ipod_library_sort_script = string_utf8_from_window((HWND)lp);
					break;
				case IDC_AUTOSEND | (BN_CLICKED << 16) :
					settings::devices_panel_autosend = Button_GetCheck(m_wnd_devices_panel_autosend) == BST_CHECKED;
					break;
				case IDC_AUTOSENDPLAYLIST | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::devices_panel_autosend_playlist = string_utf8_from_window((HWND)lp);
					break;
				case IDC_QUIETSYNC | (BN_CLICKED << 16) :
					settings::quiet_sync = Button_GetCheck(m_wnd_quiet_sync) == BST_CHECKED;
					break;
			}
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}

BOOL t_config_tab4::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	t_config_tab4 * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_tab4*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_tab4*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);
	return FALSE;
}
