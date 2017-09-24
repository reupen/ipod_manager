#include "stdafx.h"

#include "reader.h"

namespace ipod
{
	namespace tasks
	{
		void load_database_t::update_albumlist()
		{

			t_size track_count = m_tracks.get_count();
			{
				mmh::Permutation pTrackAIDInit(track_count);
				mmh::sort_get_permutation(m_tracks.get_ptr(), pTrackAIDInit, t_track::g_compare_album_id, false);

				for (t_size i = 0; i<m_album_list.m_master_list.get_count(); i++)
				{
					t_size index;
					if (pfc::bsearch_permutation_t(track_count, m_tracks, t_track::g_compare_album_id_value, m_album_list.m_master_list[i]->id, pTrackAIDInit, index))
					{
						m_album_list.m_master_list[i]->temp_track_pid = m_tracks[index]->pid;
						m_album_list.m_master_list[i]->temp_media_type = m_tracks[index]->media_type2;
					}
				}
			}
			mmh::Permutation
				pMaster(m_album_list.m_master_list.get_count());

			//pfc::list_t<t_album::ptr> newSongs, newPodcasts, newTvShows;

			mmh::sort_get_permutation(m_album_list.m_master_list.get_ptr(), pMaster, t_album::g_compare_album_mixed, false);
			m_album_list.m_master_list.reorder(pMaster.get_ptr());
			//pfc::list_permutation_t<t_album> masterListOrdered(

			//m_album_list.m_normal.reorder(pNormal.get_ptr());

			t_uint32 next_id = 0x81;
			t_size current_album_count = m_album_list.m_master_list.get_count();
			if (current_album_count)
			{
				mmh::Permutation perm(current_album_count);
				mmh::sort_get_permutation(m_album_list.m_master_list.get_ptr(), perm, t_album::g_compare_id, false);
				next_id = m_album_list.m_master_list[perm[current_album_count - 1]]->id + 1;
			}
			//console::formatter() << (m_album_list.m_normal.get_count() + m_album_list.m_podcasts.get_count() + m_album_list.m_tv_shows.get_count()) << " " << m_album_list.m_master_list.get_count();

			t_size i, count = m_tracks.get_count();
			for (i = 0; i<count; i++)
			{
				t_track::ptr p_track = m_tracks[i];
				t_size index = pfc_infinite;
				bool b_found = false, b_other_type = false;

				pfc::rcptr_t<itunesdb::t_track> track = m_tracks[i];

				t_uint32 type = 0;
				if (m_tracks[i]->media_type == itunesdb::t_track::type_audio
					//|| m_tracks[i]->media_type == itunesdb::t_track::type_video
					|| m_tracks[i]->media_type == itunesdb::t_track::type_music_video
					//|| m_tracks[i]->media_type == itunesdb::t_track::type_audiobook
					)
					type = ai_types::song;
				else if (m_tracks[i]->media_type & itunesdb::t_track::type_podcast)
					type = ai_types::podcast;
				else if (m_tracks[i]->media_type & itunesdb::t_track::type_tv_show)
					type = ai_types::tv_show;
				else
				{
					type = ai_types::song;
					b_other_type = true;
				}

				if (b_found = m_album_list.m_master_list.bsearch_t(t_album::g_compare_album_mixed_track, m_tracks[i], index))
					m_tracks[i]->album_id = m_album_list.m_master_list[index]->id;

				if (!b_found)
				{
					t_album::ptr temp = new t_album;
					temp->id = next_id;
					temp->kind = type;

					if (type == ai_types::song)
					{
						if (!b_other_type)
						{
							if (m_tracks[i]->album_artist_valid)
							{
								temp->artist_valid = true;
								temp->artist = m_tracks[i]->album_artist;

								temp->album_artist_strict = temp->artist;
								temp->album_artist_strict_valid = true;
							}
							else if (m_tracks[i]->artist_valid)
							{
								temp->artist_valid = true;
								temp->artist = m_tracks[i]->artist;
							}
							if (m_tracks[i]->album_valid)
							{
								temp->album_valid = true;
								temp->album = m_tracks[i]->album;
							}
						}

						//m_album_list.m_normal.insert_item(temp, index);
					}
					else if (type == ai_types::podcast)
					{
						if (m_tracks[i]->album_valid)
						{
							temp->album_valid = true;
							temp->album = m_tracks[i]->album;
						}
						if (m_tracks[i]->podcast_rss_url_valid)
						{
							temp->podcast_url_valid = true;
							temp->podcast_url = m_tracks[i]->podcast_rss_url;
						}
						//m_album_list.m_podcasts.insert_item(temp, index);
					}
					else if (type == ai_types::tv_show)
					{
						if (m_tracks[i]->show_valid)
						{
							temp->show_valid = true;
							temp->show = m_tracks[i]->show;
						}
						temp->season_number = m_tracks[i]->season_number;
						//m_album_list.m_tv_shows.insert_item(temp, index);
					}

					temp->temp_track_pid = m_tracks[i]->pid;
					temp->temp_media_type = m_tracks[i]->media_type2;

					m_tracks[i]->album_id = temp->id;
					m_album_list.m_master_list.insert_item(temp, index);
					next_id++;

				}
			}

			//purge dead entries
			current_album_count = m_album_list.m_master_list.get_count();
			pfc::array_staticsize_t<bool> mask(current_album_count);
			mmh::Permutation perm_aid(m_tracks.get_count());

			mmh::sort_get_permutation(m_tracks.get_ptr(), perm_aid, t_track::g_compare_album_id, false);

			t_size dummy;

			for (i = 0; i<current_album_count; i++)
				mask[i] = !pfc::bsearch_permutation_t(m_tracks.get_count(), m_tracks, t_track::g_compare_album_id_value, m_album_list.m_master_list[i]->id, perm_aid, dummy);

			m_album_list.m_master_list.remove_mask(mask.get_ptr());

			//generate pids
			current_album_count = m_album_list.m_master_list.get_count();

			mmh::Permutation perm_pid(current_album_count);
			mmh::sort_get_permutation(m_album_list.m_master_list.get_ptr(), perm_pid, t_album::g_compare_pid, false);

			genrand_service::ptr p_genrand = genrand_service::g_create();
			p_genrand->seed(GetTickCount());

			for (i = 0; i<current_album_count; i++)
			{
				if (m_album_list.m_master_list[i]->pid == NULL)
				{
					t_uint64 new_pid = NULL;
					t_size attempts = 66;
					do {
						t_uint64 p1 = p_genrand->genrand(MAXUINT32 - 1) + 1;
						t_uint64 p2 = p_genrand->genrand(MAXUINT32);
						new_pid = p1 | (p2 << 32);
					} while (m_album_list.have_pid(new_pid) && --attempts);
					if (attempts)
						m_album_list.m_master_list[i]->pid = new_pid;
					else
						break;
				}
			}

			//fill artwork_item_pids
			{
				pfc::list_t< pfc::rcptr_t <t_track>, pfc::alloc_fast_aggressive > p_artwork_tracks;
				p_artwork_tracks.prealloc(count);
				for (i = 0; i<count; i++)
					if (m_tracks[i]->artwork_flag == 0x1)
						p_artwork_tracks.add_item(m_tracks[i]);
				t_size count_artwork_tracks = p_artwork_tracks.get_count();
				mmh::Permutation perm_track_pid(count_artwork_tracks), perm_track_album_id(count_artwork_tracks);
				mmh::sort_get_permutation(p_artwork_tracks.get_ptr(), perm_track_pid, ipod::tasks::load_database_t::g_compare_track_dbid, false);
				mmh::sort_get_permutation(p_artwork_tracks.get_ptr(), perm_track_album_id, ipod::tasks::load_database_t::g_compare_track_album_id, false);

				for (i = 0; i<current_album_count; i++)
				{
					t_album::ptr album = m_album_list.m_master_list[i];
					t_size dummy_index = pfc_infinite;
					if (album->artwork_item_pid == NULL || !p_artwork_tracks.bsearch_permutation_t(ipod::tasks::load_database_t::g_compare_track_dbid_with_dbid, album->artwork_item_pid, perm_track_pid, dummy_index))
					{
						t_size index;
						if (p_artwork_tracks.bsearch_permutation_t(t_track::g_compare_album_id_value, album->id, perm_track_album_id, index))
						{
							album->artwork_item_pid = p_artwork_tracks[index]->pid;
						}
						else album->artwork_item_pid = NULL;
					}
				}
			}

			//rebuild seperated lists 
			m_album_list.m_normal.remove_all();
			m_album_list.m_podcasts.remove_all();
			m_album_list.m_tv_shows.remove_all();

			for (i = 0; i<current_album_count; i++)
			{
				t_album::ptr album = m_album_list.m_master_list[i];
				switch (album->kind)
				{
					case itunesdb::ai_types::song:
						m_album_list.m_normal.add_item(album);
						break;
					case itunesdb::ai_types::podcast:
						m_album_list.m_podcasts.add_item(album);
						break;
					case itunesdb::ai_types::tv_show:
						m_album_list.m_tv_shows.add_item(album);
						break;
				}
			}
		}

		void load_database_t::repopulate_albumlist()
		{
			update_albumlist();
			update_artistlist();
		}
	}
}