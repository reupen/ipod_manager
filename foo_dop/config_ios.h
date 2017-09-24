#pragma once

#include "config.h"

class t_config_tab3 : public preferences_tab {
private:
	HWND m_wnd_enabled;

	bool m_initialising;

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_MOBILE, parent, g_DialogProc, (LPARAM)this);
	}
	const char * get_name() { return "Mobile Devices"; }
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:config:mobile_devices";
		return true;
	}
	t_config_tab3() :
		m_wnd_enabled(NULL), m_initialising(false)
	{};
};
