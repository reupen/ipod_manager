#pragma once

class t_config_tab2 : public preferences_tab {
private:
	HWND m_wnd_artwork_string;

	HWND m_wnd_add_artwork;
	HWND m_wnd_add_gapless;
	HWND m_wnd_dummy_gapless;

	HWND m_wnd_fb2k_artwork;
	HWND m_wnd_video_thumbailer_enabled;

	bool m_initialising;

	void on_add_artwork_change()
	{
		EnableWindow(m_wnd_artwork_string, settings::add_artwork);
		EnableWindow(m_wnd_fb2k_artwork, settings::add_artwork);
		EnableWindow(m_wnd_video_thumbailer_enabled, settings::add_artwork);
	}

	void on_add_gapless_change()
	{
		EnableWindow(m_wnd_dummy_gapless, settings::add_gapless);
	}

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_TOOLS, parent, g_DialogProc, (LPARAM)this);
	}
	const char * get_name() { return "iPod Features"; }
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:config:ipod_features";
		return true;
	}
	t_config_tab2() :
		m_wnd_artwork_string(0),
		m_wnd_add_artwork(NULL), m_wnd_add_gapless(NULL), m_wnd_dummy_gapless(NULL),
		m_wnd_fb2k_artwork(NULL), m_wnd_video_thumbailer_enabled(NULL), m_initialising(false)
	{};
};
