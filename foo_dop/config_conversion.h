#pragma once

#include "config.h"
#include "helpers.h"
#include "resource.h"

class verify_encoder_settings_t : public threaded_process_v2_t {
public:
	verify_encoder_settings_t(const settings::conversion_preset_t & p_encoder, const char * p_temp_folder)
		: threaded_process_v2_t("Verify Encoder Settings",
			threaded_process_v2_t::flag_show_progress_window | threaded_process_v2_t::flag_progress_marquee |
			threaded_process_v2_t::flag_show_button | threaded_process_v2_t::flag_block_app_close), m_encoder(p_encoder), m_temp_folder(p_temp_folder)
	{};

private:
	void on_run();
	void on_exit()
	{
		popup_message::g_show(m_error_log, "Result - Verify Encoder Settings");
		delete this;
	}
	settings::conversion_preset_t m_encoder;
	pfc::string8 m_error_log, m_temp_folder;
};

class encoder_manager_t {
public:
	encoder_manager_t() : m_wnd(NULL), m_wnd_conversion_command(NULL), m_wnd_conversion_command_browse(NULL), m_wnd_conversion_extension(NULL),
		m_wnd_conversion_parameters(NULL), m_wnd_delete(NULL)
	{};

	enum { MSG_SELECTION_CHANGED = WM_USER + 2 };

	HWND create_or_activate(HWND wnd_owner = core_api::get_main_window())
	{
		if (m_wnd)
			SetActiveWindow(m_wnd);
		else
		{
			m_wnd = CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_ENCODER_MANAGER), wnd_owner, g_DialogProc, (LPARAM)this);
			ShowWindow(m_wnd, SW_SHOWNORMAL);
		}
		return m_wnd;
	}
	void sort_encoder_list();
private:
	class encoder_list_view_t : public uih::ListView {
	public:
		encoder_list_view_t() : m_edit_column(pfc_infinite), m_edit_index(pfc_infinite) {};
		template<class TArray>
		void get_insert_items(TArray & p_items)
		{
			for (t_size n = 0, count = p_items.get_count(); n<count; n++)
			{
				p_items[n].m_subitems.emplace_back(settings::encoder_list[n].m_name);
			}
		}
		void populate()
		{
			t_size count = settings::encoder_list.get_count();
			pfc::list_t<uih::ListView::InsertItem> items;
			items.set_count(count);
			get_insert_items(items);
			insert_items(0, count, items.get_ptr());
		}
		void on_sort()
		{
			t_size count = settings::encoder_list.get_count();
			pfc::list_t<uih::ListView::InsertItem> items;
			items.set_count(count);
			get_insert_items(items);
			replace_items(0, items);
		}
	private:
		void notify_on_initialisation()
		{
			set_single_selection(true);
			set_autosize(true);
			set_columns({ {"Name", 100} });
		}
		void notify_on_selection_change(const bit_array & p_affected, const bit_array & p_status, notification_source_t p_notification_source)
		{
			SendMessage(GetAncestor(get_wnd(), GA_PARENT), MSG_SELECTION_CHANGED, NULL, NULL);
		}
		virtual void execute_default_action(t_size index, t_size column, bool b_keyboard, bool b_ctrl)
		{
			activate_inline_editing();
		}
		virtual bool notify_before_create_inline_edit(const pfc::list_base_const_t<t_size> & indices, unsigned column, bool b_source_mouse)
		{
			if (column == 0 && indices.get_count() == 1)
				return true;
			return false;
		};
		virtual bool notify_create_inline_edit(const pfc::list_base_const_t<t_size> & indices, unsigned column, pfc::string_base & p_text, t_size & p_flags, mmh::ComPtr<IUnknown> & pAutocompleteEntries);;
		virtual void notify_save_inline_edit(const char * value);
		t_size m_edit_column, m_edit_index;
	};

	void on_selection_change(t_size index)
	{
		m_initialising = true;
		bool b_enable;
		if ((b_enable = index < settings::encoder_list.get_count()))
		{
			uSetWindowText(m_wnd_conversion_command, settings::encoder_list[index].m_executable);
			uSetWindowText(m_wnd_conversion_parameters, settings::encoder_list[index].m_parameters);
			uSetWindowText(m_wnd_conversion_extension, settings::encoder_list[index].m_file_extension);
			Button_SetCheck(m_wnd_encoder_ral, settings::encoder_list[index].m_encoder_requires_accurate_length ? BST_CHECKED : BST_UNCHECKED);
			ComboBox_SetCurSel(m_wnd_max_bps_dropdown, settings::encoder_list[index].m_highest_bps_supported - 2);
		}
		else
		{
			uSetWindowText(m_wnd_conversion_command, "");
			uSetWindowText(m_wnd_conversion_parameters, "");
			uSetWindowText(m_wnd_conversion_extension, "");
			Button_SetCheck(m_wnd_encoder_ral, BST_UNCHECKED);
			ComboBox_SetCurSel(m_wnd_max_bps_dropdown, settings::conversion_preset_t::bps_32 - 2);
		}
		EnableWindow(m_wnd_conversion_command, b_enable);
		EnableWindow(m_wnd_conversion_parameters, b_enable);
		EnableWindow(m_wnd_conversion_extension, b_enable);
		EnableWindow(m_wnd_conversion_command_browse, b_enable);
		EnableWindow(m_wnd_delete, b_enable);
		EnableWindow(m_wnd_encoder_ral, b_enable);
		EnableWindow(m_wnd_max_bps_dropdown, b_enable);
		m_initialising = false;
	}

	bool get_selection_index(t_size & p_index)
	{
		p_index = m_encoder_list_view.get_selected_item_single();
		return (p_index < settings::encoder_list.get_count());
	}

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	HWND m_wnd;
	encoder_list_view_t m_encoder_list_view;
	HWND m_wnd_conversion_command, m_wnd_delete;
	HWND m_wnd_conversion_command_browse;
	HWND m_wnd_conversion_parameters;
	HWND m_wnd_conversion_extension, m_wnd_encoder_ral, m_wnd_max_bps_dropdown;
	bool m_initialising;
};

extern encoder_manager_t g_encoder_manager;

class t_config_tab_conversion : public preferences_tab {
	friend void g_update_conversion_prefs_encoders();
private:
	//HWND m_wnd_conversion_command;
	//HWND m_wnd_conversion_command_browse;
	//HWND m_wnd_conversion_parameters;
	//HWND m_wnd_conversion_extension;
	HWND m_wnd_conversion_thread_mode;
	HWND m_wnd_conversion_thread_count;
	HWND m_wnd_conversion_thread_count_spin;
	HWND m_wnd_convert_files;
	HWND m_wnd_convert_files_above_bitrate;
	HWND m_wnd_conversion_bitrate_limit_value;
	HWND m_wnd_conversion_bitrate_limit_value_spin;
	HWND m_wnd_replaygain_processing_mode;
	HWND m_wnd_encoder_dropdown;

	bool m_initialising, m_have_rgscan;

	void on_conversion_thread_mode_change()
	{
		pfc::vartoggle_t<bool> pTogg(m_initialising, true);
		bool b_enable = settings::convert_files || settings::conversion_use_bitrate_limit;
		EnableWindow(m_wnd_conversion_thread_count_spin, b_enable && settings::conversion_use_custom_thread_count);
		EnableWindow(m_wnd_conversion_thread_count, b_enable && settings::conversion_use_custom_thread_count);
		SendMessage(m_wnd_conversion_thread_count_spin, UDM_SETPOS32, 0,
			settings::conversion_use_custom_thread_count ? settings::conversion_custom_thread_count : std::thread::hardware_concurrency());
	}

	void on_convert_files_change()
	{
		bool b_enable = settings::convert_files || settings::conversion_use_bitrate_limit;
		//EnableWindow(m_wnd_conversion_command, b_enable);
		//EnableWindow(m_wnd_conversion_command_browse, b_enable);
		//EnableWindow(m_wnd_conversion_parameters, b_enable);
		//EnableWindow(m_wnd_conversion_extension, b_enable);
		on_conversion_thread_mode_change();
		EnableWindow(m_wnd_replaygain_processing_mode, b_enable);
		EnableWindow(m_wnd_conversion_thread_mode, b_enable);
		EnableWindow(m_wnd_encoder_dropdown, b_enable);
		EnableWindow(m_wnd_conversion_bitrate_limit_value_spin, settings::conversion_use_bitrate_limit);
		EnableWindow(m_wnd_conversion_bitrate_limit_value, settings::conversion_use_bitrate_limit);
	}

	void repopulate_encoder_list()
	{
		if (m_wnd_encoder_dropdown)
		{
			ComboBox_ResetContent(m_wnd_encoder_dropdown);
			for (t_size i = 0, count = settings::encoder_list.get_count(); i<count; i++)
				ComboBox_AddString(m_wnd_encoder_dropdown, pfc::stringcvt::string_os_from_utf8(settings::encoder_list[i].m_name));
			ComboBox_SetCurSel(m_wnd_encoder_dropdown, settings::active_encoder);
		}
	}

	BOOL DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

	static BOOL CALLBACK g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

public:
	HWND create(HWND parent)
	{
		return CreateDialogParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_CONVERSION), parent, g_DialogProc, (LPARAM)this);
	}
	const char * get_name() { return "Conversion"; }
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:config:conversion";
		return true;
	}
	t_config_tab_conversion() :
		//m_wnd_conversion_command(NULL), m_wnd_conversion_extension(NULL),
		m_wnd_convert_files(NULL), m_wnd_encoder_dropdown(NULL), m_wnd_replaygain_processing_mode(NULL),
		m_initialising(false), m_have_rgscan(false)
	{};
};

