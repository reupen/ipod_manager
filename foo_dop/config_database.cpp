#include "main.h"

const mapping_t g_mappings[] = {
	mapping_t("Artist", settings::artist_mapping),
	mapping_t("Album artist", settings::album_artist_mapping),
	mapping_t("Title", settings::title_mapping),
	mapping_t("Album", settings::album_mapping),
	mapping_t("Composer", settings::composer_mapping),
	mapping_t("Genre", settings::genre_mapping),
	mapping_t("Comment", settings::comment_mapping),
	mapping_t("Compilation", settings::compilation_mapping),
	mapping_t("Sort artist", settings::sort_artist_mapping),
	mapping_t("Sort album artist", settings::sort_album_artist_mapping),
	mapping_t("Sort title", settings::sort_title_mapping),
	mapping_t("Sort album", settings::sort_album_mapping),
	mapping_t("Sort composer", settings::sort_composer_mapping),
	mapping_t("VoiceOver title", settings::voiceover_title_mapping),
};

void t_config_tab1::get_insert_items(t_size base, t_size count, pfc::list_t<t_list_view::t_item_insert>& items)
{
	t_size i;
	items.set_count(count);
	for (i = 0; i<count; i++)
	{
		items[i].m_subitems.add_item(g_mappings[base + i].m_field);
		items[i].m_subitems.add_item(g_mappings[base + i].m_value);
	}
}

BOOL t_config_tab1::DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			m_initialising = true;
			m_wnd_use_ipod_sorting = GetDlgItem(wnd, IDC_OMIT_THE);

			m_wnd_numbers_last = GetDlgItem(wnd, IDC_NUMBERS_LAST);
			m_wnd_sort_playlists = GetDlgItem(wnd, IDC_SORT_PLAYLISTS);
			uSendDlgItemMessage(wnd, IDC_SPIN_SC, UDM_SETRANGE32, -12, 12);
			m_wnd_date_added = GetDlgItem(wnd, IDC_ADDED);
			m_wnd_rgmode = GetDlgItem(wnd, IDC_RGMODE);
			m_wnd_allow_sort_order = GetDlgItem(wnd, IDC_ALLOW_SORTORDER);

			ComboBox_AddString(m_wnd_rgmode, L"Track gain");
			ComboBox_AddString(m_wnd_rgmode, L"Album gain");
			ComboBox_SetCurSel(m_wnd_rgmode, settings::soundcheck_rgmode);

			ComboBox_AddString(m_wnd_date_added, L"Date added to iPod");
			ComboBox_AddString(m_wnd_date_added, L"Date added to media library");

			uSendDlgItemMessage(wnd, IDC_SPIN_SC, UDM_SETPOS32, 0, settings::soundcheck_adjustment);
			Button_SetCheck(m_wnd_use_ipod_sorting, settings::use_ipod_sorting ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_numbers_last, settings::numbers_last ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_sort_playlists, settings::sort_playlists ? BST_CHECKED : BST_UNCHECKED);
			Button_SetCheck(m_wnd_allow_sort_order, settings::allow_sort_fields ? BST_CHECKED : BST_UNCHECKED);

			ComboBox_SetCurSel(m_wnd_date_added, settings::date_added_mode);

			HWND wnd_fields = m_field_list.create_in_dialog_units(wnd, ui_helpers::window_position_t(11, 28, 314 - 11 - 11, 88));

			pfc::list_t<t_list_view::t_item_insert> items;
			t_size count = tabsize(g_mappings);
			get_insert_items(0, count, items);
			m_field_list.insert_items(0, items.get_count(), items.get_ptr());

			SetWindowPos(wnd_fields, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			ShowWindow(wnd_fields, SW_SHOWNORMAL);
			m_initialising = false;
		}
		break;
		case WM_COMMAND:
			switch (wp)
			{
				case IDC_OMIT_THE | (BN_CLICKED << 16) :
					settings::use_ipod_sorting = Button_GetCheck(m_wnd_use_ipod_sorting) == BST_CHECKED;
					break;
				case IDC_ALLOW_SORTORDER | (BN_CLICKED << 16) :
					settings::allow_sort_fields = Button_GetCheck(m_wnd_allow_sort_order) == BST_CHECKED;
					break;
				case IDC_NUMBERS_LAST | (BN_CLICKED << 16) :
					settings::numbers_last = Button_GetCheck(m_wnd_numbers_last) == BST_CHECKED;
					break;
				case IDC_SORT_PLAYLISTS | (BN_CLICKED << 16) :
					settings::sort_playlists = Button_GetCheck(m_wnd_sort_playlists) == BST_CHECKED;
					break;
				case IDC_ADDED | (CBN_SELCHANGE << 16) :
					settings::date_added_mode = ComboBox_GetCurSel(m_wnd_date_added);
					break;
				case IDC_RGMODE | (CBN_SELCHANGE << 16) :
					settings::soundcheck_rgmode = ComboBox_GetCurSel(m_wnd_rgmode);
					break;
				case (EN_CHANGE << 16) | IDC_SC:
				{
					if (!m_initialising)
					{
						BOOL result;
						int new_val = GetDlgItemInt(wnd, IDC_SC, &result, TRUE);
						if (result)
							settings::soundcheck_adjustment = new_val;
					}

				}
				break;
			}
			break;
		case WM_DESTROY:
			m_field_list.remove_items(bit_array_true(), false);
			m_field_list.destroy();
			break;
	}
	return FALSE;
}

BOOL t_config_tab1::g_DialogProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
	t_config_tab1 * p_this = NULL;
	switch (msg)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_tab1*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_tab1*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);
	return FALSE;
}

bool t_config_tab1::t_list_view_filter::notify_create_inline_edit(const pfc::list_base_const_t<t_size>& indices, unsigned column, pfc::string_base & p_text, t_size & p_flags, mmh::comptr_t<IUnknown>& pAutocompleteEntries)
{
	t_size indices_count = indices.get_count();
	if (indices_count == 1 && indices[0] < tabsize(g_mappings))
	{
		m_edit_index = indices[0];
		m_edit_column = column;

		p_text = g_mappings[m_edit_index].m_value;

		return true;
	}
	return false;
}

void t_config_tab1::t_list_view_filter::notify_save_inline_edit(const char * value)
{
	if (m_edit_index < tabsize(g_mappings))
	{
		g_mappings[m_edit_index].m_value = value;

		{
			pfc::list_t<t_list_view::t_item_insert> items;
			items.set_count(1);
			{
				items[0].m_subitems.add_item(g_mappings[m_edit_index].m_field);
				items[0].m_subitems.add_item(value);
			}
			replace_items(m_edit_index, items);
		}
	}
	m_edit_column = pfc_infinite;
	m_edit_index = pfc_infinite;
}
