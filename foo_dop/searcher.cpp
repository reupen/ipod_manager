#include "main.h"

void find_songs_in_library::run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	p_status.update_text("Locating song");
	p_status.update_progress_subpart_helper(0,3);
	t_size count_tracks = p_library.m_tracks.get_count();
	t_size count_items = items.get_count();
	t_size i;

	m_result.set_size(count_items);
	//m_result.fill(t_result(false, );
	m_stats.set_count(count_tracks);

	pfc::list_t<t_entry, pfc::alloc_fast_aggressive> entries;
	entries.set_count(count_tracks);

	for (i=0; i<count_tracks; i++)
	{
		metadb_info_container::ptr p_info;
		if (p_library.m_handles[i]->get_async_info_ref(p_info))
		{
			g_print_meta(p_info->info(), "TITLE", entries[i].title);
			g_print_meta(p_info->info(), "ARTIST", entries[i].artist);
			g_print_meta(p_info->info(), "ALBUM", entries[i].album);
		}
	}

	mmh::permutation_t partist, ptitle, palbum;
	partist.set_size(count_tracks);
	palbum.set_size(count_tracks);
	ptitle.set_size(count_tracks);

	mmh::g_sort_get_permutation_qsort(entries, partist, t_entry::g_compare_artist, false);
	mmh::g_sort_get_permutation_qsort(entries, ptitle, t_entry::g_compare_title, false);
	mmh::g_sort_get_permutation_qsort(entries, palbum, t_entry::g_compare_album, false);

	pfc::list_const_permutation_t<t_entry, const mmh::permutation_t &> sorted_by_artist(entries, partist);
	pfc::list_const_permutation_t<t_entry, const mmh::permutation_t &> sorted_by_title(entries, ptitle);
	pfc::list_const_permutation_t<t_entry, const mmh::permutation_t &> sorted_by_album(entries, palbum);

	for (i=0; i<count_items; i++)
	{
		metadb_info_container::ptr p_info;
		if (items[i]->get_async_info_ref(p_info))
		{
			pfc::string8 title, artist, album;
			g_print_meta(p_info->info(), "TITLE", title);
			t_size index_start;
			if (sorted_by_title.bsearch_t(g_compare_title_string, title, index_start))
			{
				g_print_meta(p_info->info(), "ARTIST", artist);
				g_print_meta(p_info->info(), "ALBUM", album);
				t_size index = index_start;
				do
				{
					if (!g_compare_artist_string(sorted_by_title[index], artist) && !g_compare_album_string(sorted_by_title[index], album))
					{
						m_result[i].have = true;
						m_result[i].index = ptitle[index];
					}
				}
				while (!m_result[i].have && index+1 < count_tracks && !g_compare_title_string( sorted_by_title[++index], title));

				index = index_start;
				while (!m_result[i].have && index > 0 && !g_compare_title_string( sorted_by_title[index-1], title))
				{
					if (!g_compare_artist_string(sorted_by_title[index-1], artist) && !g_compare_album_string(sorted_by_title[index-1], album))
					{
						m_result[i].have = true;
						m_result[i].index = ptitle[index-1];
					}
					index--;
				}
			}

		}
		//p_status.update_progress_subpart_helper(i+1+count_tracks,count_tracks+count_items);
	}
}

