#include "stdafx.h"

#include "ipod_manager.h"
#include "writer.h"
#include "writer_sort_helpers.h"
#include "zlib.h"

namespace ipod
{

namespace tasks
{

void database_writer_t::write_itunesdb(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & m_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	const bool b_numbers_last = p_mappings.numbers_last;

	//string_print_drive m_path(p_ipod->drive);
	pfc::string8 base;
	p_ipod->get_database_path(base);
	pfc::string8 newpath;
	bool b_opened = false;

	pfc::string8 path;
	pfc::string8 backup_path;

	try
	{
		bool sixg = (p_ipod->is_6g_format());
		bool sixg_format = sixg;
		t_size dbversion = p_ipod->m_device_properties.m_db_version;
		t_uint8 encoding = m_library.encoding; //FIXME
		t_uint8 format = m_library.format;
		bool sqlite_db = p_ipod->m_device_properties.m_SQLiteDB;

		bool compressed = format == 2 && encoding == 1;
		bool sign_2x = dbversion >= 4;

		path = base;
		path << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr();
		pfc::string8 path_cdb=path, path_db=path;
		path_cdb << "iTunesCDB";
		path_db << "iTunesDB";
		path << (compressed ? "iTunesCDB" : "iTunesDB");

		newpath << path << ".dop.temp";

		//static_api_ptr_t<metadb> metadb_api;
		//in_metadb_sync metadb_lock;
		//tlhm
		stream_writer_mem dbhm;
		//stream_writer_mem db;

		dbhm.write_lendian_t(t_uint8(format), p_abort);
		dbhm.write_lendian_t(t_uint8(m_library.unk0[0]), p_abort);
		dbhm.write_lendian_t(t_uint8(m_library.unk0[1]), p_abort);
		dbhm.write_lendian_t(t_uint8(m_library.unk0[2]), p_abort);
		dbhm.write_lendian_t(t_uint32(55), p_abort); //16 //itunes 10.1
		t_uint32 dshm_count = sixg_format ? 4 : 3;
		if (dbversion >= 4 || sqlite_db) dshm_count++;
		if (dbversion >= 4) dshm_count++;
		if (dbversion >= 4) dshm_count++;
		if (m_library.m_special_playlists_valid) dshm_count++;
		if (m_library.m_genius_cuid_valid) dshm_count++;
		dbhm.write_lendian_t(t_uint32(dshm_count), p_abort); //20 //dshm count
		dbhm.write_lendian_t(t_uint64(0/*m_library.dbid*/), p_abort); //24
		dbhm.write_lendian_t(t_uint32(m_library.unk1), p_abort); //32
		dbhm.write_lendian_t(t_uint64(m_library.unk1_1), p_abort); //36 //m_library.track_dbid_base
		dbhm.write_lendian_t(t_uint32(m_library.unk2), p_abort); //44
		t_uint16 candy_version = 0;

		if (dbversion >= 5)
			candy_version = 3;
		else if (dbversion == 4)
			candy_version = 2;
		else if (dbversion == 3 || sixg)
			candy_version = 1;
		else
			candy_version = 0;

		dbhm.write_lendian_t(t_uint16 (candy_version), p_abort);//48 //sixg ? 0x1 : 0
		//char tempbuff[20];
		memset (m_library.hash1, 0, sizeof(m_library.hash1));
		dbhm.write(m_library.hash1, sizeof(m_library.hash1), p_abort); //50
		dbhm.write_lendian_t(m_library.unk4, p_abort); //70
		dbhm.write_lendian_t(m_library.pid, p_abort);
		dbhm.write_lendian_t(m_library.unk6, p_abort);
		dbhm.write_lendian_t(m_library.unk7, p_abort);

		memset (m_library.hash2, 0, sizeof(m_library.hash2));
		dbhm.write(m_library.hash2, sizeof(m_library.hash2), p_abort);
		dbhm.write_lendian_t(m_library.time_zone_offset_seconds, p_abort);
		//dbhm.write_lendian_t(m_library.unk9, p_abort);
		t_uint16 sign_flags = 0;
		if (candy_version)
		{
			if (candy_version == 3) sign_flags |= ipod::candy_flag_version_3;
			if (candy_version <= 2) sign_flags |= ipod::candy_flag_version_2;
			if (candy_version <= 1) sign_flags |= ipod::candy_flag_version_1;
		}

		dbhm.write_lendian_t(t_uint16 (sign_flags)/*m_library.unk3*/, p_abort);//bit-flags!
		//char tempbuff2[46];
		memset (m_library.hash3, 0, sizeof(m_library.hash3));
		dbhm.write(m_library.hash3, sizeof(m_library.hash3), p_abort);
		dbhm.write_lendian_t(m_library.audio_language, p_abort);
		dbhm.write_lendian_t(m_library.subtitle_language, p_abort);
		dbhm.write_lendian_t(m_library.unk11_1, p_abort);
		dbhm.write_lendian_t(m_library.unk11_2, p_abort);
		dbhm.write_lendian_t(encoding, p_abort);
		dbhm.write_lendian_t(m_library.unk12, p_abort);
		dbhm.write_lendian_t(m_library.unk13, p_abort);
		//dbhm.write(m_library.unk12, sizeof(m_library.unk12), p_abort);
		memset (m_library.hash4, 0, sizeof(m_library.hash4));
		dbhm.write(m_library.hash4, sizeof(m_library.hash4), p_abort);
		dbhm.write(m_library.unk14, sizeof(m_library.unk14), p_abort);


		/*{
			t_size k = 14;
			for (;k;k--) dbhm.write_lendian_t(t_uint32(0), p_abort);
		}*/

		//{

		stream_writer_mem ds4hm;
		stream_writer_mem ds4;

		if (sixg_format)
		{
			m_library.repopulate_albumlist(); //hack: need to read it it instead. update: now fixed

			ds4hm.write_lendian_t(t_uint32(dataset_albumlist), p_abort);

			{
				t_size k = 20;
				for (;k;k--) ds4hm.write_lendian_t(t_uint32(0), p_abort);
			}

			stream_writer_mem alhm;
			stream_writer_mem al;

			{
				t_size k = 20;
				for (;k;k--) alhm.write_lendian_t(t_uint32(0), p_abort);
			}

			writer p_al(al);
			writer p_ds4(ds4);

			t_size i, count = m_library.m_album_list.m_master_list.get_count();
			for (i=0; i<count; i++)
			{
				stream_writer_mem aihm;
				stream_writer_mem ai;
				writer p_ai(ai);

				t_uint32 count_do = 0;
				if (m_library.m_album_list.m_master_list[i]->album_valid)
				{
					p_ai.write_do_string(do_types::album_list_album, m_library.m_album_list.m_master_list[i]->album, p_abort);
					count_do++;
				}
				if (m_library.m_album_list.m_master_list[i]->artist_valid)
				{
					p_ai.write_do_string(do_types::album_list_artist, m_library.m_album_list.m_master_list[i]->artist, p_abort);
					count_do++;
				}
				if (m_library.m_album_list.m_master_list[i]->album_artist_strict_valid)
				{
					p_ai.write_do_string(do_types::album_list_album_artist, m_library.m_album_list.m_master_list[i]->album_artist_strict, p_abort);
					count_do++;
				}
				if (m_library.m_album_list.m_master_list[i]->show_valid)
				{
					p_ai.write_do_string(do_types::album_list_show, m_library.m_album_list.m_master_list[i]->show, p_abort);
					count_do++;
				}
				if (m_library.m_album_list.m_master_list[i]->podcast_url_valid)
				{
					p_ai.write_do_string(do_types::album_list_podcast_url, m_library.m_album_list.m_master_list[i]->podcast_url, p_abort);
					count_do++;
				}
				aihm.write_lendian_t(count_do, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->id, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->pid, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->kind, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->all_compilations, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->artwork_status, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->unk1, p_abort);

				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->artwork_item_pid, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->user_rating, p_abort);
				aihm.write_lendian_t(m_library.m_album_list.m_master_list[i]->season_number, p_abort);
				{
					t_size k = 10;
					for (;k;k--) aihm.write_lendian_t(t_uint32(0), p_abort);
				}
				p_al.write_section(identifiers::aihm, aihm.get_ptr(), aihm.get_size(), ai.get_ptr(), ai.get_size(), p_abort);
			}
			p_ds4.write_section(identifiers::alhm, alhm.get_ptr(), alhm.get_size(), al.get_ptr(), al.get_size(), count, p_abort);
		}

		stream_writer_mem ds8hm;
		stream_writer_mem ds8;

		bool b_ds8 = dbversion>=4 || p_ipod->m_device_properties.m_SQLiteDB;
		
		if (b_ds8)
		{
			ds8hm.write_lendian_t(t_uint32(dataset_artistlist), p_abort);

			{
				t_size k = 20;
				for (;k;k--) ds8hm.write_lendian_t(t_uint32(0), p_abort);
			}

			stream_writer_mem ilhm;
			stream_writer_mem il;

			{
				t_size k = 20;
				for (;k;k--) ilhm.write_lendian_t(t_uint32(0), p_abort);
			}

			writer p_il(il);
			writer p_ds8(ds8);

			t_size i, count = m_library.m_artist_list.get_count();
			for (i=0; i<count; i++)
			{
				stream_writer_mem iihm;
				stream_writer_mem ii;
				writer p_ii(ii);

				t_uint32 count_do = 0;
				if (m_library.m_artist_list[i]->artist_valid)
				{
					p_ii.write_do_string(do_types::artist_list_artist, m_library.m_artist_list[i]->artist, p_abort);
					count_do++;
				}
				if (m_library.m_artist_list[i]->sort_artist_valid)
				{
					p_ii.write_do_string(do_types::artist_list_sort_artist, m_library.m_artist_list[i]->sort_artist, p_abort);
					count_do++;
				}

				iihm.write_lendian_t(count_do, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->id, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->pid, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->type, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->artwork_status, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->unk1, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->unk2, p_abort);
				iihm.write_lendian_t(m_library.m_artist_list[i]->artwork_album_pid, p_abort);
				{
					t_size k = 10;
					for (;k;k--) iihm.write_lendian_t(t_uint32(0), p_abort);
				}
				p_il.write_section(identifiers::iihm, iihm.get_ptr(), iihm.get_size(), ii.get_ptr(), ii.get_size(), p_abort);
			}
			p_ds8.write_section(identifiers::ilhm, ilhm.get_ptr(), ilhm.get_size(), il.get_ptr(), il.get_size(), count, p_abort);
		}

		stream_writer_mem ds6hm, ds9hm;
		stream_writer_mem ds6;

		stream_writer_mem dsahm;
		stream_writer_mem dsa;

		{
			//writer(ds6).write_section(identifiers::tlhm, tlhm.get_ptr(), tlhm.get_size(), NULL, 0, 0, p_abort);
		}

		stream_writer_mem dshm;
		stream_writer_mem ds;

		dshm.write_lendian_t(t_uint32(dataset_tracklist), p_abort);
		ds6hm.write_lendian_t(t_uint32(dataset_tracklist2), p_abort);
		dsahm.write_lendian_t(t_uint32(dataset_tracklist3), p_abort);
		ds9hm.write_lendian_t(t_uint32(dataset_genius_cuid), p_abort);

		{
			t_size k = 20;
			for (;k;k--) dshm.write_lendian_t(t_uint32(0), p_abort);
		}

		{
			t_size k = 20;
			for (;k;k--) ds6hm.write_lendian_t(t_uint32(0), p_abort);
		}

		{
			t_size k = 20;
			for (;k;k--) dsahm.write_lendian_t(t_uint32(0), p_abort);
		}

		{
			t_size k = 20;
			for (;k;k--) ds9hm.write_lendian_t(t_uint32(0), p_abort);
		}

		stream_writer_mem tlhm;
		stream_writer_mem tl;

		stream_writer_mem tl6hm;
		stream_writer_mem tl6;

		stream_writer_mem tlahm;
		stream_writer_mem tla;

		{
			t_size k = 20;
			for (;k;k--) tlhm.write_lendian_t(t_uint32(0), p_abort);
		}

		{
			t_size k = 20;
			for (;k;k--) tl6hm.write_lendian_t(t_uint32(0), p_abort);
		}

		{
			t_size k = 20;
			for (;k;k--) tlahm.write_lendian_t(t_uint32(0), p_abort);
		}

		writer p_tl(tl);
		writer p_ds(ds);

		writer p_tl6(tl6);
		writer p_ds6(ds6);

		writer p_tla(tla);
		writer p_dsa(dsa);

		t_size i, count_tracks = m_library.m_tracks.get_count(), ti_counter=0, ti6_counter=0, tia_counter=0;

		//file_info_impl info_dummy;

		for (i=0; i<count_tracks; i++)
		{
			pfc::rcptr_t<t_track> track = m_library.m_tracks[i];
			metadb_handle_ptr handle = m_library.m_handles[i];

			stream_writer_mem tihm;
			stream_writer_mem ti;

			writer p_ti(ti, p_ipod->m_device_properties.m_db_version);

			t_uint32 count_do = 0;
			if (track->title_valid)
			{
				p_ti.write_do_string(do_types::title, track->title, p_abort);
				count_do++;
			}
			/*else
			{
				pfc::string_filename filename(handle->get_path());
				p_ti.write_do_string(do_types::title, filename, p_abort);
				count_do++;
			}*/

			if (track->artist_valid)
			{
				p_ti.write_do_string(do_types::artist, track->artist, p_abort);
				count_do++;
			}

#if 0
			if (track->unk_preceeding_the_valid)
			{
				p_ti.write_do_string(do_types::unk_preceeding_the, track->unk_preceeding_the, p_abort);
				count_do++;
			}
#endif

			if (track->album_artist_valid)
			{
				p_ti.write_do_string(do_types::album_artist, track->album_artist, p_abort);
				count_do++;
			}

			if (track->composer_valid)
			{
				p_ti.write_do_string(do_types::composer, track->composer, p_abort);
				count_do++;
			}

			if (track->album_valid)
			{
				p_ti.write_do_string(do_types::album, track->album, p_abort);
				count_do++;
			}

			if (track->grouping_valid)
			{
				p_ti.write_do_string(do_types::grouping, track->grouping, p_abort);
				count_do++;
			}
			if (track->genre_valid)
			{
				p_ti.write_do_string(do_types::genre, track->genre, p_abort);
				count_do++;
			}

			if (track->filetype_valid)
			{
				p_ti.write_do_string(do_types::filetype, track->filetype, p_abort);
				count_do++;
			}

			if (track->eq_settings_valid)
			{
				p_ti.write_do_string(do_types::eq_setting, track->eq_settings, p_abort);
				count_do++;
			}

			if (track->comment_valid)
			{
				p_ti.write_do_string(do_types::comment, track->comment, p_abort);
				count_do++;
			}
			if (track->category_valid)
			{
				p_ti.write_do_string(do_types::category, track->category, p_abort);
				count_do++;
			}

			{
				p_ti.write_do_string(do_types::location, track->location, p_abort);
				count_do++;
			}
			



			if (track->show_valid)
			{
				p_ti.write_do_string(do_types::show, track->show, p_abort);
				count_do++;
			}
			if (track->tv_network_valid)
			{
				p_ti.write_do_string(do_types::tv_network, track->tv_network, p_abort);
				count_do++;
			}

			if (track->episode_valid)
			{
				p_ti.write_do_string(do_types::episode_number, track->episode, p_abort);
				count_do++;
			}
			if (track->publication_id_valid)
			{
				p_ti.write_do_string(do_types::publication_id, track->publication_id, p_abort);
				count_do++;
			}

			if (track->sort_title_valid)
			{
				p_ti.write_do_string(do_types::sort_title, track->sort_title, p_abort);
				count_do++;
			}

			if (track->sort_album_valid)
			{
				p_ti.write_do_string(do_types::sort_album, track->sort_album, p_abort);
				count_do++;
			}

			if (track->sort_artist_valid)
			{
				p_ti.write_do_string(do_types::sort_artist, track->sort_artist, p_abort);
				count_do++;
			}

			if (track->sort_album_artist_valid)
			{
				p_ti.write_do_string(do_types::sort_album_artist, track->sort_album_artist, p_abort);
				count_do++;
			}

			if (track->sort_composer_valid)
			{
				p_ti.write_do_string(do_types::sort_composer, track->sort_composer, p_abort);
				count_do++;
			}

			if (track->sort_show_valid)
			{
				p_ti.write_do_string(do_types::sort_show, track->sort_show, p_abort);
				count_do++;
			}
			if (track->extended_content_rating_valid)
			{
				p_ti.write_do_string(do_types::extended_content_rating, track->extended_content_rating, p_abort);
				count_do++;
			}
			if (track->subtitle_valid)
			{
				p_ti.write_do_string(do_types::subtitle, track->subtitle, p_abort);
				count_do++;
			}
			if (track->description_valid)
			{
				p_ti.write_do_string(do_types::description, track->description, p_abort);
				count_do++;
			}
			if (track->collection_description_valid)
			{
				p_ti.write_do_string(do_types::collection_description, track->collection_description, p_abort);
				count_do++;
			}
			if (track->copyright_valid)
			{
				p_ti.write_do_string(do_types::copyright, track->copyright, p_abort);
				count_do++;
			}
			if (track->podcast_enclosure_url_valid)
			{
				p_ti.write_do_string_utf8(do_types::podcast_enclosure_url, track->podcast_enclosure_url, p_abort);
				count_do++;
			}
			if (track->podcast_rss_url_valid)
			{
				p_ti.write_do_string_utf8(do_types::podcast_rss_url, track->podcast_rss_url, p_abort);
				count_do++;
			}

			if (track->keywords_valid)
			{
				p_ti.write_do_string(do_types::keywords, track->keywords, p_abort);
				count_do++;
			}

			if (track->chapter_data_valid)
			{
				p_ti.write_do(do_types::chapter_data, 0, 0, track->do_chapter_data.get_ptr(),
					track->do_chapter_data.get_size(), p_abort);
#if 0
				p_ti.write_section(identifiers::dohm, track->dohm_chapter_data.get_ptr(),
					track->dohm_chapter_data.get_size(),
					track->do_chapter_data.get_ptr(),
					track->do_chapter_data.get_size(), p_abort);
#endif
				count_do++;
			}
			if (track->store_data_valid)
			{
				p_ti.write_do(do_types::store_data, 0, 0, track->do_store_data.get_ptr(),
					track->do_store_data.get_size(), p_abort);
				count_do++;
			}
			for (t_size n = 0, vccount = track->video_characteristics_entries.get_count(); n < vccount; n++)
			{
				stream_writer_mem vcd;
				itunesdb::t_video_characteristics & vc = track->video_characteristics_entries[n];

				writer p_vcd(vcd);
				p_vcd.write_lendian_auto_t(vc.width, p_abort);
				p_vcd.write_lendian_auto_t(vc.height, p_abort);
				p_vcd.write_lendian_auto_t(vc.width, p_abort);
				p_vcd.write_lendian_auto_t(vc.track_id, p_abort);
				p_vcd.write_lendian_auto_t(vc.codec, p_abort);
				p_vcd.write_lendian_auto_t(vc.percentage_encrypted, p_abort); //*1000
				p_vcd.write_lendian_auto_t(vc.bit_rate, p_abort);
				p_vcd.write_lendian_auto_t(vc.peak_bit_rate, p_abort);
				p_vcd.write_lendian_auto_t(vc.buffer_size, p_abort);
				p_vcd.write_lendian_auto_t(vc.profile, p_abort);
				p_vcd.write_lendian_auto_t(vc.level, p_abort);
				p_vcd.write_lendian_auto_t(vc.complexity_level, p_abort);
				p_vcd.write_lendian_auto_t(vc.frame_rate, p_abort); //*1000
				p_vcd.write_lendian_auto_t(vc.unk1, p_abort);
				vcd.write(&vc.unk2[0], sizeof(vc.unk2), p_abort);
				p_ti.write_do(do_types::video_characteristics, 0, 0, vcd.get_ptr(),
					vcd.get_size(), p_abort);
				count_do++;
			}

			tihm.write_lendian_t(count_do, p_abort);
			tihm.write_lendian_t(track->id, p_abort);
			tihm.write_lendian_t(track->location_type, p_abort);
			t_uint temp = 0;
			pfc::string_extension ext(handle->get_path());
			string_upper extupper(ext);
			{
				t_size extlen = extupper.length();
				for (t_size n = 0; n < 4; n++)
				{
					temp |= (n<extlen ? extupper[n] : ' ')<<(24-n*8);
					//temp <<= 8;
				}
				//if (extupper.length() == 3)
					//temp = extupper[0]<<24 | extupper[1] <<16 | extupper[2] << 8 | ' ';
			}
			tihm.write_lendian_t(temp, p_abort);
			tihm.write_lendian_t(t_uint8(track->type1), p_abort);
			tihm.write_lendian_t(t_uint8(track->type2), p_abort);
			tihm.write_lendian_t(t_uint8(track->compilation), p_abort);
			tihm.write_lendian_t(t_uint8(track->rating), p_abort);
			tihm.write_lendian_t(t_uint32(track->lastmodifiedtime), p_abort);

			tihm.write_lendian_t(t_uint32(track->file_size_32), p_abort);
			t_uint32 length = t_uint32(track->length);
			tihm.write_lendian_t(length, p_abort);
			tihm.write_lendian_t(t_uint32(track->tracknumber), p_abort);
			tihm.write_lendian_t(t_uint32(track->totaltracks), p_abort);
			tihm.write_lendian_t(t_uint32(track->year), p_abort);

			t_uint32 samplerate = track->samplerate;

			tihm.write_lendian_t(t_uint32(track->bitrate), p_abort);
			tihm.write_lendian_t(t_uint16(track->unk8), p_abort);
			tihm.write_lendian_t(t_uint16(track->samplerate), p_abort);
			tihm.write_lendian_t(t_uint32(track->volume), p_abort);
			tihm.write_lendian_t(t_uint32(track->starttime), p_abort);
			tihm.write_lendian_t(t_uint32(track->stoptime), p_abort);
			tihm.write_lendian_t(t_uint32(track->volume_normalisation_energy), p_abort);
			tihm.write_lendian_t(t_uint32(track->play_count_user), p_abort);
			tihm.write_lendian_t(t_uint32(track->play_count_recent), p_abort);
			tihm.write_lendian_t(t_uint32(track->lastplayedtime), p_abort);
			tihm.write_lendian_t(t_uint32(track->discnumber), p_abort);
			tihm.write_lendian_t(t_uint32(track->totaldiscs), p_abort);
			tihm.write_lendian_t(t_uint32(track->userid), p_abort);
			tihm.write_lendian_t(t_uint32(track->dateadded), p_abort);
			tihm.write_lendian_t(t_uint32(track->bookmarktime), p_abort);

			tihm.write_lendian_t(t_uint64(track->pid), p_abort);
			tihm.write_lendian_t(t_uint8(track->checked), p_abort);
			tihm.write_lendian_t(t_uint8(track->application_rating), p_abort);
			tihm.write_lendian_t(t_uint16(track->bpm), p_abort);
			tihm.write_lendian_t(t_uint16(track->artwork_count), p_abort);
			tihm.write_lendian_t(t_uint16(track->unk9), p_abort);
			tihm.write_lendian_t(t_uint32(track->artwork_size), p_abort);
			tihm.write_lendian_t(t_uint32(track->unk11), p_abort);
			tihm.write_lendian_t(float(samplerate), p_abort);
			tihm.write_lendian_t(t_uint32(track->datereleased), p_abort);
			tihm.write_lendian_t(t_uint16(track->audio_format), p_abort);
			tihm.write_lendian_t(t_uint8(track->content_rating), p_abort);
			tihm.write_lendian_t(t_uint8(track->unk12), p_abort);
			tihm.write_lendian_t(t_uint32(track->store_key_versions), p_abort);

			tihm.write_lendian_t(track->skip_count_user, p_abort);
			tihm.write_lendian_t(track->skip_count_recent, p_abort);
			tihm.write_lendian_t(track->last_skipped, p_abort);

			tihm.write_lendian_t(track->artwork_flag, p_abort);
			tihm.write_lendian_t(track->skip_on_shuffle, p_abort);
			tihm.write_lendian_t(track->remember_playback_position, p_abort);
			tihm.write_lendian_t(track->podcast_flag, p_abort);

			tihm.write_lendian_t(track->dbid2, p_abort);
			tihm.write_lendian_t(track->lyrics_flag, p_abort);
			tihm.write_lendian_t(track->video_flag, p_abort);
			tihm.write_lendian_t(track->played_marker, p_abort);

			tihm.write_lendian_t(track->unk37, p_abort);
			tihm.write_lendian_t(track->bookmark_time_ms_common, p_abort);
			tihm.write_lendian_t(track->gapless_encoding_delay, p_abort);
			tihm.write_lendian_t(track->samplecount, p_abort);

			//tihm.write_lendian_t(track->unk24, p_abort);
			tihm.write_lendian_t(track->lyrics_checksum, p_abort);
			tihm.write_lendian_t(track->gapless_encoding_drain, p_abort);
			tihm.write_lendian_t(track->gapless_heuristic_info, p_abort);

			tihm.write_lendian_t(track->media_type, p_abort);
			tihm.write_lendian_t(track->season_number, p_abort);
			tihm.write_lendian_t(track->episode_sort_id, p_abort);
			tihm.write_lendian_t(track->date_purchased, p_abort);
			tihm.write_lendian_t(track->legacy_store_item_id, p_abort);
			tihm.write_lendian_t(track->legacy_store_genre_id, p_abort);
			tihm.write_lendian_t(track->legacy_store_artist_id, p_abort);
			tihm.write_lendian_t(track->legacy_store_composer_id, p_abort);
			tihm.write_lendian_t(track->legacy_store_playlist_id, p_abort);

			tihm.write_lendian_t(track->legacy_store_storefront_id, p_abort);
			tihm.write_lendian_t(track->gapless_last_frame_resync, p_abort);
			tihm.write_lendian_t(track->unk43_1, p_abort);
			tihm.write_lendian_t(track->gapless_album, p_abort);
			tihm.write_lendian_t(track->unk43_2, p_abort);
			tihm.write(track->unk44, sizeof(track->unk44), p_abort);
			//tihm.write_lendian_t(track->unk45, p_abort);
			//tihm.write_lendian_t(track->unk46, p_abort);
			//tihm.write_lendian_t(track->unk47, p_abort);
			//tihm.write_lendian_t(track->unk48, p_abort);
			tihm.write_lendian_t(track->unk49, p_abort);
			tihm.write_lendian_t(track->unk50, p_abort);
			tihm.write_lendian_t(track->album_id, p_abort); //288
			tihm.write_lendian_t(track->unk52, p_abort);
			tihm.write_lendian_t(track->unk53, p_abort);
			tihm.write_lendian_t(track->file_size_64, p_abort);
			tihm.write_lendian_t(track->unk56, p_abort);
			tihm.write_lendian_t(track->unk57, p_abort);
			tihm.write_lendian_t(track->unk58, p_abort);
			tihm.write_lendian_t(track->unk59, p_abort);
			tihm.write_lendian_t(track->legacy_key_id, p_abort);
			tihm.write_lendian_t(track->is_self_contained, p_abort);
			tihm.write_lendian_t(track->is_compressed, p_abort);
			tihm.write_lendian_t(track->unk61_1, p_abort);
			tihm.write_lendian_t(track->analysis_inhibit_flags, p_abort);
			tihm.write_lendian_t(track->unk62, p_abort);
			tihm.write_lendian_t(track->unk63, p_abort);
			tihm.write_lendian_t(track->unk64, p_abort);
			tihm.write_lendian_t(track->audio_fingerprint, p_abort);
			tihm.write_lendian_t(track->unk66, p_abort);
			tihm.write_lendian_t(track->mhii_id, p_abort); //352
			tihm.write_lendian_t(track->unk68, p_abort); //356
			tihm.write_lendian_t(track->unk69_1, p_abort); //360
			tihm.write_lendian_t(track->unk69_2, p_abort); //361
			tihm.write_lendian_t(track->unk69_3, p_abort); //362
			tihm.write_lendian_t(track->is_anamorphic, p_abort); //363
			tihm.write_lendian_t(track->unk70, p_abort); //364
			tihm.write_lendian_t(track->is_demo, p_abort); //368
			tihm.write_lendian_t(track->unk71_2, p_abort); //369
			tihm.write_lendian_t(track->has_alternate_audio, p_abort); //370
			tihm.write_lendian_t(track->has_subtitles, p_abort); //371
			tihm.write_lendian_t(track->audio_language, p_abort); //372
			tihm.write_lendian_t(track->audio_track_index, p_abort); //374
			tihm.write_lendian_t(track->audio_track_id, p_abort); //376
			tihm.write_lendian_t(track->subtitle_language, p_abort); //380
			tihm.write_lendian_t(track->subtitle_track_index, p_abort); //382
			tihm.write_lendian_t(track->subtitle_track_id, p_abort); //384
			if (sixg_format)
			{
				tihm.write_lendian_t(track->rental_playback_date_started, p_abort); //388
				tihm.write_lendian_t(track->rental_playback_duration, p_abort); //392
				tihm.write_lendian_t(track->rental_date_started, p_abort); //396
				tihm.write_lendian_t(track->rental_duration, p_abort); //400
				tihm.write_lendian_t(track->characteristics_valid, p_abort); //404
				tihm.write_lendian_t(track->unk80_1, p_abort); //405
				tihm.write_lendian_t(track->is_hd, p_abort); //406
				tihm.write_lendian_t(track->store_kind, p_abort); //407
				tihm.write_lendian_t(track->account_id_primary, p_abort); //408
				tihm.write_lendian_t(track->account_id_secondary, p_abort); //416
				tihm.write_lendian_t(track->key_id, p_abort); //424
				tihm.write_lendian_t(track->store_item_id, p_abort); //432
				tihm.write_lendian_t(track->store_genre_id, p_abort); //440
				tihm.write_lendian_t(track->store_artist_id, p_abort); //448
				tihm.write_lendian_t(track->store_composer_id, p_abort); //456
				tihm.write_lendian_t(track->store_playlist_id, p_abort); //464
				tihm.write_lendian_t(track->store_front_id, p_abort); //472
				tihm.write_lendian_t(track->artist_id, p_abort); //480
				tihm.write_lendian_t(track->genius_id, p_abort); //484
				tihm.write_lendian_t(track->key_platform_id, p_abort); //492
				tihm.write_lendian_t(track->unk100, p_abort); //496
				tihm.write_lendian_t(track->unk101, p_abort); //500
				tihm.write_lendian_t(track->media_type2, p_abort); //504
				tihm.write_lendian_t(track->unk102, p_abort); //508
				tihm.write_lendian_t(track->unk103, p_abort); //512
				tihm.write_lendian_t(track->key_id2, p_abort); //516
				tihm.write_lendian_t(track->channel_count, p_abort); //524
				tihm.write_lendian_t(track->unk104_2, p_abort); //526
				tihm.write_lendian_t(track->unk105, p_abort); //528
				tihm.write_lendian_t(track->unk106_1, p_abort); //532
				tihm.write_lendian_t(track->chosen_by_auto_fill, p_abort); //533
				tihm.write_lendian_t(track->unk106_3, p_abort); //534
				tihm.write_lendian_t(track->unk106_4, p_abort); //535
				tihm.write_lendian_t(track->unk107, p_abort); //536
				tihm.write_lendian_t(track->unk108, p_abort); //540
				tihm.write_lendian_t(track->unk109, p_abort); //544
				tihm.write_lendian_t(track->unk110, p_abort); //548
				tihm.write_lendian_t(track->unk111, p_abort); //552
				tihm.write_lendian_t(track->unk112, p_abort); //556
				tihm.write_lendian_t(track->unk113, p_abort); //560
				tihm.write_lendian_t(track->unk114, p_abort); //564
				tihm.write_lendian_t(track->unk115, p_abort); //568
				tihm.write_lendian_t(track->unk116, p_abort); //572
				tihm.write_lendian_t(track->unk117, p_abort); //576
				tihm.write_lendian_t(track->unk118, p_abort); //580
				tihm.write_lendian_t(track->unk119, p_abort); //584
				tihm.write_lendian_t(track->unk120, p_abort); //588
				tihm.write_lendian_t(track->unk121, p_abort); //592
				tihm.write_lendian_t(track->unk122, p_abort); //596
				tihm.write_lendian_t(track->unk123, p_abort); //600
				tihm.write_lendian_t(track->unk124, p_abort); //604
				tihm.write_lendian_t(track->unk125, p_abort); //608
				tihm.write_lendian_t(track->unk126, p_abort); //612
				tihm.write_lendian_t(track->unk127, p_abort); //616
				tihm.write_lendian_t(track->unk128, p_abort); //620
			}

			if (track->dshm_type_6)
			{
				p_tl6.write_section(identifiers::tihm, tihm.get_ptr(), tihm.get_size(), ti.get_ptr(), ti.get_size(), p_abort);
				ti6_counter++;
			}
			else
			{
				p_tl.write_section(identifiers::tihm, tihm.get_ptr(), tihm.get_size(), ti.get_ptr(), ti.get_size(), p_abort);
				ti_counter++;
			}
		}
		p_ds.write_section(identifiers::tlhm, tlhm.get_ptr(), tlhm.get_size(), tl.get_ptr(), tl.get_size(), ti_counter, p_abort);
		p_ds6.write_section(identifiers::tlhm, tl6hm.get_ptr(), tl6hm.get_size(), tl6.get_ptr(), tl6.get_size(), ti6_counter, p_abort);
		p_dsa.write_section(identifiers::tlhm, tlahm.get_ptr(), tlahm.get_size(), tla.get_ptr(), tla.get_size(), tia_counter, p_abort);

		//}
		stream_writer_mem ds2hm, ds3hm, ds5hm;
		stream_writer_mem ds2, ds3, ds5;

		ds2hm.write_lendian_t(t_uint32(dataset_playlistlist), p_abort);
		ds3hm.write_lendian_t(t_uint32(dataset_playlistlist_v2), p_abort);
		ds5hm.write_lendian_t(t_uint32(dataset_specialplaylists), p_abort);

		{
			t_size k = 20;
			for (;k;k--) 
			{
				ds2hm.write_lendian_t(t_uint32(0), p_abort);
				ds3hm.write_lendian_t(t_uint32(0), p_abort);
				ds5hm.write_lendian_t(t_uint32(0), p_abort);
			}
		}
		stream_writer_mem plhm2, plhm3, plhm5;
		stream_writer_mem pl2, pl3, pl5;

		{
			t_size k = 20;
			for (;k;k--) 
			{
				plhm3.write_lendian_t(t_uint32(0), p_abort);
				plhm2.write_lendian_t(t_uint32(0), p_abort);
				plhm5.write_lendian_t(t_uint32(0), p_abort);
			}
		}

		writer p_ds2(ds2), p_ds3(ds3), p_ds5(ds5);
		writer p_pl2(pl2), p_pl3(pl3), p_pl5(pl5);
		{

			pfc::list_t<t_sort_entry> sort_entries;

			sort_entries.set_count(count_tracks);

			for (i=0; i<count_tracks; i++)
			{
				sort_entries[i].set_from_track(m_use_ipod_sorting, *m_library.m_tracks[i]);
			}

			struct t_index_pattern
			{
				library_index_types::t_type type;
			};

			t_index_pattern library_indices[] = 
			{ 
				{library_index_types::title},
				{library_index_types::album_disc_tracknumber_title},
				{library_index_types::artist_album_disc_tracknumber_title},
				{library_index_types::genre_artist_album_disc_tracknumber_title},
				{library_index_types::composer_title},
				{library_index_types::show_episode_1},
				{library_index_types::show_episode_2},
				{library_index_types::episode},
				{library_index_types::artist_album_disc_tracknumber},
				{library_index_types::albumartist_artist_album_disc_tracknumber_title}
			};

			stream_writer_mem pyhm;
			stream_writer_mem py;

			t_uint32 count_do_library = 1;

			writer p_py(py);

			p_py.write_do_string(do_types::title, m_library.m_library_playlist->name, p_abort);

			if (m_library.m_library_playlist->column_data_valid)
			{
				p_py.write_section(identifiers::dohm, m_library.m_library_playlist->dohm_column_data.get_ptr(),
					m_library.m_library_playlist->dohm_column_data.get_size(),
					m_library.m_library_playlist->do_column_data.get_ptr(),
					m_library.m_library_playlist->do_column_data.get_size(), p_abort);
				count_do_library++;
			}
			if (m_library.m_library_playlist->itunes_data_102_valid)
			{
				p_py.write_do(do_types::itunes_data_102, 0,	0, m_library.m_library_playlist->do_itunes_data_102.get_ptr(),
					m_library.m_library_playlist->do_itunes_data_102.get_size(), p_abort);
				count_do_library++;
			}

			if (!p_ipod->m_device_properties.m_SQLiteDB && !p_ipod->m_device_properties.m_ShadowDB)
			{

				mmh::Permutation sort_permutations[tabsize(library_indices)];

				{
					t_size j;
					for (j=0; j<tabsize(library_indices); j++)
						sort_permutations[j].set_size(count_tracks);
				}

				for (i=0; i<tabsize(library_indices); i++)
				{
					//pfc::hires_timer timer;
					//timer.start();
					//profiler (indicies_run);
					stream_writer_mem do_index;

					do_index.write_lendian_t(t_uint32(library_indices[i].type), p_abort);
					do_index.write_lendian_t(count_tracks, p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
					do_index.write_lendian_t(t_uint32(0), p_abort);
				
					mmh::Permutation ptemp(count_tracks);

					//m_library.m_handles.sort_by_format_get_order(permutation.get_ptr(), library_indices[i].sort_pattern, NULL);
					
	//#pragma warning( disable : 4101 )

					t_size j;
					if (i==0)
					{
						//profiler (indicies_0);
						if (b_numbers_last)
							mmh::sort_get_permutation(sort_entries, sort_permutations[i], ipod_sort_helpers_t<true>::g_compare_title, false); 
						else
							mmh::sort_get_permutation(sort_entries, sort_permutations[i], ipod_sort_helpers_t<false>::g_compare_title, false); 
					}
					else if (i==1)
					{
						//profiler (indicies_1);
						if (b_numbers_last)
						{
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								ipod_sort_helpers_t<true>::g_compare_album_and_track,
								ipod_sort_helpers_t<true>::g_compare_title
								>,
								false); 
						}
						else 
						{
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								ipod_sort_helpers_t<false>::g_compare_album_and_track,
								ipod_sort_helpers_t<false>::g_compare_title
								>,
								false); 
						}
					}
					else if (i==2)
					{
						//profiler (indicies_2);
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
					}
					else if (i==3)
					{
						//profiler (indicies_3);
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_genre,
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_genre,
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
					}
					else if (i==4)
					{
						//profiler (indicies_4);
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_composer,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_composer,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>,
								false); 
						}
					}
					else if (i==5)
					{
						//profiler (indicies_5);
						//order_helper::g_fill(sort_permutations[i]);
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_show1,
								sorter.g_compare_seasonnumber,
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>,
								false); 
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_show1,
								sorter.g_compare_seasonnumber,
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>,
								false); 
						}
					}
					else if (i==6)
					{
						//				profiler (indicies_6);

						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_seasonnumber,
								sorter.g_compare_show1,
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>, false);
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_seasonnumber,
								sorter.g_compare_show1,
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>, false);
						}
					}
					else if (i==7)
					{
						//				profiler (indicies_7);
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>, false);
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_episodenumber,
								sorter.g_compare_episodeid,
								sorter.g_compare_title,
								sorter.g_compare_artist
								>, false);
						}
					}
					else if (i==8)
					{
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track2,
								sorter.g_compare_title
								>, false);
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track2,
								sorter.g_compare_title
								>, false);
						}
					}
					else if (i==9)
					{
						if (b_numbers_last)
						{
							ipod_sort_helpers_t<true> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_album_artist,
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>, false);
						}
						else
						{
							ipod_sort_helpers_t<false> sorter;
							mmh::sort_get_permutation(sort_entries, sort_permutations[i],
								t_sort_entry::g_compare_stack<
								sorter.g_compare_album_artist,
								sorter.g_compare_artist,
								sorter.g_compare_album_and_track,
								sorter.g_compare_title
								>, false);
						}
					}
	//#pragma warning( default : 4101 )

					do_index.write(sort_permutations[i].get_ptr(), count_tracks*sizeof(t_uint32), p_abort);

					p_py.write_do(do_types::library_index, 0, 0, do_index.get_ptr(), do_index.get_size(), p_abort);
					count_do_library++;

					//console::formatter() << "library index " << i << " generated in " << timer.query() << " seconds";

					if (sixg_format && t_sort_entry::g_need_letter_table(library_indices[i].type))
					{
						struct t_letter
						{
							t_uint32 character;
							t_uint32 index;
							t_uint32 count;
							t_letter(t_uint32 c, t_uint32 i, t_uint32 t) : character(c), index(i), count(t) {};
							t_letter() : character(0), index(0), count(0) {};

							static bool g_test_char_alpha(char c)
							{
								return (c > 'A' && c < 'Z') || (c > 'a' && c < 'z');
							}
						};
						stream_writer_mem do_letter;

						do_letter.write_lendian_t(t_uint32(library_indices[i].type), p_abort);

						pfc::list_t<t_letter, pfc::alloc_fast> data;
						j = 0;

						while (j < count_tracks)
						{
							t_size start = j, lastchar=ipod_sort_helpers_t<true>::g_get_first_character_upper(sort_entries[sort_permutations[i][j]].get_letter_string(library_indices[i].type));
							if (lastchar >= '0' && lastchar <= '9')
							{
								lastchar='0';
								while (j +1 < count_tracks 
									&& ipod_sort_helpers_t<true>::g_test_first_character_latin_number(sort_entries[sort_permutations[i][j+1]].get_letter_string(library_indices[i].type))
									) j++;
							}
							else
							{
								while (j +1 < count_tracks 
									&& lastchar == ipod_sort_helpers_t<true>::g_get_first_character_upper(sort_entries[sort_permutations[i][j+1]].get_letter_string(library_indices[i].type))
									) j++;
							}
							data.add_item(t_letter(lastchar, start, j-start+1));
							j++;
						}

	#if 0
						if (!b_numbers_last)
						{
							while (j < count_tracks && !t_letter::g_test_char_alpha( ipod_sort_helpers_t<b_numbers_last>::g_get_first_character_ascii_upper (sort_entries[sort_permutations[i][j]].get_letter_string(library_indices[i].type)))) j++;
							if (j)
								data.add_item(t_letter('0', 0, j));
						}
						
						char k;
						for (k='A'; k<='Z'; k++)
						{
							t_size start = j;
							while (j < count_tracks && k == ipod_sort_helpers_t<b_numbers_last>::g_get_first_character_ascii_upper(sort_entries[sort_permutations[i][j]].get_letter_string(library_indices[i].type))) j++;
							if (j>start)
								data.add_item(t_letter(k, start, j-start));

						}
						
						if (b_numbers_last)
						{
							if (j<count_tracks)
								data.add_item(t_letter('0', j, count_tracks-j));
						}
	#endif

						do_letter.write_lendian_t(data.get_count(), p_abort);
						do_letter.write_lendian_t(t_uint32(0), p_abort);
						do_letter.write_lendian_t(t_uint32(0), p_abort);
						do_letter.write(data.get_ptr(), data.get_count()*sizeof(t_letter), p_abort);

						p_py.write_do(do_types::library_index_letters, 0, 0, do_letter.get_ptr(), do_letter.get_size(), p_abort);
						count_do_library++;
					}

					p_status.update_progress_subpart_helper(i,15 + (sqlite_db ? 15 : 0) );
				}
			}

			//count_do_library+=tabsize (library_indices);

			pyhm.write_lendian_t(t_uint32(count_do_library), p_abort);
			pyhm.write_lendian_t(t_uint32(count_tracks), p_abort);
			pyhm.write_lendian_t(t_uint8(1), p_abort);
			pyhm.write_lendian_t(t_uint8(0), p_abort);
			pyhm.write_lendian_t(t_uint8(0), p_abort);
			pyhm.write_lendian_t(t_uint8(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint64(m_library.m_library_playlist->id), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint16(1), p_abort);
			pyhm.write_lendian_t(t_uint16(0), p_abort);
			pyhm.write_lendian_t(t_uint32(5), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(0), p_abort);
			pyhm.write_lendian_t(t_uint32(1), p_abort);

			{
				t_size k = 15;
				for (;k;k--) 
					pyhm.write_lendian_t(t_uint32(0), p_abort);
			}
			for (i=0; i<count_tracks; i++)
			{
				stream_writer_mem pihm;
				stream_writer_mem pi;
				pihm.write_lendian_t(t_uint32(1), p_abort);
				pihm.write_lendian_t(t_uint32(0), p_abort);
				pihm.write_lendian_t(t_uint32(0x27c7 + i), p_abort); //hrm
				pihm.write_lendian_t(t_uint32(m_library.m_tracks[i]->id), p_abort);
				pihm.write_lendian_t(t_uint32(m_library.m_tracks[i]->dateadded), p_abort);
				pihm.write_lendian_t(t_uint32(0), p_abort);
				pihm.write_lendian_t(t_uint32(0), p_abort);
				pihm.write_lendian_t(t_uint32(0), p_abort);
				pihm.write_lendian_t(t_uint64(m_library.m_tracks[i]->pid), p_abort);

				{
					t_size k = 6;
					for (;k;k--) 
						pihm.write_lendian_t(t_uint32(0), p_abort);
				}

				writer p_pi(pi);

				stream_writer_mem dohm_position;
				stream_writer_mem do_position;
				dohm_position.write_lendian_t(t_uint32(do_types::position), p_abort);
				dohm_position.write_lendian_t(t_uint32(0), p_abort);
				dohm_position.write_lendian_t(t_uint32(0), p_abort);
				do_position.write_lendian_t(t_uint32(i+1), p_abort);
				do_position.write_lendian_t(t_uint64(0), p_abort);
				do_position.write_lendian_t(t_uint64(0), p_abort);

				p_pi.write_section(identifiers::dohm, dohm_position.get_ptr(), dohm_position.get_size(), do_position.get_ptr()
					,do_position.get_size(), p_abort);
				p_py.write_section(identifiers::pihm, pihm.get_ptr(), pihm.get_size(), pi.get_ptr(),
					pi.get_size(), p_abort);
			}
			p_pl2.write_section(identifiers::pyhm, pyhm.get_ptr(), pyhm.get_size(), py.get_ptr(),
				py.get_size(), p_abort);
			p_pl3.write_section(identifiers::pyhm, pyhm.get_ptr(), pyhm.get_size(), py.get_ptr(),
				py.get_size(), p_abort);
		}
		t_size j, count_playlists = m_library.m_playlists.get_size();
		for (j=0; j<count_playlists; j++)
		{
			stream_writer_mem pyhm, pyhm2;
			stream_writer_mem py, py2;

			t_size count_items = m_library.m_playlists[j]->items.get_count();

			t_uint32 count_do = 1;

			writer p_py(py), p_py2(py2);

			bool py2_valid = m_library.m_playlists[j]->podcast_flag!=0;

			pfc::stringcvt::string_wide_from_utf8_t<pfc::alloc_fast_aggressive> str(m_library.m_playlists[j]->name.get_ptr());

			p_py.write_do_string(do_types::title, str.get_ptr(), str.length(), p_abort);

			if (m_library.m_playlists[j]->column_data_valid)
			{
				p_py.write_section(identifiers::dohm, m_library.m_playlists[j]->dohm_column_data.get_ptr(),
					m_library.m_playlists[j]->dohm_column_data.get_size(),
					m_library.m_playlists[j]->do_column_data.get_ptr(),
					m_library.m_playlists[j]->do_column_data.get_size(), p_abort);
				count_do++;
			}

			if (m_library.m_playlists[j]->itunes_data_102_valid)
			{
				p_py.write_do(do_types::itunes_data_102, 0, 0, m_library.m_playlists[j]->do_itunes_data_102.get_ptr(),
					m_library.m_playlists[j]->do_itunes_data_102.get_size(), p_abort);
				count_do++;
			}

			if (m_library.m_playlists[j]->smart_data_valid)
			{
				p_py.write_do_smart_playlist_data(m_library.m_playlists[j]->smart_playlist_data, p_abort);
				/*p_py.write_section(identifiers::dohm, m_library.m_playlists[j]->dohm_smart_data.get_ptr(),
					m_library.m_playlists[j]->dohm_smart_data.get_size(),
					m_library.m_playlists[j]->do_smart_data.get_ptr(),
					m_library.m_playlists[j]->do_smart_data.get_size(), p_abort);*/
				count_do++;
			}

			if (m_library.m_playlists[j]->smart_rules_valid)
			{
				p_py.write_do_smart_playlist_rules(m_library.m_playlists[j]->smart_playlist_rules, p_abort);
				/*p_py.write_section(identifiers::dohm, m_library.m_playlists[j]->dohm_smart_rules.get_ptr(),
					m_library.m_playlists[j]->dohm_smart_rules.get_size(),
					m_library.m_playlists[j]->do_smart_rules.get_ptr(),
					m_library.m_playlists[j]->do_smart_rules.get_size(), p_abort);*/
				count_do++;
			}

			if (py2_valid) py2.write(py.get_ptr(), py.get_size(), p_abort);

			t_uint32 count2=0;//, index=0;

			for (i=0; i<count_items; i++)
			{
				t_size item_do_count = 0;

				const t_playlist_entry & item = m_library.m_playlists[j]->items[i];
				bool pi2_valid = py2_valid && !item.is_podcast_group;
				
				if (item.is_podcast_group) //=> pi2_valid == false
				{
					if (item.podcast_title_valid)
						item_do_count++;
					if (item.podcast_sort_title_valid)
						item_do_count++;
				} else item_do_count++;

				stream_writer_mem pihm, pihm2;
				stream_writer_mem pi;
				pihm.write_lendian_t(t_uint32(item_do_count), p_abort);
				pihm.write_lendian_t(t_uint8(item.unk0), p_abort);
				pihm.write_lendian_t(t_uint8(item.is_podcast_group), p_abort);
				pihm.write_lendian_t(t_uint8(item.is_podcast_group_expanded), p_abort);
				pihm.write_lendian_t(t_uint8(item.podcast_group_name_flags), p_abort);
				pihm.write_lendian_t(t_uint32(item.group_id), p_abort);
				pihm.write_lendian_t(t_uint32(m_library.m_playlists[j]->items[i].track_id), p_abort);
				pihm.write_lendian_t(t_uint32(item.timestamp), p_abort);
				pihm.write_lendian_t(t_uint32(item.podcast_group), p_abort);
				pihm.write_lendian_t(t_uint32(item.unk1), p_abort);
				pihm.write_lendian_t(t_uint32(item.unk2), p_abort);
				pihm.write_lendian_t(t_uint64(item.item_pid), p_abort);
				{
					t_size k = 6;
					for (;k;k--) 
						pihm.write_lendian_t(t_uint32(0), p_abort);
				}

				if (pi2_valid)
				{
					pihm2.write_lendian_t(t_uint32(1), p_abort); //item_do_count
					pihm2.write_lendian_t(t_uint32(0), p_abort);
					pihm2.write_lendian_t(t_uint32(item.group_id), p_abort);
					pihm2.write_lendian_t(t_uint32(m_library.m_playlists[j]->items[i].track_id), p_abort);
					pihm2.write_lendian_t(t_uint32(item.timestamp), p_abort);
					pihm2.write_lendian_t(t_uint32(0), p_abort);
					pihm2.write_lendian_t(t_uint32(item.unk1), p_abort);
					pihm2.write_lendian_t(t_uint32(item.unk2), p_abort);
					pihm2.write_lendian_t(t_uint64(item.item_pid), p_abort);
					{
						t_size k = 6;
						for (;k;k--) 
							pihm2.write_lendian_t(t_uint32(0), p_abort);
					}
					count2++;
				}

				writer p_pi(pi);

				if (item.is_podcast_group)
				{
					if (item.podcast_title_valid)
						p_pi.write_do_string(do_types::playilst_item_podcast_title, item.podcast_title, p_abort);
					if (item.podcast_sort_title_valid)
						p_pi.write_do_string(do_types::playilst_item_podcast_sort_title, item.podcast_sort_title, p_abort);
				}
				else
				{
					stream_writer_mem dohm_position;
					stream_writer_mem do_position;
					dohm_position.write_lendian_t(t_uint32(do_types::position), p_abort);
					dohm_position.write_lendian_t(t_uint32(0), p_abort);
					dohm_position.write_lendian_t(t_uint32(0), p_abort);
					do_position.write_lendian_t(t_uint32(item.position_valid ? item.position : 0), p_abort);
					do_position.write_lendian_t(t_uint64(0), p_abort);
					do_position.write_lendian_t(t_uint64(0), p_abort);

					p_pi.write_section(identifiers::dohm, dohm_position.get_ptr(), dohm_position.get_size(), do_position.get_ptr()
						,do_position.get_size(), p_abort);
				}
				p_py.write_section(identifiers::pihm, pihm.get_ptr(), pihm.get_size(), pi.get_ptr(),
					pi.get_size(), p_abort);
				if (pi2_valid)
					p_py2.write_section(identifiers::pihm, pihm2.get_ptr(), pihm2.get_size(), pi.get_ptr(),
						pi.get_size(), p_abort);
			}

			{
				pyhm.write_lendian_t(t_uint32(count_do), p_abort);
				pyhm.write_lendian_t(t_uint32(count_items), p_abort);
				pyhm.write_lendian_t(t_uint8(0), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_playlists[j]->shuffle_items), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_playlists[j]->has_been_shuffled), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_playlists[j]->unk3), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_playlists[j]->timestamp), p_abort);
				pyhm.write_lendian_t(t_uint64(m_library.m_playlists[j]->id), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_playlists[j]->unk4), p_abort);
				pyhm.write_lendian_t(t_uint16(m_library.m_playlists[j]->repeat_mode), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_playlists[j]->podcast_flag), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_playlists[j]->folder_flag), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_playlists[j]->sort_order), p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->parentid, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->workout_template_id, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk6, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk7, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk8, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk9, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->album_field_order, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk11, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk12, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk13, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->sort_direction, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->unk14, p_abort);
				pyhm.write_lendian_t(m_library.m_playlists[j]->date_modified, p_abort);

				{
					t_size k = 6+6;
					for (;k;k--) 
						pyhm.write_lendian_t(t_uint32(0), p_abort);
				}
				if (py2_valid)
				{
					pyhm2.write_lendian_t(t_uint32(count_do), p_abort);
					pyhm2.write_lendian_t(t_uint32(count2), p_abort);
					pyhm2.write_lendian_t(t_uint8(0), p_abort);
					pyhm2.write_lendian_t(t_uint8(m_library.m_playlists[j]->shuffle_items), p_abort);
					pyhm2.write_lendian_t(t_uint8(m_library.m_playlists[j]->has_been_shuffled), p_abort);
					pyhm2.write_lendian_t(t_uint8(m_library.m_playlists[j]->unk3), p_abort);
					pyhm2.write_lendian_t(t_uint32(m_library.m_playlists[j]->timestamp), p_abort);
					pyhm2.write_lendian_t(t_uint64(m_library.m_playlists[j]->id), p_abort);
					pyhm2.write_lendian_t(t_uint32(m_library.m_playlists[j]->unk4), p_abort);
					pyhm2.write_lendian_t(t_uint16(1), p_abort);
					pyhm2.write_lendian_t(t_uint8(m_library.m_playlists[j]->podcast_flag), p_abort);
					pyhm2.write_lendian_t(t_uint8(0), p_abort);
					pyhm2.write_lendian_t(t_uint32(m_library.m_playlists[j]->sort_order), p_abort);
					{
						t_size k = 15;
						for (;k;k--) 
							pyhm2.write_lendian_t(t_uint32(0), p_abort);
					}
				}

			}


			if (py2_valid)
			{
				p_pl2.write_section(identifiers::pyhm, pyhm2.get_ptr(), pyhm2.get_size(), py2.get_ptr(),
					py2.get_size(), p_abort);
			}
			else
				p_pl2.write_section(identifiers::pyhm, pyhm.get_ptr(), pyhm.get_size(), py.get_ptr(),
					py.get_size(), p_abort);
			p_pl3.write_section(identifiers::pyhm, pyhm.get_ptr(), pyhm.get_size(), py.get_ptr(),
				py.get_size(), p_abort);

		}





		t_size count_special_playlists = m_library.m_special_playlists.get_size();
		for (j=0; j<count_special_playlists; j++)
		{
			stream_writer_mem pyhm;
			stream_writer_mem py;

			t_size count_items = 0;//m_library.count_special_playlists[j]->items.get_count();

			t_uint32 count_do = 1;

			writer p_py(py);

			pfc::stringcvt::string_wide_from_utf8_t<pfc::alloc_fast_aggressive> str(m_library.m_special_playlists[j]->name.get_ptr());

			p_py.write_do_string(do_types::title, str.get_ptr(), str.length(), p_abort);

			if (m_library.m_special_playlists[j]->column_data_valid)
			{
				p_py.write_section(identifiers::dohm, m_library.m_special_playlists[j]->dohm_column_data.get_ptr(),
					m_library.m_special_playlists[j]->dohm_column_data.get_size(),
					m_library.m_special_playlists[j]->do_column_data.get_ptr(),
					m_library.m_special_playlists[j]->do_column_data.get_size(), p_abort);
				count_do++;
			}

			if (m_library.m_special_playlists[j]->itunes_data_102_valid)
			{
				p_py.write_do(do_types::itunes_data_102, 0, 0, m_library.m_special_playlists[j]->do_itunes_data_102.get_ptr(),
					m_library.m_special_playlists[j]->do_itunes_data_102.get_size(), p_abort);
				count_do++;
			}

			if (m_library.m_special_playlists[j]->smart_data_valid)
			{
				p_py.write_do_smart_playlist_data(m_library.m_special_playlists[j]->smart_playlist_data, p_abort);
				count_do++;
			}

			if (m_library.m_special_playlists[j]->smart_rules_valid)
			{
				p_py.write_do_smart_playlist_rules(m_library.m_special_playlists[j]->smart_playlist_rules, p_abort);
				count_do++;
			}

			t_uint32 count2=0;//, index=0;

			{
				pyhm.write_lendian_t(t_uint32(count_do), p_abort);
				pyhm.write_lendian_t(t_uint32(count_items), p_abort);
				pyhm.write_lendian_t(t_uint8(0), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_special_playlists[j]->shuffle_items), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_special_playlists[j]->has_been_shuffled), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_special_playlists[j]->unk3), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_special_playlists[j]->timestamp), p_abort);
				pyhm.write_lendian_t(t_uint64(m_library.m_special_playlists[j]->id), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_special_playlists[j]->unk4), p_abort);
				pyhm.write_lendian_t(t_uint16(m_library.m_special_playlists[j]->repeat_mode), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_special_playlists[j]->podcast_flag), p_abort);
				pyhm.write_lendian_t(t_uint8(m_library.m_special_playlists[j]->folder_flag), p_abort);
				pyhm.write_lendian_t(t_uint32(m_library.m_special_playlists[j]->sort_order), p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->parentid, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->workout_template_id, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk6, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk7, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk8, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk9, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->album_field_order, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk11, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk12, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk13, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->sort_direction, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->unk14, p_abort);
				pyhm.write_lendian_t(m_library.m_special_playlists[j]->date_modified, p_abort);

				{
					t_size k = 6+6;
					for (;k;k--) 
						pyhm.write_lendian_t(t_uint32(0), p_abort);
				}
			}

			p_pl5.write_section(identifiers::pyhm, pyhm.get_ptr(), pyhm.get_size(), py.get_ptr(),
				py.get_size(), p_abort);

		}





		p_ds2.write_section(identifiers::plhm, plhm2.get_ptr(), plhm2.get_size(), pl2.get_ptr(),
			pl2.get_size(), count_playlists+1, p_abort);
		p_ds3.write_section(identifiers::plhm, plhm3.get_ptr(), plhm3.get_size(), pl3.get_ptr(),
			pl3.get_size(), count_playlists+1, p_abort);
		p_ds5.write_section(identifiers::plhm, plhm5.get_ptr(), plhm5.get_size(), pl5.get_ptr(),
			pl5.get_size(), count_special_playlists, p_abort);
		p_status.update_progress_subpart_helper(11,15 + (sqlite_db ? 15 : 0));

		stream_writer_mem db, db_header;
		const t_size header_size = dbhm.get_size() + 4*3;
		/*const t_size total_size_decompressed = header_size
			+ 4*3 + dshm.get_size() + ds.get_size() 
			+ 4*3 + ds2hm.get_size() + ds2.get_size()
			+ 4*3 + ds3hm.get_size() + ds3.get_size()
			+ (sixg_format ? 4*3 + ds4hm.get_size() + ds4.get_size() : 0)
			+ (b_ds8 ? 4*3 + ds8hm.get_size() + ds8.get_size() : 0)
			+ (dbversion == 4 ? 4*3 + ds6hm.get_size() + ds6.get_size() : 0);*/

		if (sixg_format)
		{
			db.write_bendian_t(identifiers::dshm, p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds4hm.get_size()), p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds4hm.get_size() + ds4.get_size()), p_abort);
			db.write(ds4hm.get_ptr(), ds4hm.get_size(), p_abort);
			db.write(ds4.get_ptr(), ds4.get_size(), p_abort);

			ds4hm.force_reset();
			ds4.force_reset();
		}

		if (b_ds8)
		{
			db.write_bendian_t(identifiers::dshm, p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds8hm.get_size()), p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds8hm.get_size() + ds8.get_size()), p_abort);
			db.write(ds8hm.get_ptr(), ds8hm.get_size(), p_abort);
			db.write(ds8.get_ptr(), ds8.get_size(), p_abort);

			ds8hm.force_reset();
			ds8.force_reset();
		}

		db.write_bendian_t(identifiers::dshm, p_abort);
		db.write_lendian_t(t_uint32(4*3 + dshm.get_size()), p_abort);
		db.write_lendian_t(t_uint32(4*3 + dshm.get_size() + ds.get_size()), p_abort);
		db.write(dshm.get_ptr(), dshm.get_size(), p_abort);
		db.write(ds.get_ptr(), ds.get_size(), p_abort);

		dshm.force_reset();
		ds.force_reset();

		if (dbversion >= 4)
		{
			if (dbversion >= 4)
			{
				db.write_bendian_t(identifiers::dshm, p_abort);
				db.write_lendian_t(t_uint32(4*3 + dsahm.get_size()), p_abort);
				db.write_lendian_t(t_uint32(4*3 + dsahm.get_size() + dsa.get_size()), p_abort);
				db.write(dsahm.get_ptr(), dsahm.get_size(), p_abort);
				db.write(dsa.get_ptr(), dsa.get_size(), p_abort);
				
				dsahm.force_reset();
				dsa.force_reset();
			}
			db.write_bendian_t(identifiers::dshm, p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds6hm.get_size()), p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds6hm.get_size() + ds6.get_size()), p_abort);
			db.write(ds6hm.get_ptr(), ds6hm.get_size(), p_abort);
			db.write(ds6.get_ptr(), ds6.get_size(), p_abort);
			
			ds6hm.force_reset();
			ds6.force_reset();
		}

		db.write_bendian_t(identifiers::dshm, p_abort);
		db.write_lendian_t(t_uint32(4*3 + ds3hm.get_size()), p_abort);
		db.write_lendian_t(t_uint32(4*3 + ds3hm.get_size() + ds3.get_size()), p_abort);
		db.write(ds3hm.get_ptr(), ds3hm.get_size(), p_abort);
		db.write(ds3.get_ptr(), ds3.get_size(), p_abort);

		ds3hm.force_reset();
		ds3.force_reset();

		db.write_bendian_t(identifiers::dshm, p_abort);
		db.write_lendian_t(t_uint32(4*3 + ds2hm.get_size()), p_abort);
		db.write_lendian_t(t_uint32(4*3 + ds2hm.get_size() + ds2.get_size()), p_abort);
		db.write(ds2hm.get_ptr(), ds2hm.get_size(), p_abort);
		db.write(ds2.get_ptr(), ds2.get_size(), p_abort);

		ds2hm.force_reset();
		ds2.force_reset();

		if (m_library.m_special_playlists_valid)
		{
			db.write_bendian_t(identifiers::dshm, p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds5hm.get_size()), p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds5hm.get_size() + ds5.get_size()), p_abort);
			db.write(ds5hm.get_ptr(), ds5hm.get_size(), p_abort);
			db.write(ds5.get_ptr(), ds5.get_size(), p_abort);

			ds5hm.force_reset();
			ds5.force_reset();
		}
		if (m_library.m_genius_cuid_valid)
		{
			db.write_bendian_t(identifiers::dshm, p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds9hm.get_size()), p_abort);
			db.write_lendian_t(t_uint32(4*3 + ds9hm.get_size() + m_library.m_genius_cuid.get_size()), p_abort);
			db.write(ds9hm.get_ptr(), ds9hm.get_size(), p_abort);
			db.write(m_library.m_genius_cuid.get_ptr(), m_library.m_genius_cuid.get_size(), p_abort);

			ds9hm.force_reset();
		}

		pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> compressed_data;
		if (compressed)
		{
			zlib_stream zs;
			zs.compress_singlerun(db.get_ptr(), db.get_size(), compressed_data);
		}

		db_header.write_bendian_t(identifiers::dbhm, p_abort);
		db_header.write_lendian_t(t_uint32(header_size), p_abort);
		db_header.write_lendian_t(t_uint32(header_size + (compressed ? compressed_data.get_size() : db.get_size())), p_abort);
		db_header.write(dbhm.get_ptr(), dbhm.get_size(), p_abort);
		if (compressed)
			db_header.write(compressed_data.get_ptr(), compressed_data.get_size(), p_abort);
		else
			db_header.write(db.get_ptr(), db.get_size(), p_abort);

		compressed_data.force_reset();
		db.force_reset();

		t_uint8 * ptr = db_header.get_ptr();
		if (sign_flags & ipod::candy_flag_version_3)
		{
			pfc::string8 struid;
			p_ipod->get_deviceuniqueid(struid);
			t_size udid_length = struid.get_length()/2;

			pfc::array_t<t_uint8> uid, hashAB;
			uid.set_size (udid_length);
			uid.fill_null();
			//t_size i;
			for (i=0; i<udid_length; i++)
			{
				//if (!struid[i*2] || !struid[i*2 + 1]) break;
				uid[i] = (unsigned char)mmh::strtoul64_n(&struid[i*2], 2, 0x10);
			}

			hashAB.set_size(57);
			hashAB.fill_null();

			unsigned char sha1digest[20];
			mmh::hash::sha1(db_header.get_ptr(), db_header.get_size(), sha1digest);

			if (m_iPhoneCalc.is_valid_gen())
				m_iPhoneCalc.xgen_hash_ab_gen(sha1digest, uid.get_ptr(), udid_length, hashAB.get_ptr());
			else if (m_iPhoneCalc.is_valid() && udid_length == 20)
				m_iPhoneCalc.xgen_hash_ab(sha1digest, uid.get_ptr(), hashAB.get_ptr());

			memcpy(ptr+0xAB, hashAB.get_ptr(), hashAB.get_size());
		}
		if (sign_flags & ipod::candy_flag_version_2)
		{
			//console::formatter() << "iPod manager: Writing iPhone 2.x firmware signature";
			pfc::string8 struid;
			pfc::array_t<t_uint8> uid, hash72;
			uid.set_size (20);
			uid.fill_null();
			p_ipod->get_deviceuniqueid(struid);
			//t_size i;
			for (i=0; i<20; i++)
			{
				if (!struid[i*2] || !struid[i*2 + 1]) break;
				uid[i] = (unsigned char)mmh::strtoul64_n(&struid[i*2], 2, 0x10);
			}

			hash72.set_size(46);
			hash72.fill_null();

			itunescrypt::cbk_generate_for_data(db_header.get_ptr(), db_header.get_size(), uid.get_ptr(), hash72.get_ptr());

			memcpy(ptr+0x72, hash72.get_ptr(), hash72.get_size());
		}
		if (sign_flags & ipod::candy_flag_version_1)
		{
			pfc::array_t<t_uint8> key, hash;
			pfc::string8 fwguid;
			p_ipod->get_firewireguid(fwguid);
			//console::formatter() << fwguid;
			if (fwguid.get_length() == 16)
			{
				t_uint64 fwid = mmh::strtoul64_n(fwguid.get_ptr(), 16, 0x10);
				byte_order::order_native_to_be_t(fwid);
				key.set_size(mmh::hash::sha1_digestsize);
				itunescrypt::hash58_generate_key((t_uint8*)&fwid, key.get_ptr());
				hash.set_size(mmh::hash::sha1_digestsize);
				mmh::hash::sha1_hmac(key.get_ptr(), key.get_size(), db_header.get_ptr(), db_header.get_size(), hash.get_ptr());

				memcpy(ptr+0x58, hash.get_ptr(), hash.get_size());
			}
			else throw pfc::exception(pfc::string8() << "Expected 16 character FireWireGUID. Got: " << fwguid);
		}

		t_uint64 dbid = m_library.dbid;
		byte_order::order_native_to_le_t(dbid);
		memcpy(ptr+0x18, &dbid, sizeof(dbid));

		backup_path << path.get_ptr() << ".dop.backup";
		try { filesystem::g_remove(backup_path, p_abort); } catch (exception_io_not_found const &) {};

		service_ptr_t<file> p_file;
		filesystem::g_open_write_new(p_file, newpath, p_abort);
		b_opened=true;

		p_file->write(db_header.get_ptr(), db_header.get_size(), p_abort);

		p_file.release();
		b_opened=false;

		if (filesystem::g_exists(path, p_abort))
			filesystem::g_move(path, backup_path, p_abort);
		filesystem::g_move(newpath, path, p_abort);

		if (compressed && sqlite_db)
		{
			try {
				filesystem::g_open_write_new(file::ptr(), path_db, p_abort);
			} catch (exception_io const &) {};
		}
#if 0//_DEBUG //FIXME TEST
		try
		{
			if (m_library.m_playcounts_plist.is_valid())
			{
				t_filestats fs;
				bool blah;
				filesystem::g_get_stats(path_db, fs, blah, p_abort);

				cfobject::object_t::ptr_t DBTimestampMasOSDate;
				if (m_library.m_playcounts_plist->m_dictionary.get_child(L"DBTimestampMasOSDate", DBTimestampMasOSDate))
					DBTimestampMasOSDate->m_integer = apple_time_from_filetime(fs.m_timestamp, false);

				abort_callback_impl p_dummy_abort;
				pfc::string8 playcounts_path, xml;
				p_ipod->get_database_path(playcounts_path);
				playcounts_path << "/iTunes/" << "PlayCounts.plist";

				cfobject::g_export_object_to_xml(m_library.m_playcounts_plist, xml);
				file::ptr p_playcounts_file;
				filesystem::g_open_write_new(p_playcounts_file, playcounts_path, p_dummy_abort);
				p_playcounts_file->write(xml.get_ptr(), xml.get_length(), p_dummy_abort);

				popup_message::g_show(xml, "blah");
			}
		}
		catch (pfc::exception const & ex)
		{
		};
#endif

		p_status.update_progress_subpart_helper(13,15 + (sqlite_db ? 15 : 0));
	}
	catch (const exception_aborted &) 
	{
		try
		{
			abort_callback_impl p_dummy_abort;
			if (b_opened)
				filesystem::g_remove(newpath, p_dummy_abort);
		}
		catch (const pfc::exception &) {}
		throw;
	}
	catch (const pfc::exception & ex)
	{
		try
		{
			abort_callback_impl p_dummy_abort;
			if (b_opened)
				filesystem::g_remove(newpath, p_dummy_abort);
		}
		catch (const pfc::exception &) {}
		throw pfc::exception(pfc::string_formatter() << "Error writing iTunesDB file : " << ex.what());
	}

}
}
}