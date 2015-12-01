#include "main.h"

namespace ipod
{
	namespace tasks
	{
		void load_database_t::update_artistlist()
		{
			//m_artist_list.remove_all();


			t_size track_count = m_tracks.get_count();
			t_size current_artist_count = m_artist_list.get_count();
			t_size album_count = m_album_list.m_master_list.get_count();
			mmh::permutation_t permAlbumListbyID(album_count);
			mmh::g_sort_get_permutation_qsort_v2(m_album_list.m_master_list.get_ptr(), permAlbumListbyID, t_album::g_compare_id, false);


			{
				mmh::permutation_t pTrackAIDInit(track_count);
				mmh::g_sort_get_permutation_qsort_v2(m_tracks.get_ptr(), pTrackAIDInit, t_track::g_compare_artist_id, false);

				for (t_size i = 0; i<current_artist_count; i++)
				{
					t_size index;
					if (pfc::bsearch_permutation_t(track_count, m_tracks, t_track::g_compare_artist_id_value, m_artist_list[i]->id, pTrackAIDInit, index))
					{
						m_artist_list[i]->temp_season_number = m_tracks[index]->season_number;
						m_artist_list[i]->temp_show = m_tracks[index]->show;
						m_artist_list[i]->temp_track_pid = m_tracks[index]->pid;
					}
				}
			}

			{
				mmh::permutation_t pNormal(current_artist_count);
				mmh::g_sort_get_permutation_qsort_v2(m_artist_list.get_ptr(), pNormal, t_artist::g_compare_standard, false);
				m_artist_list.reorder(pNormal.get_ptr());
			}

			t_uint32 next_id = 0x81;
			if (current_artist_count)
			{
				mmh::permutation_t perm(current_artist_count);
				mmh::g_sort_get_permutation_qsort_v2(m_artist_list.get_ptr(), perm, t_artist::g_compare_id, false);
				next_id = m_artist_list[perm[current_artist_count - 1]]->id + 1;
			}

			t_size i;
			for (i = 0; i<track_count; i++)
			{
				t_size index = pfc_infinite;
				bool b_found = false;

				t_uint32 type = 0;
				//if (m_tracks[i]->album_artist_valid || m_tracks[i]->artist_valid || m_tracks[i]->show_valid || m_tracks[i]->season_number)
				type = ai_types::song;

				//if (type == ai_types::song)
				{
					if (b_found = m_artist_list.bsearch_t(t_artist::g_compare_standard_track, m_tracks[i], index))
						m_tracks[i]->artist_id = m_artist_list[index]->id;
				}

				if (!b_found)
				{
					t_artist::ptr temp = new t_artist;
					temp->id = next_id;
					temp->type = type;

					temp->temp_season_number = m_tracks[i]->season_number;
					temp->temp_show = m_tracks[i]->show;
					temp->temp_track_pid = m_tracks[i]->pid;

					//if (type == ai_types::song)
					{
						if (m_tracks[i]->album_artist_valid || m_tracks[i]->artist_valid)
						{
							temp->artist_valid = true;
							temp->artist = (m_tracks[i]->album_artist_valid ? m_tracks[i]->album_artist : m_tracks[i]->artist);
						}

						if (m_tracks[i]->album_artist_valid)
						{
							temp->sort_artist_valid = m_tracks[i]->sort_album_artist_valid;
							temp->sort_artist = m_tracks[i]->sort_album_artist;
						}
						else if (m_tracks[i]->sort_artist_valid)
						{
							temp->sort_artist_valid = true;
							temp->sort_artist = m_tracks[i]->sort_artist;
						}
					}
					m_artist_list.insert_item(temp, index);

					//else
					//	temp.release();

					if (temp.is_valid())
					{
						m_tracks[i]->artist_id = temp->id;
						//m_artist_list.add_item(temp);
						next_id++;
					}
				}
			}

#if 1
			//purge dead entries
			current_artist_count = m_artist_list.get_count();
			pfc::array_staticsize_t<bool> mask(current_artist_count);
			mmh::permutation_t perm_aid(m_tracks.get_count());

			mmh::g_sort_get_permutation_qsort_v2(m_tracks.get_ptr(), perm_aid, t_track::g_compare_artist_id, false);

			t_size dummy;

			for (i = 0; i<current_artist_count; i++)
				mask[i] = !pfc::bsearch_permutation_t(m_tracks.get_count(), m_tracks, t_track::g_compare_artist_id_value, m_artist_list[i]->id, perm_aid, dummy);

			m_artist_list.remove_mask(mask.get_ptr());
#endif

			//generate pids
			current_artist_count = m_artist_list.get_count();

			mmh::permutation_t perm_pid(current_artist_count);
			mmh::g_sort_get_permutation_qsort_v2(m_artist_list.get_ptr(), perm_pid, t_artist::g_compare_pid, false);

			genrand_service::ptr p_genrand = genrand_service::g_create();
			p_genrand->seed(GetTickCount());

			for (i = 0; i<current_artist_count; i++)
			{
				if (m_artist_list[i]->pid == NULL)
				{
					t_uint64 new_pid = NULL;
					t_size attempts = 66;
					do {
						t_uint64 p1 = p_genrand->genrand(MAXUINT32 - 1) + 1;
						t_uint64 p2 = p_genrand->genrand(MAXUINT32);
						new_pid = p1 | (p2 << 32);
					} while (m_artist_list.have_pid(new_pid) && --attempts);
					if (attempts)
						m_artist_list[i]->pid = new_pid;
					else
						break;
				}
			}

			//fill album artist_pids
			{
				mmh::permutation_t permArtistListbyID(current_artist_count);
				mmh::g_sort_get_permutation_qsort_v2(m_artist_list.get_ptr(), permArtistListbyID, t_artist::g_compare_id, false);

				for (t_size i = 0; i<track_count; i++)
				{
					t_size index_album, index_artist;
					if (m_album_list.m_master_list.bsearch_permutation_t(t_album::g_compare_id_value, m_tracks[i]->album_id, permAlbumListbyID, index_album)
						&& m_artist_list.bsearch_permutation_t(t_artist::g_compare_id_value, m_tracks[i]->artist_id, permArtistListbyID, index_artist))
						m_album_list.m_master_list[index_album]->artist_pid = m_artist_list[index_artist]->pid;
				}
			}
			//fill artwork_album_pids
			{
				t_size count_albums = m_album_list.m_master_list.get_count();
				pfc::list_t< t_album::ptr, pfc::alloc_fast_aggressive > p_artwork_albums;
				p_artwork_albums.prealloc(count_albums);
				for (i = 0; i<count_albums; i++)
					if (m_album_list.m_master_list[i]->artwork_item_pid)
						p_artwork_albums.add_item(m_album_list.m_master_list[i]);

				current_artist_count = m_artist_list.get_count();
				t_size count_artwork_albums = p_artwork_albums.get_count();
				mmh::permutation_t perm_album_pid(count_artwork_albums), perm_album_artist_pid(count_artwork_albums);
				mmh::g_sort_get_permutation_qsort_v2(p_artwork_albums.get_ptr(), perm_album_pid, t_album::g_compare_pid, false);
				mmh::g_sort_get_permutation_qsort_v2(p_artwork_albums.get_ptr(), perm_album_artist_pid, t_album::g_compare_artist_pid, false);

				for (i = 0; i<current_artist_count; i++)
				{
					t_artist::ptr artist = m_artist_list[i];
					t_size dummy_index = pfc_infinite;
					if (artist->artwork_album_pid == NULL || !p_artwork_albums.bsearch_permutation_t(t_album::g_compare_pid_value, artist->artwork_album_pid, perm_album_pid, dummy_index))
					{
						t_size index;
						if (p_artwork_albums.bsearch_permutation_t(t_album::g_compare_artist_pid_value, artist->pid, perm_album_artist_pid, index))
						{
							artist->artwork_album_pid = p_artwork_albums[index]->pid;
						}
						else artist->artwork_album_pid = NULL;
					}
				}
			}
		}

	}
}