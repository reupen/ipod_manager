#pragma once

class t_smart_playlist_editor
{
	class t_playlist_map_entry
	{
	public:
		t_uint64 pid;
		pfc::string8 name;
	};
	
public:
	class t_playlist_map : public pfc::list_t<t_playlist_map_entry, pfc::alloc_fast_aggressive>
	{
	public:
		const char * get_name_by_pid( t_uint64 pid ) const
		{
			for (t_size i=0, count=get_count(); i<count; i++)
				if ((*this)[i].pid == pid) return (*this)[i].name;
			return "<unknown>";
		}
	};

	void init(const ipod::tasks::load_database_t & p_library, t_uint64 pid_exclude = NULL)
	{
		m_playlist_map.prealloc(p_library.m_playlists.get_count());
		for (t_size i=0, count=p_library.m_playlists.get_count(); i<count; i++)
		{
			if (pid_exclude != p_library.m_playlists[i]->id)
			{
				t_playlist_map_entry entry;
				entry.pid = p_library.m_playlists[i]->id;
				p_library.get_playlist_path(entry.pid, entry.name);
				m_playlist_map.add_item(entry);
			}
		}
	};

	itunesdb::t_smart_playlist_data m_data;
	itunesdb::t_smart_playlist_rules m_rules;
	t_uint32 m_sort_order;
	bool m_sort_direction;
	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	t_smart_playlist_editor(const ipod::tasks::load_database_t & p_library, const itunesdb::t_smart_playlist_data & p_data,
		const itunesdb::t_smart_playlist_rules & p_rules, t_uint32 p_sort_order, bool p_sort_direction, t_uint64 pid_exclude = NULL)
		: m_data(p_data), m_rules(p_rules), m_new(false), m_sort_order(p_sort_order), m_sort_direction(p_sort_direction) {init(p_library, pid_exclude);};
	t_smart_playlist_editor() : m_new(true), m_sort_order(itunesdb::playlist_sort_orders::manual), m_sort_direction(false) {};

private:
	BOOL DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	bool m_new;
	t_playlist_map m_playlist_map;
};

class t_smart_playlist_rule_editor
{
	BOOL DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	void update_operator_list(HWND wnd);
	void update_value_windows(HWND wnd, bool b_reset = false);
	bool m_new;
	t_smart_playlist_editor::t_playlist_map m_playlist_map;
public:
	void set_playlist_map (const t_smart_playlist_editor::t_playlist_map & p_playlist_map)
	{ m_playlist_map = p_playlist_map; }
	itunesdb::t_smart_playlist_rule m_rule;
	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	t_smart_playlist_rule_editor(
		const itunesdb::t_smart_playlist_rule & p_rule, const t_smart_playlist_editor::t_playlist_map & p_playlist_map);
	t_smart_playlist_rule_editor() : m_new(true) {};
};

