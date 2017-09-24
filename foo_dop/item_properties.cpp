#include "stdafx.h"

#include "item_properties.h"


BOOL CALLBACK item_info_dialog_v2_t::g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	item_info_dialog_v2_t * p_this = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(wnd, DWL_USER, lp);
		p_this = reinterpret_cast<item_info_dialog_v2_t*>(lp);
		break;
	default:
		p_this = reinterpret_cast<item_info_dialog_v2_t*>(GetWindowLongPtr(wnd, DWL_USER));
		break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

void g_format_date(t_filetimestamp time, std::basic_string<TCHAR> & str)
{
	SYSTEMTIME st;
	FileTimeToSystemTime((LPFILETIME)&time, &st);
	int size = GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, NULL);
	int size2 = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, NULL, NULL);
	pfc::array_t<TCHAR> buf, buf2;
	buf.set_size(size);
	buf2.set_size(size2);
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buf.get_ptr(), size);
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buf2.get_ptr(), size2);
	str = _T("");
	str += buf.get_ptr();
	str += _T(" ");
	str += buf2.get_ptr();
}

BOOL item_info_dialog_v2_t::DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			//uxtheme_api_ptr uxtheme;
			//if (uxtheme_handle::g_create(uxtheme))
			//EnableThemeDialogTexture(wnd, ETDT_ENABLETAB);
			modeless_dialog_manager::g_add(wnd);
			HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
			Button_SetCheck(GetDlgItem(wnd, IDC_REMPOS), m_track->remember_playback_position != 0);
			Button_SetCheck(GetDlgItem(wnd, IDC_SKIPSHUFFLE), m_track->skip_on_shuffle != 0);

			pfc::string_extension ext(m_track->location);
			bool m4b = !stricmp_utf8(ext, "m4b");
			bool aa = !stricmp_utf8(ext, "aa");

			if (m4b||aa)
			{
				EnableWindow(GetDlgItem(wnd, IDC_REMPOS), FALSE);
				EnableWindow(GetDlgItem(wnd, IDC_SKIPSHUFFLE), FALSE);
			}
			
			t_uint32 atime = m_track->lastplayedtime;
			t_filetimestamp time = filetime_time_from_appletime(atime, false);
			t_uint32 stime = m_track->last_skipped;
			t_filetimestamp skiptime = filetime_time_from_appletime(stime, false);
			if (atime)
			{
				std::basic_string<TCHAR> str;
				g_format_date(time, str);
				SetWindowText(GetDlgItem(wnd, IDC_PLAYED), str.data());
			}
			else
				SetWindowText(GetDlgItem(wnd, IDC_PLAYED), _T("n/a"));
			if (stime)
			{
				std::basic_string<TCHAR> str;
				g_format_date(skiptime, str);
				SetWindowText(GetDlgItem(wnd, IDC_SKIPPED), str.data());
			}
			else
				SetWindowText(GetDlgItem(wnd, IDC_SKIPPED), _T("n/a"));
			t_uint32 sc = m_track->soundcheck;
			if (sc)
			{
				float rggain = log10f((float)sc / 1000.0f)/-0.1f;
				uSetWindowText(GetDlgItem(wnd, IDC_SCHECK), pfc::format_float(rggain, 0, 2));
			}
			else
				SetWindowText(GetDlgItem(wnd, IDC_SCHECK), _T("n/a"));

			uSetDlgItemInt(wnd, IDC_PLAYCOUNT, m_track->playcount, FALSE);
			uSetDlgItemInt(wnd, IDC_SKIPCOUNT, m_track->skip_count_user, FALSE);
		}
		break;
	case WM_COMMAND:
		switch (wp)
		{
		case IDCANCEL:
			DestroyWindow(wnd);
			return 0;
		case IDC_REMPOS:
			m_track->remember_playback_position = Button_GetCheck((HWND)lp) != BST_UNCHECKED;
			break;
		case IDC_SKIPSHUFFLE:
			m_track->skip_on_shuffle = Button_GetCheck((HWND)lp) != BST_UNCHECKED;
			break;
		}
		break;
	case WM_SIZE:
		{
			/*
			HWND wnd_list = GetDlgItem(wnd, IDC_LIST);
			HWND wnd_lv = GetDlgItem(wnd, IDC_SONGS);
			HWND wnd_close = GetDlgItem(wnd, IDCANCEL);
			HDWP dwp = BeginDeferWindowPos(3);
			RECT rc_list, rc_close;
			GetWindowRect(wnd_list, &rc_list);
			GetWindowRect(wnd_close, &rc_close);
			unsigned cx_list = rc_list.right - rc_list.left;
			unsigned cy_close = rc_close.bottom - rc_close.top;
			unsigned cx_close = rc_close.right - rc_close.left;
			dwp = DeferWindowPos(dwp, wnd_lv, NULL, cx_list+2, 0, LOWORD(lp)-cx_list+2, HIWORD(lp)-cy_close-2, SWP_NOZORDER);
			dwp = DeferWindowPos(dwp, wnd_list, NULL, 0, 0, cx_list, HIWORD(lp)-cy_close-2, SWP_NOZORDER);
			dwp = DeferWindowPos(dwp, wnd_close, NULL, LOWORD(lp)-cx_close, HIWORD(lp)-cy_close, cx_close, cy_close, SWP_NOZORDER);
			EndDeferWindowPos(dwp);*/
		}
		return 0;
	case WM_ERASEBKGND:
		SetWindowLongPtr(wnd, DWL_MSGRESULT, TRUE);
		return TRUE;
	case WM_PAINT:
		uih::handle_modern_background_paint(wnd, GetDlgItem(wnd, IDCANCEL));
		return TRUE;
	case WM_CTLCOLORSTATIC:
		SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
		SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
		return (BOOL)GetSysColorBrush(COLOR_WINDOW);
	case WM_CLOSE:
		DestroyWindow(wnd);
		return 0;
	case WM_NCDESTROY:
		modeless_dialog_manager::g_remove(wnd);
		SetWindowLongPtr(wnd, DWL_USER, NULL);
		//m_this.release();
		delete this;
		break;
	}
	return FALSE;
}

