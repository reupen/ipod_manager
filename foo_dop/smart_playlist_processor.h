#pragma once

#include "reader.h"

namespace ipod
{

namespace smart_playlist
{

class generator_t
{
public:
	generator_t ( const ipod::tasks::load_database_t & p_library )
		: m_library (p_library) 
	{
		t_filetimestamp time;
		GetSystemTimeAsFileTime((LPFILETIME)&time);
		m_timestamp = apple_time_from_filetime(time);
	};

	void run (const itunesdb::t_smart_playlist_data & p_data, const itunesdb::t_smart_playlist_rules & p_rules);
	const pfc::list_t <pfc::rcptr_t <itunesdb::t_track> , pfc::alloc_fast_aggressive> & get_tracks();
	void to_playlist (itunesdb::t_playlist & p_out)
	{
		p_out.items.remove_all();
		t_size i, count = m_tracks.get_count();
		p_out.items.set_count(count);
		for (i=0; i<count; i++)
		{
			p_out.items[i].track_id = m_tracks[i]->id;
			p_out.items[i].position_valid = true;
			p_out.items[i].position = i+1;
		}
	}
	void run (itunesdb::t_playlist & p_playlist)
	{
		run(p_playlist.smart_playlist_data, p_playlist.smart_playlist_rules);
		//sort(p_playlist.sort_order, p_playlist.sort_direction!=0);
		to_playlist(p_playlist);
	}
private:
	void process_limits (const itunesdb::t_smart_playlist_data & p_data);
	void process_rules (const itunesdb::t_smart_playlist_rules & p_rules);
	void process_tracks (const pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > & p_tracks, const itunesdb::t_smart_playlist_rule & p_rule);
	void sort(t_uint32 order, bool b_desc);
	bool g_test_track(const pfc::rcptr_t<t_track> & track, const itunesdb::t_smart_playlist_rule & rule);

	const ipod::tasks::load_database_t & m_library;
	pfc::list_t <pfc::rcptr_t <itunesdb::t_track> , pfc::alloc_fast_aggressive> m_tracks;
	t_uint32 m_timestamp;
};

};
};