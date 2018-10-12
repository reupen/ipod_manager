#include "stdafx.h"

#include "chapter.h"
#include "file_adder.h"
#include "plist.h"
#include "mp4.h"
#include "writer_sort_helpers.h"

namespace ipod
{
	namespace tasks
	{
		void load_database_t::read_storepurchases(ipod_device_ptr_ref_t p_ipod, store_purchases_type_t p_store_purchases_type, abort_callback & p_abort)
		{
			pfc::string8 path_podcastsSPI, path_podcasts;
			p_ipod->get_root_path(path_podcasts);
			pfc::string8 folder;
			if (p_store_purchases_type == store_purchases_standard)
				folder = "Purchases";
			else if (p_store_purchases_type == store_purchases_podcasts)
				folder = "Podcasts";
			path_podcasts << folder << p_ipod->get_path_separator_ptr();
			path_podcastsSPI << path_podcasts << "StorePurchasesInfo.plist";
			//file::ptr p_PodcastsSPI;
			try {
				t_uint32 last_tid;
				t_uint64 last_dbid;
				p_ipod->m_database.get_next_ids(last_tid, last_dbid);
				//filesystem::g_open(p_PodcastsSPI, path_podcastsSPI, filesystem::open_mode_read, p_abort);
				//bplist::reader p_reader(p_abort), p_reader_data(p_abort);
				PlistParserFromFile p_reader(path_podcastsSPI, p_abort);
				if (p_reader.m_root_object.is_valid())
				{
					cfobject::object_t::ptr_t data;
					if (p_reader.m_root_object->m_dictionary.get_child(L"data", data))
					{

						PlistParser p_reader_data(data->m_data.get_ptr(), data->m_data.get_size());

						if (p_reader_data.m_root_object.is_valid())
						{
							cfobject::object_t::ptr_t assetOrdering;
							if (p_reader_data.m_root_object->m_dictionary.get_child(L"assetOrdering", assetOrdering))
							{
								for (t_size i = 0, count = assetOrdering->m_array.size(); i<count; i++)
								{
									pfc::string8 fname;

									pfc::string8 relativeTrackPropertiesPath;
									if (assetOrdering->m_array[i]->m_dictionary.get_child(L"relativeMediaAssetPath", fname)
										&& filesystem::g_exists(pfc::string8() << path_podcasts << fname, abort_callback_dummy())
										&& assetOrdering->m_array[i].is_valid() && assetOrdering->m_array[i]->m_dictionary.get_child(L"relativeTrackPropertiesPath", relativeTrackPropertiesPath))
									{
										pfc::rcptr_t<t_track> track = pfc::rcnew_t<t_track>();

										XMLPlistParserFromFile pTrackProperties(pfc::string8() << path_podcasts << relativeTrackPropertiesPath, p_abort);
										cfobject::object_t::ptr_t TrackProperties, downloadInfo;
										pTrackProperties.run_cfobject(TrackProperties);

										if (TrackProperties.is_valid())
										{
											if (TrackProperties->m_dictionary.get_child(L"com.apple.iTunesStore.downloadInfo", downloadInfo))
											{
												downloadInfo->m_dictionary.get_child(L"trackPersistentID", track->pid);
											}
											if (track->pid && !have_track(track->pid))
											{
												TrackProperties->m_dictionary.get_child(L"description", track->subtitle);
												//TrackProperties->m_dictionary.get_child(L"duration", track->length);
												TrackProperties->m_dictionary.get_child(L"sampleRate", track->samplerate);
												track->samplerate *= 0x10000;
												TrackProperties->m_dictionary.get_child(L"duration", track->length);
												track->keywords_valid = TrackProperties->m_dictionary.get_child(L"keywords", track->keywords);
												pfc::string8 kind, podcast_type, type;
												TrackProperties->m_dictionary.get_child(L"kind", kind);
												TrackProperties->m_dictionary.get_child(L"type", type);
												if (p_store_purchases_type == store_purchases_podcasts)
												{
													//type == podcast-episode
													if (!stricmp_utf8(type, "podcast-episode"))
													{
														TrackProperties->m_dictionary.get_child(L"podcast-type", podcast_type);
														bool b_podcast = !stricmp_utf8(kind, "podcast");
														bool b_video_podcast = !stricmp_utf8(kind, "videoPodcast");
														if (!stricmp_utf8(podcast_type, "itunes-u"))
														{
															track->media_type = (track->media_type2 = t_track::type_itunes_u) | t_track::type_audio;
															track->genre_valid = true;
															track->genre = "iTunes U";
															track->podcast_flag = 1;
															track->remember_playback_position = 1;
															track->skip_on_shuffle = 1;
														}
														else
														{
															track->media_type2 = (track->media_type = t_track::type_podcast);
															track->genre_valid = true;
															track->genre = "Podcast";
															track->podcast_flag = 1;
															track->remember_playback_position = 1;
															track->skip_on_shuffle = 1;
														}
													}

													TrackProperties->m_dictionary.get_child(L"collection-id", track->legacy_store_playlist_id);
													TrackProperties->m_dictionary.get_child(L"collection-id", track->store_playlist_id);
													track->album_valid = TrackProperties->m_dictionary.get_child(L"collection-name", track->album);
													track->artist_valid = TrackProperties->m_dictionary.get_child(L"artist-name", track->artist);
													track->podcast_enclosure_url_valid = TrackProperties->m_dictionary.get_child(L"episode-guid", track->podcast_enclosure_url);
													track->category_valid = TrackProperties->m_dictionary.get_child(L"genre-name", track->category);
													TrackProperties->m_dictionary.get_child(L"item-id", track->legacy_store_item_id);
													TrackProperties->m_dictionary.get_child(L"item-id", track->store_item_id);
													track->description_valid = TrackProperties->m_dictionary.get_child(L"long-description", track->description);
													track->podcast_rss_url_valid = TrackProperties->m_dictionary.get_child(L"podcast-feed-url", track->podcast_rss_url);
													track->title_valid = TrackProperties->m_dictionary.get_child(L"title", track->title);
													cfobject::object_t::ptr_t releaseDate;
													if (TrackProperties->m_dictionary.get_child(L"release-date", releaseDate))
													{
														track->datereleased = apple_time_from_filetime(releaseDate->m_date, true);
													}
												}
												else
												{
													track->genre_valid = TrackProperties->m_dictionary.get_child(L"genre", track->genre);
													track->artist_valid = TrackProperties->m_dictionary.get_child(L"artistName", track->artist);
													track->title_valid = TrackProperties->m_dictionary.get_child(L"itemName", track->title);
													track->description_valid = TrackProperties->m_dictionary.get_child(L"longDescription", track->description);
													if (!stricmp_utf8(kind, "feature-movie"))
													{
														track->media_type2 = (track->media_type = t_track::type_video);
														track->video_flag = 1;
													}
													else if (!stricmp_utf8(kind, "music-video"))
													{
														track->media_type2 = (track->media_type = t_track::type_music_video);
														track->video_flag = 1;
													}
													else if (!stricmp_utf8(kind, "tv-episode"))
													{
														track->media_type2 = (track->media_type = t_track::type_tv_show);
														track->video_flag = 1;
													}
													else if (!stricmp_utf8(kind, "song"))
													{
														track->media_type2 = (track->media_type = t_track::type_audio);
													}
												}
												track->sort_album_valid = g_get_sort_string_for_ipod(track->album, track->sort_album, true);
												track->sort_artist_valid = g_get_sort_string_for_ipod(track->artist, track->sort_artist, true);
												//release-date
												TrackProperties->m_dictionary.get_child(L"storefront", track->legacy_store_storefront_id);
												TrackProperties->m_dictionary.get_child(L"storefront", track->store_front_id);

												{
													track->location_valid = true;
													track->location << ":" << folder << ":" << fname;
												}
												track->id = ++last_tid;
												track->dshm_type_6 = true;
												track->dshm_type_6_is_new = true;
												track->unk80_1 = 1;
												track->unk69_1 = 1;
												track->unk69_3 = 1;
												track->artwork_flag = 2;
												//track->mhii_id = LODWORD(track->pid);
												//track->unk68 = HIDWORD(track->pid);

												metadb_handle_ptr handle;
												static_api_ptr_t<metadb>()->handle_create(handle, make_playable_location(pfc::string8() << path_podcasts << fname, 0/*items[i]->get_subsong_index()*/));
												m_tracks.add_item(track);
												m_handles.add_item(handle);

												//console::formatter() << "iPod manager: Importing: PID: " << track->pid << "; Title : " << track->title;
											}
											//pfc::string8 dump;
											//cfobject::g_export_object_to_xml(TrackProperties, dump);
											//popup_message::g_show(dump, "StorePurchase");
										}
									}
								}
							}
						}


					}
				}
			}
			catch (exception_io_not_found const &) {}
			catch (pfc::exception const & ex)
			{
				console::formatter() << "iPod manager: Warning whilst importing store downloads: " << ex.what();
			};
		}
		void load_database_t::read_storepurchases(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort)
		{
			if (p_ipod->mobile)
			{
				//read_storepurchases(p_ipod, store_purchases_standard, p_abort);
				read_storepurchases(p_ipod, store_purchases_podcasts, p_abort);
				repopulate_albumlist();
			}
		}
		void load_database_t::merge_storepurchases(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort)
		{
			if (p_ipod->mobile)
			{
				bool sixg = (p_ipod->is_6g_format());
				for (t_size i = 0, count = m_tracks.get_count(); i<count; i++)
				{
					if (m_tracks[i]->dshm_type_6_is_new)
					{
						try
						{
							pfc::string_extension ext(m_tracks[i]->location);
							if (!stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a"))
							{
								g_get_itunes_chapters_mp4(m_handles[i]->get_path(), m_tracks[i]->m_chapter_list, p_abort);
								itunesdb::chapter_writer(&stream_writer_memblock_ref(m_tracks[i]->do_chapter_data)).write(m_tracks[i]->m_chapter_list, p_abort);
								m_tracks[i]->chapter_data_valid = true;
							}
						}
						catch (pfc::exception & ex)
						{
							console::formatter() << "iPod manager: Error reading MP4 chapters: " << ex.what();
						}
						try
						{
							album_art_extractor_instance_ptr api = g_get_album_art_extractor_instance(m_handles[i]->get_path(), p_abort);
							album_art_data_ptr artwork_data = api->query(album_art_ids::cover_front, p_abort);

							if (!m_artwork_valid)
								m_artwork.initialise_artworkdb(p_ipod);
							m_artwork_valid = true;
							//pfc::dynamic_assert(p_library.m_artwork_valid, "No ArtworkDB found");
							bool b_incRef = m_artwork.add_artwork_v3(p_ipod, m_tracks[i]->media_type, m_tracks[i]->pid, m_tracks[i]->artwork_cache_id, artwork_data, 100, p_abort, &m_tracks[i]->m_chapter_list);
							m_tracks[i]->artwork_count = 1;
							m_tracks[i]->artwork_flag = 1;
							m_tracks[i]->artwork_size = artwork_data->get_size();

							if (sixg && b_incRef)
							{
								t_size ii_index;
								if (m_artwork.find_by_image_id(ii_index, m_tracks[i]->artwork_cache_id))
								{
									m_artwork.image_list[ii_index].refcount++;
									m_artwork.image_list[ii_index].unk8 = 1;
								}
							}
						}
						catch (exception_album_art_not_found const &)
						{
						}
						catch (pfc::exception & ex)
						{
							console::formatter() << "iPod manager: Error reading MP4 chapters: " << ex.what();
						}
						//g_get_artwork_for_track(m_handles[i], artwork_data, m_mappings, true, m_process.get_abort());
						//chapters
						//artwork
					}
				}
				m_artwork.finalise_add_artwork_v2(p_ipod, abort_callback_dummy());
			}
		}

	}
}