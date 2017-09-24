#include "main.h"

#include "helpers.h"

void threaded_process_v2_t::run(HWND wnd)
{
	RECT rc1;
	GetClientRect(wnd, &rc1);
	MapWindowPoints(wnd, HWND_DESKTOP, (LPPOINT)(&rc1), 2);
	RECT rc = { 0, 0, 355 + 11 * 4, 11 * 2 + 15 };
	AdjustWindowRectEx(&rc, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, false, NULL);
	int cx = RECT_CX(rc);
	int cy = RECT_CY(rc);
	int x = (m_flags & flag_position_bottom_right) ? rc1.right - cx : rc1.left;
	int y = (m_flags & flag_position_bottom_right) ? rc1.bottom - cy : rc1.top;

	m_steps.set_size(m_range);
	m_steps.fill(1);

	m_window_cx = cx;
	m_window_cy = cy;

	create(wnd, 0, ui_helpers::window_position_t(x, y, cx, cy));
	create_thread();
}

void threaded_process_v2_t::resize()
{
	t_size height = 11 + 11;
	if (m_flags & flag_show_text)
		height += m_titlefont_height + 7;
	if (1)
		height += 15;
	if (m_flags & flag_show_button)
		height += m_textfont_height + 10 + 11 + 11;//23;
	{
		insync(m_sync);
		if (m_detail_entries.get_count())
			height += m_textfont_height*m_detail_entries.get_count() + 5;
	}
	RECT rc = { 0, 0, 450, height };
	AdjustWindowRectEx(&rc, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, false, NULL);
	int cx = progress_width + 11 * 2 + 11 * 2 + RECT_CX(rc) - 450;
	int cy = RECT_CY(rc);
	RECT rc_current = { 0 };
	GetWindowRect(get_wnd(), &rc_current);
	if (RECT_CX(rc_current) != cx || RECT_CY(rc_current) != cy)
	{
		m_window_cx = cx;
		m_window_cy = cy;
		//SendMessage(get_wnd(), WM_CANCELMODE, NULL, NULL);
		SetWindowPos(get_wnd(), NULL, NULL, NULL, cx, cy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}
}

void threaded_process_v2_t::on_size(t_size cx, t_size cy)
{
	RedrawWindow(get_wnd(), NULL, NULL, RDW_INVALIDATE);
	HDWP dwp = BeginDeferWindowPos(2);
	t_size y_progress = 11;
	if (m_flags & flag_show_text)
		y_progress += 7 + m_titlefont_height;
	{
		insync(m_sync);
		if (m_detail_entries.get_count())
			y_progress += m_textfont_height*m_detail_entries.get_count() + 5;
	}
	dwp = DeferWindowPos(dwp, m_wnd_progress, NULL, 11 * 2, y_progress, cx - 11 * 2 - 11 * 2, 15, SWP_NOZORDER);
	if (m_wnd_button)
		dwp = DeferWindowPos(dwp, m_wnd_button, NULL, cx - 11 * 2 - 73, y_progress + 15 + 11 + 11, 73, m_textfont_height + 10, SWP_NOZORDER);
	EndDeferWindowPos(dwp);
}

void threaded_process_v2_t::refresh_title_font()
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	m_titlecolour = GetSysColor(COLOR_WINDOWTEXT);

	HTHEME thm_textstyle = IsThemeActive() && IsAppThemed() ? OpenThemeData(get_wnd(), L"TextStyle") : NULL;
	if (thm_textstyle)
	{
		if (SUCCEEDED(GetThemeFont(thm_textstyle, NULL, TEXT_MAININSTRUCTION, 0, TMT_FONT, &lf)))
		{
			m_titlefont = CreateFontIndirect(&lf);
			GetThemeColor(thm_textstyle, TEXT_MAININSTRUCTION, NULL, TMT_TEXTCOLOR, &m_titlecolour);
		}
		CloseThemeData(thm_textstyle);
	}

	if (!m_titlefont.is_valid())
	{
		uGetIconFont(&lf);
		lf.lfWeight = FW_BOLD;
		m_titlefont = CreateFontIndirect(&lf);
	}

	m_titlefont_height = uGetFontHeight(m_titlefont);
}

LRESULT threaded_process_v2_t::on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_TIMER:
			if (wp == 1)
			{
				ShowWindow(wnd, SW_SHOWNORMAL);
				KillTimer(wnd, 1);
				return 0;
			}
			else if (wp == 667)
			{
				KillTimer(wnd, 667);
				m_timer_active = false;
				RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				return 0;
			}
			break;
		case WM_NCCREATE:
			modeless_dialog_manager::g_add(wnd);
			break;
		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED:
			refresh_title_font();
			resize();
			break;
		case WM_CREATE:
		{
			if (m_flags & flag_show_button)
			{
				m_textfont = uCreateIconFont();
				m_textfont_height = uGetFontHeight(m_titlefont);
			}
			if (m_flags & flag_show_text)
				refresh_title_font();

			uSetWindowText(wnd, m_title);
			//t_size y_progress = 11;
			//if (m_flags & flag_show_text)
			//	y_progress+=5+uGetFontHeight(m_titlefont);
			//RECT rc;
			//GetClientRect(wnd, &rc);
			//m_wnd_caption = CreateWindowEx(0, WC_STATIC, L"STATIC", WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, wnd, (HMENU)1001, core_api::get_my_instance(), NULL);
			m_wnd_progress = CreateWindowEx(0, PROGRESS_CLASS, L"PROGRESS", WS_SYSMENU | WS_CHILD | WS_VISIBLE | ((m_flags & flag_progress_marquee) ? 0x08 : 0) | PBS_SMOOTH | WS_GROUP, 0, 0, 0, 0, wnd, (HMENU)1002, core_api::get_my_instance(), NULL);
			if (0 == (m_flags & flag_progress_marquee))
				SendMessage(m_wnd_progress, PBM_SETRANGE32, 0, progress_width/*m_range*/);
			else
				SendMessage(m_wnd_progress, WM_USER + 10, TRUE, 20);
			if (m_flags & flag_show_button)
			{
				m_wnd_button = CreateWindowEx(0, WC_BUTTON, L"Stop", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON | WS_GROUP, 0, 0, 0, 0, wnd, (HMENU)IDCANCEL, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_button, WM_SETFONT, (WPARAM)m_textfont.get(), MAKELPARAM(1, 0));
				//SetFocus(m_wnd_button);
			}
			else
			{
				HMENU menu = GetSystemMenu(wnd, FALSE);
				EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
			}
			resize();
			if (m_flags & flag_no_delay)
			{
				ShowWindow(wnd, SW_SHOWNORMAL);
			}
			else
			{
				SetTimer(wnd, 1, 300, NULL);
			}
			on_init();
		}
		return 0;
		case WM_SHOWWINDOW:
			if (wp == TRUE && lp == 0 && m_wnd_button)
				SetFocus(m_wnd_button);
			break;
		case WM_MOVING:
		{
			LPRECT lprc = (LPRECT)lp;
			lprc->right = lprc->left + m_window_cx;
			lprc->bottom = lprc->top + m_window_cy;
		}
		return TRUE;
		case DM_GETDEFID:
			if (m_flags & flag_show_button)
				return IDCANCEL | (DC_HASDEFID << 16);
			return 0;
		case WM_DESTROY:
			on_destroy_thread();
			m_textfont.release();
			m_titlefont.release();
			return 0;
		case WM_NCDESTROY:
			modeless_dialog_manager::g_remove(wnd);
			break;
		case WM_CLOSE:
			m_abort.abort();
			return 0;
		case WM_SETFOCUS:
			break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDCANCEL:
					if (m_wnd_button)
						EnableWindow(m_wnd_button, FALSE);
					m_abort.abort();
					return 0;
			}
			break;
		case WM_PAINT:
		{
			pfc::string8 p_text;
			pfc::array_t<detail_entry> p_detail_entries;
			{
				insync(m_sync);
				p_text = m_text;
				p_detail_entries = m_detail_entries;
			}
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
					rc_fill.bottom -= RECT_CY(rc_button) + 9;
					rc_fill.bottom -= 11;
				}

				FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_WINDOW));

				if (m_wnd_button)
				{
					rc_fill.top = rc_fill.bottom;
					rc_fill.bottom += 1;
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DLIGHT));
				}

				rc_fill.top = rc_fill.bottom;
				rc_fill.bottom = rc_client.bottom;
				if (rc_fill.top < rc_fill.bottom)
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DFACE));

				if (m_flags & flag_show_text)
				{
					SetTextAlign(dc, TA_LEFT);
					SetBkMode(dc, TRANSPARENT);
					//SetTextColor(dc, m_titlecolour);
					RECT rc;
					GetClientRect(wnd, &rc);
					RECT rc2 = { 11 * 2, 11, RECT_CX(rc) - 11 * 2, m_titlefont_height + 11 };
					//FillRect(dc, &rc2, GetSysColorBrush(COLOR_BTNFACE));
					HFONT fnt_old = SelectFont(dc, m_titlefont);
					uih::text_out_colours_ellipsis(dc, p_text, p_text.get_length(), 0, 11, &rc2, false, false, m_titlecolour, uih::ALIGN_LEFT);
					//uExtTextOut(dc, 11*2, 11, ETO_CLIPPED, &rc2, m_text, m_text.get_length(), NULL);
					t_size detail_entry_count = 0;
					if (detail_entry_count = p_detail_entries.get_count())
					{
						RECT rc3 = rc2;
						rc3.top = rc2.bottom + 7;
						rc3.bottom = rc3.top + m_textfont_height;
						//SetTextColor(dc, GetSysColor(COLOR_BTNTEXT));
						SelectFont(dc, m_textfont);
						int max_x = 0;
						for (t_size i = 0; i<detail_entry_count; i++)
						{
							int last_x = 0;
							uih::text_out_colours_ellipsis(dc, p_detail_entries[i].m_label, p_detail_entries[i].m_label.get_length(), 0, rc3.top,
								&rc3, false, false, GetSysColor(COLOR_WINDOWTEXT), uih::ALIGN_LEFT, NULL, true, &last_x);
							max_x = max(last_x, max_x);
							rc3.top = rc3.bottom;
							rc3.bottom = rc3.top + m_textfont_height;
							//uExtTextOut(dc, 11*2, rc3.top, ETO_CLIPPED, &rc3, m_detail_entries[i].m_label, m_detail_entries[i].m_label.get_length(), NULL);
						}
						rc3.top = rc2.bottom + 7;
						rc3.bottom = rc3.top + m_textfont_height;
						rc3.left = max_x + 5;
						for (t_size i = 0; i<detail_entry_count; i++)
						{
							uih::text_out_colours_ellipsis(dc, p_detail_entries[i].m_value, p_detail_entries[i].m_value.get_length(), 0, rc3.top,
								&rc3, false, true, GetSysColor(COLOR_WINDOWTEXT), uih::ALIGN_LEFT);
							rc3.top = rc3.bottom;
							rc3.bottom = rc3.top + m_textfont_height;
						}
					}
					SelectFont(dc, fnt_old);
				}
				EndPaint(wnd, &ps);
			}
		}
		return 0;
		case MSG_REDRAW:
			resize();
			if (!m_timer_active)
			{
				LARGE_INTEGER current = { 0 }, freq = { 0 };
				QueryPerformanceCounter(&current);
				QueryPerformanceFrequency(&freq);
				t_uint64 tenth = 5;
				if (m_time_last_redraw.QuadPart)
				{
					tenth = (current.QuadPart - m_time_last_redraw.QuadPart) / (freq.QuadPart / 100);
				}
				if (tenth < 10)
				{
					SetTimer(get_wnd(), 667, 100 - t_uint32(tenth) * 10, NULL);
					m_timer_active = true;
				}
				else RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			return 0;
		case MSG_UPDATE_PROGRESS:
			if (abs((int)(wp - SendMessage(m_wnd_progress, PBM_GETPOS, 0, 0))) > 0)
				SendMessage(m_wnd_progress, PBM_SETPOS, wp, 0);
			return 0;
		case MSG_END:
			destroy();
			on_exit();
			return 0;
	}
	return DefWindowProc(wnd, msg, wp, lp);
}
