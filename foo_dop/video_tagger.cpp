#include "main.h"

// {3E217C0C-F003-4fde-A5C8-AA473B1CC0EA}
static const GUID g_guid_cfg_video_tagger_position = 
{ 0x3e217c0c, 0xf003, 0x4fde, { 0xa5, 0xc8, 0xaa, 0x47, 0x3b, 0x1c, 0xc0, 0xea } };

cfg_struct_t<ui_helpers::window_position_t> cfg_video_tagger_position(g_guid_cfg_video_tagger_position, ui_helpers::window_position_t(0, 0, 500, 300));

class video_tagger_window : public ui_helpers::container_window_autorelease_t
{
public:
	class_data & get_class_data() const 
	{
		__implement_get_class_data_ex(_T("Dop_Video_Tagger"), _T(""), false, 0, WS_POPUP | WS_CLIPSIBLINGS| WS_CLIPCHILDREN  | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, WS_EX_DLGMODALFRAME, 0);
	}
#define video_tagger_position_edit(dwp, y, name) \
		if (m_wnd_##name##_label) \
			dwp = DeferWindowPos(dwp, m_wnd_##name##_label, NULL, 11*2, y+2, label_width, m_font_height+2, SWP_NOZORDER); \
		if (m_wnd_##name##_edit) \
			dwp = DeferWindowPos(dwp, m_wnd_##name##_edit, NULL, 11*2+label_width+7, y, cx - (11*2+100+7) -11*2, edit_height, SWP_NOZORDER); \
		y+=edit_height+7

	void on_size(t_size cx, t_size cy)
	{
		RECT rc_combo;
		GetWindowRect(m_wnd_mediatype_combo, &rc_combo);
		HDWP dwp = BeginDeferWindowPos(6);
		int m_font_height = uGetFontHeight(m_font);
		t_size edit_y=11,edit_height=m_font_height+6,combo_height=RECT_CY(rc_combo), label_width=100;
		{
			HDC dc = GetDC(m_wnd_episodenumber_label);
			HFONT fnt_old =  SelectFont(dc, m_font);
			SIZE sz;
			sz.cx=0;
			uGetTextExtentPoint32(dc, "Episode Number", 14, &sz);
			SelectFont(dc, fnt_old);
			ReleaseDC(m_wnd_seasonnumber_label, dc);
			label_width=sz.cx+7;
		}
		if (m_wnd_mediatype_label)
			dwp = DeferWindowPos(dwp, m_wnd_mediatype_label, NULL, 11*2, edit_y+3, label_width, m_font_height+2, SWP_NOZORDER);
		if (m_wnd_mediatype_combo)
			dwp = DeferWindowPos(dwp, m_wnd_mediatype_combo, NULL, 11*2+label_width+7, edit_y, cx - (11*2+100+7) -11*2, combo_height, SWP_NOZORDER);
		edit_y+=combo_height+7;
		video_tagger_position_edit(dwp, edit_y, title);
		video_tagger_position_edit(dwp, edit_y, show);
		video_tagger_position_edit(dwp, edit_y, seasonnumber);
		video_tagger_position_edit(dwp, edit_y, episodeid);
		video_tagger_position_edit(dwp, edit_y, episodenumber);
		if (m_wnd_apply)
			dwp = DeferWindowPos(dwp, m_wnd_apply, NULL, cx-11*2-73*1, cy-11-m_font_height-10, 73, m_font_height+10, SWP_NOZORDER);
		if (m_wnd_close)
			dwp = DeferWindowPos(dwp, m_wnd_close, NULL, cx-11*2-73*2-7, cy-11-m_font_height-10, 73, m_font_height+10, SWP_NOZORDER);
		if (m_wnd_button)
			dwp = DeferWindowPos(dwp, m_wnd_button, NULL, cx-11*2-73*3-7*2, cy-11-m_font_height-10, 73, m_font_height+10, SWP_NOZORDER);

		m_min_width = 11*2*2 + 7 + label_width + 200;
		m_min_height = edit_y + 11*3 + 1 + m_font_height+10;
		EndDeferWindowPos(dwp);
	}
	void on_size()
	{
		RECT rc;
		GetClientRect(get_wnd(), &rc);
		on_size(RECT_CX(rc), RECT_CY(rc));
	}
	void g_get_metafield_for_display(pfc::array_t<metadb_info_container::ptr> & infos, const char * field, bool & b_same, pfc::string8 & p_display)
	{
		t_size i, count=infos.get_count();
		b_same=true;
		p_display.reset();
		for (i=1; i<count; i++)
		{
			const char * prev = infos[i-1]->info().meta_get(field, 0), * curr = infos[i]->info().meta_get(field, 0);
			if ((prev!=0) != (curr!=0) || (curr && stricmp_utf8(prev, curr)))
			{
				b_same=false;
				break;
			}
		}
		if (b_same && count)
		{
			const char * str = infos[0]->info().meta_get(field, 0);
			if (str)
				p_display=str;
		}
		if (!b_same)
			p_display="<multiple values>";
	}
#define video_tagger_initialise_edit_window(name,field) \
	bool b_##name##Same=true; \
	pfc::string8 name##Value; \
	g_get_metafield_for_display(infos, field, b_##name##Same, name##Value); \
	uSetWindowText(m_wnd_##name##_edit, name##Value)

	void on_task_completion(t_size task, t_size code)
	{
		m_tagging_in_progress=false;
		{
			if (code == metadb_io_v3::update_info_success)
			{
				if (m_closing)
					PostMessage(get_wnd(), WM_CLOSE, NULL, NULL);
			}
			else
			{
				EnableWindow(m_wnd_apply, TRUE);
				m_closing=false;
			}
		}
	}

	void initialise()
	{
		t_size i, count=m_handles.get_count();
		{
			pfc::string8 title;
			if (count==1)
				title << pfc::string_filename_ext(m_handles[0]->get_path());
			else
				title << "Multiple Items";
			title << " - iPod Tag Editor";
			uSetWindowText(get_wnd(), title);
		}

		{
			pfc::array_t<metadb_info_container::ptr> infos;
			infos.set_count(count);
			for (i=0; i<count; i++)
				if (!m_handles[i]->get_info_ref(infos[i]))
					throw pfc::exception(pfc::string8() << "Failed to retrieve file information.");

			bool b_mediaTypeSame=true;
			pfc::string8 mediaType;
			g_get_metafield_for_display(infos, "MEDIA KIND", b_mediaTypeSame, mediaType);

			video_tagger_initialise_edit_window(title, "TITLE");
			video_tagger_initialise_edit_window(show, "SHOW");
			video_tagger_initialise_edit_window(seasonnumber, "SEASONNUMBER");
			video_tagger_initialise_edit_window(episodeid, "EPISODEID");
			video_tagger_initialise_edit_window(episodenumber, "EPISODENUMBER");

			if (b_mediaTypeSame)
			{
				if (!stricmp_utf8(mediaType, ""))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 0);
				else if (!stricmp_utf8(mediaType, "movie"))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 1);
				else if (!stricmp_utf8(mediaType, "music video"))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 2);
				else if (!stricmp_utf8(mediaType, "tv show"))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 3);
				else if (!stricmp_utf8(mediaType, "audiobook"))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 4);
				else if (!stricmp_utf8(mediaType, "podcast"))
					ComboBox_SetCurSel(m_wnd_mediatype_combo, 5);
			}
			else
				ComboBox_SetCurSel(m_wnd_mediatype_combo, -1);
		}
	}
#define video_tagger_save_edit(name, field) \
	if (m_##name##_changed) \
	{ \
		b_changed=true; \
		pfc::string8 newValue; \
		uGetWindowText(m_wnd_##name##_edit, newValue); \
		for (i=0; i<count; i++) \
			if (newValue.is_empty()) infos[i].meta_remove_field(field); \
			else infos[i].meta_set(field, newValue); \
	}
	void save()
	{
		if (m_closing)
			return;
		if (!IsWindowEnabled(m_wnd_apply))
			return;
		bool b_changed=false;
		t_size i, count=m_handles.get_count();

		static_api_ptr_t<metadb_io_v3> api;
		pfc::array_t<file_info_impl> infos;
		pfc::list_t<const file_info *> infos_const;
		infos.set_count(count);
		infos_const.set_count(count);
		for (i=0; i<count; i++)
		{
			m_handles[i]->get_info(infos[i]);
			infos_const[i] = &infos[i];
		}

		if (m_mediatype_changed)
		{
			b_changed=true;
			pfc::string8 newValue;
			t_size index = ComboBox_GetCurSel(m_wnd_mediatype_combo);
			uComboBox_GetText(m_wnd_mediatype_combo, index, newValue);
			for (i=0; i<count; i++)
				if (index==0)
					infos[i].meta_remove_field("MEDIA KIND");
				else
					infos[i].meta_set("MEDIA KIND", newValue);
		}
		video_tagger_save_edit(title, "TITLE");
		video_tagger_save_edit(show, "SHOW");
		video_tagger_save_edit(seasonnumber, "SEASONNUMBER");
		video_tagger_save_edit(episodeid, "EPISODEID");
		video_tagger_save_edit(episodenumber, "EPISODENUMBER");
		
		if (b_changed)
		{
			m_tagging_in_progress=true;
			EnableWindow(m_wnd_apply, FALSE);
			api->update_info_async_simple(m_handles, infos_const, get_wnd(), api->op_flag_delay_ui, completion_notify_create(this, 0));
		}
	}
	enum {IDC_MEDIATYPE=1001,IDC_SHOW,IDC_SEASONNUMBER,IDC_EPISODEID,IDC_EPISODENUMBER,IDC_TITLE1,IDC_APPLY1};

#define video_tagger_create_edit(name, label, id) \
	m_wnd_##name##_label = CreateWindowEx(0, WC_STATIC, label, WS_CHILD|WS_VISIBLE|WS_GROUP, 0, 0, 0, 0, wnd, (HMENU)NULL, core_api::get_my_instance(), NULL); \
	SendMessage(m_wnd_##name##_label, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0)); \
	m_wnd_##name##_edit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP, 0, 0, 0, 0, wnd, (HMENU)id, core_api::get_my_instance(), NULL); \
	SendMessage(m_wnd_##name##_edit, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0))

#define video_tagger_define_edit(name) \
	HWND m_wnd_##name##_label, \
	m_wnd_##name##_edit; \
	bool m_##name##_changed

#define video_tagger_initialise_edit(name) \
	m_wnd_##name##_label(NULL), m_wnd_##name##_edit(NULL), m_##name##_changed(false)

#define video_tagger_handle_command_change(name, id, msg) \
	case ((id)|(msg<<16)): \
	if (!m_initialising) { \
	EnableWindow(m_wnd_apply, TRUE); \
	m_##name##_changed=true; } \
		break

//#define video_tagger_handle_command_change_edit (name, id) \
//	video_tagger_handle_command_change(name, id, EN_CHANGE)
//#define video_tagger_handle_command_change_combo (name, id) \
	//video_tagger_handle_command_change(name, id, CBN_SELCHANGE)

	virtual LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch (msg)
		{
		case WM_NCCREATE:
			modeless_dialog_manager::g_add(wnd);
			break;
		case WM_CREATE:
			{
				m_initialising=true;

				m_font = uCreateIconFont();

				m_wnd_mediatype_label = CreateWindowEx(0, WC_STATIC, L"Media type", WS_CHILD|WS_VISIBLE|WS_GROUP, 0, 0, 0, 0, wnd, (HMENU)NULL, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_mediatype_label, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				m_wnd_mediatype_combo = CreateWindowEx(0, WC_COMBOBOX, L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_TABSTOP, 0, 0, 0, 100, wnd, (HMENU)IDC_MEDIATYPE, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_mediatype_combo, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));
				ComboBox_AddString(m_wnd_mediatype_combo, L"<unset>");
				ComboBox_AddString(m_wnd_mediatype_combo, L"Movie");
				ComboBox_AddString(m_wnd_mediatype_combo, L"Music Video");
				ComboBox_AddString(m_wnd_mediatype_combo, L"TV Show");
				ComboBox_AddString(m_wnd_mediatype_combo, L"Audiobook");
				ComboBox_AddString(m_wnd_mediatype_combo, L"Podcast");

				video_tagger_create_edit(title, L"Title", IDC_TITLE1);
				video_tagger_create_edit(show, L"Show", IDC_SHOW);
				video_tagger_create_edit(seasonnumber, L"Season Number", IDC_SEASONNUMBER);
				video_tagger_create_edit(episodeid, L"Episode ID", IDC_EPISODEID);
				video_tagger_create_edit(episodenumber, L"Episode Number", IDC_EPISODENUMBER);

				m_wnd_button = CreateWindowEx(0, WC_BUTTON, L"OK", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON|WS_GROUP, 0, 0,0,0, wnd, (HMENU)IDOK, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_button, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				m_wnd_close = CreateWindowEx(0, WC_BUTTON, L"Cancel", WS_CHILD|WS_VISIBLE|WS_TABSTOP, 0, 0,0,0, wnd, (HMENU)IDCANCEL, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_close, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				m_wnd_apply = CreateWindowEx(0, WC_BUTTON, L"Apply", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_DISABLED, 0, 0,0,0, wnd, (HMENU)IDC_APPLY1, core_api::get_my_instance(), NULL);
				SendMessage(m_wnd_apply, WM_SETFONT, (WPARAM)m_font.get(), MAKELPARAM(1,0));

				on_size();
				try 
				{
					initialise();
				}
				catch (pfc::exception const & ex) 
				{
					fbh::show_info_box(core_api::get_main_window(), "Error - iPod Tag Editor", ex.what(), OIC_ERROR);
					return -1;
				}

				m_initialising=false;
			}
			return 0;
		case WM_DESTROY:
			m_font.release();
			return 0;
		case WM_NCDESTROY:
			modeless_dialog_manager::g_remove(wnd);
			break;
		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpmmi = LPMINMAXINFO(lp);
				RECT rc;
				rc.left=0;
				rc.top=0;
				rc.bottom=m_min_height;
				rc.right=m_min_width;
				AdjustWindowRectEx(&rc, GetWindowLongPtr(get_wnd(), GWL_STYLE), FALSE, GetWindowLongPtr(get_wnd(), GWL_EXSTYLE));

				lpmmi->ptMinTrackSize.x = RECT_CX(rc);
				lpmmi->ptMinTrackSize.y = RECT_CY(rc);
			}
			return 0;
		case WM_SHOWWINDOW:
			if (wp == TRUE && lp == 0)
				SetFocus(m_wnd_button);
			break;
		case DM_GETDEFID:
			return IDOK|(DC_HASDEFID<<16);
		case WM_SIZE:
			{
				RECT rc;
				GetRelativeRect(get_wnd(), core_api::get_main_window(), &rc);
				cfg_video_tagger_position.get_value().set_from_rect(rc);
				on_size(LOWORD(lp), HIWORD(lp));
				RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE);
			}
			return 0;
		case WM_MOVE:
			{
				RECT rc;
				GetRelativeRect(get_wnd(), core_api::get_main_window(), &rc);
				cfg_video_tagger_position.get_value().set_from_rect(rc);
			}
			return 0;
		case WM_COMMAND:
			switch (wp)
			{
			case IDOK:
				if (IsWindowEnabled(m_wnd_apply))
				{
					save();
					m_closing=true;
				}
				else
					SendMessage(wnd, WM_CLOSE, NULL, NULL);
				return 0;
			case IDC_APPLY1:
				save();
				return 0;
			case IDCANCEL:
				SendMessage(wnd, WM_CLOSE, NULL, NULL);
				return 0;
			video_tagger_handle_command_change(mediatype, IDC_MEDIATYPE, CBN_SELCHANGE);
			video_tagger_handle_command_change(show, IDC_SHOW, EN_CHANGE);
			video_tagger_handle_command_change(episodenumber, IDC_EPISODENUMBER, EN_CHANGE);
			video_tagger_handle_command_change(seasonnumber, IDC_SEASONNUMBER, EN_CHANGE);
			video_tagger_handle_command_change(episodeid, IDC_EPISODEID, EN_CHANGE);
			video_tagger_handle_command_change(title, IDC_TITLE1, EN_CHANGE);
			}
			break;
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
			SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
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
					rc_fill.top=rc_fill.bottom;
					rc_fill.bottom=rc_client.bottom;
					FillRect(dc, &rc_fill, GetSysColorBrush(COLOR_3DFACE));

					EndPaint(wnd, &ps);
				}
			}
			return 0;
		case WM_CLOSE:
			if (!m_tagging_in_progress)
			{
				destroy();
				delete this;
			}
			return 0;
		}
		return DefWindowProc(wnd, msg, wp, lp);
	}

	static void g_run(metadb_handle_list_cref p_handles)
	{
		if (p_handles.get_count())
		{
			video_tagger_window * p_test = new video_tagger_window(p_handles);
			RECT rc;
			cfg_video_tagger_position.get_value().convert_to_rect(rc);
			MapWindowPoints(core_api::get_main_window(), HWND_DESKTOP, (LPPOINT)&rc, 2);
			POINT pt_topleft = {rc.left, rc.top};
			POINT pt_bottomright = {rc.bottom, rc.right};
			ui_helpers::window_position_t pos(rc);
			{
				HMONITOR mon = MonitorFromPoint(pt_bottomright, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(info));
				info.cbSize=sizeof(info);
				GetMonitorInfo(mon, &info);
				if (pos.x+(int)pos.cx>=info.rcWork.right)
					pos.x=info.rcWork.right-(int)pos.cx;
				if (pos.y+(int)pos.cy>=info.rcWork.bottom)
					pos.y=info.rcWork.bottom-(int)pos.cy;
			}
			{
				HMONITOR mon = MonitorFromPoint(pt_topleft, MONITOR_DEFAULTTONEAREST);
				MONITORINFOEXW info;
				memset(&info, 0, sizeof(info));
				info.cbSize=sizeof(info);
				GetMonitorInfo(mon, &info);
				if (pos.x<info.rcWork.left)
					pos.x=info.rcWork.left;
				else if (pos.x>=info.rcWork.right)
					pos.x=info.rcWork.right-pos.cx;
				if (pos.y<info.rcWork.top)
					pos.y=info.rcWork.top;
				else if (pos.y>=info.rcWork.bottom)
					pos.y=info.rcWork.bottom-pos.cy;
			}
			HWND wnd = p_test->create(core_api::get_main_window(), NULL, pos);
			if (wnd)
				ShowWindow(wnd, SW_SHOWNORMAL);
			else delete p_test;
		}
	}

private:
	video_tagger_window(metadb_handle_list_cref p_handles) : m_handles(p_handles),
		m_wnd_mediatype_label(NULL), m_wnd_mediatype_combo(NULL), m_wnd_button(NULL), m_mediatype_changed(false) ,
		m_wnd_close(NULL), m_wnd_show_label(NULL), m_wnd_show_edit(NULL), m_show_changed(false),
		video_tagger_initialise_edit(seasonnumber), video_tagger_initialise_edit(episodeid),
		video_tagger_initialise_edit(episodenumber), video_tagger_initialise_edit(title), m_wnd_apply(NULL),
		m_tagging_in_progress(false), m_closing(false), m_min_height(0), m_min_width(0), m_initialising(false)
	{
		fbh::metadb_handle_list_remove_duplicates(m_handles);
	};
	HWND m_wnd_mediatype_label,
		m_wnd_mediatype_combo,
		m_wnd_button,m_wnd_close,m_wnd_apply;

	video_tagger_define_edit(show);
	video_tagger_define_edit(seasonnumber);
	video_tagger_define_edit(episodeid);
	video_tagger_define_edit(episodenumber);
	video_tagger_define_edit(title);

	t_size m_min_height,m_min_width;

	bool m_mediatype_changed;
	bool m_tagging_in_progress, m_closing, m_initialising;
	gdi_object_t<HFONT>::ptr_t m_font;
	metadb_handle_list m_handles;
};

void g_video_tagger_run(metadb_handle_list_cref p_handles)
{
	video_tagger_window::g_run(p_handles);
}

