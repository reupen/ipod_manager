#pragma once

#include "config.h"

class t_config_tab4 : public preferences_tab {
private:
	HWND m_wnd_sort_library,
		m_wnd_sort_library_script,
		m_wnd_devices_panel_autosend,
		m_wnd_devices_panel_autosend_playlist;

	HWND m_wnd_quiet_sync;

	bool m_initialising;

	void on_sort_library_change()
	{
		EnableWindow(m_wnd_sort_library_script, settings::sort_ipod_library);
	}

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_BEHAVIOUR, parent, g_DialogProc, (LPARAM)this);
	}
	const char * get_name() { return "Behaviour"; }
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:config:behaviour";
		return true;
	}
	t_config_tab4() :
		m_wnd_sort_library(NULL), m_wnd_sort_library_script(NULL),
		m_wnd_devices_panel_autosend(NULL), m_wnd_devices_panel_autosend_playlist(NULL),
		m_initialising(false)
	{};
};
