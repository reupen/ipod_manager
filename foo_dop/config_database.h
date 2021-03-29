#pragma once

#include "config.h"
#include "resource.h"

struct mapping_t {
	const char * m_field;
	cfg_string & m_value;

	mapping_t(const char * p_field, cfg_string & p_value) : m_field(p_field), m_value(p_value) {};
};


class t_config_tab1 : public preferences_tab {
private:

	class t_list_view_filter : public uih::ListView {
	public:
		t_size m_edit_index, m_edit_column;
		t_list_view_filter() : m_edit_index(pfc_infinite), m_edit_column(pfc_infinite) {};

		virtual void notify_on_create()
		{
			set_single_selection(true);
			const std::vector<Column> columns = {{"Field", 95}, {"Mapping", 310}};
			set_columns(columns);
		};
		virtual void execute_default_action(t_size index, t_size column, bool b_keyboard, bool b_ctrl)
		{
			activate_inline_editing();
		}
		virtual bool notify_before_create_inline_edit(const pfc::list_base_const_t<t_size> & indices, unsigned column, bool b_source_mouse)
		{
			if (column == 1 && indices.get_count() == 1)
				return true;
			return false;
		};
		virtual bool notify_create_inline_edit(const pfc::list_base_const_t<t_size> & indices, unsigned column, pfc::string_base & p_text, t_size & p_flags, mmh::ComPtr<IUnknown> & pAutocompleteEntries);;
		virtual void notify_save_inline_edit(const char * value);
	private:
	} m_field_list;

	HWND m_wnd_use_ipod_sorting;
	HWND m_wnd_allow_sort_order;
	HWND m_wnd_numbers_last;
	HWND m_wnd_sort_playlists;
	HWND m_wnd_date_added;
	HWND m_wnd_rgmode;

	bool m_initialising;

	void get_insert_items(t_size base, t_size count, pfc::list_t<uih::ListView::InsertItem> & items);

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

public:
	HWND create(HWND parent)
	{
		return CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_CONFIG), parent, g_DialogProc, (LPARAM)this);
	}
	const char * get_name() { return "Database"; }
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:config:database";
		return true;
	}
	t_config_tab1() : m_wnd_use_ipod_sorting(NULL), m_wnd_allow_sort_order(NULL),
		m_initialising(false), m_wnd_numbers_last(NULL), m_wnd_rgmode(NULL), m_wnd_sort_playlists(NULL)
	{};
};
