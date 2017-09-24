#pragma once

#include "itunesdb.h"

void g_format_date(t_filetimestamp time, std::basic_string<TCHAR> & str);

class item_info_dialog_v2_t
{
	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	BOOL DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
public:
	static void g_run(HWND wnd, const pfc::rcptr_t<itunesdb::t_track> & ptr)
	{
		item_info_dialog_v2_t * iteminfo = new item_info_dialog_v2_t(ptr);
		HWND wndi = uCreateDialog(IDD_INFO, wnd, g_DialogProc, (LPARAM)iteminfo);
		ShowWindow(wndi, SW_SHOWNORMAL);
	}
	item_info_dialog_v2_t(const pfc::rcptr_t<itunesdb::t_track> & ptr)
		: m_track(ptr)
	{};

	pfc::rcptr_t<itunesdb::t_track> m_track;
	//pfc::refcounted_object_ptr_t<ipod_browse_dialog> m_this; ////

};
