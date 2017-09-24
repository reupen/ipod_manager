#include "stdafx.h"

#include "plist.h"


namespace ipod
{
	namespace tasks
	{
		void load_database_t::read_playcounts(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort)
		{
			{
				//profiler(readingd);
				try
				{
					service_ptr_t<file> p_counts_file;

					pfc::string8 database_folder;
					p_ipod->get_database_path(database_folder);
					pfc::string8 path = database_folder;

					{

						pfc::array_t<bool> mask_to_remove;
						mask_to_remove.set_count(m_tracks.get_count());
						mask_to_remove.fill_null();

						if (!p_ipod->mobile)
						{
							path << "\\iTunes\\" << "Play Counts";
							if (filesystem::g_exists(path, p_abort))
							{
								filesystem::g_open_read(p_counts_file, path, p_abort);
								t_filesize filesize = p_counts_file->get_size_ex(p_abort);
								pfc::array_t<t_uint8> _data;
								_data.set_size(pfc::downcast_guarded<t_size>(filesize));
								p_counts_file->read(_data.get_ptr(), _data.get_size(), p_abort);
								itunesdb::stream_reader_memblock_ref_dop stream(_data.get_ptr(), _data.get_size());

								reader p_counts(&stream);

								t_header_marker<identifiers::pdhm> dphm;

								p_counts.read_header(dphm, p_abort);

								itunesdb::stream_reader_memblock_ref_dop dphm_data(dphm.data.get_ptr(), dphm.data.get_size());

								t_uint32 count;

								dphm_data.read_lendian_t(count, p_abort);

								t_size i;

								pfc::array_t<t_uint8> data;
								data.set_size(dphm.section_size);

								m_playcounts.set_size(count);
								for (i = 0; i<count; i++)
								{
									stream.read(data.get_ptr(), dphm.section_size, p_abort);
									itunesdb::stream_reader_memblock_ref_dop entry(data.get_ptr(), dphm.section_size);
									try {
										t_play_count_entry & item = m_playcounts[i];
										entry.read_lendian_t(m_playcounts[i].play_count, p_abort);
										entry.read_lendian_t(m_playcounts[i].last_played, p_abort);
										entry.read_lendian_t(m_playcounts[i].bookmark_position, p_abort);
										entry.read_lendian_t(m_playcounts[i].rating, p_abort);
										entry.read_lendian_t(m_playcounts[i].unk1, p_abort);
										entry.read_lendian_t(m_playcounts[i].play_state, p_abort);
										entry.read_lendian_t(m_playcounts[i].unk2, p_abort);
										entry.read_lendian_t(m_playcounts[i].unk3, p_abort);
										entry.read_lendian_t(m_playcounts[i].skip_count, p_abort);
										entry.read_lendian_t(m_playcounts[i].last_skipped, p_abort);
									}
									catch (exception_io_data_truncation) {};
								}
							}
						}
						else
						{

							try {
								path << "/iTunes/" << "PlayCounts.plist";

								m_playcounts.set_size(m_tracks.get_count());

#ifdef _DEBUG
								//bool blah; t_filestats stats;
								//filesystem::g_get_stats(path, stats, blah, p_abort);
								//console::formatter() << stats.m_timestamp;
#endif
								class mobile_playcount_t
								{
								public:
									t_uint64 persistentID;
									t_uint32 playCount;
									t_uint32 lastPlayDate;
									t_uint32 skipCount;
									t_uint32 lastSkipDate;
									t_uint32 userRating;
									t_uint32 bookmarkTime;
									t_uint32 playedState;
									bool deleted;
									mobile_playcount_t()
										: persistentID(NULL), playCount(NULL), lastPlayDate(NULL), skipCount(NULL),
										lastSkipDate(NULL), userRating(NULL), bookmarkTime(-1), deleted(false),
										playedState(NULL)
									{};
								};

								file::ptr f;
								filesystem::g_open_read(f, path, p_abort);

								cfobject::object_t::ptr_t root;

								root = PlistParserFromFile(path, p_abort).m_root_object;

								m_playcounts_plist = root;

								pfc::array_t<mobile_playcount_t> mobilecounts;
								cfobject::object_t::ptr_t tracks;

								if (root.is_valid())
								{
									if (root->m_dictionary.get_child(L"tracks", tracks))
									{
										t_size j, jcount = tracks->m_array.get_count();
										mobilecounts.set_size(jcount);
										for (j = 0; j<jcount; j++)
										{
											mobile_playcount_t & data = mobilecounts[j];
											cfobject::object_t::ptr_t dictEntry;

											if (tracks->m_array[j]->m_dictionary.get_child(L"persistentID", dictEntry))
												data.persistentID = reinterpret_cast<t_uint64&>(dictEntry->m_integer);
											if (tracks->m_array[j]->m_dictionary.get_child(L"playCount", dictEntry))
												data.playCount = pfc::downcast_guarded<t_uint32>(dictEntry->m_integer);
											if (tracks->m_array[j]->m_dictionary.get_child(L"playMacOSDate", dictEntry))
												data.lastPlayDate = dictEntry->get_flat_uint32();
											if (tracks->m_array[j]->m_dictionary.get_child(L"skipCount", dictEntry))
												data.skipCount = pfc::downcast_guarded<t_uint32>(dictEntry->m_integer);
											if (tracks->m_array[j]->m_dictionary.get_child(L"skipMacOSDate", dictEntry))
												data.lastSkipDate = dictEntry->get_flat_uint32();
											if (tracks->m_array[j]->m_dictionary.get_child(L"userRating", dictEntry))
												data.userRating = pfc::downcast_guarded<t_uint32>(dictEntry->m_integer);
											if (tracks->m_array[j]->m_dictionary.get_child(L"playedState", dictEntry))
												data.playedState = dictEntry->get_bool();
											if (tracks->m_array[j]->m_dictionary.get_child(L"deleted", dictEntry))
												data.deleted = dictEntry->get_bool();
											if (tracks->m_array[j]->m_dictionary.get_child(L"bookmarkTimeInMS", dictEntry))
												data.bookmarkTime = (dictEntry->m_type == cfobject::kTagReal ? (t_uint32)dictEntry->m_float : pfc::downcast_guarded<t_uint32>(dictEntry->m_integer));

											if (data.lastPlayDate == -1) data.lastPlayDate = 0;
											if (data.lastSkipDate == -1) data.lastSkipDate = 0;
										}
									}
								}

								t_size i, count = mobilecounts.get_count();

								pfc::list_t<cfobject::object_t::ptr_t> preservedEntries;

								if (count)
								{
									mmh::Permutation perm_dbid(m_tracks.get_count());
									mmh::sort_get_permutation(m_tracks.get_ptr(), perm_dbid, g_compare_track_dbid, false);

									for (i = 0; i<count; i++)
									{
										t_size index;
										if (m_tracks.bsearch_permutation_t(g_compare_track_dbid_with_dbid, mobilecounts[i].persistentID, perm_dbid, index))
										{
											m_playcounts[index].play_count = mobilecounts[i].playCount;
											if (m_playcounts[index].play_count)
												m_playcounts[index].last_played = mobilecounts[i].lastPlayDate;
											m_playcounts[index].skip_count = mobilecounts[i].skipCount;
											if (m_playcounts[index].skip_count)
												m_playcounts[index].last_skipped = mobilecounts[i].lastSkipDate;
											m_playcounts[index].play_state = (mobilecounts[i].playedState ? 1 : 2);
											m_playcounts[index].rating = mobilecounts[i].userRating;
											m_playcounts[index].bookmark_position = mobilecounts[i].bookmarkTime;

											if (mobilecounts[i].deleted)
											{
												mask_to_remove[index] = true;
												m_tracks_to_remove.add_item(m_tracks[index]);
												//console::formatter() << "Setting deleted: " << (t_int64)m_tracks[index]->pid << " : " << m_tracks[index]->title;
											}
#if 0
											if (m_playcounts[index].play_count)
											{
												std::wstring date;
												g_format_date(filetime_time_from_appletime(m_playcounts[index].last_played), date);
												console::formatter() << "Got play: " << m_tracks[index]->artist << " - " << m_tracks[index]->title << ", " << m_playcounts[index].play_count << ", " << pfc::stringcvt::string_utf8_from_wide(date.data());
											}
											if (m_playcounts[index].bookmark_position)
											{
												std::wstring date;
												g_format_date(filetime_time_from_appletime(m_playcounts[index].bookmark_position), date);
												console::formatter() << "Got bookmark: " << m_tracks[index]->artist << " - " << m_tracks[index]->title << ", " << m_playcounts[index].play_count << ", " << pfc::stringcvt::string_utf8_from_wide(date.data());
											}
#endif
#define PRESERVE_DSHMPC 0
#if PRESERVE_DSHMPC//_DEBUG //FIXME
											if (m_tracks[index]->dshm_type_6)
												preservedEntries.add_item(tracks->m_array[i]);
#endif
										}
										else
										{
											console::formatter() << "iPod manager: Failed to find persistent ID in iPod database: " << mobilecounts[i].persistentID;
#if PRESERVE_DSHMPC//_DEBUG //FIXME
											preservedEntries.add_item(tracks->m_array[i]); //?
#endif
										}
									}
#if PRESERVE_DSHMPC//_DEBUG //FIXME
									if (tracks.is_valid())
									{
										//tracks->m_array.set_size(0);
										tracks->m_array = preservedEntries;
									}
#endif
								}
							}
							catch (const exception_io_not_found &) {};
						}

						t_size i, count = m_playcounts.get_count();
						if (count != m_tracks.get_count())
						{
							if (count)
								console::print("iPod manager: Warning: Play Count data invalid");
						}
						else
						{
							service_ptr_t<main_thread_playbackdata> playbackdata = new service_impl_t<main_thread_playbackdata>;
							m_playbackdata_callback = playbackdata;

							playbackdata->playback_data.set_count(count);
							playbackdata->playback_data_ptrs.prealloc(count);
							playbackdata->m_device_name = m_library_playlist->name;

							for (i = 0; i<count; i++)
							{
								pfc::rcptr_t<itunesdb::t_track> & p_track = m_tracks[i];
								if (m_playcounts[i].last_played)
								{
									if (m_playcounts[i].play_count)
									{
										playbackdata->playback_data[i].m_playcountdata.m_valid = true;
										playbackdata->playback_data[i].m_playcountdata.m_playcount = m_playcounts[i].play_count;
										playbackdata->playback_data[i].m_playcountdata.m_lastplayed_timestamp = filetime_time_from_appletime(m_playcounts[i].last_played);
									}

									m_tracks[i]->lastplayedtime = m_playcounts[i].last_played;
									//m_tracks[i]->playcount2 += m_playcounts[i].play_count;
									m_tracks[i]->playcount += m_playcounts[i].play_count;
								}
								if (m_playcounts[i].bookmark_position != -1)
								{
									m_tracks[i]->bookmarktime = m_playcounts[i].bookmark_position;
									m_tracks[i]->bookmark_time_ms_common = m_tracks[i]->bookmarktime;
									//console::formatter() << "Setting bookmark: " << (t_int64)m_tracks[i]->pid << " : " << m_tracks[i]->title << " : " << m_playcounts[i].bookmark_position;
								}
								if (m_playcounts[i].play_state)
								{
									m_tracks[i]->played_marker = m_playcounts[i].play_state;
								}
								m_tracks[i]->skip_count_user += m_playcounts[i].skip_count;
								if (m_playcounts[i].last_skipped)
								{
									m_tracks[i]->last_skipped = m_playcounts[i].last_skipped;

									if (m_playcounts[i].skip_count)
									{
										playbackdata->playback_data[i].m_skipcountdata.m_valid = true;
										playbackdata->playback_data[i].m_skipcountdata.m_skipcount = m_playcounts[i].skip_count;
										playbackdata->playback_data[i].m_skipcountdata.m_lastskipped_timestamp = filetime_time_from_appletime(m_playcounts[i].last_skipped);
									}
								}
								if ((m_playcounts[i].last_played || m_playcounts[i].play_state || m_playcounts[i].rating) && m_playcounts[i].rating != -1 && m_tracks[i]->rating != m_playcounts[i].rating)
								{
									playbackdata->playback_data[i].m_ratingdata.m_valid = true;
									playbackdata->playback_data[i].m_ratingdata.m_rating = m_playcounts[i].rating;

									m_tracks[i]->application_rating = m_tracks[i]->rating;
									m_tracks[i]->rating = m_playcounts[i].rating;
								}

								if (playbackdata->playback_data[i].m_playcountdata.m_valid
									|| playbackdata->playback_data[i].m_skipcountdata.m_valid
									|| playbackdata->playback_data[i].m_ratingdata.m_valid)
								{
									if (m_tracks[i]->media_type == t_track::type_audio)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_audio;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_song;
									}
									else if (m_tracks[i]->media_type == t_track::type_podcast)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_audio;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_podcast;
									}
									else if (m_tracks[i]->media_type == t_track::type_audiobook)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_audio;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_audiobook;
									}
									else if (m_tracks[i]->media_type == t_track::type_video)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_video;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_movie;
									}
									else if (m_tracks[i]->media_type & t_track::type_music_video)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_video;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_music_video;
									}
									else if (m_tracks[i]->media_type == t_track::type_tv_show)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_video;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_tv_show;
									}
									else if (m_tracks[i]->media_type == t_track::type_video_podcast)
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_video;
										playbackdata->playback_data[i].m_mediasubtype = dop::mediasubtype_video_podcast;
									}
									else
									{
										playbackdata->playback_data[i].m_mediatype = dop::mediatype_unknown;
										playbackdata->playback_data[i].m_mediasubtype = NULL;
									}
									playbackdata->playback_data[i].m_track_on_device = m_handles[i];
									if (m_tracks[i]->last_known_path_valid || m_tracks[i]->original_path_valid)
									{
										static_api_ptr_t<metadb>()->handle_create(
											playbackdata->playback_data[i].m_track_local,
											make_playable_location(m_tracks[i]->last_known_path_valid ? m_tracks[i]->last_known_path : m_tracks[i]->original_path, m_tracks[i]->original_subsong_valid ? m_tracks[i]->original_subsong : 0));
									}
									playbackdata->playback_data_ptrs.add_item(&playbackdata->playback_data[i]);
								}
							}
						}
						m_tracks.remove_mask(mask_to_remove.get_ptr());
						m_handles.remove_mask(mask_to_remove.get_ptr());
					}
				}
				catch (const exception_aborted &)
				{
					throw;
				}
				catch (const pfc::exception & e)
				{
					console::formatter() << "iPod manager: Error reading play counts file: " << e.what();
				}
			}
		}
		void load_database_t::merge_playcount_data(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status, abort_callback & p_abort)
		{
			if (m_playbackdata_callback.is_valid())
			{
				service_ptr_t<dop::portable_device_playbackdata_callback_v2> ptr;
				service_enum_t<dop::portable_device_playbackdata_callback_v2> e;
				while (e.next(ptr))
					m_playbackdata_callback->m_callbacks_v2.add_item(ptr);
				pfc::array_t<HANDLE> events;
				t_size i, count = m_playbackdata_callback->m_callbacks_v2.get_count();
				events.set_count(count);
				m_playbackdata_callback->m_callback_notifiers.set_count(count);
				for (i = 0; i<count; i++)
				{
					events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
					m_playbackdata_callback->m_callback_notifiers[i] = new service_impl_t<portable_device_playbackdata_notifier_impl>(events[i]);
				}

				m_playbackdata_callback->m_event_completed.create(true, false);
				m_playbackdata_callback->m_status = &p_status;

				static_api_ptr_t<main_thread_callback_manager>()->add_callback(m_playbackdata_callback);

				WaitForMultipleObjectsEx(count, events.get_ptr(), TRUE, pfc_infinite, FALSE);
				//WaitForSingleObjectEx (m_playbackdata_callback->m_event_completed.get(), INFINITE, FALSE);

				for (i = 0; i<count; i++)
					CloseHandle(events[i]);
			}
			m_playbackdata_callback.release();

			try
			{
				pfc::string8 database_folder;
				p_ipod->get_database_path(database_folder);
				if (!p_ipod->mobile)
				{
					pfc::string8 temp; temp << database_folder << "\\iTunes\\" << "Play Counts";
					filesystem::g_remove(temp, p_abort);
				}
				else
				{
					pfc::string8 temp; temp << database_folder << "/iTunes/" << "PlayCounts.plist";
#if 0 //_DEBUG
					bool blah; t_filestats stats;
					filesystem::g_get_stats(temp, stats, blah, p_abort);
					console::formatter() << stats.m_timestamp;
#endif
					filesystem::g_remove(temp, p_abort);
				}
			}
			catch (const exception_io_not_found &) {}
			catch (const pfc::exception & ex)
			{
				console::formatter() << "iPod manager: Warning: failed to remove iPod Play Counts file; Reason: " << ex.what();
			};
		}


	}
}