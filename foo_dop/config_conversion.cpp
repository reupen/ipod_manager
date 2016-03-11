#include "main.h"

encoder_manager_t g_encoder_manager;

void verify_encoder_settings_t::on_run()
{
	try
	{
		t_uint32 rand32 = 1;
		mmh::genrand_t().run(&rand32, 4);


		pfc::string8 tempFolder = m_temp_folder, tempFile;
		if (!tempFolder.length() && !uGetTempPath(tempFolder))
			throw pfc::exception("uGetTempPath failed");
		char last_char = tempFolder.is_empty() ? 0 : tempFolder[tempFolder.get_length() - 1];
		if (last_char != '\\' && last_char != '/')
			tempFolder.add_byte('\\');
		DWORD attribs = uGetFileAttributes(tempFolder);
		if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY))
			throw pfc::exception("Invalid conversion temporary files folder");
		tempFile << tempFolder << "dop" << pfc::format_hex(rand32, 8) << ".etest.dop.tmp." << m_encoder.m_file_extension;

		metadb_handle_ptr p_handle;
		static_api_ptr_t<metadb>()->handle_create(p_handle, make_playable_location("tone://1000,5", 0));
		g_convert_file_v2(p_handle, tempFile, m_encoder, 0, 0, get_abort());
		filesystem::g_remove(pfc::string8() << "file://" << tempFile, abort_callback_dummy());
		m_error_log << "Testing concluded successfully.";
	}
	catch (pfc::exception const & ex)
	{
		m_error_log << "Error during testing: " << ex.what();
	}
}

void encoder_manager_t::sort_encoder_list()
{
	t_size count = settings::encoder_list.get_count();
	mmh::permutation_t permuation(count);
	mmh::g_sort_get_permutation_qsort_v2(settings::encoder_list.get_ptr(), permuation, settings::conversion_preset_t::g_compare, true);
	mmh::permutation_inverse_t permuation_inverse(permuation);
	if (settings::active_encoder < count)
		settings::active_encoder = permuation_inverse[settings::active_encoder];
	settings::encoder_list.reorder(permuation.get_ptr());
	if (m_encoder_list_view.get_wnd())
	{
		t_size index = m_encoder_list_view.get_selected_item_single();
		m_encoder_list_view.on_sort();
		if (index < count)
		{
			m_encoder_list_view.set_item_selected_single(permuation_inverse[index]);
			//for (t_size i=0; i<count; i++)
			//	console::formatter() << index << " " << index << " " << permuation_inverse[index] << " " << permuation[index];
		}
		else
			m_encoder_list_view.update_window();

	}
}

BOOL encoder_manager_t::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			modeless_dialog_manager::g_add(wnd);
			m_wnd_conversion_command = GetDlgItem(wnd, IDC_COMMAND);
			m_wnd_conversion_command_browse = GetDlgItem(wnd, IDC_BROWSE);
			m_wnd_conversion_parameters = GetDlgItem(wnd, IDC_PARAMETERS);
			m_wnd_conversion_extension = GetDlgItem(wnd, IDC_EXTENSION);
			m_wnd_delete = GetDlgItem(wnd, IDC_DELETE);
			m_wnd_encoder_ral = GetDlgItem(wnd, IDC_ENCODER_RAL);
			m_wnd_max_bps_dropdown = GetDlgItem(wnd, IDC_MAX_BPS);

			SHAutoComplete(m_wnd_conversion_command, SHACF_FILESYS_ONLY);

			uih::ComboBox_AddStringData(m_wnd_max_bps_dropdown, L"16", settings::conversion_preset_t::bps_16);
			uih::ComboBox_AddStringData(m_wnd_max_bps_dropdown, L"24", settings::conversion_preset_t::bps_24);
			uih::ComboBox_AddStringData(m_wnd_max_bps_dropdown, L"32", settings::conversion_preset_t::bps_32);

			HWND wnd_lv = m_encoder_list_view.create_in_dialog_units(wnd, ui_helpers::window_position_t(7, 7, 300, 86));
			m_encoder_list_view.populate();
			t_size count = m_encoder_list_view.get_item_count();
			if (count)
				m_encoder_list_view.set_item_selected_single(settings::active_encoder < count ? settings::active_encoder : 0);
			else
				on_selection_change(pfc_infinite);
			ShowWindow(wnd_lv, SW_SHOWNORMAL);
			SetFocus(wnd_lv);
		};
		break;
		case WM_PAINT:
			uih::HandleModernBackgroundPaint(wnd, GetDlgItem(wnd, IDOK));
			return TRUE;
		case WM_CTLCOLORSTATIC:
			SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
			SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		case MSG_SELECTION_CHANGED:
		{
			t_size index = m_encoder_list_view.get_selected_item_single();
			on_selection_change(index);
		}
		break;
		case WM_COMMAND:
		{
			switch (LOWORD(wp))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(wnd);
					break;
				case IDC_RESET:
					if (MessageBox(wnd, L"Are you sure you restore the default encoders?", L"Reset Defined Encoders", MB_YESNO) == IDYES)
					{
						settings::encoder_list.reset();
						settings::active_encoder = 0;
						m_encoder_list_view.remove_items(bit_array_true());
						m_encoder_list_view.populate();
						m_encoder_list_view.set_item_selected_single(0);
						g_update_conversion_prefs_encoders();
					}
					break;
				case IDC_NEW:
				{
					const char * p_name = "<enter name>";
					t_size index = settings::encoder_list.add_item(settings::conversion_preset_t(p_name, "", "", "", settings::conversion_preset_t::bps_32));
					t_list_view::t_item_insert item;
					item.m_subitems.add_item(p_name);

					m_encoder_list_view.insert_items(index, 1, &item);
					m_encoder_list_view.set_item_selected_single(index);
					m_encoder_list_view.activate_inline_editing();
				}
				break;
				case IDC_DELETE:
				{
					t_size index;
					if (get_selection_index(index))
					{
						settings::encoder_list.remove_by_idx(index);
						m_encoder_list_view.remove_items(bit_array_one(index));
						if (index)
							m_encoder_list_view.set_item_selected_single(index - 1);
						else if (index < m_encoder_list_view.get_item_count())
							m_encoder_list_view.set_item_selected_single(index);
						else
							on_selection_change(pfc_infinite);
						if (index < settings::active_encoder)
							settings::active_encoder = settings::active_encoder - 1;
						else if (index == settings::active_encoder)
							settings::active_encoder = pfc_infinite;
						g_update_conversion_prefs_encoders();
					}
				};
				break;
				case IDC_BROWSE:
				{
					t_size index = m_encoder_list_view.get_selected_item_single();
					pfc::string8 temp;
					if (index < settings::encoder_list.get_count())
					{
						uGetFullPathName(settings::encoder_list[index].m_executable, temp);
						if (uGetOpenFileName(wnd, "Executables (*.exe)|*.exe", 0, "exe", "Choose encoder executable", NULL, temp, FALSE))
						{
							settings::encoder_list[index].m_executable = temp;
							uSetWindowText(m_wnd_conversion_command, settings::encoder_list[index].m_executable);
						}
					}
				}
				break;
				case IDC_COMMAND:
					if (!m_initialising && HIWORD(wp) == EN_CHANGE)
					{
						t_size index;
						if (get_selection_index(index))
							settings::encoder_list[index].m_executable = string_utf8_from_window((HWND)lp);
					}
					break;
				case IDC_PARAMETERS:
					if (!m_initialising && HIWORD(wp) == EN_CHANGE)
					{
						t_size index;
						if (get_selection_index(index))
							settings::encoder_list[index].m_parameters = string_utf8_from_window((HWND)lp);
					}
					break;
				case IDC_EXTENSION:
					if (!m_initialising && HIWORD(wp) == EN_CHANGE)
					{
						t_size index;
						if (get_selection_index(index))
							settings::encoder_list[index].m_file_extension = string_utf8_from_window((HWND)lp);
					}
					break;
				case IDC_ENCODER_RAL:
					if (!m_initialising && HIWORD(wp) == BN_CLICKED)
					{
						t_size index;
						if (get_selection_index(index))
							settings::encoder_list[index].m_encoder_requires_accurate_length = (Button_GetCheck((HWND)lp) == BST_CHECKED);
					}
					break;
				case IDC_MAX_BPS:
					if (!m_initialising && HIWORD(wp) == CBN_SELCHANGE)
					{
						t_size index;
						INT_PTR val = ComboBox_GetCurSel((HWND)lp);
						if (get_selection_index(index) && val != CB_ERR)
							settings::encoder_list[index].m_highest_bps_supported = val + 2;
					}
					break;
			}
		}
		break;
		case WM_DESTROY:
			m_encoder_list_view.remove_items(bit_array_true());
			break;
		case WM_NCDESTROY:
			modeless_dialog_manager::g_remove(wnd);
			m_wnd = NULL;
			break;
	}
	return FALSE;
}

BOOL encoder_manager_t::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	encoder_manager_t * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<encoder_manager_t*>(lp);
			break;
		default:
			p_this = reinterpret_cast<encoder_manager_t*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}

	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

BOOL t_config_tab_conversion::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialising = true;
			//m_wnd_conversion_command = GetDlgItem(wnd, IDC_COMMAND);
			//m_wnd_conversion_command_browse = GetDlgItem(wnd, IDC_BROWSE);
			//m_wnd_conversion_parameters = GetDlgItem(wnd, IDC_PARAMETERS);
			//m_wnd_conversion_extension = GetDlgItem(wnd, IDC_EXTENSION);
			m_wnd_convert_files = GetDlgItem(wnd, IDC_CONVERT);
			m_wnd_conversion_thread_mode = GetDlgItem(wnd, IDC_THREADMODE);
			m_wnd_conversion_thread_count = GetDlgItem(wnd, IDC_THREADCOUNT);
			m_wnd_conversion_thread_count_spin = GetDlgItem(wnd, IDC_THREADCOUNTSPIN);
			m_wnd_replaygain_processing_mode = GetDlgItem(wnd, IDC_RGMODE);
			m_wnd_encoder_dropdown = GetDlgItem(wnd, IDC_ENCODER_COMBO);

			m_wnd_conversion_bitrate_limit_value = GetDlgItem(wnd, IDC_CONVERT_BITRATE_VALUE);
			m_wnd_conversion_bitrate_limit_value_spin = GetDlgItem(wnd, IDC_CONVERT_BITRATE_VALUE_SPIN);
			m_wnd_convert_files_above_bitrate = GetDlgItem(wnd, IDC_CONVERT_BITRATE);

			repopulate_encoder_list();


			ComboBox_AddString(m_wnd_conversion_thread_mode, L"Auto");
			ComboBox_AddString(m_wnd_conversion_thread_mode, L"Custom");
			ComboBox_SetCurSel(m_wnd_conversion_thread_mode, settings::conversion_use_custom_thread_count ? 1 : 0);
			SendMessage(m_wnd_conversion_thread_count_spin, UDM_SETRANGE32, 1, MAXLONG);
			//SendMessage(m_wnd_conversion_thread_count_spin, UDM_SETPOS32, 0, settings::conversion_custom_thread_count);

			SendMessage(m_wnd_conversion_bitrate_limit_value_spin, UDM_SETRANGE32, 1, MAXLONG);
			SendMessage(m_wnd_conversion_bitrate_limit_value_spin, UDM_SETPOS32, 0, settings::conversion_bitrate_limit);

			//uSetWindowText(m_wnd_conversion_command, settings::conversion_command);
			//uSetWindowText(m_wnd_conversion_parameters, settings::conversion_parameters);
			//uSetWindowText(m_wnd_conversion_extension, settings::conversion_extension);
			Button_SetCheck(m_wnd_convert_files, settings::convert_files ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_convert_files_above_bitrate, settings::conversion_use_bitrate_limit ? BST_CHECKED : BST_UNCHECKED);
			{
				try
				{
					static_api_ptr_t<replaygain_scanner_entry>();
					m_have_rgscan = true;
				}
				catch (exception_service_not_found const &) {};

				if (!m_have_rgscan)
					ShowWindow(GetDlgItem(wnd, IDC_RGWARNING), SW_SHOWNORMAL);
				//EnableWindow(m_wnd_replaygain_processing_mode, m_have_rgscan);
				//Button_SetCheck(m_wnd_replaygain_processing_mode, m_have_rgscan && settings::replaygain_processing_mode ? BST_CHECKED : BST_UNCHECKED);
			}
			uih::ComboBox_AddStringData(m_wnd_replaygain_processing_mode, L"None", 0);
			uih::ComboBox_AddStringData(m_wnd_replaygain_processing_mode, L"Apply gain before encoding", 2);
			if (m_have_rgscan)
				uih::ComboBox_AddStringData(m_wnd_replaygain_processing_mode, L"Calculate gain after encoding", 1);
			ComboBox_SetCurSel(m_wnd_replaygain_processing_mode, settings::replaygain_processing_mode ? 3 - settings::replaygain_processing_mode : 0);

			on_convert_files_change();
			on_conversion_thread_mode_change();

			m_initialising = false;
		}
		break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDC_VERIFY_ENCODER:
				{
					if (settings::active_encoder < settings::encoder_list.get_count())
					{
						pfc::string8 p_temp_folder;
						settings::conversion_temp_files_folder.get_static_instance().get_state(p_temp_folder);
						verify_encoder_settings_t * p_verify_encoder_settings = new verify_encoder_settings_t(settings::encoder_list[settings::active_encoder], p_temp_folder);
						p_verify_encoder_settings->run(core_api::get_main_window());
					}
					else message_window_t::g_run(wnd, "Verify Encoder Settings", "Error: No encoder selected");
				}
				break;
				case IDC_ENCODER_MANAGER:
					g_encoder_manager.create_or_activate(GetAncestor(wnd, GA_ROOT));
					break;
				case IDC_CONVERT | (BN_CLICKED << 16) :
					settings::convert_files = Button_GetCheck(m_wnd_convert_files) == BST_CHECKED;
					on_convert_files_change();
					break;
				case IDC_CONVERT_BITRATE | (BN_CLICKED << 16) :
					settings::conversion_use_bitrate_limit = Button_GetCheck(m_wnd_convert_files_above_bitrate) == BST_CHECKED;
					on_convert_files_change();
					break;
				case IDC_THREADCOUNT | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::conversion_custom_thread_count = strtoul_n(string_utf8_from_window((HWND)lp), pfc_infinite);
					break;
				case IDC_CONVERT_BITRATE_VALUE | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::conversion_bitrate_limit = strtoul_n(string_utf8_from_window((HWND)lp), pfc_infinite);
					break;
				case IDC_THREADMODE | (CBN_SELCHANGE << 16) :
					if (!m_initialising)
					{
						settings::conversion_use_custom_thread_count = ComboBox_GetCurSel(m_wnd_conversion_thread_mode) != 0;
						on_conversion_thread_mode_change();
					}
															break;
				case IDC_ENCODER_COMBO | (CBN_SELCHANGE << 16) :
					if (!m_initialising)
					{
						settings::active_encoder = ComboBox_GetCurSel(m_wnd_encoder_dropdown);
					}
															   break;
				case IDC_RGMODE | (CBN_SELCHANGE << 16) :
					if (!m_initialising)
					{
						int sel = ComboBox_GetCurSel(m_wnd_replaygain_processing_mode);
						if (sel >= 0 && sel <= 2)
							settings::replaygain_processing_mode = (t_uint8)ComboBox_GetItemData(m_wnd_replaygain_processing_mode, sel);
					}
														break;
#if 0
				case IDC_BROWSE:
				{
					pfc::string8 temp;
					uGetFullPathName(settings::conversion_command, temp);
					if (uGetOpenFileName(wnd, "Executables (*.exe)|*.exe", 0, "exe", "Choose encoder", NULL, temp, FALSE))
					{
						settings::conversion_command = temp;
						uSetWindowText(m_wnd_conversion_command, settings::conversion_command);
					}
				}
				break;
				case IDC_COMMAND | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::conversion_command = string_utf8_from_window((HWND)lp);
					break;
				case IDC_PARAMETERS | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::conversion_parameters = string_utf8_from_window((HWND)lp);
					break;
				case IDC_EXTENSION | (EN_CHANGE << 16) :
					if (!m_initialising)
						settings::conversion_extension = string_utf8_from_window((HWND)lp);
					break;
#endif
			}
			break;
		case WM_DESTROY:
			//m_wnd_conversion_command = NULL;
			//m_wnd_conversion_command_browse = NULL;
			//m_wnd_conversion_parameters = NULL;
			//m_wnd_conversion_extension = NULL;
			m_wnd_convert_files = NULL;
			m_wnd_conversion_thread_mode = NULL;
			m_wnd_conversion_thread_count = NULL;
			m_wnd_conversion_thread_count_spin = NULL;
			m_wnd_replaygain_processing_mode = NULL;
			m_wnd_encoder_dropdown = NULL;
			m_wnd_replaygain_processing_mode = NULL;
			break;
	}
	return FALSE;
}

BOOL t_config_tab_conversion::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	t_config_tab_conversion * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_tab_conversion*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_tab_conversion*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);
	return FALSE;
}

bool encoder_manager_t::encoder_list_view_t::notify_create_inline_edit(const pfc::list_base_const_t<t_size>& indices, unsigned column, pfc::string_base & p_text, t_size & p_flags, mmh::comptr_t<IUnknown>& pAutocompleteEntries)
{
	t_size indices_count = indices.get_count();
	if (indices_count == 1)
	{
		m_edit_index = indices[0];
		m_edit_column = column;

		if (m_edit_index < settings::encoder_list.get_count())
			p_text = settings::encoder_list[m_edit_index].m_name;

		return true;
	}
	return false;
}

void encoder_manager_t::encoder_list_view_t::notify_save_inline_edit(const char * value)
{
	if (m_edit_index < settings::encoder_list.get_count())
	{
		settings::encoder_list[m_edit_index].m_name = value;

		{
			pfc::list_t<t_list_view::t_item_insert> items;
			items.set_count(1);
			{
				items[0].m_subitems.add_item(settings::encoder_list[m_edit_index].m_name);
			}
			replace_items(m_edit_index, items);
			g_sort_converstion_encoders();
		}
	}
	m_edit_column = pfc_infinite;
	m_edit_index = pfc_infinite;
}
