#include "stdafx.h"

#include "resource.h"
#include "smart_playlist_editor.h"

struct t_operator_pair
{
	const WCHAR * text;
	t_uint32 value, type;
	t_operator_pair(const WCHAR * ptext, t_uint32 pval, t_uint32 ptype)
		: text(ptext), value(pval), type(ptype)
	{};
};

struct t_operators
{
	const t_operator_pair * operators;
	t_size count;
	t_operators(const t_operator_pair * poperators, t_size pcount)
		: operators(poperators), count(pcount)
	{};
};

struct t_value_pair
{
	const WCHAR * text;
	t_uint32 value, type;
	t_value_pair(const WCHAR * ptext, t_uint32 pval, t_uint32 ptype)
		: text(ptext), value(pval), type(ptype)
	{};
};

#if 0
struct t_smart_field
{
	const WCHAR * text;
	t_uint32 value, type;
	t_operators operators;
	t_value_pair(const WCHAR * ptext, t_uint32 pval, t_uint32 ptype)
		: text(ptext), value(pval), type(ptype)
	{};
};
#endif

void g_format_appletime(t_uint32 at, pfc::string8 & p_out)
{
	t_filetimestamp ft = filetime_time_from_appletime(at, false);
	SYSTEMTIME st;
	//VarDateFromStr
	FileTimeToSystemTime((LPFILETIME)&ft, &st);
	p_out.reset();
	p_out << pfc::format_uint(st.wYear, 2) << "."<< pfc::format_uint(st.wMonth, 2) << "."<< pfc::format_uint(st.wDay, 2);
}

void g_format_appletime_transformed(t_uint32 at, pfc::string8 & p_out)
{
	g_format_appletime(at*60*60*24, p_out);
}

t_uint32 g_str_to_appletime(const char * str)
{
	SYSTEMTIME st;
	memset(&st, 0, sizeof(SYSTEMTIME));
	const char * start = str, *ptr = str;
	while (*ptr && *ptr != '.')
		ptr++;
	st.wYear = mmh::strtoul_n(start, ptr-start);
	if (*ptr=='.')
		ptr++;
	start=ptr;
	while (*ptr && *ptr != '.')
		ptr++;
	st.wMonth = mmh::strtoul_n(start, ptr-start);
	if (*ptr=='.')
		ptr++;
	start=ptr;
	while (*ptr && *ptr != '.')
		ptr++;
	st.wDay = mmh::strtoul_n(start, ptr-start);

	t_filetimestamp ft = NULL;
	SystemTimeToFileTime(&st, (LPFILETIME)&ft);
	return apple_time_from_filetime(ft, false);
}

t_uint32 g_str_to_appletime_transformed(const char * str)
{
	return g_str_to_appletime(str)/(24*60*60);
}
void g_transform_rule(itunesdb::t_smart_playlist_rule & rule);
void g_detransform_rule(itunesdb::t_smart_playlist_rule & rule);

const t_value_pair g_smart_fields[] = 
{
	t_value_pair(L"album", itunesdb::smart_playlist_fields::album, 0),
	t_value_pair(L"album artist", itunesdb::smart_playlist_fields::album_artist, 0), //string
	t_value_pair(L"artist", itunesdb::smart_playlist_fields::artist, 0), //string
	t_value_pair(L"bitrate", itunesdb::smart_playlist_fields::bitrate, 1), //integer
	t_value_pair(L"category", itunesdb::smart_playlist_fields::category,0), //string
	t_value_pair(L"comment", itunesdb::smart_playlist_fields::comment,0), //string
	t_value_pair(L"compilation", itunesdb::smart_playlist_fields::compilation,1), //integer
	t_value_pair(L"composer", itunesdb::smart_playlist_fields::composer,0), //string
	t_value_pair(L"bpm", itunesdb::smart_playlist_fields::bpm,1), //integer
	t_value_pair(L"date added", itunesdb::smart_playlist_fields::date_added,2), //timestamp
	t_value_pair(L"date modified", itunesdb::smart_playlist_fields::date_modified,2), //timestamp
	t_value_pair(L"description", itunesdb::smart_playlist_fields::description,0), //string
	t_value_pair(L"disc number", itunesdb::smart_playlist_fields::disc_number,1), //integer
	t_value_pair(L"last played", itunesdb::smart_playlist_fields::last_played,2), //timestamp
	t_value_pair(L"last skipped", itunesdb::smart_playlist_fields::last_skipped,2), //timestamp
	t_value_pair(L"genre", itunesdb::smart_playlist_fields::genre,0), //string
	t_value_pair(L"grouping", itunesdb::smart_playlist_fields::grouping,0), //string (see special note)
	t_value_pair(L"kind", itunesdb::smart_playlist_fields::kind,0), //string
	t_value_pair(L"length", itunesdb::smart_playlist_fields::time,1), //integer
	t_value_pair(L"play count", itunesdb::smart_playlist_fields::play_count,1), //integer
	t_value_pair(L"playlist", itunesdb::smart_playlist_fields::playlist,3), //integer - the playlist id number (see special note)
	t_value_pair(L"podcast", itunesdb::smart_playlist_fields::podcast,1), //integer
	t_value_pair(L"rating", itunesdb::smart_playlist_fields::rating,1), //integer (multiply by 20 for stars/rating)
	t_value_pair(L"sample rate", itunesdb::smart_playlist_fields::sample_rate,1), //integer
	t_value_pair(L"season number", itunesdb::smart_playlist_fields::season_number,1), //integer
	t_value_pair(L"size", itunesdb::smart_playlist_fields::size,1), //integer
	t_value_pair(L"skip count", itunesdb::smart_playlist_fields::skip_count,1), //integer
	t_value_pair(L"sort album", itunesdb::smart_playlist_fields::sort_album, 0),
	t_value_pair(L"sort album artist", itunesdb::smart_playlist_fields::sort_album_artist, 0), //string
	t_value_pair(L"sort artist", itunesdb::smart_playlist_fields::sort_artist, 0), //string
	t_value_pair(L"sort composer",itunesdb::smart_playlist_fields::sort_composer, 0),
	t_value_pair(L"sort show", itunesdb::smart_playlist_fields::sort_show, 0),
	t_value_pair(L"sort title",itunesdb::smart_playlist_fields::sort_title, 0),
	t_value_pair(L"title",itunesdb::smart_playlist_fields::title, 0),
	t_value_pair(L"track number", itunesdb::smart_playlist_fields::track_number,1), //integer
	t_value_pair(L"tv show", itunesdb::smart_playlist_fields::tv_show,0), //string
	//t_value_pair(L"video kind", itunesdb::smart_playlist_fields::video_kind, 3), //0xe62 //logic integer, itunesdb::smart_playlist_fields::), works on mediatype
	t_value_pair(L"year", itunesdb::smart_playlist_fields::year,1) //integer
};

t_size g_smart_field_to_index (t_uint32 field)
{
	t_size i = tabsize (g_smart_fields);
	for (;i;i--)
	{
		if (g_smart_fields[i-1].value == field)
			return i-1;
	}
	return pfc_infinite;
}

const t_operator_pair g_smart_operators_strings[] =
{
	t_operator_pair(L"is", (1<<24)|(1<<0), 0),
	t_operator_pair(L"is not", (1<<24)|(1<<0)|(1<<25), 0),
	t_operator_pair(L"contains", (1<<24)|(1<<1), 0),
	t_operator_pair(L"does not contain", (1<<24)|(1<<1)|(1<<25), 0),
	t_operator_pair(L"begins with", (1<<24)|(1<<2), 0),
	t_operator_pair(L"ends with", (1<<24)|(1<<3), 0)
};

const t_operator_pair g_smart_operators_timestamps[] =
{
	t_operator_pair(L"is", (0<<24)|(1<<0), 0),
	t_operator_pair(L"is not", (0<<24)|(1<<0)|(1<<25), 0),
	t_operator_pair(L"is greater than", (0<<24)|(1<<4), 0),
	//t_operator_pair(L"is greater than or equal to", (0<<24)|(1<<5), 0),
	t_operator_pair(L"is less than", (0<<24)|(1<<6), 0),
	//t_operator_pair(L"is less than or equal to", (0<<24)|(1<<7), 0),
	t_operator_pair(L"is between", (0<<24)|(1<<8), 1),
	t_operator_pair(L"is in the last", (0<<24)|(1<<9), 2),
	t_operator_pair(L"is not in the last", (0<<24)|(1<<9)|(1<<25), 2)
};

const t_operator_pair g_smart_operators_integers[] =
{
	t_operator_pair(L"is", (0<<24)|(1<<0), 0),
	t_operator_pair(L"is not", (0<<24)|(1<<0)|(1<<25), 0),
	t_operator_pair(L"is greater than", (0<<24)|(1<<4), 0),
	//t_operator_pair(L"is greater than or equal to", (0<<24)|(1<<5), 0),
	t_operator_pair(L"is less than", (0<<24)|(1<<6), 0),
	//t_operator_pair(L"is less than or equal to", (0<<24)|(1<<7), 0),
	t_operator_pair(L"is between", (0<<24)|(1<<8), 1),
};

const t_operator_pair g_smart_operators_playlist[] =
{
	t_operator_pair(L"is", (0<<24)|(1<<0), 3),
	t_operator_pair(L"is not", (0<<24)|(1<<0)|(1<<25), 3),
};

void g_transform_rule(itunesdb::t_smart_playlist_rule & rule, t_uint32 step)
{
	rule.from_value *= rule.from_units;
	rule.to_value *= rule.to_units;
	rule.from_units = 1;
	rule.to_units = 1;
	rule.from_value /= step;
	rule.to_value /= step;
	if (rule.from_value == rule.to_value)
	{
		if (rule.action == (1<<8))
			rule.action = (1<<0);
		else if (rule.action == ((1<<8)|(1<<25)))
			rule.action = ((1<<0)|(1<<25));
	}
}
void g_detransform_rule(itunesdb::t_smart_playlist_rule & rule, t_uint32 step)
{
	if (rule.action == (1<<0) || rule.action == ((1<<0)|(1<<25)))
	{
		rule.from_value *= step;
		rule.to_value = (rule.to_value+1)*step-1;
		rule.action = (1<<8)|(rule.action & (1<<25));
	}
	else if (rule.action == ((0<<24)|(1<<4)))
	{
		rule.to_value = (rule.from_value = ((rule.from_value+1)*step-1));
	}
	else if (rule.action == ((0<<24)|(1<<6)))
	{
		rule.to_value = (rule.from_value *= step);
	}
	else if (rule.action == ((0<<24)|(1<<8)))
	{
		rule.from_value *= step;
		rule.to_value = ((rule.to_value+1)*step-1);
	}
	else throw pfc::exception_bug_check();
}

const t_operators g_smart_operators_by_type(t_size type)
{
	switch (type)
	{	
	case 0: return t_operators(&g_smart_operators_strings[0], tabsize(g_smart_operators_strings));
	case 1: return t_operators(&g_smart_operators_integers[0], tabsize(g_smart_operators_integers));
	case 2: return t_operators(&g_smart_operators_timestamps[0], tabsize(g_smart_operators_timestamps));
	case 3: return t_operators(&g_smart_operators_playlist[0], tabsize(g_smart_operators_playlist));
	default: return t_operators(NULL, 0);
	};
}

#if 0
const t_operators g_smart_operators_by_type[] =
{
	t_operators(&g_smart_operators_strings[0], tabsize(g_smart_operators_strings)),
	t_operators(&g_smart_operators_integers[0], tabsize(g_smart_operators_integers)),
	t_operators(&g_smart_operators_timestamps[0], tabsize(g_smart_operators_timestamps))
};
#endif

t_size g_operator_to_index(t_size field_index, t_uint32 action)
{
	if (field_index < tabsize(g_smart_fields))
	{
		//assert (g_smart_fields[field_index].type < tabsize(g_smart_operators_by_type));
		t_size i = g_smart_operators_by_type(g_smart_fields[field_index].type).count;
		for (; i; i--)
		{
			if (g_smart_operators_by_type(g_smart_fields[field_index].type).operators[i-1].value == action)
				return i-1;
		}
		return pfc_infinite;
	}
	return pfc_infinite;
}


const t_value_pair g_smart_sort_types[] = 
{
	t_value_pair(L"random", 2, 0),
	t_value_pair(L"album", 4, 0),
	t_value_pair(L"artist", 5, 0),
	t_value_pair(L"genre", 7, 0),
	t_value_pair(L"title", 3, 0),

	t_value_pair(L"time on iPod", 0x10, 1),
	t_value_pair(L"play count", 0x14, 2),
	t_value_pair(L"time since last played", 0x15, 1),
	t_value_pair(L"rating", 0x17, 2),
};

const t_value_pair g_playlist_sort_types[] = 
{
	t_value_pair(L"manual", itunesdb::playlist_sort_orders::manual, 0),
	//t_value_pair(L"unk", itunesdb::playlist_sort_orders::unk, 0),1
	t_value_pair(L"title", itunesdb::playlist_sort_orders::title, 0),
	t_value_pair(L"album", itunesdb::playlist_sort_orders::album, 0),
	t_value_pair(L"artist", itunesdb::playlist_sort_orders::artist, 0),
	t_value_pair(L"bitrate", itunesdb::playlist_sort_orders::bitrate, 0),
	t_value_pair(L"genre", itunesdb::playlist_sort_orders::genre, 0),
	t_value_pair(L"kind", itunesdb::playlist_sort_orders::kind, 0),
	t_value_pair(L"date modified", itunesdb::playlist_sort_orders::date_modified, 0), 
	t_value_pair(L"track number", itunesdb::playlist_sort_orders::track_number, 0), 
	t_value_pair(L"size", itunesdb::playlist_sort_orders::size, 0),
	t_value_pair(L"time", itunesdb::playlist_sort_orders::time, 0),
	t_value_pair(L"year", itunesdb::playlist_sort_orders::year, 0),
	t_value_pair(L"sample rate", itunesdb::playlist_sort_orders::sample_rate, 0),
	t_value_pair(L"comment", itunesdb::playlist_sort_orders::comment, 0),
	t_value_pair(L"date added", itunesdb::playlist_sort_orders::date_added, 0), 
	t_value_pair(L"equalizer", itunesdb::playlist_sort_orders::equalizer, 0),
	t_value_pair(L"composer", itunesdb::playlist_sort_orders::composer, 0),
	//t_value_pair(L"unk2", itunesdb::playlist_sort_orders::unk, 0),
	t_value_pair(L"play count", itunesdb::playlist_sort_orders::play_count, 0), 
	t_value_pair(L"last played", itunesdb::playlist_sort_orders::last_played, 0),
	t_value_pair(L"disc number", itunesdb::playlist_sort_orders::disc_number, 0),
	t_value_pair(L"rating", itunesdb::playlist_sort_orders::rating, 0),
	t_value_pair(L"release date", itunesdb::playlist_sort_orders::release_date, 0),
	t_value_pair(L"bpm", itunesdb::playlist_sort_orders::bpm, 0),
	t_value_pair(L"grouping", itunesdb::playlist_sort_orders::grouping, 0),
	t_value_pair(L"category", itunesdb::playlist_sort_orders::category, 0),
	t_value_pair(L"description", itunesdb::playlist_sort_orders::description, 0),
	t_value_pair(L"show", itunesdb::playlist_sort_orders::show, 0),
	t_value_pair(L"season", itunesdb::playlist_sort_orders::season, 0),
	t_value_pair(L"episode number", itunesdb::playlist_sort_orders::episode_number, 0)
};

t_size g_limit_sort_type_to_index(t_uint32 limit_sort_type)
{
	t_size i = tabsize (g_smart_sort_types);
	for (;i;i--)
	{
		if (g_smart_sort_types[i-1].value == limit_sort_type)
			return i-1;
	}
	return pfc_infinite;
}

t_size g_playlist_sort_type_to_index(t_uint32 sort_type)
{
	t_size i = tabsize (g_playlist_sort_types);
	for (;i;i--)
	{
		if (g_playlist_sort_types[i-1].value == sort_type)
			return i-1;
	}
	return pfc_infinite;
}

void g_transform_rule(itunesdb::t_smart_playlist_rule & rule)
{
	t_size index = g_smart_field_to_index(rule.field);
	t_size index_operator = g_operator_to_index(index, rule.action);

	if (index != pfc_infinite && index_operator != pfc_infinite)
	{
		t_size type_field = g_smart_fields[index].type;
		t_size operator_type = g_smart_operators_by_type(g_smart_fields[index].type).operators[index_operator].type;

		if (rule.field == itunesdb::smart_playlist_fields::rating)
			g_transform_rule(rule, 20);
		else if (rule.field == itunesdb::smart_playlist_fields::time)
			g_transform_rule(rule, 1000);
		else if (type_field == 2 && operator_type != 2)
			g_transform_rule(rule, 60*60*24);
	}
}

void g_detransform_rule(itunesdb::t_smart_playlist_rule & rule)
{
	t_size index = g_smart_field_to_index(rule.field);
	t_size index_operator = g_operator_to_index(index, rule.action);

	if (index != pfc_infinite && index_operator != pfc_infinite)
	{
		t_size type_field = g_smart_fields[index].type;
		t_size operator_type = g_smart_operators_by_type(g_smart_fields[index].type).operators[index_operator].type;

		if (rule.field == itunesdb::smart_playlist_fields::rating)
			g_detransform_rule(rule, 20);
		else if (rule.field == itunesdb::smart_playlist_fields::time)
			g_detransform_rule(rule, 1000);
		else if (type_field == 2 && operator_type != 2)
			g_detransform_rule(rule, 60*60*24);
	}
}

void g_get_smart_rule_desc(const itunesdb::t_smart_playlist_rule & p_rule, const t_smart_playlist_editor::t_playlist_map & p_playlist_map, pfc::string_base & p_out)
{
	itunesdb::t_smart_playlist_rule rule = p_rule;
	g_transform_rule(rule);
	t_size index = g_smart_field_to_index(rule.field);
	p_out = index == pfc_infinite ? "<unknown field>" : pfc::stringcvt::string_utf8_from_os(g_smart_fields[index].text).get_ptr();
	if (index == pfc_infinite) console::formatter() << "unknown field: " << pfc::format_hex(rule.field);
	t_size index_operator = g_operator_to_index(index, rule.action);
	p_out << " " <<
		( index_operator==pfc_infinite ? "<unknown operator>" : pfc::stringcvt::string_utf8_from_os(g_smart_operators_by_type(g_smart_fields[index].type).operators[index_operator].text).get_ptr());
	if (index_operator == pfc_infinite) console::formatter() << "unknown operator: " << pfc::format_hex(rule.action);
	if (index != pfc_infinite)
	{
		if (g_smart_fields[index].type == 3)
		{
			p_out << " " << p_playlist_map.get_name_by_pid(rule.from_value);
		}
		else if (g_smart_fields[index].type == 0)
			p_out << " " << pfc::stringcvt::string_utf8_from_os(rule.string);
		else if (index_operator!=pfc_infinite)
		{
			if (g_smart_operators_by_type(g_smart_fields[index].type).operators[index_operator].type==1)
			{
				if (g_smart_fields[index].type == 2)
				{
					pfc::string8 temp;
					g_format_appletime_transformed((t_uint32)rule.from_value, temp);
					p_out << " " << temp << " to "; 
					g_format_appletime_transformed((t_uint32)rule.to_value, temp);
					p_out << temp;
				}
				else if (g_smart_fields[index].type == 1)
					p_out << " " << rule.from_value << "*" << rule.from_units << " to " << rule.to_value << "*" << rule.to_units;
			}
			else
			{
				if (g_smart_fields[index].type == 1)
					p_out << " " << rule.from_value << "*" << rule.from_units;
				else if (g_smart_fields[index].type == 2)
				{
					if (g_smart_operators_by_type(g_smart_fields[index].type).operators[index_operator].type==2)
					{
						p_out << " " << -rule.from_date << " ";
						if (rule.from_units==60*60*24)
							p_out << "days";
						else if (rule.from_units==60*60*24*7)
							p_out << "weeks";
						else if (rule.from_units==2628000)
							p_out << "months";
						else 
							p_out << rule.from_units << " seconds";
					}
					else
					{
						pfc::string8 temp;
						g_format_appletime_transformed((t_uint32)rule.from_value, temp);
						p_out << " " << temp;
					}
				}
			}
		}
	}
}

t_smart_playlist_rule_editor::t_smart_playlist_rule_editor(
	const itunesdb::t_smart_playlist_rule & p_rule, const t_smart_playlist_editor::t_playlist_map & p_playlist_map)
	: m_rule(p_rule), m_new(false), m_playlist_map(p_playlist_map)
{
	g_transform_rule(m_rule);
};

BOOL CALLBACK t_smart_playlist_rule_editor::g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	t_smart_playlist_rule_editor * p_this = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(wnd, DWL_USER, lp);
		p_this = reinterpret_cast<t_smart_playlist_rule_editor*>(lp);
		break;
	default:
		p_this = reinterpret_cast<t_smart_playlist_rule_editor*>(GetWindowLongPtr(wnd, DWL_USER));
		break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

BOOL CALLBACK t_smart_playlist_editor::g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	t_smart_playlist_editor * p_this = NULL;
	switch(msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(wnd, DWL_USER, lp);
		p_this = reinterpret_cast<t_smart_playlist_editor*>(lp);
		break;
	default:
		p_this = reinterpret_cast<t_smart_playlist_editor*>(GetWindowLongPtr(wnd, DWL_USER));
		break;
	}
	if (p_this)
		return p_this->DialogProc(wnd, msg, wp, lp);

	return FALSE;
}

BOOL t_smart_playlist_editor::DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			//uxtheme_api_ptr uxtheme;
			//if (uxtheme_handle::g_create(uxtheme))
			//	uxtheme->EnableThemeDialogTexture(wnd, ETDT_ENABLETAB);
			SetWindowText(wnd, m_new ? L"New smart playlist" : L"Edit smart playlist");
			HWND wnd_rules = GetDlgItem(wnd, IDC_RULES);
			uih::list_view_set_explorer_theme(wnd_rules);
			ListView_SetExtendedListViewStyleEx(wnd_rules, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

			LVCOLUMN lvc;
			memset(&lvc, 0, sizeof(LVCOLUMN));
			lvc.mask = LVCF_TEXT|LVCF_WIDTH;

			uih::list_view_insert_column_text(wnd_rules, 0, _T("Rule"), 550);

			HWND wnd_match_type = GetDlgItem(wnd, IDC_MATCH_TYPE);
			ComboBox_AddString(wnd_match_type, L"all");
			ComboBox_AddString(wnd_match_type, L"any");
			HWND wnd_limit_type = GetDlgItem(wnd, IDC_LIMIT_TYPE);
			ComboBox_AddString(wnd_limit_type, L"minutes");
			ComboBox_AddString(wnd_limit_type, L"megabytes");
			ComboBox_AddString(wnd_limit_type, L"songs");
			ComboBox_AddString(wnd_limit_type, L"hours");
			ComboBox_AddString(wnd_limit_type, L"gigabytes");
			HWND wnd_limit_order = GetDlgItem(wnd, IDC_LIMIT_ORDER);
			ComboBox_AddString(wnd_limit_order, L"ascending");
			ComboBox_AddString(wnd_limit_order, L"descending");
			HWND wnd_limit_sort = GetDlgItem(wnd, IDC_LIMIT_SORT);
			t_size i, count=tabsize(g_smart_sort_types);
			for (i=0; i<count; i++)
				ComboBox_AddString(wnd_limit_sort, g_smart_sort_types[i].text);

			HWND wnd_sort_order = GetDlgItem(wnd, IDC_SORT_ORDER);
			count = tabsize(g_playlist_sort_types);
			for (i=0; i<count; i++)
				ComboBox_AddString(wnd_sort_order, g_playlist_sort_types[i].text);

			HWND wnd_sort_direction = GetDlgItem(wnd, IDC_SORT_DIRECTION);
			ComboBox_AddString(wnd_sort_direction, L"ascending");
			ComboBox_AddString(wnd_sort_direction, L"descending");

			ComboBox_SetCurSel(wnd_sort_order, g_playlist_sort_type_to_index(m_sort_order));
			ComboBox_SetCurSel(wnd_sort_direction, m_sort_direction?1:0);

			HWND wnd_match = GetDlgItem(wnd, IDC_MATCH);
			Button_SetCheck(wnd_match, m_data.check_rules);

			HWND wnd_live = GetDlgItem(wnd, IDC_LIVE_UPDATING);
			Button_SetCheck(wnd_live, m_data.live_update);

			HWND wnd_limit = GetDlgItem(wnd, IDC_LIMIT);
			Button_SetCheck(wnd_limit, m_data.check_limits);

			ComboBox_SetCurSel(wnd_match_type, m_rules.rule_operator);
			ComboBox_SetCurSel(wnd_limit_type, m_data.limit_type-1);
			t_size sort_index = g_limit_sort_type_to_index(m_data.limit_sort);
			ComboBox_SetCurSel(wnd_limit_sort, sort_index);
			bool b_enable_limit_order = false, b_reverse = false;
			if (sort_index != pfc_infinite && sort_index < tabsize(g_smart_sort_types))
			{
				if (g_smart_sort_types[sort_index].type)
					b_enable_limit_order = true;
				b_reverse = g_smart_sort_types[sort_index].type == 2;
			}

			ComboBox_SetCurSel(wnd_limit_order, b_reverse ? (1-m_data.reverse_limit_sort) : m_data.reverse_limit_sort);
			EnableWindow(wnd_limit_order, b_enable_limit_order);

			count = m_rules.rules.get_count();
			for (i=0; i<count; i++)
			{
				pfc::string8 temp;
				g_get_smart_rule_desc(m_rules.rules[i],m_playlist_map,temp);
				uih::list_view_insert_item_text(wnd_rules, i, 0, temp );
			}

			HWND wnd_limit_value = GetDlgItem(wnd, IDC_LIMIT_VALUE);
			uSetWindowText(wnd_limit_value, pfc::format_uint(m_data.limit_value));
			HWND wnd_limit_value_spin = GetDlgItem(wnd, IDC_LIMIT_VALUE_SPIN);
			SendMessage(wnd_limit_value_spin, UDM_SETRANGE32, 0, 999999);
				
			EnableWindow(GetDlgItem(wnd, IDC_DELETE), m_rules.rules.get_count()>1);

			//m_this=this;
			//m_wnd=wnd;
			//modeless_dialog_manager::g_add(wnd);
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lp;
			switch (lpnm->idFrom)
			{
			case IDC_RULES:
				{
					switch (lpnm->code)
					{
					case NM_DBLCLK:
						{
							LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lp;
							if (lpnmitem->iItem != -1)
								SendMessage(wnd, WM_COMMAND, IDC_EDIT, NULL);
						}
						break;
					};
				}
				break;
			};
		}
		break;
	case WM_COMMAND:
		switch (wp)
		{
		case IDCANCEL:
			EndDialog(wnd, 0);
			return 0;
		case IDOK:
			EndDialog(wnd,1);
			break;
		case IDC_NEW:
			{
				t_smart_playlist_rule_editor editor;
				if (DialogBoxParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_SMART_RULE), wnd, &t_smart_playlist_rule_editor::g_DialogProc, (LPARAM)&editor))
				{
					t_size index = m_rules.rules.add_item(editor.m_rule);
					pfc::string8 temp;
					g_get_smart_rule_desc(m_rules.rules[index],m_playlist_map,temp);
					uih::list_view_insert_item_text(GetDlgItem(wnd, IDC_RULES), index, 0, temp );

					if (m_rules.rules.get_count()==2)
						EnableWindow(GetDlgItem(wnd, IDC_DELETE), TRUE);
				}
			}
			return 0;
		case IDC_DELETE:
			{
			int lbi = ListView_GetNextItem(GetDlgItem(wnd, IDC_RULES), -1, LVNI_SELECTED);
				if (lbi != -1 && m_rules.rules.get_count() > 1)
				{
					m_rules.rules.remove_by_idx(lbi);
					ListView_DeleteItem(GetDlgItem(wnd, IDC_RULES), lbi);
				}
				if (m_rules.rules.get_count()==1)
					EnableWindow((HWND(lp)), FALSE);
			}
			return 0;
		case IDC_EDIT:
			{
				int lbi = ListView_GetNextItem(GetDlgItem(wnd, IDC_RULES), -1, LVNI_SELECTED);
				if (lbi != -1)
				{
					t_smart_playlist_rule_editor editor(m_rules.rules[lbi], m_playlist_map);
					if (DialogBoxParam(mmh::get_current_instance(), MAKEINTRESOURCE(IDD_SMART_RULE), wnd, &t_smart_playlist_rule_editor::g_DialogProc, (LPARAM)&editor))
					{
						m_rules.rules[lbi]=editor.m_rule;
						pfc::string8 temp;
						g_get_smart_rule_desc(m_rules.rules[lbi],m_playlist_map,temp);
						uih::list_view_insert_item_text(GetDlgItem(wnd, IDC_RULES), lbi, 0, temp , true);
					}
				}
			}
			return 0;
		case IDC_MATCH:
			m_data.check_rules = Button_GetCheck(HWND(lp)) == BST_CHECKED ? 1 : 0;
			return 0;
		case IDC_MATCH_TYPE|(CBN_SELCHANGE<<16):
			m_rules.rule_operator = ComboBox_GetCurSel(HWND(lp));
			return 0;
		case IDC_LIVE_UPDATING:
			m_data.live_update = Button_GetCheck(HWND(lp)) == BST_CHECKED ? 1 : 0;
			//if (!m_data.live_update)
				//MessageBox(wnd, L"This component does not update smart playlists, therefore disabling live updating is not recommended.", L"Warning", MB_OK);
			return 0;
		case IDC_LIMIT:
			m_data.check_limits =Button_GetCheck(HWND(lp)) == BST_CHECKED ? 1 : 0;
			return 0;
		case IDC_LIMIT_VALUE|(EN_CHANGE<<16):
			{
				string_utf8_from_window val((HWND)lp);
				m_data.limit_value = mmh::strtoul_n(val.get_ptr(), val.length(), 10);
			}
			return 0;
		case IDC_LIMIT_ORDER|(CBN_SELCHANGE<<16):
			{
				t_size index_sort = pfc_infinite;
				bool b_reverse = false;
				index_sort = g_limit_sort_type_to_index(m_data.limit_sort);
				if (index_sort < tabsize (g_smart_sort_types))
					b_reverse = g_smart_sort_types[index_sort].type == 2;
				m_data.reverse_limit_sort = b_reverse ? 1-ComboBox_GetCurSel(HWND(lp)):ComboBox_GetCurSel(HWND(lp));
			}
			return 0;
		case IDC_SORT_ORDER|(CBN_SELCHANGE<<16):
			{
				t_size index = ComboBox_GetCurSel(HWND(lp));
				if (index < tabsize (g_playlist_sort_types))
				{
					m_sort_order = g_playlist_sort_types[index].value;
				}
			}
			return 0;
		case IDC_SORT_DIRECTION|(CBN_SELCHANGE<<16):
			{
				t_size index = ComboBox_GetCurSel(HWND(lp));
				m_sort_direction = index!=0;
			}
			return 0;
		case IDC_LIMIT_SORT|(CBN_SELCHANGE<<16):
			{
				t_size index = ComboBox_GetCurSel(HWND(lp));
				if (index < tabsize (g_smart_sort_types))
				{
					m_data.limit_sort = g_smart_sort_types[index].value;
					bool b_enable_limit_order = false, b_reverse = false;
					{
						if (g_smart_sort_types[index].type)
							b_enable_limit_order = true;
						b_reverse = g_smart_sort_types[index].type == 2;
					}

					HWND wnd_limit_order = GetDlgItem(wnd, IDC_LIMIT_ORDER);
					t_size index_order = ComboBox_GetCurSel(wnd_limit_order);

					m_data.reverse_limit_sort = b_reverse ? 1-index_order:index_order;
					EnableWindow(wnd_limit_order, b_enable_limit_order);
				}
			}
			return 0;
		case IDC_LIMIT_TYPE|(CBN_SELCHANGE<<16):
			m_data.limit_type = ComboBox_GetCurSel(HWND(lp))+1;
			return 0;
		}
		break;
	case WM_DESTROY:
		return 0;
	case WM_NCDESTROY:
		//modeless_dialog_manager::g_remove(wnd);
		SetWindowLongPtr(wnd, DWL_USER, NULL);
		//m_this.release();
		break;
	}
	return FALSE;
}

t_size combo_box_add_string_data(HWND wnd, const TCHAR * str, LPARAM data)
{
	t_size index = ComboBox_AddString(wnd, str);
	ComboBox_SetItemData(wnd, index, data);
	return index;
}
void t_smart_playlist_rule_editor::update_operator_list(HWND wnd)
{
	HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
	HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
	ComboBox_ResetContent(wnd_operator);

	t_size index = ComboBox_GetCurSel(wnd_field);

	if (index < tabsize(g_smart_fields))
	{
		//if (g_smart_fields[index].type < tabsize(g_smart_operators_by_type))
		{
		t_size i , count = (g_smart_operators_by_type(g_smart_fields[index].type).count);
		for (i=0; i<count; i++)
			combo_box_add_string_data(wnd_operator, g_smart_operators_by_type(g_smart_fields[index].type).operators[i].text,
			g_smart_operators_by_type(g_smart_fields[index].type).operators[i].value);
		}
	}
}

void g_reset_rule(t_size index_field, t_size index_operator, itunesdb::t_smart_playlist_rule & rule)
{
	t_uint32 type = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index_operator].type;
	if (g_smart_fields[index_field].type==1)
	{
		rule.from_date=0;
		rule.to_date=0;
		rule.from_value=0;
		rule.to_value=0;
		rule.from_units=1;
		rule.to_units=1;
	}
	else if (g_smart_fields[index_field].type==2)
	{
		t_filetimestamp ft = NULL;
		GetSystemTimeAsFileTime((LPFILETIME)&ft);
		//ft = (ft-(ft%(60*60*24)));
		t_uint32 at = apple_time_from_filetime(ft, true);
		at /= 60*60*24;
		rule.from_date= 0;
		rule.to_date=0;
		rule.from_value=type == 2 ? 0x2dae2dae2dae2dae : at;
		rule.to_value=type == 2 ? 0x2dae2dae2dae2dae : at;
		rule.from_units=type==2?60*60*24:1;
		rule.to_units=type==2?60*60*24:1;
	}
}

void t_smart_playlist_rule_editor::update_value_windows(HWND wnd, bool b_reset)
{
	HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
	HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
	HWND wnd_value1 = GetDlgItem(wnd, IDC_VALUE);
	HWND wnd_value2 = GetDlgItem(wnd, IDC_VALUE2);
	HWND wnd_combo = GetDlgItem(wnd, IDC_COMBO);
	HWND wnd_combo_value = GetDlgItem(wnd, IDC_COMBOVALUE);
	HWND wnd_to = GetDlgItem(wnd, IDC_TO);
	HWND wnd_ok = GetDlgItem(wnd, IDOK);

	int value1cmd=SW_HIDE, value2cmd=SW_HIDE, combocmd=SW_HIDE, operatorcmd = SW_HIDE, okcmd=FALSE, combovaluecmd = SW_HIDE;
	ComboBox_ResetContent(wnd_combo);
	ComboBox_ResetContent(wnd_combo_value);

	t_size index_field = ComboBox_GetCurSel(wnd_field);
	t_size index_operator = ComboBox_GetCurSel(wnd_operator);

	if (index_field < tabsize(g_smart_fields))
	{
		operatorcmd=SW_SHOW;
		//if (g_smart_fields[index_field].type < tabsize(g_smart_operators_by_type))
		if (index_operator < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
		{
			if (b_reset)
				g_reset_rule(index_field, index_operator, m_rule);
			value1cmd=SW_SHOW;
			okcmd=TRUE;
			t_uint32 type = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index_operator].type;
			if (type==3)
			{
				value1cmd=SW_HIDE;
				combovaluecmd = SW_SHOW;
				//combovaluecmd = SW_SHOW;
				t_size j = pfc_infinite;
				for (t_size i = 0, count = m_playlist_map.get_count(); i<count; i++)
				{
					if (m_playlist_map[i].pid == m_rule.from_value) j = i;
					ComboBox_AddString(wnd_combo_value, pfc::stringcvt::string_os_from_utf8(m_playlist_map[i].name));
				}
				if (j != pfc_infinite)
					ComboBox_SetCurSel(wnd_combo_value, j);

			}
			else
				if (type==0)
			{
				if (g_smart_fields[index_field].type==0)
					SetWindowText(wnd_value1, m_rule.string.get_ptr());
				else if (g_smart_fields[index_field].type==1)
					SetWindowText(wnd_value1, pfc::stringcvt::string_os_from_utf8(pfc::string8() << m_rule.from_value));
				else if (g_smart_fields[index_field].type==2)
				{
					pfc::string8 temp;
					g_format_appletime_transformed((t_uint32)m_rule.from_value, temp);
					SetWindowText(wnd_value1, pfc::stringcvt::string_os_from_utf8(temp));
				}
			}
			else if (type==1)
			{
				value2cmd=SW_SHOW;
				if (g_smart_fields[index_field].type==1)
				{
					SetWindowText(wnd_value1, pfc::stringcvt::string_os_from_utf8(pfc::string8() << m_rule.from_value));
					SetWindowText(wnd_value2, pfc::stringcvt::string_os_from_utf8(pfc::string8() << m_rule.to_value));
				}
				else if (g_smart_fields[index_field].type==2)
				{
					pfc::string8 temp;
					g_format_appletime_transformed((t_uint32)m_rule.from_value, temp);
					SetWindowText(wnd_value1, pfc::stringcvt::string_os_from_utf8(temp));
					g_format_appletime_transformed((t_uint32)m_rule.to_value, temp);
					SetWindowText(wnd_value2, pfc::stringcvt::string_os_from_utf8(temp));
				}
			}
			else if (type==2)
			{
				combocmd=SW_SHOW;
				if (g_smart_fields[index_field].type==2)
				{
					SetWindowText(wnd_value1, pfc::stringcvt::string_os_from_utf8(pfc::string8() << -m_rule.from_date));
					ComboBox_AddString(wnd_combo, L"days");
					ComboBox_AddString(wnd_combo, L"weeks");
					ComboBox_AddString(wnd_combo, L"months");
					if (m_rule.from_units==60*60*24)
						ComboBox_SetCurSel(wnd_combo, 0);
					else if (m_rule.from_units==60*60*24*7)
						ComboBox_SetCurSel(wnd_combo, 1);
					else if (m_rule.from_units==2628000)
						ComboBox_SetCurSel(wnd_combo, 2);
					
				}
			}
		}
	}
	ShowWindow(wnd_value1, value1cmd);
	ShowWindow(wnd_value2, value2cmd);
	ShowWindow(wnd_to, value2cmd);
	ShowWindow(wnd_combo, combocmd);
	ShowWindow(wnd_combo_value, combovaluecmd);
	ShowWindow(wnd_operator, operatorcmd);
	EnableWindow(wnd_ok, okcmd);
}

BOOL t_smart_playlist_rule_editor::DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			//uxtheme_api_ptr uxtheme;
			//if (uxtheme_handle::g_create(uxtheme))
			//	uxtheme->EnableThemeDialogTexture(wnd, ETDT_ENABLETAB);
			SetWindowText(wnd, m_new ? L"New rule" : L"Edit rule");
			HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
			t_size i, count = tabsize(g_smart_fields);
			for (i=0; i<count; i++)
				ComboBox_AddString(wnd_field, g_smart_fields[i].text);
			HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);

			ComboBox_SetCurSel(wnd_field, g_smart_field_to_index(m_rule.field));	
			update_operator_list(wnd);
			ComboBox_SetCurSel(wnd_operator, g_operator_to_index(g_smart_field_to_index(m_rule.field), m_rule.action));	
			update_value_windows(wnd);

			//m_this=this;
			//m_wnd=wnd;
			//modeless_dialog_manager::g_add(wnd);
		}
		break;
	case WM_COMMAND:
		switch (wp)
		{
		case IDCANCEL:
			EndDialog(wnd, 0);
			return 0;
		case IDOK:
			g_detransform_rule(m_rule);
			EndDialog(wnd,1);
			break;
		case IDC_FIELD|(CBN_SELCHANGE<<16):
			{
				t_size index = ComboBox_GetCurSel(HWND(lp));
				m_rule.field = g_smart_fields[index].value;
				update_operator_list(wnd);
				update_value_windows(wnd);
			}
			break;
		case IDC_RULE|(CBN_SELCHANGE<<16):
			{
				HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
				t_size index = ComboBox_GetCurSel(HWND(lp));
				t_size index_field = ComboBox_GetCurSel(wnd_field);
				if (index_field < tabsize(g_smart_fields))
				{
					//if (g_smart_fields[index_field].type < tabsize(g_smart_operators_by_type))
						if (index < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
						{
							m_rule.action = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index].value;
						}
				}
				update_value_windows(wnd, true);
			}
			break;
		case IDC_COMBO|(CBN_SELCHANGE<<16):
			{
				HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
				HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
				t_size index_field = ComboBox_GetCurSel(wnd_field);
				t_size index_operator = ComboBox_GetCurSel(wnd_operator);
				if (index_field < tabsize(g_smart_fields))
				{
					//if (g_smart_fields[index_field].type < tabsize(g_smart_operators_by_type))
						if (index_operator < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
						{
							t_uint32 type = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index_operator].type;
							if (type==2)
							{
								if (g_smart_fields[index_field].type==2)
								{
									int sel = ComboBox_GetCurSel((HWND)lp);
									if (sel==0)
										m_rule.from_units=60*60*24;
									else if (sel==1)
										m_rule.from_units=60*60*24*7;
									else if (sel==2)
										m_rule.from_units=2628000;
									m_rule.to_units = m_rule.from_units;
								}
							}
						}
				}
			}
			break;
		case IDC_COMBOVALUE|(CBN_SELCHANGE<<16):
			{
				HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
				HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
				t_size index_field = ComboBox_GetCurSel(wnd_field);
				t_size index_operator = ComboBox_GetCurSel(wnd_operator);
				if (index_field < tabsize(g_smart_fields))
				{
						if (index_operator < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
						{
							{
								if (g_smart_fields[index_field].type==3)
								{
									int sel = ComboBox_GetCurSel((HWND)lp);
									if (sel >=0 && (t_size)sel < m_playlist_map.get_count() )
										m_rule.from_value=m_playlist_map[sel].pid;
									m_rule.to_value = m_rule.from_units;
								}
							}
						}
				}
			}
			break;
		case IDC_VALUE|(EN_CHANGE<<16):
			{
				HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
				HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
				t_size index_field = ComboBox_GetCurSel(wnd_field);
				t_size index_operator = ComboBox_GetCurSel(wnd_operator);
				if (index_field < tabsize(g_smart_fields))
				{
					//if (g_smart_fields[index_field].type < tabsize(g_smart_operators_by_type))
						if (index_operator < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
						{
							t_uint32 type = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index_operator].type;
							if (type==0)
							{
								if (g_smart_fields[index_field].type==0)
									m_rule.string = pfc::stringcvt::string_wide_from_utf8(string_utf8_from_window(HWND(lp)));
								else if (g_smart_fields[index_field].type==1)
								{
									m_rule.from_value= mmh::strtol64_n(string_utf8_from_window(HWND(lp)).get_ptr(), pfc_infinite);
									m_rule.to_value=m_rule.from_value;
								}
								else if (g_smart_fields[index_field].type==2)
								{
									m_rule.from_value=g_str_to_appletime_transformed(string_utf8_from_window(HWND(lp)).get_ptr()); //FUCKO: DATE
									m_rule.to_value=m_rule.from_value;
								}
							}
							else if (type==1)
							{
								if (g_smart_fields[index_field].type==1)
								{
									m_rule.from_value= mmh::strtol64_n(string_utf8_from_window(HWND(lp)).get_ptr(), pfc_infinite);
								}
								else if (g_smart_fields[index_field].type==2)
								{
									m_rule.from_value=g_str_to_appletime_transformed(string_utf8_from_window(HWND(lp)).get_ptr()); //FUCKO: DATE
								}
							}
							else if (type==2)
							{
								if (g_smart_fields[index_field].type==2)
								{
									m_rule.from_date=-mmh::strtol64_n(string_utf8_from_window(HWND(lp)).get_ptr(), pfc_infinite);								
									m_rule.to_date = 0;
								}
							}
						}
				}
			}
			break;
		case IDC_VALUE2|(EN_CHANGE<<16):
			{
				HWND wnd_operator = GetDlgItem(wnd, IDC_RULE);
				HWND wnd_field = GetDlgItem(wnd, IDC_FIELD);
				t_size index_field = ComboBox_GetCurSel(wnd_field);
				t_size index_operator = ComboBox_GetCurSel(wnd_operator);
				if (index_field < tabsize(g_smart_fields))
				{
					//if (g_smart_fields[index_field].type < tabsize(g_smart_operators_by_type))
						if (index_operator < (g_smart_operators_by_type(g_smart_fields[index_field].type).count))
						{
							t_uint32 type = g_smart_operators_by_type(g_smart_fields[index_field].type).operators[index_operator].type;
							if (type==1)
							{
								if (g_smart_fields[index_field].type==1)
								{
									m_rule.to_value= mmh::strtol64_n(string_utf8_from_window(HWND(lp)).get_ptr(), pfc_infinite);
								}
								else if (g_smart_fields[index_field].type==2)
								{
									m_rule.to_value=g_str_to_appletime_transformed(string_utf8_from_window(HWND(lp)).get_ptr()); //FUCKO: DATE
								}
							}
						}
				}
			}
		}
		break;
	case WM_DESTROY:
		return 0;
	case WM_NCDESTROY:
		//modeless_dialog_manager::g_remove(wnd);
		SetWindowLongPtr(wnd, DWL_USER, NULL);
		//m_this.release();
		break;
		}
		return FALSE;
}
