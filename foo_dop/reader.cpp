#include "main.h"




namespace ipod
{
	namespace tasks
	{

		void load_database_t::cleanup_before_write(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort)
		{
			if (!IsValidCodePage(pfc::stringcvt::codepage_ascii))
			{
				throw pfc::exception("ASCII code page is not available");
			}
			p_ipod->do_before_sync(); //FIXME
			if (p_ipod->mobile)
				read_playcounts(p_ipod, p_abort);
			merge_storepurchases(p_ipod, p_abort);
			merge_on_the_go_playlists(p_ipod, p_status,p_abort);
			merge_playcount_data(p_ipod, p_status, p_abort);
			t_size i = m_tracks_to_remove.get_count();
			for (;i;i--)
			{
				remove_artwork(p_ipod, m_tracks_to_remove[i-1]);
				m_tracks_to_remove.remove_by_idx(i-1);
			}
		}

		void load_database_t::merge_on_the_go_playlists(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort)
		{
			pfc::string8 database_folder;
			p_ipod->get_database_path(database_folder);
			t_filetimestamp time;
			GetSystemTimeAsFileTime((LPFILETIME)&time);
			t_size i, count_otg = m_onthego_playlists.get_size(), count_old = m_playlists.get_size(), count = count_old + count_otg;
			t_size name_index = 1;

			m_playlists.set_size(count);
			for (i=0; i<count_otg; i++)
				m_playlists[count_old+i] = pfc::rcnew_t<t_playlist>();

			while (have_playlist(pfc::string8() << "On-The-Go " << name_index)) name_index++;
			for (i=0; i<count_otg; i++)
			{
				m_playlists[count_old+i]->name = pfc::string8() << "On-The-Go " << name_index;
				t_size j, count_items = m_onthego_playlists[i].items.get_size();
				m_playlists[count_old+i]->items.set_count(count_items);
				m_playlists[count_old+i]->timestamp = apple_time_from_filetime(time);
				m_playlists[count_old+i]->id = get_new_playlist_pid();
				for (j=0; j<count_items; j++)
				{
					t_uint32 index = m_onthego_playlists[i].items[j];
					if (index < m_tracks.get_count())
						m_playlists[count_old+i]->items[j].track_id = m_tracks[index]->id;
					m_playlists[count_old+i]->items[j].timestamp = apple_time_from_filetime(time);
					m_playlists[count_old+i]->items[j].position = j+1;
					m_playlists[count_old+i]->items[j].position_valid = true;
				}
				if (i+1 < count_otg)
				{
					name_index++;
					while (have_playlist(pfc::string8() << "On-The-Go " << name_index)) name_index++;
				}
			}
			pfc::string8 current = database_folder;
			if (filesystem::g_exists(current << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr() << "OTGPlaylistInfo", p_abort))
			{
				t_size index = 0;
				do 
				{
					try
					{
						filesystem::g_remove(current, p_abort);
					}
					catch (const exception_aborted &) 
					{
						throw;
					}
					catch (const pfc::exception & e) 
					{
						console::formatter () << "iPod manager: Error removing " << current << ": " << e.what();
					}
					current = database_folder;
					index++;
				}
				while (filesystem::g_exists(current << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr() << "OTGPlaylistInfo" << "_" << index, p_abort));
			}
			/*
			for (i=0; i<count_otg; i++)
			{
			pfc::string8 temp;
			temp << m_path << "iPod_Control\\iTunes\\OTGPlaylistInfo";
			if (i>0) temp << "_" << i;
			try
			{
			filesystem::g_remove(temp, p_abort);
			}
			catch (const pfc::exception &) {};
			}*/
			m_onthego_playlists.set_size(0);
		}
		void load_database_t::remove_artwork (ipod_device_ptr_ref_t p_ipod, const pfc::rcptr_t<itunesdb::t_track> & p_track)
		{
			bool sixg = (p_ipod->is_6g_format());
			bool b_index_valid = false;
			t_size ii_index;
			if (p_track->mhii_id == NULL)
				b_index_valid = !sixg && m_artwork.find_by_dbid(ii_index ,p_track->pid);//remove_by_dbid(m_tracks[index]->dbid);
			else
				b_index_valid = m_artwork.find_by_image_id(ii_index, p_track->mhii_id);//remove_by_image_id(m_tracks[index]->mhii_id);
			if (b_index_valid)
			{
				if (sixg)
				{
					if (m_artwork.image_list[ii_index].refcount)
						m_artwork.image_list[ii_index].refcount--;
					if (m_artwork.image_list[ii_index].refcount == 0)
						m_artwork.__remove_by_index(p_ipod, ii_index, abort_callback_dummy());
				}
				else
				{
					m_artwork.__remove_by_index(p_ipod, ii_index, abort_callback_dummy());
				}
			}
		}
		void load_database_t::run(ipod_device_ptr_ref_t p_ipod, threaded_process_v2_t & p_status,abort_callback & p_abort, bool b_photos)
		{
			//pfc::hires_timer timer;
			//timer.start();

			p_status.checkpoint();

			p_status.update_text("Loading database files");
			service_ptr_t<file> p_file;
			pfc::string8 database_folder;
			p_ipod->get_database_path(database_folder);
			try
			{
				//profiler(readinga);
				//p_status.update_progress_subpart_helper(0);
				pfc::string8 base = database_folder;
				base << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr();

				try 
				{
					filesystem::g_open_read(p_file, pfc::string8() << base << "iTunesCDB", p_abort);
				}
				catch (exception_io_not_found const &)
				{
					filesystem::g_open_read(p_file, pfc::string8() << base << "iTunesDB", p_abort);
				}

				p_status.checkpoint();

				t_filesize filesize = p_file->get_size_ex(p_abort);
				pfc::array_t<t_uint8> data;
				data.set_size(pfc::downcast_guarded<t_size>(filesize));
				{
					//profiler(readingaa);
					p_file->read(data.get_ptr(), data.get_size(), p_abort);
				}
				p_status.checkpoint();

				itunesdb::stream_reader_memblock_ref_dop stream(data.get_ptr(), data.get_size());

				itunesdb::reader reader(&stream);
				itunesdb::t_header_marker<identifiers::dbhm> dbhm;
				reader.read_header(dbhm, p_abort);

				t_uint32 dbhm_version, dshm_count;
				itunesdb::stream_reader_memblock_ref_dop p_dbhm(dbhm.data.get_ptr(), dbhm.data.get_size());

				p_dbhm.read_lendian_t(format, p_abort); //12 1,2
				p_dbhm.read(&unk0, sizeof(unk0), p_abort); //13,14,15
				p_dbhm.read_lendian_t(dbhm_version, p_abort); //16
				p_dbhm.read_lendian_t(dshm_count, p_abort); //20
				p_dbhm.read_lendian_t(dbid, p_abort); //24
				p_dbhm.read_lendian_t(unk1, p_abort); //32
				try {
					p_dbhm.read_lendian_t(unk1_1, p_abort); //36
					p_dbhm.read_lendian_t(unk2, p_abort); //44
					p_dbhm.read_lendian_t(candy_version, p_abort); //48 CandyVersion 1,2,3
					p_dbhm.read(&hash1, sizeof(hash1), p_abort); //50
					p_dbhm.read_lendian_t(unk4, p_abort); //70
					p_dbhm.read_lendian_t(pid, p_abort); //72
					p_dbhm.read_lendian_t(unk6, p_abort); //80
					p_dbhm.read_lendian_t(unk7, p_abort); //84
					p_dbhm.read(&hash2, sizeof(hash2), p_abort); //88
					p_dbhm.read_lendian_t(time_zone_offset_seconds, p_abort); //108 time zone ofst secs signed
					p_dbhm.read_lendian_t(candy_flags, p_abort); //112 signature flags
					p_dbhm.read(&hash3, sizeof(hash3), p_abort); //114
					p_dbhm.read_lendian_t(audio_language, p_abort); //160
					p_dbhm.read_lendian_t(subtitle_language, p_abort); //162
					p_dbhm.read_lendian_t(unk11_1, p_abort); //164
					p_dbhm.read_lendian_t(unk11_2, p_abort); //166
					p_dbhm.read_lendian_t(encoding, p_abort); //168 BYTE encoding
					p_dbhm.read_lendian_t(unk12, p_abort); //169
					p_dbhm.read_lendian_t(unk13, p_abort); //170
					//p_dbhm.read(&unk12, sizeof(unk12), p_abort);
					p_dbhm.read(&hash4, sizeof(hash4), p_abort);
					p_dbhm.read(&unk14, sizeof(unk14), p_abort);
				} catch (exception_io_data_truncation &) {};

				if (format > 2)
					throw pfc::exception(pfc::string8() << "Unknown file format: " << format);
				if (encoding > 1)
					throw pfc::exception(pfc::string8() << "Unknown file encoding: " << encoding);

				pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> decompressed_data;
				if (format == 2 && encoding == 1)
				{
					zlib_stream zs;
					zs.decompress_singlerun(data.get_ptr()+stream.get_position(p_abort), stream.get_remaining(), decompressed_data);
					stream.set_data(decompressed_data);
				}

				bool b_got_playlists = false;

				unsigned i;
				for (i=0; i<dshm_count; i++)
				{
					t_header_marker<identifiers::dshm> dshm;
					reader.read_header(dshm, p_abort);
					{
						t_uint32 type;
						itunesdb::stream_reader_memblock_ref_dop p_dshm(dshm.data.get_ptr(), dshm.data.get_size());
						p_dshm.read_lendian_t(type, p_abort);
						if (type == dataset_tracklist
#if 1
							|| type == dataset_tracklist2
#endif
							)
						{
							//profiler(readingab);
							t_header_marker<identifiers::tlhm> tlhm;
							reader.read_header(tlhm, p_abort);

							m_tracks.prealloc(tlhm.section_size);

							unsigned j;
							for (j=0; j<tlhm.section_size; j++)
							{
								t_header_marker<identifiers::tihm> tihm;
								reader.read_header(tihm, p_abort);

								pfc::rcptr_t <t_track> track;
								reader.read_tihm(tihm, track, p_abort);

								if (track.is_valid())
								{
									track->dshm_type_6 = (type == dataset_tracklist2);
									m_tracks.add_item(track);
								}
							}
						}
						else if ( (type == dataset_playlistlist || type == dataset_playlistlist_v2 /*|| type == dataset_specialplaylists*/)
							&& (!b_got_playlists /*|| type == dataset_specialplaylists*/) )
						{
							//profiler(readingad);
							t_header_marker<identifiers::plhm> plhm;
							reader.read_header(plhm, p_abort);
							if (plhm.section_size)
							{
								{
									t_header_marker<identifiers::pyhm> pyhm;
									reader.read_header(pyhm, p_abort);
									reader.read_pyhm(pyhm, m_library_playlist, p_abort);
									if (!m_library_playlist->is_master) throw pfc::exception("Expected master playlist in first position");
								}

								t_size base = m_playlists.get_count();
								m_playlists.set_size(base+plhm.section_size-1);
								unsigned j;
								for (j=0; j<plhm.section_size-1; j++)
								{
									t_header_marker<identifiers::pyhm> pyhm;
									reader.read_header(pyhm, p_abort);
									reader.read_pyhm(pyhm, m_playlists[base+j], p_abort);
								}
							}
							b_got_playlists = (type == dataset_playlistlist || type == dataset_playlistlist_v2);
						}
						else if ( type == dataset_specialplaylists )
						{
							t_header_marker<identifiers::plhm> plhm;
							reader.read_header(plhm, p_abort);
							if (plhm.section_size)
							{
								t_size base = m_special_playlists.get_count();
								m_special_playlists.set_size(base+plhm.section_size);
								unsigned j;
								for (j=0; j<plhm.section_size; j++)
								{
									t_header_marker<identifiers::pyhm> pyhm;
									reader.read_header(pyhm, p_abort);
									reader.read_pyhm(pyhm, m_special_playlists[base+j], p_abort);
								}
							}
							m_special_playlists_valid = true;
						}
						else if (type == dataset_genius_cuid)
						{
							t_size len = dshm.section_size-dshm.header_size;
							m_genius_cuid.set_size(len);
							stream.read(m_genius_cuid.get_ptr(), len, p_abort);
							m_genius_cuid_valid = true;
						}
						else if (type == dataset_albumlist)
						{
							//profiler(readingab);
							t_header_marker<identifiers::alhm> alhm;
							reader.read_header(alhm, p_abort);

							m_album_list.m_master_list.set_count(alhm.section_size);

							unsigned j;
							for (j=0; j<alhm.section_size; j++)
							{
								t_album::ptr album;
								t_header_marker<identifiers::aihm> aihm;
								reader.read_header(aihm, p_abort);

								reader.read_aihm(aihm, album, p_abort);

								m_album_list.m_master_list[j] = (album);

								switch(album->kind)
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
						else if (type == dataset_artistlist)
						{
							//profiler(readingab);
							t_header_marker<identifiers::ilhm> ilhm;
							reader.read_header(ilhm, p_abort);

							m_artist_list.set_count(ilhm.section_size);

							unsigned j;
							for (j=0; j<ilhm.section_size; j++)
							{
								//t_artist::ptr artist;
								t_header_marker<identifiers::iihm> iihm;
								reader.read_header(iihm, p_abort);

								reader.read_iihm(iihm, m_artist_list[j], p_abort);
								//m_artist_list.add_item(artist);
							}
						}
						else
							stream.skip(dshm.section_size - dshm.header_size,p_abort);
						p_status.update_progress_subpart_helper(i, dshm_count);
					}

				}
			}
			catch (const exception_aborted &) 
			{
				throw;
			}
			catch (const pfc::exception & e) 
			{
				m_failed=true;
				throw pfc::exception(pfc::string_formatter() << "Error reading iTunesDB : " << e.what());
			}

			p_status.checkpoint();

			//console::formatter() << "itunesdb read and parsed in : " << pfc::format_time_ex(timer.query(),6);
			//timer.start();
			if (1)
			{
				//profiler(readingb);
				try
				{
					service_list_t<file, pfc::alloc_fast_aggressive> files;

					pfc::string8 path, current = database_folder;
					if (filesystem::g_exists(current << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr() << "OTGPlaylistInfo", p_abort))
					{
						t_size index = 0;
						do 
						{
							try
							{
								filesystem::g_open_read(p_file, current, p_abort);
								files.add_item(p_file);
							}
							catch (const exception_aborted &) 
							{
								throw;
							}
							catch (const pfc::exception & e) 
							{
								console::formatter () << "iPod manager: Error reading " << current << ": " << e.what();
							}
							current = database_folder;
							index++;
						}
						while (filesystem::g_exists(current << "\\iTunes\\" << "OTGPlaylistInfo" << "_" << index, p_abort));
					}

					t_size i, count_otg = files.get_count();

					m_onthego_playlists.set_size(count_otg);
					for (i=0; i<count_otg; i++)
					{
						try {

							t_header_marker<identifiers::ophm> ophm;

							t_filesize filesize = files[i]->get_size_ex(p_abort);
							pfc::array_t<t_uint8> data;
							data.set_size(pfc::downcast_guarded<t_size>(filesize));
							files[i]->read(data.get_ptr(), data.get_size(), p_abort);
							itunesdb::stream_reader_memblock_ref_dop stream(data.get_ptr(), data.get_size());

							reader p_reader(&stream);
							p_reader.read_header(ophm, p_abort);
							itunesdb::stream_reader_memblock_ref_dop ophm_data(ophm.data.get_ptr(), ophm.data.get_size());

							t_uint32 count_items;
							ophm_data.read_lendian_t(count_items, p_abort);

							m_onthego_playlists[i].items.set_size(count_items);

							t_size j;
							for (j=0; j<count_items; j++)
							{
								stream.read_lendian_t(m_onthego_playlists[i].items[j], p_abort);
							}
						}
						catch (const exception_aborted &) 
						{
							throw;
						}
						catch (const pfc::exception & e) 
						{
							m_onthego_playlists[i].items.set_size(0);
							console::formatter () << "iPod manager: Error reading OTGPlaylistInfo #" << i << ": " << e.what();
						}
					}
				}
				catch (const exception_aborted &) 
				{
					throw;
				}
				catch (const pfc::exception & e) 
				{
					console::formatter () << "iPod manager: Error reading OTGPlaylistInfo file : " << e.what();
				}
			}
			p_status.checkpoint();

			{

				t_size i, count = m_tracks.get_count();

				{
					//profiler(readingca);
					m_handles.prealloc(count);

					for (i=0; i<count; i++)
					{
						pfc::string8 temp = m_tracks[i]->location, path;
						p_ipod->get_root_path(path);
						if (temp.get_length() && temp.get_ptr()[0] == ':')
							temp.remove_chars(0,1);
						temp.replace_char(':', p_ipod->get_path_separator());
						path << temp;
						metadb_handle_ptr ptr;
						static_api_ptr_t<metadb>()->handle_create(ptr, make_playable_location(path, 0));
						if (!ptr.is_valid())
							throw pfc::exception_bug_check("Bug check: metadb::handle_create");
						//if (stricmp_utf8(can, ptr->get_path()))
						//	throw pfc::exception(pfc::string_formatter() << "Bug check! " << can << " != " << ptr->get_path());

						t_size index = m_handles.add_item(ptr);
					}
				}

				{
					//profiler(readingcb);
					ipod_read_dopdb(database_folder, *this, p_status, p_abort);
				}

				{
					pfc::string8 path;
					p_ipod->get_database_path(path);
					path << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr() << "iTunesPrefs";
					try
					{
						filesystem::g_open_read(p_file, path, p_abort);
						t_filesize filesize = p_file->get_size_ex(p_abort);
						pfc::array_t<t_uint8> data;
						data.set_size(pfc::downcast_guarded<t_size>(filesize));
						p_file->read(data.get_ptr(), data.get_size(), p_abort);

						if (data.get_size() > 0xF9 + 1)
						{
							m_itunesprefs.m_voiceover_enabled = data[0xF9] != 0;
						}
					}
					catch (const pfc::exception & ex)
					{
						console::formatter() << "Error reading iTunesPrefs file: " << ex.what();
					}
				}
				p_status.checkpoint();
				if (1/*b_photos*/)
				{
					//profiler(readingcc);

					pfc::string8 pathartwork = database_folder, path;
					p_ipod->get_root_path(path);
					//path << "iPod_Control\\Artwork\\ArtworkDB";
					path << "Photos" << p_ipod->get_path_separator_ptr() << "Photo Database";
					pathartwork << p_ipod->get_path_separator_ptr() << "Artwork" << p_ipod->get_path_separator_ptr() << "ArtworkDB";
					try
					{
						if (filesystem::g_exists(path, p_abort))
						{

							filesystem::g_open_read(p_file, path, p_abort);
							t_filesize filesize = p_file->get_size_ex(p_abort);
							pfc::array_t<t_uint8> data;
							data.set_size(pfc::downcast_guarded<t_size>(filesize));
							p_file->read(data.get_ptr(), data.get_size(), p_abort);
							itunesdb::stream_reader_memblock_ref_dop stream(data.get_ptr(), data.get_size());

							photodb::reader reader(&stream);
							//photodb::reader reader(p_file);
							reader.read_photodb(m_photos, p_abort);
							m_photos_valid=true;
						}
					}
					catch (const pfc::exception & ex)
					{
						throw pfc::exception( pfc::string_formatter() << "Error reading Photo Database: " << ex.what() );
					}
					try
					{
						if (filesystem::g_exists(pathartwork, p_abort))
						{

							filesystem::g_open_read(p_file, pathartwork, p_abort);
							t_filesize filesize = p_file->get_size_ex(p_abort);
							pfc::array_t<t_uint8> data;
							data.set_size(pfc::downcast_guarded<t_size>(filesize));
							p_file->read(data.get_ptr(), data.get_size(), p_abort);
							itunesdb::stream_reader_memblock_ref_dop stream(data.get_ptr(), data.get_size());

							photodb::reader reader(&stream);
							reader.read_photodb(m_artwork, p_abort);
							m_artwork_valid=true;
						}
					}
					catch (const pfc::exception & ex)
					{
						throw pfc::exception( pfc::string_formatter() << "Error reading ArtworkDB database: " << ex.what() );
					}

				}


			}
			p_status.checkpoint();

			read_storepurchases(p_ipod, p_abort);

			p_status.checkpoint();

			if (!m_writing)
				rebuild_podcast_playlist();

			if (!m_writing || !p_ipod->mobile)
				read_playcounts(p_ipod, p_abort);

			p_status.checkpoint();

			//console::formatter() << "otherdb read and parsed in : " << pfc::format_time_ex(timer.query(),6);
		}
		void load_database_t::rebuild_podcast_playlist()
		{
			class podcast_group_t
			{
			public:
				pfc::list_t < pfc::rcptr_t<t_track> > m_tracks;
				pfc::string8 m_album, m_sort_album;
				bool m_sort_album_valid;

				static int g_compare_album (const podcast_group_t & item1, const podcast_group_t &item2)
				{
					return stricmp_utf8(item1.m_sort_album_valid ? item1.m_sort_album : item1.m_album,
						item2.m_sort_album_valid ? item2.m_sort_album : item2.m_album);
				}

				podcast_group_t() : m_sort_album_valid(false) {};
			};
			t_playlist::ptr p_playlist = pfc::rcnew_t<t_playlist>();
			t_playlist::ptr p_playlist_old;
			for (t_size i = 0, count = m_playlists.get_count(); i<count; i++)
				if (m_playlists[i]->podcast_flag) {p_playlist_old = m_playlists[i]; break;}

			p_playlist->id = (p_playlist_old.is_valid() ? p_playlist_old->id : get_new_playlist_pid());

			t_filetimestamp time;
			GetSystemTimeAsFileTime((LPFILETIME)&time);

			p_playlist->date_modified = apple_time_from_filetime(time);
			p_playlist->timestamp = (p_playlist_old.is_valid() ? p_playlist_old->timestamp : p_playlist->date_modified);
			p_playlist->repeat_mode = 1; //??
			p_playlist->podcast_flag = 1;
			p_playlist->sort_order = 0x18;
			p_playlist->album_field_order = 1;
			p_playlist->name = "Podcasts";
			if (p_playlist_old.is_valid())
			{
				p_playlist->column_data_valid = p_playlist_old->column_data_valid;
				p_playlist->dohm_column_data = p_playlist_old->dohm_column_data;
				p_playlist->do_column_data = p_playlist_old->do_column_data;
				p_playlist->itunes_data_102_valid = p_playlist_old->itunes_data_102_valid;
				p_playlist->do_itunes_data_102 = p_playlist_old->do_itunes_data_102;
			}

			pfc::list_t < pfc::rcptr_t<t_track> > podcast_tracks;
			for (t_size i = 0, count = m_tracks.get_count(); i<count; i++)
				if (m_tracks[i]->podcast_flag) podcast_tracks.add_item(m_tracks[i]);

			mmh::g_sort_qsort_v2(podcast_tracks.get_ptr(), podcast_tracks.get_count(), t_track::g_compare_podcast, false);

			pfc::list_t<podcast_group_t> podcast_groups;

			t_size j = 0;

			for (t_size i = 0, count = podcast_tracks.get_count(); i<count; i++)
			{
				if (i==0 || t_track::g_compare_podcast_group(podcast_tracks[i-1], podcast_tracks[i]))
				{
					//console::formatter() << podcast_tracks[i]->podcast_rss_url;

					j = podcast_groups.add_item(podcast_group_t());
					podcast_groups[j].m_album = podcast_tracks[i]->album;
					podcast_groups[j].m_sort_album = podcast_tracks[i]->sort_album;
					podcast_groups[j].m_sort_album_valid = podcast_tracks[i]->sort_album_valid;
				}
				podcast_groups[j].m_tracks.add_item(podcast_tracks[i]);
			}
			
			mmh::g_sort_qsort_v2(podcast_groups.get_ptr(), podcast_groups.get_size(),  podcast_group_t::g_compare_album, false);
			t_uint32 group_id_counter = 0x200, position_counter = 0;
			for (t_size i = 0, count = podcast_groups.get_count(); i<count; i++)
			{
				t_playlist_entry group_entry;
				group_entry.group_id = group_id_counter++;
				group_entry.is_podcast_group = 1;
				group_entry.is_podcast_group_expanded = 0;
				group_entry.podcast_group_name_flags = 0x80 | (podcast_groups[i].m_sort_album_valid ? 0x1 : 0);
				group_entry.podcast_title_valid = true;
				group_entry.podcast_title = podcast_groups[i].m_album;
				group_entry.podcast_sort_title_valid = podcast_groups[i].m_sort_album_valid;
				group_entry.podcast_sort_title = podcast_groups[i].m_sort_album;

				//console::formatter() << group_entry.podcast_title << " : " << podcast_groups[i].m_tracks.get_count();

				p_playlist->items.add_item(group_entry);

				for (t_size j = 0, count_enries = podcast_groups[i].m_tracks.get_count(); j<count_enries; j++)
				{
					pfc::rcptr_t<t_track> track = podcast_groups[i].m_tracks[j];
					t_playlist_entry entry;
					entry.item_pid = track->pid;
					entry.podcast_group = group_entry.group_id;
					entry.position_valid = true;
					entry.position = ++position_counter;
					entry.timestamp = track->dateadded;
					entry.track_id = track->id;
					entry.group_id = group_id_counter++;

					p_playlist->items.add_item(entry);
				}

				//timesttamp == dateadded
			}
			for (t_size i = m_playlists.get_count(); i; i--)
				if (m_playlists[i-1]->podcast_flag) {m_playlists.remove_by_idx(i-1);}

			if (p_playlist->items.get_count())
				m_playlists.add_item(p_playlist);
		}


		void load_database_t::get_next_ids (t_uint32 & next_tid, t_uint64 & next_dbid)
		{
			mmh::permutation_t permutation, permutationdbid;
			t_size count = m_tracks.get_count();
			permutation.set_size(count);
			permutationdbid.set_size(count);

			mmh::g_sort_get_permutation_qsort(m_tracks, permutation, g_compare_track_id, false);
			mmh::g_sort_get_permutation_qsort(m_tracks, permutationdbid, g_compare_track_dbid, false);

			next_tid = count ? m_tracks[permutation[count-1]]->id : 1;
			next_dbid = 0;
			if (count) next_dbid = m_tracks[permutationdbid[count-1]]->pid;
			else
			{
				service_ptr_t<genrand_service> p_rand = genrand_service::g_create();
				p_rand->seed(GetTickCount());
				t_uint64 p1 = p_rand->genrand(MAXLONG);
				t_uint64 p2 = p_rand->genrand(MAXLONG);
				next_dbid = p1|(p2<<32);
			}
		}

		t_uint64 load_database_t::get_new_playlist_pid()
		{
			t_uint64 pid = NULL;
			mmh::permutation_t permutation;
			t_size count = m_playlists.get_count();
			permutation.set_size(count);

			mmh::g_sort_get_permutation_qsort(m_playlists, permutation, g_compare_playlist_id, false);

			t_size index, counter = max(100 + 10*m_playlists.get_count(), m_playlists.get_count());

			mmh::genrand_t p_genrand;
			//service_ptr_t<genrand_service> p_rand = genrand_service::g_create();
			do
			{
				//p_rand->seed(GetTickCount());
				//t_uint64 p1 = p_rand->genrand(MAXLONG - 1) + 1; //Zero is banned
				//t_uint64 p2 = p_rand->genrand(MAXLONG - 0x1000); //Stay away from top-end - in case other sw just increments
				//pid = p1|(p2<<32);
				pid = p_genrand.run_uint64(0x100, -0x10000);
			}
			while ((m_playlists.bsearch_permutation_t(t_playlist::g_compare_pid_by_value, pid, permutation, index) || pid == m_library_playlist->id) && --counter);

			if (counter == 0) {pid = NULL; console::formatter() << "iPod manager: Error - couldn't generate a new playlist pid.";}

			return pid;
		}
		t_uint64 load_database_t::get_new_track_pid()
		{
			t_uint64 pid = NULL;
			mmh::permutation_t permutation;
			t_size count = m_tracks.get_count();
			permutation.set_size(count);

			mmh::g_sort_get_permutation_qsort(m_tracks, permutation, g_compare_track_dbid, false);

			t_size index, counter = max(100 + 10*m_tracks.get_count(), m_tracks.get_count());

			mmh::genrand_t p_genrand;
			//service_ptr_t<genrand_service> p_rand = genrand_service::g_create();
			do
			{
				//p_rand->seed(GetTickCount());
				//t_uint64 p1 = p_rand->genrand(MAXLONG - 1) + 1; //Zero is banned
				//t_uint64 p2 = p_rand->genrand(MAXLONG - 0x1000); //Stay away from top-end - in case other sw just increments
				//pid = p1|(p2<<32);
				pid = p_genrand.run_uint64(0x100, -0x10000);
			}
			while ((m_tracks.bsearch_permutation_t(g_compare_track_dbid_with_dbid, pid, permutation, index)) && --counter);

			if (counter == 0) {pid = NULL; console::formatter() << "iPod manager: Error - couldn't generate a new track pid.";}

			return pid;
		}
		void load_database_t::get_playlist_path(t_uint64 pid, pfc::string8 & p_out) const
		{
			pfc::list_t<pfc::string8> nodes;
			pfc::list_t< pfc::rcptr_t <t_playlist> > playlists = m_playlists; //keep our own copy to prevent recursion
			p_out.reset();

			while (pid)
			{
				for (t_size i = 0, count = playlists.get_count(); i<count; i++)
				{
					if (playlists[i]->id == pid)
					{
						nodes.insert_item(playlists[i]->name, 0);
						pid = playlists[i]->parentid;
						playlists.remove_by_idx(i);
						break;
					}
				}
			}
			for (t_size i = 0, count = nodes.get_count(); i<count; i++)
			{
				p_out << nodes[i]; 
				if (i+1 < count) p_out << "/";
			}
		}
		void load_database_t::glue_items (t_size start)
		{
			mmh::permutation_t permutation, permutationdbid;
			permutation.set_size(start);
			permutationdbid.set_size(start);

			t_size i;

			mmh::g_sort_get_permutation_qsort(m_tracks, permutation, g_compare_track_id, false);
			mmh::g_sort_get_permutation_qsort(m_tracks, permutationdbid, g_compare_track_dbid, false);

			//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, pfc::array_t<t_size> > sorted_array(m_library.m_tracks, permutation);

			t_size new_count = m_tracks.get_count();
			t_uint32 id_base = start ? m_tracks[permutation[start-1]]->id : 1;
			t_uint64 dbid_base = 0;
			if (start) dbid_base = m_tracks[permutation[start-1]]->pid;
			//else if (track_dbid_base) dbid_base = track_dbid_base;
			else
			{
				service_ptr_t<genrand_service> p_rand = genrand_service::g_create();
				p_rand->seed(GetTickCount());
				t_uint64 p1 = p_rand->genrand(MAXLONG);
				t_uint64 p2 = p_rand->genrand(MAXLONG);
				dbid_base = p1|(p2<<32);
				//track_dbid_base = dbid_base;
			}

			t_filetimestamp ft;

			GetSystemTimeAsFileTime((LPFILETIME)&ft);

			t_uint32 appletime = apple_time_from_filetime(ft);

			for (i=start; i<new_count; i++)
			{
				if (id_base == pfc_infinite)
					throw pfc::exception("Gave up looking for suitable track id");
				if (dbid_base == 0xffffffffffffffff)
					throw pfc::exception("Gave up looking for suitable track dbid");
				m_tracks[i]->dateadded = appletime;
				m_tracks[i]->id = ++id_base;
				m_tracks[i]->pid = ++dbid_base;
				m_tracks[i]->dbid2 = dbid_base;
			}
		}

		void load_database_t::__update_folder_smart_playlists_recur(pfc::array_t<bool> & mask_processed, t_uint64 id)
		{
			t_size i, count = m_playlists.get_count();
			for (i=0; i<count; i++)
			{
				if (!mask_processed[i] && m_playlists[i]->parentid == id && (m_playlists[i]->smart_data_valid || m_playlists[i]->smart_rules_valid))
				{
					if (m_playlists[i]->folder_flag)
						__update_folder_smart_playlists_recur(mask_processed, m_playlists[i]->id);
					ipod::smart_playlist::generator_t(*this).run(*m_playlists[i]);
					mask_processed[i] = true;
				}
			}
		}
		void load_database_t::update_smart_playlists()
		{
			t_size i, count_playlists = m_playlists.get_count();
			pfc::array_t<bool> mask_processed;
			mask_processed.set_count(count_playlists);
			mask_processed.fill_null();
			for (i=0; i<count_playlists; i++)
			{
				if (!mask_processed[i] && (m_playlists[i]->smart_data_valid || m_playlists[i]->smart_rules_valid)) 
				{
					if(m_playlists[i]->folder_flag)
					{
						__update_folder_smart_playlists_recur(mask_processed, m_playlists[i]->id);
					}
					ipod::smart_playlist::generator_t(*this).run(*m_playlists[i]);
					mask_processed[i] = true;
				}
			}
		}
			void load_database_t::remove_playlist_voiceover_title(ipod_device_ptr_cref_t p_ipod, t_uint64 pid)
			{
				if (p_ipod->m_device_properties.m_Speakable)
				{
					pfc::string8 speakablePlaylists;
					p_ipod->get_database_path(speakablePlaylists);
					speakablePlaylists  << p_ipod->get_path_separator_ptr() << "Speakable" << p_ipod->get_path_separator_ptr() << "Playlists" << p_ipod->get_path_separator_ptr();
					try 
					{
						if (p_ipod->m_device_properties.m_Speakable)
						{
							filesystem::g_remove(pfc::string8() << speakablePlaylists << pfc::format_hex(pid, 16) << ".wav", abort_callback_dummy());
						}
					}
					catch (exception_io_not_found const &) {};
				}
			}
			void load_database_t::add_system_voiceover_messages(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings)
			{
				if (p_ipod->m_device_properties.m_ShadowDBVersion == 2 && p_ipod->m_device_properties.m_Speakable)
				{
					try 
					{
						abort_callback_dummy p_abort;
						pfc::string8 playlistsVoicePath;
						p_ipod->get_database_path(playlistsVoicePath);
						playlistsVoicePath << p_ipod->get_path_separator_ptr() << "Speakable";
						try { filesystem::g_create_directory(playlistsVoicePath, p_abort); } catch (exception_io_already_exists const &) {};

						PlistParserFromFile messages(pfc::string8() << playlistsVoicePath << p_ipod->get_path_separator_ptr() << "en-US.plist", abort_callback_dummy());

						playlistsVoicePath << p_ipod->get_path_separator_ptr() << "Messages";
						try { filesystem::g_create_directory(playlistsVoicePath, p_abort); } catch (exception_io_already_exists const &) {};
						playlistsVoicePath << p_ipod->get_path_separator_ptr();

						cfobject::object_t::ptr_t uiStrings;
						if (messages.m_root_object.is_valid() && messages.m_root_object->m_dictionary.get_child(L"UI Strings", uiStrings))
						{
							for (t_size i=0,count=uiStrings->m_dictionary.get_count(); i<count; i++)
							{
								if (uiStrings->m_dictionary[i].m_key.is_valid() && uiStrings->m_dictionary[i].m_value.is_valid())
								{
									pfc::string8 path = playlistsVoicePath;
									path << pfc::stringcvt::string_utf8_from_wide(uiStrings->m_dictionary[i].m_key->m_string) << ".wav";
									if (!filesystem::g_exists(path, p_abort))
									{
										coinitialise_scope coinit(COINIT_MULTITHREADED);
										sapi pSAPI;
										if (pSAPI.is_valid())
										{
											drive_space_info_t spaceinfo;
											p_ipod->get_capacity_information(spaceinfo);
											if ((t_sfilesize)spaceinfo.m_freespace <= ((t_sfilesize)p_mappings.reserved_diskspace * (t_sfilesize)spaceinfo.m_capacity) / 1000)
												throw pfc::exception(pfc::string8 () << "Reserved disk space limit exceeded (" << "Capacity: " << spaceinfo.m_capacity << "; Free: " << spaceinfo.m_freespace << "; Reserved 0.1%s: " << p_mappings.reserved_diskspace << ")");
											pSAPI.run_mapped(pfc::stringcvt::string_utf8_from_wide(uiStrings->m_dictionary[i].m_value->m_string), p_ipod->m_device_properties.m_SpeakableSampleRate, path);
										}
									}
								}
							}
						}
					}
					catch (pfc::exception const & ex) 
					{
						console::formatter() << "iPod manager: Failed to generate VoiceOver sound for system messages: " << ex.what();
					}
				}
			}
			void load_database_t::add_playlist_voiceover_title(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings, t_uint64 pid, const char * title, bool b_check_if_exists)
			{
				if (p_ipod->m_device_properties.m_ShadowDBVersion == 2 && p_ipod->m_device_properties.m_Speakable)
				{
					abort_callback_dummy p_abort;
					pfc::string8 playlistsVoicePath;
					p_ipod->get_database_path(playlistsVoicePath);
					playlistsVoicePath << p_ipod->get_path_separator_ptr() << "Speakable";
					try { filesystem::g_create_directory(playlistsVoicePath, p_abort); } catch (exception_io_already_exists const &) {};
					playlistsVoicePath << p_ipod->get_path_separator_ptr() << "Playlists";
					try { filesystem::g_create_directory(playlistsVoicePath, p_abort); } catch (exception_io_already_exists const &) {};
					playlistsVoicePath << p_ipod->get_path_separator_ptr();

					pfc::string8 path, text = title;
					path << playlistsVoicePath << pfc::format_hex(pid, 16) << ".wav";
					if (text.is_empty()) text = "Unknown";

					try 
					{
						if (!b_check_if_exists || !filesystem::g_exists(path, abort_callback_dummy()))
						{
							coinitialise_scope coinit(COINIT_MULTITHREADED);
							sapi pSAPI;
							if (pSAPI.is_valid())
							{


								drive_space_info_t spaceinfo;
								p_ipod->get_capacity_information(spaceinfo);
								if ((t_sfilesize)spaceinfo.m_freespace <= ((t_sfilesize)p_mappings.reserved_diskspace * (t_sfilesize)spaceinfo.m_capacity) / 1000)
									throw pfc::exception(pfc::string8 () << "Reserved disk space limit exceeded (" << "Capacity: " << spaceinfo.m_capacity << "; Free: " << spaceinfo.m_freespace << "; Reserved 0.1%s: " << p_mappings.reserved_diskspace << ")");
								pSAPI.run_mapped(text, p_ipod->m_device_properties.m_SpeakableSampleRate, path);
							}
						}
					}
					catch (pfc::exception const & ex) 
					{
						console::formatter() << "iPod manager: Failed to generate VoiceOver sound for playlist: " << ex.what();
					}
				}
			}


	}
}