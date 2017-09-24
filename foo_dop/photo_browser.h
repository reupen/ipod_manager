#pragma once

#include "actions_base.h"

#ifdef _DEBUG
#define PHOTO_BROWSER _DEBUG
#endif

#ifdef PHOTO_BROWSER
class bitmap_viewer_window : public ui_helpers::container_window
{
public:
	void set_bitmap(HBITMAP bm)
	{
		if (m_bitmap)
		{
			DeleteBitmap(m_bitmap);
			m_bitmap=NULL;
		}
		m_bitmap = bm;
	}
	virtual ui_helpers::container_window::class_data & get_class_data()const 
	{
		__implement_get_class_data_child_ex(_T("dop_bitmap_viewer"), _T("Photo viewer"), false);
	}

	LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch (msg)
		{
		case WM_PAINT:
			{
				RECT rc_client;
				GetClientRect(wnd, &rc_client);
				PAINTSTRUCT ps; 
				HDC dc = BeginPaint(wnd, &ps);

				if (m_bitmap)
				{
					HDC hdc_mem=0;
					HBITMAP hbm_old=0; 
					hdc_mem = CreateCompatibleDC(dc);
					hbm_old = (HBITMAP)SelectObject(hdc_mem, m_bitmap);
					BitBlt(dc,	0,0,rc_client.right,rc_client.bottom, hdc_mem, 0, 0, SRCCOPY);
					SelectObject(hdc_mem, hbm_old);
					DeleteDC(hdc_mem);
				}

				EndPaint(wnd, &ps);
			}
			return 0;
		case WM_NCDESTROY:
			if (m_bitmap)
			{
				DeleteBitmap(m_bitmap);
				m_bitmap=NULL;
			}
			break;
		}
		return DefWindowProc(wnd, msg, wp, lp);
	}

	bitmap_viewer_window() : m_bitmap(NULL) {};

private:
	HBITMAP m_bitmap;
};

class t_photo_browser : public ipod_action_v2_t
{
	virtual void on_run();
	virtual void on_exit();
public:
	DOP_IPOD_ACTION_ENTRY(t_photo_browser);

	t_photo_browser()
		: m_failed(false), ipod_action_v2_t("Browse iPod")
	{};
	bool m_failed;
};

class t_threaded_file_reader : public threaded_process_callback
{
	virtual void run(threaded_process_status & p_status,abort_callback & p_abort);
	//virtual void on_done(HWND p_wnd,bool p_was_aborted);
public:
	t_threaded_file_reader(const char * path, t_filesize offset, t_filesize size)
		: m_failed(false), m_path(path), m_offset(offset), m_size(size)
	{};
	bool m_failed;

	t_filesize m_size, m_offset;
	pfc::string8 m_path;
	pfc::array_t<t_uint8> m_data;
};
#endif

