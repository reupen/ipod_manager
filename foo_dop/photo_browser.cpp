#include "main.h"

#define artwork_object m_artwork

#ifdef PHOTO_BROWSER
class ipod_browse_photo_dialog : public pfc::refcounted_object_root//, initquit_autoreg
{
	bitmap_viewer_window m_viewer;
	//virtual void on_run();
	//virtual void on_exit();
	BOOL DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	void on_size (HWND, unsigned width, unsigned height);
	void refresh_photo();
public:
	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	ipod::tasks::load_database_t m_library;
	ipod::tasks::drive_scanner_t m_drive_scanner;
	ipod_browse_photo_dialog(t_photo_browser * pbrowser)
		: m_library(pbrowser->m_library), m_drive_scanner(pbrowser->m_drive_scanner) ,m_failed(false), m_index(0), m_format(0)//, ipod_action_v2_t("Browse iPod", 4, flag_show_text|flag_show_progress_window|flag_show_button)
	{};
	bool m_failed;

	t_size m_index, m_format;
	pfc::refcounted_object_ptr_t<ipod_browse_photo_dialog> m_this;
};
BOOL CALLBACK ipod_browse_photo_dialog::g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	ipod_browse_photo_dialog * p_this = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(wnd, DWL_USER, lp);
		p_this = reinterpret_cast<ipod_browse_photo_dialog*>(lp);
		break;
	default:
		p_this = reinterpret_cast<ipod_browse_photo_dialog*>(GetWindowLongPtr(wnd, DWL_USER));
		break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

void ipod_browse_photo_dialog::on_size (HWND wnd, unsigned width, unsigned height)
{
	HWND wnd_close = GetDlgItem(wnd, IDCANCEL);
	HWND wnd_next = GetDlgItem(wnd, IDC_NEXT);
	HWND wnd_last = GetDlgItem(wnd, IDC_LAST);
	HWND wnd_prev = GetDlgItem(wnd, IDC_PREVIOUS);
	HWND wnd_nextf = GetDlgItem(wnd, IDC_NEXT_FORMAT);
	HWND wnd_prevf = GetDlgItem(wnd, IDC_PREVIOUS_FORMAT);
	HDWP dwp = BeginDeferWindowPos(6);
	RECT rc_close, rc_next, rc_next_format, rc_prev, rc_prev_format, rc_last;
	GetWindowRect(wnd_close, &rc_close);
	GetRelativeRect(wnd_next, wnd, &rc_next);
	GetRelativeRect(wnd_last, wnd, &rc_last);
	GetRelativeRect(wnd_nextf, wnd, &rc_next_format);
	GetRelativeRect(wnd_prev, wnd, &rc_prev);
	GetRelativeRect(wnd_prevf, wnd, &rc_prev_format);
	unsigned cy_close = rc_close.bottom - rc_close.top;
	unsigned cx_close = rc_close.right - rc_close.left;
	dwp = DeferWindowPos(dwp, m_viewer.get_wnd(), NULL, 0, 0, width, height-cy_close-2, SWP_NOZORDER);
	dwp = DeferWindowPos(dwp, wnd_close, NULL, width-cx_close, height-cy_close, cx_close, cy_close, SWP_NOZORDER|SWP_NOSIZE);
	int cx = width-cx_close-2-rc_next.right+rc_next.left;
	dwp = DeferWindowPos(dwp, wnd_last, NULL, cx, height-cy_close, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	dwp = DeferWindowPos(dwp, wnd_next, NULL, cx -= (rc_last.right-rc_last.left +2), height-cy_close, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	dwp = DeferWindowPos(dwp, wnd_prev, NULL, cx -= (rc_prev.right-rc_prev.left +2), height-cy_close, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	dwp = DeferWindowPos(dwp, wnd_nextf, NULL, cx -= (rc_next_format.right-rc_next_format.left + 2), height-cy_close,0, 0, SWP_NOZORDER|SWP_NOSIZE);
	dwp = DeferWindowPos(dwp, wnd_prevf, NULL, cx -= (rc_prev_format.right-rc_prev_format.left + 2), height-cy_close, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	EndDeferWindowPos(dwp);
}

BOOL ipod_browse_photo_dialog::DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			m_this=this;
			//m_wnd=wnd;
			ShowWindow(m_viewer.create(wnd, NULL), SW_SHOWNORMAL);
			modeless_dialog_manager::g_add(wnd);
			RECT rc;
			GetClientRect(wnd, &rc);
			on_size(wnd, rc.right, rc.bottom);
			refresh_photo();
		}
		break;
	case WM_COMMAND:
		switch (wp)
		{
		case IDCANCEL:
			DestroyWindow(wnd);
			return 0;
		case IDC_NEXT:
			if (m_index + 1< m_library.artwork_object.image_list.get_count())
			{
				m_index++;
				m_format=0;
				refresh_photo();
			}
			return 0;
		case IDC_LAST:
			if (m_library.artwork_object.image_list.get_count())
			{
				m_index = m_library.artwork_object.image_list.get_count()-1;
				m_format=0;
				refresh_photo();
			}
			return 0;
		case IDC_PREVIOUS:
			if (m_index)
			{
				m_index--;
				m_format=0;
				refresh_photo();
			}
			return 0;
		case IDC_PREVIOUS_FORMAT:
			if (m_format)
			{
				m_format--;
				refresh_photo();
			}
			return 0;
		case IDC_NEXT_FORMAT:
			if (m_index < m_library.artwork_object.image_list.get_count() && m_format + 1 < m_library.artwork_object.image_list[m_index].image_names.get_count())
			{
				m_format++;
				refresh_photo();
			}
			return 0;
		}
		break;
	case WM_SIZE:
		on_size (wnd, LOWORD(lp), HIWORD(lp));
		return 0;
	case WM_DESTROY:
		m_viewer.destroy();
		return 0;
	case WM_NCDESTROY:
		modeless_dialog_manager::g_remove(wnd);
		SetWindowLongPtr(wnd, DWL_USER, NULL);
		m_this.release();
		break;
	}
	return FALSE;
}

void t_photo_browser::on_run()
{
	m_failed = false;
	try {
		m_drive_scanner.run( m_process,m_process.get_abort());
		//p_status.update_progress(1,2);
		m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort(), true);
		//p_status.update_progress(2,2);
	}
	catch (exception_aborted) {}
	catch (const pfc::exception & e) 
	{
		fbh::show_info_box_threadsafe("Error", e.what());
		m_failed = true;
	}
}

enum t_bitmap_format
{
	format_uyvy,
	format_rgb565_le,
	format_rgb565_be_90,
};

struct t_bitmap_info
{
	t_bitmap_format format;
	t_uint32 width;
	t_uint32 height;
};

bool get_bitmap_format(t_uint32 size, t_bitmap_info & p_out)
{
	switch (size)
	{
	case 320*320*2:
		p_out.format = format_rgb565_le;
		p_out.width = 320;
		p_out.height = 320;
		return true;
	case 128*128*2:
		p_out.format = format_rgb565_le;
		p_out.width = 128;
		p_out.height = 128;
		return true;
	case 56*55*2:
		p_out.format = format_rgb565_le;
		p_out.width = 55;
		p_out.height = 56;
		return true;
	case 691200:
		p_out.format = format_uyvy;
		p_out.width = 720;
		p_out.height = 480;
		return true;
	case 153600:
		p_out.format = format_rgb565_le;
		p_out.width = 320;
		p_out.height = 240;
		return true;
	case 80000:
		p_out.format = format_rgb565_le;
		p_out.width = 200;
		p_out.height = 200;
		return true;
	case 77440:
		p_out.format = format_rgb565_be_90;
		p_out.width = 220;
		p_out.height = 176;
		return true;
	case 46464:
		p_out.format = format_rgb565_le;
		p_out.width = 176;
		p_out.height = 132;
		return true;
	case 39200:
		p_out.format = format_rgb565_le;
		p_out.width = 140;
		p_out.height = 140;
		return true;	
	case 22880:
		p_out.format = format_rgb565_le;
		p_out.width = 130;
		p_out.height = 88;
		return true;
	case 20000:
		p_out.format = format_rgb565_le;
		p_out.width = 100;
		p_out.height = 100;
		return true;
	case 6272:
		p_out.format = format_rgb565_le;
		p_out.width = 56;
		p_out.height = 56;
		return true;
	case 4100:
		p_out.format = format_rgb565_le;
		p_out.width = 50;
		p_out.height = 41;
		return true;
	case 3528:
		p_out.format = format_rgb565_le;
		p_out.width = 42;
		p_out.height = 42;
		return true;
	case 3108:
		p_out.format = format_rgb565_le;
		p_out.width = 42;
		p_out.height = 37;
		return true;
	case 2520:
		p_out.format = format_rgb565_le;
		p_out.width = 42;
		p_out.height = 30;
		return true;
	};
	return false;
}

void t_threaded_file_reader::run(threaded_process_status & p_status,abort_callback & p_abort)
{
	try 
	{
		service_ptr_t<file> p_file;
		filesystem::g_open_read(p_file, m_path, p_abort);
		m_data.set_size((t_size)m_size);
		p_file->seek(m_offset, p_abort);
		p_file->read(m_data.get_ptr() , m_data.get_size(), p_abort);
	}
	catch (const pfc::exception &) 
	{
		m_failed=true;
	};
}

void ipod_browse_photo_dialog::refresh_photo()
{
		t_size count = m_library.artwork_object.image_list.get_count();
		bool found = false;
		if (m_index < count && m_format < m_library.artwork_object.image_list[m_index].image_names.get_count())
			{
				ipod_device_ptr_cref_t p_ipod = m_this->m_drive_scanner.m_ipods[0];
				photodb::t_image_name & image = m_library.artwork_object.image_list[m_index].image_names[m_format];
				
				artwork_format_t p_format;
				if (p_ipod->m_device_properties.get_artwork_format_by_id(image.correlation_id, p_format))
				{
					//artwork_format_t const & p_format = p_ipod->m_device_properties.m_artwork_formats[artwork_format_index];

					found = true;
					pfc::string8 temp = image.location;
					temp.replace_byte(':', p_ipod->get_path_separator());
					pfc::string8 path;
					p_ipod->get_database_path(path);
					path << p_ipod->get_path_separator_ptr() << "Artwork" << temp;
					//path << "\\Photos" << temp;

					service_ptr_t<t_threaded_file_reader> callback = new service_impl_t< t_threaded_file_reader > (path, image.file_offset, image.file_size);
					;

					if (threaded_process::g_run_modal(callback, threaded_process::flag_show_abort, core_api::get_main_window(), "Loading image...") && !callback->m_failed)
					{
						if (callback->m_data.get_size() == p_format.get_raw_size())
						{
							const t_uint8 * p_data = callback->m_data.get_ptr();

							bitmap_utils::bitmap_from_alternative_pixel_order_t reordered((t_uint16*)p_data, p_format.m_render_width, p_format.m_render_height, p_format.get_row_stride() / 2);
							if (p_format.m_alternate_pixel_order)
							{
								p_data = reordered.from_alternative_order();
							}

							if (p_format.m_pixel_format == 'UYUV')
								m_viewer.set_bitmap(bitmap_utils::create_bitmap_from_uyvy(p_data, callback->m_data.get_size(), p_format.m_render_width, p_format.m_render_height));
							else if (p_format.m_pixel_format == 'L565')
								m_viewer.set_bitmap(bitmap_utils::create_bitmap_from_rgb565(p_data, callback->m_data.get_size(), p_format.m_render_width, p_format.m_render_height, p_format.get_row_stride()));
							else if (p_format.m_pixel_format == 'L555')
								m_viewer.set_bitmap(bitmap_utils::create_bitmap_from_rgb565(p_data, callback->m_data.get_size(), p_format.m_render_width, p_format.m_render_height, p_format.get_row_stride(), true));
							else if (p_format.m_pixel_format == 'jpeg')
								m_viewer.set_bitmap(bitmap_utils::create_bitmap_from_jpeg(p_data, callback->m_data.get_size(), p_format.m_render_width, p_format.m_render_height));
						}
						RedrawWindow(m_viewer.get_wnd(), NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
					}
				}
			}
}


void t_photo_browser::on_exit() 
{
	if (!m_process.get_abort().is_aborting() && !m_failed)
	{
		pfc::refcounted_object_ptr_t<ipod_browse_photo_dialog> p_test = new ipod_browse_photo_dialog(this);
		HWND wnd = uCreateDialog(IDD_BROWSE_PHOTOS, core_api::get_main_window(), ipod_browse_photo_dialog::g_DialogProc, (LPARAM)p_test.get_ptr());
		ShowWindow(wnd, SW_SHOWNORMAL);
		//m_this = this;
	}
}
#endif 
