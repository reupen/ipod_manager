#ifndef _DOP_READER_LOAD_LIBRARY_H_
#define _DOP_READER_LOAD_LIBRARY_H_

#include "cfobject.h"
#include "helpers.h"
#include "photodb.h"

namespace ipod
{
	enum
	{
		candy_flag_version_1 = (1<<0),
		candy_flag_version_2 = (1<<1),
		candy_flag_version_3 = (1<<2),
	};

	namespace tasks
	{
		struct DevicePlaylist {
			int64_t playlist_persistent_id{};
			int64_t version{};
			bool playlist_deleted{};
			int64_t saved_index{};
			pfc::string8 name;
			/** Appears to be the timestamp of iTunesDB (and not iTunesCDB or an .itdb file) */
			int64_t db_timestamp_mac_os_date{};
			std::vector<int64_t> track_persistent_ids;
		};

		class load_database_t
		{
			class portable_device_playbackdata_notifier_impl : public dop::portable_device_playbackdata_notifier_t
			{
			public:
				HANDLE m_event;

				void set_event(HANDLE p_event) {m_event = p_event;}
				
				portable_device_playbackdata_notifier_impl(HANDLE p_event) : m_event(p_event) {};
				portable_device_playbackdata_notifier_impl() : m_event(NULL) {};
				~portable_device_playbackdata_notifier_impl() {if (m_event) SetEvent(m_event);};
			};

			class main_thread_playbackdata : public main_thread_callback
			{
			public:
				pfc::array_t<dop::playback_data_t> playback_data;
				pfc::list_t<const dop::playback_data_t *, pfc::alloc_fast_aggressive> playback_data_ptrs;
				pfc::string8 m_device_name;

				service_list_t<dop::portable_device_playbackdata_callback_v2> m_callbacks_v2;
				service_list_t<portable_device_playbackdata_notifier_impl> m_callback_notifiers;

				win32_event m_event_completed;
				threaded_process_v2_t * m_status;

				main_thread_playbackdata() : m_status(NULL) {};

				virtual void callback_run()
				{
#if 0
					for (t_size j = 0, count = playback_data_ptrs.get_count(); j < count; j++)
					{
						pfc::string8 buffer;
						console::formatter output;
						if (playback_data_ptrs[j]->m_track_on_device.is_valid())
							playback_data_ptrs[j]->m_track_on_device->format_title_legacy(NULL, buffer, "%artist% - %title% - %_path_raw%", NULL);
						else
							buffer = "<bad handle>";
						output << buffer << " | ";
						if (playback_data_ptrs[j]->m_track_local.is_valid())
							playback_data_ptrs[j]->m_track_local->format_title_legacy(NULL, buffer, "%artist% - %title% - %_path_raw%", NULL);
						else
							buffer = "<bad handle>";
						output << buffer << " | ";
						output << playback_data_ptrs[j]->m_playcountdata.m_playcount << " | " << playback_data_ptrs[j]->m_ratingdata.m_rating;
					}
#endif
					t_size i, count = m_callbacks_v2.get_count();

					for (i=0; i<count; i++)
					{
						pfc::string8 name;
						m_callbacks_v2[i]->get_task_name(name);
#ifdef _DEBUG
						console::formatter() << "iPod manager: " << name;
#endif
						m_status->update_text(name);
						m_callbacks_v2[i]->on_playback_data(dop::device_identifiers::ipod, m_device_name, playback_data_ptrs, m_callback_notifiers[i]);
						//HANDLE p_event = m_callback_notifiers[i]->m_event;
						m_callback_notifiers[i].release();
						//WaitForSingleObjectEx(p_event, INFINITE, FALSE);
					}

					service_ptr_t<dop::portable_device_playbackdata_callback> ptr;
					service_enum_t<dop::portable_device_playbackdata_callback> e;
					while(e.next(ptr))
					{
						ptr->on_playback_data(dop::device_identifiers::ipod, m_device_name, playback_data_ptrs);
					}
					m_event_completed.set_state(true);

				}
			};
		public:
			void run(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort, bool b_photos=false);
			enum store_purchases_type_t {
				store_purchases_standard,
				store_purchases_podcasts,
			};
			void merge_storepurchases(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort);
			void read_storepurchases(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort);
			void read_storepurchases(ipod_device_ptr_ref_t p_ipod, store_purchases_type_t, abort_callback & p_abort);
			void read_playcounts(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort);

			/**
			 * For, at least, the nano 7G.
			 */
			void load_device_playlists(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort);

			/**
			 * For, at least, the nano 7G.
			 */
			void load_device_playlist(const DevicePlaylist& playlist);
			void clean_up_device_playlists();

            static int g_compare_playlist_id (const pfc::rcptr_t<t_playlist> & item1, const pfc::rcptr_t<t_playlist> & item2)
			{
				return pfc::compare_t(item1->id,item2->id);
			}
			static int g_compare_track_id (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
			{
				return pfc::compare_t(item1->id,item2->id);
			}
			static int g_compare_track_dbid (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
			{
				return pfc::compare_t(item1->pid,item2->pid);
			}
			static int g_compare_track_id_with_id (const pfc::rcptr_t<t_track> & item1, const t_uint32 & id)
			{
				return pfc::compare_t(item1->id,id);
			}
			static int g_compare_track_dbid_with_dbid (const pfc::rcptr_t<t_track> & item1, const t_uint64 & dbid)
			{
				return pfc::compare_t(item1->pid,dbid);
			}

			pfc::rcptr_t <t_track> get_track_by_pid(t_uint64 pid)
			{
				for (t_size i = 0, count = m_tracks.get_count(); i<count; i++)
				{
					if (m_tracks[i]->pid == pid)
						return m_tracks[i];
				}
				return {};
			}

			bool have_track(t_uint64 pid)
			{
				return get_track_by_pid(pid).is_valid();
			};

			void glue_items (t_size start);
			void get_next_ids (t_uint32 & next_tid, t_uint64 & next_dbid);

			t_uint64 get_new_playlist_pid();
			t_uint64 get_new_track_pid();
			void get_playlist_path(t_uint64 pid, pfc::string8 & p_out) const;

			bool have_playlist (const char * title)
			{
				t_uint32 i;
				return find_playlist(title, i);
			}
			bool find_playlist (const char * title, t_uint32 & index) const
			{
				t_size i, count = m_playlists.get_size();
				for (i=0; i<count; i++)
				{
					if  (!m_playlists[i]->is_master && !m_playlists[i]->podcast_flag && !stricmp_utf8(title, m_playlists[i]->name)) 
					{
						index = i;
						return true;
					}
				}
				return false;
			}
			bool find_playlist_by_id (t_uint64 id, t_uint32 & index) const
			{
				t_size i, count = m_playlists.get_size();
				for (i=0; i<count; i++)
				{
					if  (id == m_playlists[i]->id) 
					{
						index = i;
						return true;
					}
				}
				return false;
			}
			void rebuild_podcast_playlist();
			void repopulate_albumlist();
			void update_albumlist();
			void update_artistlist();
			void cleanup_before_write(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void add_playlist_to_folder( t_size index_folder, t_size index_playlist)
			{
				t_smart_playlist_rule rule;
				rule.from_value = m_playlists[index_playlist]->id;
				rule.to_value = rule.from_value;
				rule.field = smart_playlist_fields::playlist;
				rule.action = (0<<24)|(1<<0);
				m_playlists[index_folder]->smart_playlist_rules.rules.add_item(rule);
			}
			void __remove_playlist_folder_recur(t_uint64 id)
			{
				pfc::list_t<t_uint64> folders_to_process;
				t_size i = m_playlists.get_count();
				for (; i; i--)
				{
					if (m_playlists[i-1]->parentid == id)
					{
						if (m_playlists[i-1]->folder_flag == 1)
							folders_to_process.add_item(m_playlists[i-1]->id);
						//t_uint64 pid_child = m_playlists[i-1]->id;
						m_playlists_removed.add_item(m_playlists[i-1]);
						m_playlists.remove_by_idx(i-1);
						//remove_playlist_voiceover_title(p_ipod, pid_child);
					}
				}
				t_size count = folders_to_process.get_count();
				for (i=0; i<count; i++)
					__remove_playlist_folder_recur(folders_to_process[i]);
			}
			void remove_playlist(t_size index)
			{
				pfc::rcptr_t< itunesdb::t_playlist > playlist = m_playlists[index];
				m_playlists.remove_by_idx(index);
				//remove_playlist_voiceover_title(p_ipod, playlist->id);
				m_playlists_removed.add_item(playlist);

				t_size index_parent;
				if (playlist->parentid && find_playlist_by_id(playlist->parentid, index_parent))
				{
					t_size i = m_playlists[index_parent]->smart_playlist_rules.rules.get_count();
					for (; i; i--)
					{
						if (m_playlists[index_parent]->smart_playlist_rules.rules[i-1].field == smart_playlist_fields::playlist
							&& m_playlists[index_parent]->smart_playlist_rules.rules[i-1].from_value == playlist->id)
							m_playlists[index_parent]->smart_playlist_rules.rules.remove_by_idx(i-1);
					}
				}
				if (playlist->folder_flag == 1)
				{
					__remove_playlist_folder_recur(playlist->id);
				}
			}
			void __update_folder_smart_playlists_recur(pfc::array_t<bool> & mask_processed, t_uint64 id);
			void update_smart_playlists();

			void set_up_playlist(t_playlist::ptr & p_playlist, const t_uint32 * p_tracks, t_uint32 count, bool b_timestamp = true)
			{
				if (b_timestamp)
				{
					t_filetimestamp time;
					GetSystemTimeAsFileTime((LPFILETIME)&time);
					p_playlist->date_modified = apple_time_from_filetime(time);
				}

				t_size j;

				p_playlist->items.set_count(count);

				for (j=0; j<count; j++)
				{
					t_uint32 i = p_tracks[j];
					if (i >= m_tracks.get_count())
						throw pfc::exception_bug_check(pfc::string_formatter() <<"Bug check: Index " << i << " out of range " << m_tracks.get_count());

					p_playlist->items[j].track_id = m_tracks[i]->id;
					p_playlist->items[j].timestamp = p_playlist->date_modified;
					p_playlist->items[j].position = j+1;
					p_playlist->items[j].position_valid = true;
				}
			}
			void remove_playlist_voiceover_title(ipod_device_ptr_cref_t p_ipod, t_uint64 pid);
			void add_playlist_voiceover_title(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings, t_uint64 pid, const char * title, bool b_check_if_exists);
			void add_system_voiceover_messages(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings);
			t_size add_playlist(pfc::string8 & name, const t_uint32 * p_tracks, t_uint32 count, t_uint64 parentid = NULL, t_uint64 pid = NULL)
			{
				if (pid == NULL) pid = get_new_playlist_pid();

				t_filetimestamp time;
				GetSystemTimeAsFileTime((LPFILETIME)&time);
				t_size index, index_dummy, index_parent=pfc_infinite;
				pfc::rcptr_t< itunesdb::t_playlist > p_playlist;
				pfc::string8 name_original = name; t_size j=1;
				while (find_playlist(name, index_dummy))
				{
					name.reset();
					name << name_original << " (" << j << ")";
					j++;
				}

				{
					//m_playlists.get_size();
					p_playlist = pfc::rcnew_t<itunesdb::t_playlist>();
					index = m_playlists.add_item(p_playlist);
					p_playlist->id = pid;
				}

				if (parentid)
				{
					if (find_playlist_by_id(parentid, index_parent))
					{
						p_playlist->parentid = parentid;
						add_playlist_to_folder(index_parent, index);
					}
				}

				p_playlist->name = name;
				p_playlist->timestamp = apple_time_from_filetime(time);
				p_playlist->date_modified = p_playlist->timestamp;

				set_up_playlist(p_playlist, p_tracks, count);

				m_playlists_added.add_item(p_playlist);
				//add_playlist_voiceover_title(p_ipod, p_playlist->id, p_playlist->name);

				return index;

			}
		private:
			void merge_on_the_go_playlists(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void merge_playcount_data(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort);

		public:
			void load_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, bool b_CheckIfFilesChanged, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void save_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort) const;
			void refresh_cache(HWND wnd, ipod_device_ptr_ref_t p_ipod, bool b_CheckIfFilesChanged, threaded_process_v2_t & p_status,abort_callback & p_abort);
			void remove_artwork (ipod_device_ptr_ref_t p_ipod, const pfc::rcptr_t<itunesdb::t_track> & p_track);


			static int g_compare_track_album(const pfc::rcptr_t<const t_track> & track1, const pfc::rcptr_t<const t_track> & track2)
			{
				int ret = 0;
				const char * artist1 = track1->album_artist_valid ? track1->album_artist : track1->artist;
				const char * artist2 = track2->album_artist_valid ? track2->album_artist : track2->artist;
				ret = stricmp_utf8(artist1, artist2);
				if (!ret)
				{
					ret = stricmp_utf8(track1->album, track2->album);
				}
				return ret;
			}
			static int g_compare_track_album_id(const pfc::rcptr_t<const t_track> & track1, const pfc::rcptr_t<const t_track> & track2)
			{
				return pfc::compare_t(track1->album_id, track2->album_id);
			}

			metadb_handle_list_t<pfc::alloc_fast_aggressive> m_handles;
			pfc::rcptr_t< t_playlist > m_library_playlist;
			pfc::list_t< pfc::rcptr_t <t_playlist> > m_playlists, m_special_playlists;
			bool m_special_playlists_valid;
			pfc::array_t<t_uint8> m_genius_cuid;
			bool m_genius_cuid_valid;
			pfc::array_t< t_onthego_playlist > m_onthego_playlists;
			pfc::list_t< pfc::rcptr_t <t_track>, pfc::alloc_fast_aggressive > m_tracks, m_tracks_to_remove;
			pfc::list_t< t_play_count_entry > m_playcounts;
			cfobject::object_t::ptr_t m_playcounts_plist;
			service_ptr_t<main_thread_playbackdata> m_playbackdata_callback;

			pfc::list_t< pfc::rcptr_t <t_playlist> > m_playlists_added;
			pfc::list_t< pfc::rcptr_t <t_playlist> > m_playlists_removed;
			
			t_album_list m_album_list;
			t_artist_list m_artist_list;

			load_database_t(bool b_writing = false)
				: m_failed(false), dbid(0), unk1_1(0), unk2(0), format(1), unk1(2),
				m_artwork_valid(false), m_photos_valid(false), m_genius_cuid_valid(false),
				candy_version(0), unk4(0), pid(0), unk6(0), unk7(0), time_zone_offset_seconds(0), candy_flags(0), encoding(0), audio_language(0), 
				unk11_1(25), unk11_2(10), subtitle_language(0), m_writing(b_writing), unk12(0), unk13(0), m_special_playlists_valid(false)
			{
				memset (&unk0, 0, sizeof(unk0));
				memset (&unk14, 0, sizeof(unk14));
				memset (&hash1, 0, sizeof(hash1));
				memset (&hash2, 0, sizeof(hash2));
				memset (&hash3, 0, sizeof(hash3));
				memset (&hash4, 0, sizeof(hash4));

				m_library_playlist = pfc::rcnew_t<itunesdb::t_playlist>(); //dummy - in case iTunesDB is malformed
			};

			photodb::t_datafile m_photos;
			bool m_photos_valid;
			photodb::t_datafile m_artwork;
			bool m_artwork_valid;

			itunesprefs m_itunesprefs;

			t_uint64 dbid;
			t_uint64 unk1_1; //no
			t_uint8 format;
			t_uint8 unk0[3];
			t_uint32 unk1;
			t_uint32 unk2;

			t_uint16 candy_version;
			char hash1[20];
			t_uint16 unk4;
			t_uint64 pid;
			t_uint32 unk6;
			t_uint32 unk7;
			char hash2[20];
			t_uint32 time_zone_offset_seconds; //time zone ofst secs
			t_uint16 candy_flags;
			char hash3[46];

			t_uint16 audio_language, subtitle_language;
			t_uint16 unk11_1, unk11_2;
			t_uint8 encoding;
			t_uint8 unk12;
			t_uint8 unk13;
			char hash4[57];
			t_uint32 unk14[4];

		private:
			bool m_failed;
			bool m_writing;
			pfc::list_t< pfc::string8 > m_read_device_playlists;
		};
	}
}

void ipod_read_dopdb_tracklist(fbh::StreamReaderMemblock* p_file, t_uint32 root_id, t_uint32 root_size,
	const mmh::Permutation & permutationtid,
	const mmh::Permutation & permutationdbid,
	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, pfc::array_t<t_size> > & sorted_array_tid,
	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, pfc::array_t<t_size> > & sorted_array_dbid,
	const ipod::tasks::load_database_t & p_library,
	threaded_process_v2_t & p_status, abort_callback & p_abort);

void ipod_read_dopdb(const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort);


#endif //_DOP_READER_LOAD_LIBRARY_H_