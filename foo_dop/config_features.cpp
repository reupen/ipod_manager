#include "stdafx.h"

#include "config_features.h"
#include "resource.h"

BOOL t_config_tab2::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialising = true;
			m_wnd_artwork_string = GetDlgItem(wnd, IDC_ARTWORK_STRING);
			m_wnd_add_artwork = GetDlgItem(wnd, IDC_ADDARTWORK);
			m_wnd_add_gapless = GetDlgItem(wnd, IDC_SCANGAPLESS);
			m_wnd_dummy_gapless = GetDlgItem(wnd, IDC_USEDUMMYGAPLESS);
			m_wnd_video_thumbailer_enabled = GetDlgItem(wnd, IDC_THUMBAILVIDEOS);

			m_wnd_fb2k_artwork = GetDlgItem(wnd, IDC_FB2KARTWORK);

			uSetWindowText(m_wnd_artwork_string, settings::artwork_sources);
			Button_SetCheck(m_wnd_add_artwork, settings::add_artwork ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_add_gapless, settings::add_gapless ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_fb2k_artwork, settings::use_fb2k_artwork ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_dummy_gapless, settings::use_dummy_gapless_data ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_video_thumbailer_enabled, settings::video_thumbnailer_enabled ? BST_CHECKED : BST_UNCHECKED);

			on_add_artwork_change();
			on_add_gapless_change();
			m_initialising = false;
		}
		break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDC_ADDARTWORK | (BN_CLICKED << 16) :
					settings::add_artwork = Button_GetCheck(m_wnd_add_artwork) == BST_CHECKED;
					on_add_artwork_change();
					break;
				case IDC_FB2KARTWORK | (BN_CLICKED << 16) :
					settings::use_fb2k_artwork = Button_GetCheck(m_wnd_fb2k_artwork) == BST_CHECKED;
					break;
				case IDC_THUMBAILVIDEOS | (BN_CLICKED << 16) :
					settings::video_thumbnailer_enabled = Button_GetCheck(m_wnd_video_thumbailer_enabled) == BST_CHECKED;
					break;
				case IDC_SCANGAPLESS | (BN_CLICKED << 16) :
					settings::add_gapless = Button_GetCheck(m_wnd_add_gapless) == BST_CHECKED;
					on_add_gapless_change();
					break;
				case IDC_USEDUMMYGAPLESS | (BN_CLICKED << 16) :
					settings::use_dummy_gapless_data = Button_GetCheck(m_wnd_add_gapless) == BST_CHECKED;
					break;
				case IDC_ARTWORK_STRING | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::artwork_sources = string_utf8_from_window((HWND)lp);
					break;
			}
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}

BOOL t_config_tab2::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	t_config_tab2 * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_tab2*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_tab2*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);
	return FALSE;
}
