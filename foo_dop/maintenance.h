#pragma once

#include "file_adder.h"
#include "photodb.h"

class ipod_rewrite_library_t : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_rewrite_library_t);

	t_field_mappings m_field_mappings;
	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_rewrite_library_t");
		bool b_started = false, b_need_to_update_database=false;
		try 
		{
			t_uint32 steps[] = {1, 10, 50, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run();
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;

			{
				bool sixg = (m_drive_scanner.m_ipods[0]->is_6g_format());
				m_items.sort_by_pointer();
				bool b_filter = m_items.get_count() > 0;
				//in_metadb_sync sync;
				t_size i, count_tracks = m_library.m_tracks.get_count();
				for (i=0; i<count_tracks; i++)
				{
					metadb_handle_ptr handle = m_library.m_handles[i];
					bool b_excluded = !(!b_filter || m_items.bsearch_by_pointer(handle) != pfc_infinite);
					pfc::string_extension ext(handle->get_path());
					if (stricmp_utf8(ext, "WAV") && (!m_library.m_tracks[i]->podcast_flag || m_library.m_tracks[i]->original_path_valid) && !b_excluded)
					{
						metadb_handle_ptr p_source;
						if (m_library.m_tracks[i]->original_path_valid || m_library.m_tracks[i]->last_known_path_valid)
							static_api_ptr_t<metadb>()->handle_create(p_source, 
							make_playable_location(m_library.m_tracks[i]->last_known_path_valid ? m_library.m_tracks[i]->last_known_path : m_library.m_tracks[i]->original_path, m_library.m_tracks[i]->original_subsong));
						else
							p_source = handle;
						m_library.m_tracks[i]->set_from_metadb_handle(handle, p_source, m_field_mappings);
					}
				}
				m_library.repopulate_albumlist();
			}

			m_process.advance_progresstep();
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_field_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
		}
		catch (exception_aborted &) 
		{
			if (b_need_to_update_database)
			{
				try 
				{
					m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,abort_callback_dummy());
				}
				catch (const pfc::exception & e) 
				{
					fbh::show_info_box_threadsafe("Error", e.what());
				}
			}
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
private:
	ipod_rewrite_library_t(const pfc::list_base_const_t<metadb_handle_ptr> & items = metadb_handle_list())
		: m_items(items), ipod_write_action_v2_t("Reload Metadata from Files on iPod")
	{
		core_api::ensure_main_thread();
		m_artwork_script = settings::artwork_sources;
	};
	metadb_handle_list m_items;
	pfc::string8 m_artwork_script;
};


class ipod_update_artwork_library_t : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_update_artwork_library_t);

	t_field_mappings m_field_mappings;
	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_update_artwork_library_t");
		bool b_started = false, b_need_to_update_database=false;
		try 
		{
			t_uint32 steps[] = {1, 10, 10, 50, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run();
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;

			m_process.advance_progresstep();
			{
				m_process.update_text("Updating artwork");
				bool sixg = (m_drive_scanner.m_ipods[0]->is_6g_format());
				m_items.sort_by_pointer();
				bool b_filter = m_items.get_count() > 0;
				//in_metadb_sync sync;
				t_size i, count_tracks = m_library.m_tracks.get_count(), k=0, count_real = b_filter ? m_items.get_count() : count_tracks;
				if (m_field_mappings.add_artwork /*&& m_artwork_script.get_length()*/ && m_drive_scanner.m_ipods[0]->m_device_properties.m_artwork_formats.get_count())
				{
					pfc::string8 empty_album;
					{
						file_info_impl empty_info;
						//m_field_mappings.get_artist(make_playable_location("", 0), &empty_info, empty_artist); //deal with "(None)" or "?" field remappings
						m_field_mappings.get_album(metadb_handle_ptr(), empty_info, empty_album);
					}
					pfc::rcptr_t<video_thumbailer_t> p_video_thumbailer;
					mmh::Permutation permutation_album_grouping;
					permutation_album_grouping.set_count(count_tracks);
					mmh::sort_get_permutation(m_library.m_tracks, permutation_album_grouping, ipod::tasks::load_database_t::g_compare_track_album_id, false);

					pfc::array_staticsize_t<bool> mask(count_tracks);
					for (i=0; i<count_tracks; i++)
						mask[i] = (!b_filter || m_items.bsearch_by_pointer(m_library.m_handles[i]) != pfc_infinite);

					mmh::UIntegerNaturalFormatter text_count(count_real);
					string_format_metadb_handle_for_progress track_formatter;
					for (i=0; i<count_tracks; i++)
					{
						mmh::UIntegerNaturalFormatter text_remaining(count_real-k);
						pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(2);
						progress_details[0].m_label = "Item:";
						progress_details[1].m_label = "Remaining:";
						progress_details[0].m_value = track_formatter.run(m_library.m_handles[i]);
						progress_details[1].m_value = pfc::string8() << count_real-k-1;
						m_process.update_text(pfc::string8() << "Updating artwork for " << text_count << " file" << (text_count.is_plural() ? "s" : "") );

						metadb_handle_ptr handle = m_library.m_handles[i];
						pfc::rcptr_t<itunesdb::t_track> p_track = m_library.m_tracks[i];
						bool b_excluded = !mask[i];//(!b_filter || m_items.bsearch_by_pointer(handle) != pfc_infinite);
						bool b_artwork_exists = p_track->artwork_count && p_track->artwork_flag == 0x1;

						if (!b_excluded && ( !(p_track->podcast_flag || p_track->video_flag) || !b_artwork_exists ) )
						{
							bool b_album_track = m_library.m_tracks[i]->media_type == t_track::type_audio && m_library.m_tracks[i]->album.length() && strcmp(m_library.m_tracks[i]->album, empty_album) != 0;
							t_size k = permutation_album_grouping.find_item(i); //should never be pfc_infinite
							try
							{
								//ptr->format_title_legacy(NULL, artworkfiles, m_artwork_script, NULL/*&titleformat_text_filter_impl_reserved_chars("\/:*?\"<>|")*/);
								//bool relative = artworkfiles.find_first(':') == pfc_infinite;
								if (true)
								{
									pfc::string8 local_path = m_library.m_tracks[i]->last_known_path_valid ?
										m_library.m_tracks[i]->last_known_path : m_library.m_tracks[i]->original_path;

									metadb_handle_ptr ptr;
									
									bool b_local_exists = local_path.length() && filesystem::g_exists(local_path,m_process.get_abort());
									if (b_local_exists)
										static_api_ptr_t<metadb>()->handle_create(ptr, make_playable_location(local_path, 0));
									//if (1/*!relative || !stricmp_utf8_partial("file://", source_folder, 7)*/)
									if (true)
									{
										album_art_data_ptr artwork_data;
										g_get_artwork_for_track(b_local_exists ? ptr : m_library.m_handles[i], artwork_data, m_mappings, !b_local_exists, m_process.get_abort());
										if (!artwork_data.is_valid() && m_library.m_tracks[i]->artwork_flag == 2 && m_library.m_tracks[i]->video_flag && m_mappings.video_thumbnailer_enabled)
										{
											const char * path = m_library.m_handles[i]->get_path();
											if (!stricmp_utf8_max(path, "file://", 7))
											{
												if (!p_video_thumbailer.is_valid())
													p_video_thumbailer = pfc::rcnew_t<video_thumbailer_t>();
												p_video_thumbailer->create_video_thumbnail(path+7, artwork_data);
											}
										}

										if (artwork_data.is_valid())
										{

											t_uint8 sha1[20];
											memset(sha1, 0, sizeof(sha1));
											mmh::hash::sha1((const t_uint8*)artwork_data->get_ptr(), artwork_data->get_size(), sha1);

											bool b_update = false;
											if (b_artwork_exists)
											{
												if (!p_track->artwork_source_sha1_valid)
													b_update = true;
												else if (p_track->artwork_size != artwork_data->get_size())
													b_update = true;
												else if (memcmp(sha1, p_track->artwork_source_sha1, 20))
													b_update = true;
											}
											else b_update = true;

											if (b_update)
											{

											p_track->artwork_source_size = artwork_data->get_size();
											p_track->artwork_source_size_valid = true;
											memcpy(p_track->artwork_source_sha1, sha1, 20);
											p_track->artwork_source_sha1_valid = true;

											bool b_processed = false;

											if (sixg && b_album_track /*&& !b_artwork_exists*/)
											{
												t_size l = k;
												if (l)
													while (--l && !b_processed && m_library.m_tracks[i]->album_id == m_library.m_tracks[permutation_album_grouping[l]]->album_id)
													{
														pfc::rcptr_t<itunesdb::t_track> p_album_track = m_library.m_tracks[permutation_album_grouping[l]];
														if (p_album_track->mhii_id && (!p_album_track->artwork_source_sha1_valid || !memcmp(m_library.m_tracks[i]->artwork_source_sha1, p_album_track->artwork_source_sha1, mmh::hash::sha1_digestsize)))
														{
															if (m_library.m_tracks[i]->mhii_id != p_album_track->mhii_id)
																m_library.remove_artwork(m_drive_scanner.m_ipods[0], m_library.m_tracks[i]);
															m_library.m_tracks[i]->mhii_id = p_album_track->mhii_id;
															m_library.m_tracks[i]->artwork_count = 1;
															m_library.m_tracks[i]->artwork_flag = 1;
															m_library.m_tracks[i]->artwork_size = p_album_track->artwork_size;
															b_processed = true;
														}
														//l--;
													}
													l=k;
													if (l<count_tracks)
														while (++l < count_tracks && !b_processed && m_library.m_tracks[i]->album_id == m_library.m_tracks[permutation_album_grouping[l]]->album_id)
														{
															pfc::rcptr_t<itunesdb::t_track> p_album_track = m_library.m_tracks[permutation_album_grouping[l]];
															if (p_album_track->mhii_id && (!p_album_track->artwork_source_sha1_valid || !memcmp(m_library.m_tracks[i]->artwork_source_sha1, p_album_track->artwork_source_sha1, mmh::hash::sha1_digestsize)))
															{
																if (m_library.m_tracks[i]->mhii_id != p_album_track->mhii_id)
																	m_library.remove_artwork(m_drive_scanner.m_ipods[0], m_library.m_tracks[i]);
																m_library.m_tracks[i]->mhii_id = p_album_track->mhii_id;
																m_library.m_tracks[i]->artwork_count = 1;
																m_library.m_tracks[i]->artwork_flag = 1;
																m_library.m_tracks[i]->artwork_size = p_album_track->artwork_size;
																b_processed = true;
															}
														}
											}
											bool b_inc_ref = true;
											if (!b_processed)
											{
												if (!m_library.m_artwork_valid)
													m_library.m_artwork.initialise_artworkdb(m_drive_scanner.m_ipods[0]);
												m_library.m_artwork_valid=true;
												//pfc::dynamic_assert(p_library.m_artwork_valid, "No ArtworkDB found");

												if (sixg)
												{
													t_size previous_ii_index = 0;
													if (m_library.m_tracks[i]->mhii_id && m_library.m_artwork.find_by_image_id(previous_ii_index, m_library.m_tracks[i]->mhii_id) && m_library.m_artwork.image_list[previous_ii_index].refcount > 1)
													{
														m_library.m_artwork.image_list[previous_ii_index].refcount--;
														m_library.m_tracks[i]->mhii_id = NULL;
													}
												}

												b_inc_ref = m_library.m_artwork.add_artwork_v3(m_drive_scanner.m_ipods[0], m_library.m_tracks[i]->media_type, m_library.m_tracks[i]->pid, m_library.m_tracks[i]->mhii_id, artwork_data, 100,m_process.get_abort());
												m_library.m_tracks[i]->artwork_count = 1;
												m_library.m_tracks[i]->artwork_flag = 1;
												m_library.m_tracks[i]->artwork_size = artwork_data->get_size();
												b_processed = true;

#if 0

												if (sixg && b_album_track)
												{
													t_size l = k;
													/*
													while (l && m_library.m_tracks[i]->album_id == m_library.m_tracks[permutation_album_grouping[l-1]]->album_id)
														l--;
														*/
													if (l)
														while (--l && m_library.m_tracks[i]->album_id == m_library.m_tracks[permutation_album_grouping[l]]->album_id)
														{
															if (m_library.m_tracks[i]->mhii_id == m_library.m_tracks[permutation_album_grouping[l]]->mhii_id)
															{
																m_library.m_tracks[permutation_album_grouping[l]]->artwork_size = m_library.m_tracks[i]->artwork_size;
																memcpy(m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_sha1, m_library.m_tracks[i]->artwork_source_sha1, 20);
																m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_size = m_library.m_tracks[i]->artwork_source_size;
																m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_sha1_valid = m_library.m_tracks[i]->artwork_source_sha1_valid;
																m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_size_valid = m_library.m_tracks[i]->artwork_source_size_valid;
																mask[permutation_album_grouping[l]] = false;
															}
															//l--;
														}
														l=k;
														if (l<count_tracks)
															while (++l < count_tracks && m_library.m_tracks[i]->album_id == m_library.m_tracks[permutation_album_grouping[l]]->album_id)
															{
																if (m_library.m_tracks[i]->mhii_id == m_library.m_tracks[permutation_album_grouping[l]]->mhii_id)
																{
																	m_library.m_tracks[permutation_album_grouping[l]]->artwork_size = m_library.m_tracks[i]->artwork_size;
																	memcpy(m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_sha1, m_library.m_tracks[i]->artwork_source_sha1, 20);
																	m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_size = m_library.m_tracks[i]->artwork_source_size;
																	m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_sha1_valid = m_library.m_tracks[i]->artwork_source_sha1_valid;
																	m_library.m_tracks[permutation_album_grouping[l]]->artwork_source_size_valid = m_library.m_tracks[i]->artwork_source_size_valid;
																	mask[permutation_album_grouping[l]] = false;
																}
															}
												}
#endif
											}
											if (sixg && b_processed && b_inc_ref)
											{
												t_size ii_index;
												if (m_library.m_artwork.find_by_image_id(ii_index, m_library.m_tracks[i]->mhii_id))
												{
													m_library.m_artwork.image_list[ii_index].refcount++;
													m_library.m_artwork.image_list[ii_index].unk8 = 1;
												}
											}
											}
										}
									}
								}
							}
							catch (const exception_aborted &)
							{
								try {
									m_library.m_artwork.finalise_add_artwork_v2(m_drive_scanner.m_ipods[0],abort_callback_dummy());
								} catch (pfc::exception &) {};
								throw;
							}
							catch (const pfc::exception &) {};
						}

						if (!b_excluded)
							m_process.update_progress_subpart_helper(++k, count_real);
					}
					m_library.m_artwork.finalise_add_artwork_v2(m_drive_scanner.m_ipods[0],abort_callback_dummy());
				}
			}

			m_process.advance_progresstep();
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_field_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
		}
		catch (exception_aborted &) 
		{
			if (b_need_to_update_database)
			{
				try 
				{
					m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,abort_callback_dummy());
				}
				catch (const pfc::exception & e) 
				{
					fbh::show_info_box_threadsafe("Error", e.what());
				}
			}
		}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
private:
	ipod_update_artwork_library_t(const pfc::list_base_const_t<metadb_handle_ptr> & items = metadb_handle_list())
		: m_items(items), ipod_write_action_v2_t("Refresh Artwork from Files on iPod")
	{
		core_api::ensure_main_thread();
		m_artwork_script = settings::artwork_sources;
	};
	metadb_handle_list m_items;
	pfc::string8 m_artwork_script;
};



class ipod_rewrite_library_soft_t : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_rewrite_library_soft_t);
	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_rewrite_library_soft_t");
		bool b_started = false, b_need_to_update_database=false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort());
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy());
			b_started = true; b_need_to_update_database = true;
			m_process.advance_progresstep();
			m_library.update_smart_playlists();
			m_process.checkpoint();
			b_need_to_update_database = false;
			m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
			m_process.advance_progresstep();
		}
		catch (exception_aborted &) {}
		catch (const pfc::exception & e) 
		{
			fbh::show_info_box_threadsafe("Error", e.what());
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
private:
	ipod_rewrite_library_soft_t() : ipod_write_action_v2_t("Rewrite iPod Database")
	{};
};

class process_locations_notify_process_path_t : public process_locations_notify
{
public:
	virtual void on_completion(const pfc::list_base_const_t<metadb_handle_ptr> & p_items)
	{
		m_handles = p_items;
		m_signal.set_state(true);
	}
	virtual void on_aborted() 
	{
		m_aborted = true;
		m_signal.set_state(true);
	}

	process_locations_notify_process_path_t() : m_aborted(false)
	{
		m_signal.create(true, false);
	}

	win32_event m_signal;
	metadb_handle_list m_handles;
	bool m_aborted;
};

class main_thread_procress_paths_info_t : public main_thread_callback
{
public:
	main_thread_procress_paths_info_t(const pfc::string_list_const & p_list,
		HWND p_parent_window, t_uint32 flags= playlist_incoming_item_filter_v3::op_flag_no_filter|playlist_incoming_item_filter_v3::op_flag_delay_ui|playlist_incoming_item_filter_v3::op_flag_background) 
		: m_list(p_list), m_parent_window(p_parent_window), m_flags(flags)
	{
		m_callback = new service_impl_t<process_locations_notify_process_path_t>;
	};

	virtual void callback_run()
	{
		static_api_ptr_t<playlist_incoming_item_filter_v3> api;
		api->process_locations_async(m_list, m_flags, "", "",  m_parent_window, m_callback);
	}

	service_ptr_t<process_locations_notify_process_path_t> m_callback;
	const pfc::string_list_const & m_list;
	HWND m_parent_window;
	t_uint32 m_flags;
};

class ipod_recover_orphaned_files : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_recover_orphaned_files);

	ipod_add_files m_adder;
	t_field_mappings m_field_mappings;
	metadb_handle_list m_items;

	static int g_compare_metadbhandle_noncasesensitive ( const metadb_handle_ptr & p1, const metadb_handle_ptr & p2 )
	{
		int ret = stricmp_utf8(p1->get_path(), p2->get_path());
		if (ret == 0) ret = pfc::compare_t(p1->get_subsong_index(),  p2->get_subsong_index());
		return ret;
	}

	virtual void on_run()
	{
		TRACK_CALL_TEXT("ipod_recover_orphaned_files");
		bool b_started = false, b_need_to_update_database=false;
		m_failed = false;
		try 
		{
			t_uint32 steps[] = {1, 10, 30, 5, 30, 3};
			m_process.set_steps(steps, tabsize(steps));
			m_drive_scanner.run(m_process,m_process.get_abort());
			DOP_TRACK_ACTION;
			initialise();
			m_process.advance_progresstep();
			if (!prepare(m_process, m_process.get_abort()))
				m_library.run(m_drive_scanner.m_ipods[0], m_process,m_process.get_abort(), true);
			//m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], m_process,m_process.get_abort()); //for 6G artwork
			m_process.advance_progresstep();
			m_library.refresh_cache(m_process.get_wnd(), m_drive_scanner.m_ipods[0], true, m_process,m_process.get_abort());
			m_process.advance_progresstep();
			
			m_process.update_text("Reading files");
			pfc::string_list_impl temp;
			pfc::string8 path;
			m_drive_scanner.m_ipods[0]->get_database_path(path);
			path << m_drive_scanner.m_ipods[0]->get_path_separator_ptr() << "Music" << m_drive_scanner.m_ipods[0]->get_path_separator_ptr();
			temp.add_item(path);
			service_ptr_t<main_thread_procress_paths_info_t> p_info_loader = new service_impl_t<main_thread_procress_paths_info_t>
				(temp, m_process.get_wnd());
			static_api_ptr_t<main_thread_callback_manager> p_main_thread;
			p_main_thread->add_callback(p_info_loader);
			if (!p_info_loader->m_callback->m_signal.wait_for(-1))
				throw pfc::exception("File reading timeout!");
			if (p_info_loader->m_callback->m_aborted) 
				throw exception_aborted("File read was aborted");

			mmh::Permutation permutation;
			permutation.set_count(m_library.m_handles.get_count());
			mmh::sort_get_permutation(m_library.m_handles.get_ptr(), permutation, g_compare_metadbhandle_noncasesensitive, false);

			t_size i, count = p_info_loader->m_callback->m_handles.get_count(), dummy;
			for (i=0; i<count; i++)
				if (!m_library.m_handles.bsearch_permutation_t(g_compare_metadbhandle_noncasesensitive, p_info_loader->m_callback->m_handles[i], permutation, dummy))
				{
					m_items.add_item(p_info_loader->m_callback->m_handles[i]);
				}

			if (m_items.get_count())
			{
				m_library.cleanup_before_write(m_drive_scanner.m_ipods[0], m_process, abort_callback_dummy()); 
				b_started = true;
				b_need_to_update_database = true;
				m_process.advance_progresstep();
				m_process.update_progress_subpart_helper(0,1);
				m_adder.run(m_drive_scanner.m_ipods[0], m_items, m_library, m_field_mappings, m_process,m_process.get_abort());

				m_process.advance_progresstep();
				//if (m_adder.m_added_items.get_count())
				m_library.update_smart_playlists();
				m_process.checkpoint();
				b_need_to_update_database = false;
				m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,m_process.get_abort());
				m_process.advance_progresstep();
			}
		}
		catch (exception_aborted) 
		{
			if (b_need_to_update_database)
			{
				try 
				{
					m_writer.run(m_drive_scanner.m_ipods[0], m_library, m_mappings, m_process,abort_callback_dummy());
				}
				catch (const pfc::exception & e) 
				{
					fbh::show_info_box_threadsafe("Error", e.what());
				}
			}
#if 0
			try 
			{
				abort_callback_impl p_dummy_abort;
				t_size i, count = m_adder.m_added_items.get_count();
				for (i=0; i<count; i++)
				{
					filesystem::g_remove(m_adder.m_added_items[i], p_dummy_abort);
				}
			}
			catch (const pfc::exception &) {}
#endif
		}
		catch (const pfc::exception & e) 
		{
			try 
			{
				abort_callback_impl p_dummy_abort;
				t_size i, count = m_adder.m_added_items.get_count();
				for (i=0; i<count; i++)
				{
					filesystem::g_remove(m_adder.m_added_items[i], p_dummy_abort);
				}
			}
			catch (const pfc::exception &) {}
			fbh::show_info_box_threadsafe("Error", e.what());
			m_failed = true;
		}
		if (m_drive_scanner.m_ipods.get_count() && b_started)
			m_drive_scanner.m_ipods[0]->do_after_sync();
	}
	void on_exit()
	{
		if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
		{

			if (m_adder.m_errors.get_count())
			{
				results_viewer::g_run(L"Warnings - Recover Orphaned iPod Files", m_adder.m_errors);
			}
			if (m_items.get_count())
			{
				static_api_ptr_t<playlist_manager_v3> api;
				t_size index = api->find_or_create_playlist_unlocked("Recovered iPod tracks");
				api->playlist_add_items(index, m_items, bit_array_false());
				api->set_active_playlist(index);
			}

		}
	}
private:
	ipod_recover_orphaned_files()
		: m_failed(false), ipod_write_action_v2_t("Recover Orphaned iPod Files")
	{};
	bool m_failed;
};

