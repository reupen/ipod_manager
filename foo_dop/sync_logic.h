#pragma once

class t_scan_items
{
public:
	template <class t_checker>
	void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, t_checker & p_checker, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
	{
		p_status.update_text("Determining files to remove and add");
		p_status.update_progress_subpart_helper(0,3);
		t_uint32 i, count_tracks=p_library.m_tracks.get_count(),count_items = items.get_count();
		m_tracks_to_remove.set_size(count_tracks);
		m_tracks_to_remove.fill(true);
		for (i=0; i<count_tracks; i++)
		{
			if ( (((p_library.m_tracks[i]->podcast_flag) || !stricmp_utf8("m4v",pfc::string_extension(p_library.m_tracks[i]->location)) || (p_library.m_tracks[i]->media_type & (t_track::type_itunes_u|t_track::type_ringtone|t_track::type_rental|t_track::type_is_voice_memo|t_track::type_podcast|t_track::type_book|t_track::type_digital_booklet|t_track::type_audiobook))) && !p_library.m_tracks[i]->original_path_valid) || p_library.m_tracks[i]->dshm_type_6 )
				m_tracks_to_remove[i] = false;
		}
		for (i=0; i<count_items; i++)
		{
			if (p_checker.m_result[i].have)
				m_tracks_to_remove[p_checker.m_result[i].index] = false;
		}
		for (i=0; i<count_items; i++)
		{
			if (!p_checker.m_result[i].have)
			{
				m_new_tracks.add_item(items[i]);
			}
		}
		p_status.update_progress_subpart_helper(2,2);
	}

	metadb_handle_list_t<pfc::alloc_fast> m_new_tracks;
	pfc::array_t<bool> m_tracks_to_remove;
};
