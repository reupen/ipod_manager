#include "stdafx.h"

#include "ipod_manager.h"
#include "shadowdb.h"

t_int32 round_float_signed(double f)
{
	return t_int32 (f >= -0.5 ? f + 0.5 : f - 0.5f);
}

void ipod_write_shuffledb(ipod_device_ptr_ref_t p_ipod, const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	pfc::string8 newpath;
	bool b_opened = false;
	//string_print_drive m_path(p_ipod->drive);

	try
	{
		pfc::string8 path = m_path, path_shuffle, path_pstate, path_stats;
		pfc::string8 backup_path;
		path << "\\iTunes\\" << "iTunesSD";
		path_shuffle << m_path << "\\iTunes\\" << "iTunesShuffle";
		path_pstate << m_path << "\\iTunes\\" << "iTunesPState";
		path_stats << m_path << "\\iTunes\\" << "iTunesStats";

		newpath << path << ".temp";

		itunessd::writer header;
		t_size i, count = p_library.m_tracks.get_count();

		header.write_int(count, p_abort);
		header.write_int(0x010800, p_abort);
		header.write_int(0x12, p_abort);
		header.write_int(0, p_abort);
		header.write_int(0, p_abort);
		header.write_int(0, p_abort);

		for (i=0; i<count; i++)
		{
			itunessd::writer track;
			track.write_int(0x5aa501, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			int volume = 0;
			if (p_ipod->m_device_properties.m_SupportsSoundCheck)
			{
				unsigned sc = p_library.m_tracks[i]->soundcheck;
				int voli = p_library.m_tracks[i]->volume;
				double volf = (double)voli / 255.0 + 1.0;
				if (volf == 0.0)
				{
					volume = -144;
				}
				else
				{
					double scdb = sc ? 10.0*log10 (1000.0 / (double)sc) : 0; //apple limit to [-12,+12] dB .... but let's not :)
					double vol = 20.0 * log10(volf);
					volume = round_float_signed(vol + scdb);
					if (volume < -48)  volume = -144; //silencer ???
					else if (volume > 18) volume = 18;
				}
			}
			else
			{
				volume = 100 + round_float_signed(100.0 + (double)p_library.m_tracks[i]->volume/255.0);
				volume = min(volume, 200);
				volume = max(volume, 0);
			}
			track.write_int(volume, p_abort);

			t_uint32 type=0;
			pfc::string_extension ext(p_library.m_tracks[i]->location);
			if (!stricmp_utf8(ext, "mp3"))
				type = 0x1;
			else if (!stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a") || !stricmp_utf8(ext, "aa") || !stricmp_utf8(ext, "m4b"))
				type = 0x2;
			else if (!stricmp_utf8(ext, "wav"))
				type = 0x4;
			track.write_int(type, p_abort);

			bool audiobook = (!stricmp_utf8(ext, "aa") || !stricmp_utf8(ext, "m4b"));

			pfc::string8 temp(p_library.m_tracks[i]->location);
			temp.replace_byte(':', '/');
			pfc::array_t<WCHAR> buff;
			buff.set_size(256);
			buff.fill_null();
			pfc::stringcvt::convert_utf8_to_wide(buff.get_ptr(), buff.get_size(), temp, pfc_infinite);
			track.write_int(buff.get_size()*sizeof(WCHAR), p_abort);
			//track.write_string(buff.get_ptr(), buff.get_size(), p_abort);
			track.write(buff.get_ptr(), buff.get_size()*sizeof(WCHAR), p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			track.write_int(0, p_abort);
			t_uint8 dummy=0;
			track.write_bendian_t(dummy, p_abort);
			dummy=!p_library.m_tracks[i]->skip_on_shuffle;
			track.write_bendian_t(dummy, p_abort);
			dummy= (audiobook || p_library.m_tracks[i]->remember_playback_position)?1:0;
			track.write_bendian_t(dummy, p_abort);
			dummy=0;
			track.write_bendian_t(dummy, p_abort);

			header.write_int(track.get_size()+3, p_abort);
			header.write(track.get_ptr(), track.get_size(), p_abort);
		}

		service_ptr_t<file> p_file;
		filesystem::g_open_write_new(p_file, newpath, p_abort);
		b_opened=true;

		p_file->write(header.get_ptr(), header.get_size(), p_abort);


		p_file.release();
		b_opened=false;


		backup_path << path.get_ptr() << ".backup";
		if (filesystem::g_exists(backup_path, p_abort))
			filesystem::g_remove(backup_path, p_abort);

		if (filesystem::g_exists(path, p_abort))
			filesystem::g_move(path, backup_path, p_abort);
		filesystem::g_move(newpath, path, p_abort);

		if (filesystem::g_exists(path_shuffle, p_abort))
			filesystem::g_remove(path_shuffle, p_abort);

		if (filesystem::g_exists(path_pstate, p_abort))
			filesystem::g_remove(path_pstate, p_abort);

		if (filesystem::g_exists(path_stats, p_abort))
			filesystem::g_remove(path_stats, p_abort);
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
		throw pfc::exception
		//console::print
			(pfc::string_formatter() << "Error writing iTunesSD file : " << ex.what());
	}
}

void ipod_write_shadowdb_v2(ipod_device_ptr_ref_t p_ipod, const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	pfc::string8 newpath;
	bool b_opened = false;
	//string_print_drive m_path(p_ipod->drive);

	try
	{
		pfc::string8 path = m_path, path_shuffle, path_pstate, path_stats;
		pfc::string8 backup_path;
		path << "\\iTunes\\" << "iTunesSD";
		path_shuffle << m_path << "\\iTunes\\" << "iTunesShuffle";
		path_pstate << m_path << "\\iTunes\\" << "iTunesPState";
		path_stats << m_path << "\\iTunes\\" << "iTunesStats";

		newpath << path << ".temp";

		t_size i, count_tracks = p_library.m_tracks.get_count(), count_playlists = p_library.m_playlists.get_count();

		const t_uint32 shdb_length = 0x40;
		const t_uint32 shtr_length = 372;
		const t_uint32 shth_length = (5 + count_tracks)*sizeof(t_uint32);

		iTunesSD2::writer header;

		header.write_lendian_t(iTunesSD2::shdb, p_abort);
		header.write_lendian_t(0x02000003, p_abort);
		header.write_lendian_t(shdb_length, p_abort);
		header.write_lendian_t(count_tracks, p_abort);
		header.write_lendian_t(count_playlists + 1, p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint8(0), p_abort);
		header.write_lendian_t(t_uint8(p_library.m_itunesprefs.m_voiceover_enabled ? 1 : 0), p_abort); //voiceover_enabled
		header.write_lendian_t(t_uint8(0), p_abort);
		header.write_lendian_t(t_uint8(0), p_abort);
		header.write_lendian_t(count_tracks, p_abort);
		header.write_lendian_t(shdb_length, p_abort); //shth offset
		header.write_lendian_t(shdb_length + shth_length + shtr_length*count_tracks, p_abort); //shph offset
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);
		header.write_lendian_t(t_uint32(0), p_abort);

		{
			iTunesSD2::writer shth;
			shth.write_lendian_t(iTunesSD2::shth, p_abort);
			shth.write_lendian_t(shth_length, p_abort);
			shth.write_lendian_t(count_tracks, p_abort);
			shth.write_lendian_t(t_uint32(0), p_abort);
			shth.write_lendian_t(t_uint32(0), p_abort);
	
			for (i=0; i<count_tracks; i++)
				shth.write_lendian_t(shdb_length + shth_length + i*shtr_length, p_abort);

			header.write(shth, p_abort);
		}

		for (i=0; i<count_tracks; i++)
		{
			const pfc::rcptr_t<itunesdb::t_track> & p_track = p_library.m_tracks[i];
			iTunesSD2::writer shtr;
			shtr.write_lendian_t(iTunesSD2::shtr, p_abort);
			shtr.write_lendian_t(shtr_length, p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort); //start pos
			shtr.write_lendian_t(p_track->length, p_abort);

			int volume = 0;
			if (p_ipod->m_device_properties.m_SupportsSoundCheck)
			{
				unsigned sc = p_library.m_tracks[i]->soundcheck;
				int voli = p_library.m_tracks[i]->volume;
				double volf = (double)voli / 255.0 + 1.0;
				if (volf == 0.0)
				{
					volume = -144;
				}
				else
				{
					double scdb = sc ? 10.0*log10 (1000.0 / (double)sc) : 0; //apple limit to [-12,+12] dB .... but let's not :)
					double vol = 20.0 * log10(volf);
					volume = round_float_signed(vol + scdb);
					if (volume < -48)  volume = -144; //silencer ???
					else if (volume > 18) volume = 18;
				}
			}
			else
			{
				volume = 100 + round_float_signed(100.0 + (double)p_library.m_tracks[i]->volume/255.0);
				volume = min(volume, 200);
				volume = max(volume, 0);
			}

			shtr.write_lendian_t(t_int32(volume)/*p_track->volume*/, p_abort);

			t_uint32 type=0;
			pfc::string_extension ext(p_library.m_tracks[i]->location);
			if (!stricmp_utf8(ext, "mp3"))
				type = 0x1;
			else if (!stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a") || !stricmp_utf8(ext, "aa") || !stricmp_utf8(ext, "m4b"))
				type = 0x2;
			else if (!stricmp_utf8(ext, "wav"))
				type = 0x4;

			shtr.write_lendian_t(type, p_abort);

			pfc::string8 path = p_library.m_tracks[i]->location;

			path.truncate (256);
			path.replace_byte(':','/');

			t_size length_path = path.length();

			shtr.write(path.get_ptr(), length_path, p_abort);

			t_size padding = 256 - length_path;
			for (;padding;padding--)
				shtr.write_lendian_t(t_uint8(0), p_abort);

			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint8(!p_track->skip_on_shuffle), p_abort);
			shtr.write_lendian_t(t_uint8(p_track->remember_playback_position), p_abort);
			shtr.write_lendian_t(t_uint8(p_track->gapless_album), p_abort);
			shtr.write_lendian_t(t_uint8(0), p_abort);
			shtr.write_lendian_t(p_track->encoder_delay, p_abort);
			shtr.write_lendian_t(p_track->encoder_padding, p_abort);
			shtr.write_lendian_t(p_track->samplecount, p_abort);
			shtr.write_lendian_t(t_uint64(p_track->resync_frame_offset), p_abort);
			shtr.write_lendian_t(p_track->album_id, p_abort);
			shtr.write_lendian_t(t_uint16(p_track->tracknumber), p_abort);
			shtr.write_lendian_t(t_uint16(p_track->discnumber), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(p_track->pid, p_abort);
			shtr.write_lendian_t(p_track->artist_id, p_abort);

			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);
			shtr.write_lendian_t(t_uint32(0), p_abort);

			header.write(shtr, p_abort);
		}

		{
			const t_uint32 shph_length = (5+1+count_playlists)*sizeof(t_uint32);
			t_uint32 shpl_rolling_start = header.get_size()+shph_length;

			iTunesSD2::writer shph;
			shph.write_lendian_t(iTunesSD2::shph, p_abort);
			shph.write_lendian_t(shph_length, p_abort);
			shph.write_lendian_t(count_playlists+1, p_abort);
			shph.write_lendian_t(t_uint16(0xffff), p_abort); //wtf
			shph.write_lendian_t(t_uint16(0xffff), p_abort); //wtf
			shph.write_lendian_t(t_uint16(0xffff), p_abort); //wtf
			shph.write_lendian_t(t_uint16(0), p_abort); //wtf

			t_uint32 j;

			pfc::array_staticsize_t<iTunesSD2::writer> shpl_array(count_playlists + 1);

			{
				const t_uint32 shpl_size = (11 + count_tracks)*sizeof(t_uint32);

				iTunesSD2::writer & shpl = shpl_array[0];
				shpl.write_lendian_t(iTunesSD2::shpl, p_abort);
				shpl.write_lendian_t(shpl_size, p_abort);
				shpl.write_lendian_t(count_tracks, p_abort);
				shpl.write_lendian_t(count_tracks, p_abort);
				shpl.write_lendian_t(t_uint64(0), p_abort);
				shpl.write_lendian_t(t_uint32(1), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				for (j=0; j<count_tracks; j++)
					shpl.write_lendian_t(t_uint32(j), p_abort);
			}

			mmh::Permutation permuation_track_id(count_tracks);
			mmh::sort_get_permutation(p_library.m_tracks.get_ptr(), permuation_track_id, ipod::tasks::load_database_t::g_compare_track_id, false, false);

			for (i=0; i<count_playlists; i++)
			{
				const pfc::rcptr_t<itunesdb::t_playlist> & p_playlist = p_library.m_playlists[i];
				t_size count_entries = p_playlist->items.get_count();

				pfc::array_t<t_uint32> indices;
				indices.set_size(count_entries);

				t_size k=0;
				for (j=0; j<count_entries; j++)
					if (!p_playlist->items[j].is_podcast_group)
					{
						t_size index;
						if (p_library.m_tracks.bsearch_permutation_t(ipod::tasks::load_database_t::g_compare_track_id_with_id, p_playlist->items[j].track_id, permuation_track_id, index))
							indices[k++] = index;
					}

				indices.set_size(k);

				const t_uint32 shpl_size = (11 + k)*sizeof(t_uint32);
				iTunesSD2::writer & shpl = shpl_array[1+i];
				shpl.write_lendian_t(iTunesSD2::shpl, p_abort);
				shpl.write_lendian_t(shpl_size, p_abort);
				shpl.write_lendian_t(k, p_abort);
				shpl.write_lendian_t(k, p_abort);
				shpl.write_lendian_t(p_library.m_playlists[i]->id, p_abort);
				shpl.write_lendian_t(t_uint32(2), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				shpl.write_lendian_t(t_uint32(0), p_abort);
				for (j=0; j<k; j++)
					shpl.write_lendian_t(indices[j], p_abort);
			}

			for (i=0; i<count_playlists+1; i++)
			{
				shph.write_lendian_t(shpl_rolling_start, p_abort);
				shpl_rolling_start += shpl_array[i].get_size();
			}
			for (i=0; i<count_playlists+1; i++)
				shph.write(shpl_array[i], p_abort);
			header.write(shph, p_abort);
		}

		service_ptr_t<file> p_file;
		filesystem::g_open_write_new(p_file, newpath, p_abort);
		b_opened=true;

		p_file->write(header.get_ptr(), header.get_size(), p_abort);


		p_file.release();
		b_opened=false;

		abort_callback_dummy p_dummy_abort;

		backup_path << path.get_ptr() << ".backup";
		if (filesystem::g_exists(backup_path, p_dummy_abort))
			filesystem::g_remove(backup_path, p_dummy_abort);

		if (filesystem::g_exists(path, p_dummy_abort))
			filesystem::g_move(path, backup_path, p_dummy_abort);
		filesystem::g_move(newpath, path, p_dummy_abort);

		if (filesystem::g_exists(path_shuffle, p_dummy_abort))
			filesystem::g_remove(path_shuffle, p_dummy_abort);

		if (filesystem::g_exists(path_pstate, p_dummy_abort))
			filesystem::g_remove(path_pstate, p_dummy_abort);

		if (filesystem::g_exists(path_stats, p_dummy_abort))
			filesystem::g_remove(path_stats, p_dummy_abort);
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
		throw pfc::exception
		//console::print
			(pfc::string_formatter() << "Error writing iTunesSD file : " << ex.what());
	}
}
