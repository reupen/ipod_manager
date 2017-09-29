#include "stdafx.h"

#include "itunesdb.h"


namespace itunesdb
{

	void reader::read_iihm(t_header_marker< identifiers::iihm > & p_header, itunesdb::t_artist::ptr & p_out, abort_callback & p_abort)
	{
		p_out = new t_artist;

		stream_reader_memblock_ref_dop p_iihm(p_header.data.get_ptr(), p_header.data.get_size(), m_swap_endianess);
		t_uint32 dohm_count;
		p_iihm.read_lendian_auto_t(dohm_count, p_abort);
		p_iihm.read_lendian_auto_t(p_out->id, p_abort);
		p_iihm.read_lendian_auto_t(p_out->pid, p_abort); //
		p_iihm.read_lendian_auto_t(p_out->type, p_abort); //28
		p_iihm.read_lendian_auto_t(p_out->artwork_status, p_abort);
		p_iihm.read_lendian_auto_t(p_out->unk1, p_abort);
		p_iihm.read_lendian_auto_t(p_out->unk2, p_abort);
		p_iihm.read_lendian_auto_t(p_out->artwork_album_pid, p_abort);

		unsigned i;
		for (i=0; i<dohm_count; i++)
		{
			t_header_marker<identifiers::dohm> dohm;
			read_header(dohm, p_abort);
			stream_reader_memblock_ref_dop p_dohm(dohm.data.get_ptr(), dohm.data.get_size(), m_swap_endianess);
			t_uint32 type;
			p_dohm.read_lendian_auto_t(type, p_abort);

			switch (type)
			{
			case itunesdb::do_types::artist_list_sort_artist:
				read_do_string_utf16(dohm, p_out->sort_artist, p_abort);
				p_out->sort_artist_valid = true;
				break;
			case itunesdb::do_types::artist_list_artist:
				read_do_string_utf16(dohm, p_out->artist, p_abort);
				p_out->artist_valid = true;
				break;
			default:
				m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
				break;
			}
		}
	}

	void reader::read_aihm(t_header_marker< identifiers::aihm > & p_header, itunesdb::t_album::ptr & p_out, abort_callback & p_abort)
	{
		p_out = new itunesdb::t_album;

		stream_reader_memblock_ref_dop p_aihm(p_header.data.get_ptr(), p_header.data.get_size(), m_swap_endianess);
		t_uint32 dohm_count;
		p_aihm.read_lendian_auto_t(dohm_count, p_abort); //12
		p_aihm.read_lendian_auto_t(p_out->id, p_abort); //16
		p_aihm.read_lendian_auto_t(p_out->pid, p_abort); //20
		p_aihm.read_lendian_auto_t(p_out->kind, p_abort); //28
		p_aihm.read_lendian_auto_t(p_out->all_compilations, p_abort); //
		p_aihm.read_lendian_auto_t(p_out->artwork_status, p_abort); //
		p_aihm.read_lendian_auto_t(p_out->unk1, p_abort); //

		p_aihm.read_lendian_auto_t(p_out->artwork_item_pid, p_abort); //32
		p_aihm.read_lendian_auto_t(p_out->user_rating, p_abort); //40
		p_aihm.read_lendian_auto_t(p_out->season_number, p_abort); //44

		unsigned i;
		for (i=0; i<dohm_count; i++)
		{
			t_header_marker<identifiers::dohm> dohm;
			read_header(dohm, p_abort);
			stream_reader_memblock_ref_dop p_dohm(dohm.data.get_ptr(), dohm.data.get_size(), m_swap_endianess);
			t_uint32 type;
			p_dohm.read_lendian_auto_t(type, p_abort);

			switch (type)
			{
			case itunesdb::do_types::album_list_album:
				read_do_string_utf16(dohm, p_out->album, p_abort);
				p_out->album_valid = true;
				break;
			case itunesdb::do_types::album_list_album_artist:
				read_do_string_utf16(dohm, p_out->album_artist_strict, p_abort);
				p_out->album_artist_strict_valid = true;
				break;
			case itunesdb::do_types::album_list_artist:
				read_do_string_utf16(dohm, p_out->artist, p_abort);
				p_out->artist_valid = true;
				break;
			case itunesdb::do_types::album_list_podcast_url:
				read_do_string_utf16(dohm, p_out->podcast_url, p_abort);
				p_out->podcast_url_valid = true;
				break;
			case itunesdb::do_types::album_list_show:
				read_do_string_utf16(dohm, p_out->show, p_abort);
				p_out->show_valid = true;
				break;
			default:
				m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
				break;
			}
		}
	}
	void reader::read_tihm(t_header_marker<identifiers::tihm> & p_header, pfc::rcptr_t <t_track> & p_out, abort_callback & p_abort)
	{
		pfc::rcptr_t <t_track> track = pfc::rcnew_t<t_track>();
		t_uint32 dohm_count;
		stream_reader_memblock_ref_dop p_tihm(p_header.data.get_ptr(), p_header.data.get_size(), m_swap_endianess);
		try 
		{
			p_tihm.read_lendian_auto_t(dohm_count, p_abort); //12
			p_tihm.read_lendian_auto_t(track->id, p_abort); //16
			p_tihm.read_lendian_auto_t(track->location_type, p_abort); //20
			t_uint32 file_extension;
			p_tihm.read_lendian_auto_t(file_extension, p_abort); //24
			p_tihm.read_lendian_auto_t(track->type1, p_abort); //28
			p_tihm.read_lendian_auto_t(track->type2, p_abort); //29
			p_tihm.read_lendian_auto_t(track->compilation, p_abort); //30
			p_tihm.read_lendian_auto_t(track->rating, p_abort); //31
			p_tihm.read_lendian_auto_t(track->lastmodifiedtime, p_abort); //32
			p_tihm.read_lendian_auto_t(track->file_size_32, p_abort); //36
			p_tihm.read_lendian_auto_t(track->length, p_abort); //40
			p_tihm.read_lendian_auto_t(track->tracknumber, p_abort); //44
			p_tihm.read_lendian_auto_t(track->totaltracks, p_abort); //48
			p_tihm.read_lendian_auto_t(track->year, p_abort); //52
			p_tihm.read_lendian_auto_t(track->bitrate, p_abort); //56
			p_tihm.read_lendian_auto_t(track->unk8, p_abort); //60
			p_tihm.read_lendian_auto_t(track->samplerate, p_abort); //62
			track->samplecount = audio_math::time_to_samples(track->length/1000.0,track->samplerate);
			p_tihm.read_lendian_auto_t(track->volume, p_abort); //64
			p_tihm.read_lendian_auto_t(track->starttime, p_abort); //68
			p_tihm.read_lendian_auto_t(track->stoptime, p_abort); //72
			p_tihm.read_lendian_auto_t(track->volume_normalisation_energy, p_abort); //76
			p_tihm.read_lendian_auto_t(track->play_count_user, p_abort); //80
			p_tihm.read_lendian_auto_t(track->play_count_recent, p_abort); //84
			p_tihm.read_lendian_auto_t(track->lastplayedtime, p_abort); //88
			p_tihm.read_lendian_auto_t(track->discnumber, p_abort); //92
			p_tihm.read_lendian_auto_t(track->totaldiscs, p_abort); //96
			p_tihm.read_lendian_auto_t(track->userid, p_abort); //100
			p_tihm.read_lendian_auto_t(track->dateadded, p_abort); //104
			p_tihm.read_lendian_auto_t(track->bookmarktime, p_abort); //108
			p_tihm.read_lendian_auto_t(track->pid, p_abort); //112
			track->dbid2 = track->pid;
			p_tihm.read_lendian_auto_t(track->checked, p_abort); //120
			p_tihm.read_lendian_auto_t(track->application_rating, p_abort); //121
			p_tihm.read_lendian_auto_t(track->bpm, p_abort); //122
			p_tihm.read_lendian_auto_t(track->artwork_count, p_abort); //124
			p_tihm.read_lendian_auto_t(track->unk9, p_abort); //126
			p_tihm.read_lendian_auto_t(track->artwork_size, p_abort); //128
			p_tihm.read_lendian_auto_t(track->unk11, p_abort); //132
			float samplerate2;
			p_tihm.read_lendian_auto_t(samplerate2, p_abort); //136
			p_tihm.read_lendian_auto_t(track->datereleased, p_abort); //140
			p_tihm.read_lendian_auto_t(track->audio_format, p_abort); //144 
			p_tihm.read_lendian_auto_t(track->content_rating, p_abort); //146 
			p_tihm.read_lendian_auto_t(track->unk12, p_abort); //147 
			p_tihm.read_lendian_auto_t(track->store_key_versions, p_abort); //148

			p_tihm.read_lendian_auto_t(track->skip_count_user, p_abort); //152
			p_tihm.read_lendian_auto_t(track->skip_count_recent, p_abort); //156
			p_tihm.read_lendian_auto_t(track->last_skipped, p_abort); //160

			p_tihm.read_lendian_auto_t(track->artwork_flag, p_abort); //164
			p_tihm.read_lendian_auto_t(track->skip_on_shuffle, p_abort); //165
			p_tihm.read_lendian_auto_t(track->remember_playback_position, p_abort); //166
			p_tihm.read_lendian_auto_t(track->podcast_flag, p_abort); //167

			p_tihm.read_lendian_auto_t(track->dbid2, p_abort); //168
			p_tihm.read_lendian_auto_t(track->lyrics_flag, p_abort); //176
			p_tihm.read_lendian_auto_t(track->video_flag, p_abort); //177
			p_tihm.read_lendian_auto_t(track->played_marker, p_abort); //178

			p_tihm.read_lendian_auto_t(track->unk37, p_abort); //179
			p_tihm.read_lendian_auto_t(track->bookmark_time_ms_common, p_abort); //180
			p_tihm.read_lendian_auto_t(track->gapless_encoding_delay, p_abort); //184
			p_tihm.read_lendian_auto_t(track->samplecount, p_abort); //188

			//p_tihm.read_lendian_auto_t(track->unk24, p_abort);
			p_tihm.read_lendian_auto_t(track->lyrics_checksum, p_abort); //196
			p_tihm.read_lendian_auto_t(track->gapless_encoding_drain, p_abort); //200
			p_tihm.read_lendian_auto_t(track->gapless_heuristic_info, p_abort); //204

			p_tihm.read_lendian_auto_t(track->media_type, p_abort); //208
			p_tihm.read_lendian_auto_t(track->season_number, p_abort); //212
			p_tihm.read_lendian_auto_t(track->episode_sort_id, p_abort); //216
			p_tihm.read_lendian_auto_t(track->date_purchased, p_abort); //220 
			p_tihm.read_lendian_auto_t(track->legacy_store_item_id, p_abort); //224
			p_tihm.read_lendian_auto_t(track->legacy_store_genre_id, p_abort); //228
			p_tihm.read_lendian_auto_t(track->legacy_store_artist_id, p_abort); //232
			p_tihm.read_lendian_auto_t(track->legacy_store_composer_id, p_abort); //236
			p_tihm.read_lendian_auto_t(track->legacy_store_playlist_id, p_abort); //240

			p_tihm.read_lendian_auto_t(track->legacy_store_storefront_id, p_abort); //244
			p_tihm.read_lendian_auto_t(track->resync_frame_offset, p_abort); //248 64-bit !
			p_tihm.read_lendian_auto_t(track->unk43_1, p_abort); //256
			p_tihm.read_lendian_auto_t(track->gapless_album, p_abort); //258
			p_tihm.read_lendian_auto_t(track->unk43_2, p_abort); //259
			p_tihm.read(track->unk44, sizeof(track->unk44), p_abort); //260
			//p_tihm.read_lendian_auto_t(track->unk45, p_abort); //264
			//p_tihm.read_lendian_auto_t(track->unk46, p_abort); //268
			//p_tihm.read_lendian_auto_t(track->unk47, p_abort); //272
			//p_tihm.read_lendian_auto_t(track->unk48, p_abort); //276
			p_tihm.read_lendian_auto_t(track->unk49, p_abort); //280
			p_tihm.read_lendian_auto_t(track->unk50, p_abort); //284
			p_tihm.read_lendian_auto_t(track->album_id, p_abort); //288
			p_tihm.read_lendian_auto_t(track->unk52, p_abort); //292
			p_tihm.read_lendian_auto_t(track->unk53, p_abort); //296
			p_tihm.read_lendian_auto_t(track->filesize_64, p_abort); //300
			p_tihm.read_lendian_auto_t(track->unk56, p_abort); //308
			p_tihm.read_lendian_auto_t(track->unk57, p_abort); //312
			p_tihm.read_lendian_auto_t(track->unk58, p_abort); //316
			p_tihm.read_lendian_auto_t(track->unk59, p_abort); //320
			p_tihm.read_lendian_auto_t(track->unk60, p_abort); //324
			p_tihm.read_lendian_auto_t(track->is_self_contained, p_abort); //328
			p_tihm.read_lendian_auto_t(track->is_compressed, p_abort); //329
			p_tihm.read_lendian_auto_t(track->unk61_1, p_abort); //330
			p_tihm.read_lendian_auto_t(track->unk61_2, p_abort); //331
			p_tihm.read_lendian_auto_t(track->unk62, p_abort); //332
			p_tihm.read_lendian_auto_t(track->unk63, p_abort); //336
			p_tihm.read_lendian_auto_t(track->unk64, p_abort); //340
			p_tihm.read_lendian_auto_t(track->audio_fingerprint, p_abort); //344
			p_tihm.read_lendian_auto_t(track->unk66, p_abort); //348
			p_tihm.read_lendian_auto_t(track->mhii_id, p_abort); //352 artwork_cache_id
			p_tihm.read_lendian_auto_t(track->unk68, p_abort); //356
			p_tihm.read_lendian_auto_t(track->unk69_1, p_abort); //360
			p_tihm.read_lendian_auto_t(track->unk69_2, p_abort); //361
			p_tihm.read_lendian_auto_t(track->unk69_3, p_abort); //362
			p_tihm.read_lendian_auto_t(track->is_anamorphic, p_abort); //363
			p_tihm.read_lendian_auto_t(track->unk70, p_abort); //364
			p_tihm.read_lendian_auto_t(track->unk71_1, p_abort); //368
			p_tihm.read_lendian_auto_t(track->unk71_2, p_abort); //369
			p_tihm.read_lendian_auto_t(track->has_alternate_audio_and_closed_captions, p_abort); //370
			p_tihm.read_lendian_auto_t(track->has_subtitles, p_abort); //371
			p_tihm.read_lendian_auto_t(track->audio_language, p_abort); //372
			p_tihm.read_lendian_auto_t(track->audio_track_index, p_abort); //374
			p_tihm.read_lendian_auto_t(track->audio_track_id, p_abort); //376
			p_tihm.read_lendian_auto_t(track->subtitle_language, p_abort); //380 
			p_tihm.read_lendian_auto_t(track->subtitle_track_index, p_abort); //382
			p_tihm.read_lendian_auto_t(track->subtitle_track_id, p_abort); //384
			p_tihm.read_lendian_auto_t(track->unk76, p_abort); //388
			p_tihm.read_lendian_auto_t(track->unk77, p_abort); //392
			p_tihm.read_lendian_auto_t(track->unk78, p_abort); //396
			p_tihm.read_lendian_auto_t(track->unk79, p_abort); //400
			p_tihm.read_lendian_auto_t(track->characteristics_valid, p_abort); //404
			p_tihm.read_lendian_auto_t(track->unk80_1, p_abort); //405
			p_tihm.read_lendian_auto_t(track->is_hd, p_abort); //406
			p_tihm.read_lendian_auto_t(track->store_kind, p_abort); //407
			p_tihm.read_lendian_auto_t(track->account_id_primary, p_abort); //408
			p_tihm.read_lendian_auto_t(track->account_id_secondary, p_abort); //416
			p_tihm.read_lendian_auto_t(track->key_id, p_abort); //424
			p_tihm.read_lendian_auto_t(track->store_item_id, p_abort); //432 store_item_id 64-bit
			p_tihm.read_lendian_auto_t(track->store_genre_id, p_abort); //440 store_genre_id 64-bit
			p_tihm.read_lendian_auto_t(track->store_artist_id, p_abort); //448 store_composer_id 64-bit
			p_tihm.read_lendian_auto_t(track->store_composer_id, p_abort); //456 store_genre_id 64-bit
			p_tihm.read_lendian_auto_t(track->store_playlist_id, p_abort); //464 store_playlist_id 64-bit (??)
			p_tihm.read_lendian_auto_t(track->store_front_id, p_abort); //472 store_front_id 64-bit
			p_tihm.read_lendian_auto_t(track->artist_id, p_abort); //480
			p_tihm.read_lendian_auto_t(track->genius_id, p_abort); //484
			p_tihm.read_lendian_auto_t(track->key_platform_id, p_abort); //492
			p_tihm.read_lendian_auto_t(track->unk100, p_abort); //496
			p_tihm.read_lendian_auto_t(track->unk101, p_abort); //500 //another id
			p_tihm.read_lendian_auto_t(track->media_type2, p_abort); //504
			p_tihm.read_lendian_auto_t(track->unk102, p_abort); //508
			p_tihm.read_lendian_auto_t(track->unk103, p_abort); //512
			p_tihm.read_lendian_auto_t(track->key_id2, p_abort); //516
			p_tihm.read_lendian_auto_t(track->channel_count, p_abort); //524
			p_tihm.read_lendian_auto_t(track->unk104_2, p_abort); //526
			p_tihm.read_lendian_auto_t(track->unk105, p_abort); //528
			p_tihm.read_lendian_auto_t(track->unk106_1, p_abort); //532
			p_tihm.read_lendian_auto_t(track->chosen_by_auto_fill, p_abort); //532
			p_tihm.read_lendian_auto_t(track->unk106_3, p_abort); //532
			p_tihm.read_lendian_auto_t(track->unk106_4, p_abort); //532
			p_tihm.read_lendian_auto_t(track->unk107, p_abort); //536
			p_tihm.read_lendian_auto_t(track->unk108, p_abort); //540
			p_tihm.read_lendian_auto_t(track->unk109, p_abort); //544
			p_tihm.read_lendian_auto_t(track->unk110, p_abort); //548
			p_tihm.read_lendian_auto_t(track->unk111, p_abort); //552
			p_tihm.read_lendian_auto_t(track->unk112, p_abort); //556
			p_tihm.read_lendian_auto_t(track->unk113, p_abort); //560
			p_tihm.read_lendian_auto_t(track->unk114, p_abort); //564
			p_tihm.read_lendian_auto_t(track->unk115, p_abort); //568
			p_tihm.read_lendian_auto_t(track->unk116, p_abort); //572
			p_tihm.read_lendian_auto_t(track->unk117, p_abort); //576
			p_tihm.read_lendian_auto_t(track->unk118, p_abort); //580
			p_tihm.read_lendian_auto_t(track->unk119, p_abort); //584
			p_tihm.read_lendian_auto_t(track->unk120, p_abort); //588
			p_tihm.read_lendian_auto_t(track->unk121, p_abort); //592
			p_tihm.read_lendian_auto_t(track->unk122, p_abort); //596
			p_tihm.read_lendian_auto_t(track->unk123, p_abort); //600
			p_tihm.read_lendian_auto_t(track->unk124, p_abort); //604
			p_tihm.read_lendian_auto_t(track->unk125, p_abort); //608
			p_tihm.read_lendian_auto_t(track->unk126, p_abort); //612
			p_tihm.read_lendian_auto_t(track->unk127, p_abort); //616
			p_tihm.read_lendian_auto_t(track->unk128, p_abort); //620

		}
		catch (exception_io_data_truncation &)
		{
			//hit end of tihm header
		}
		if (!track->media_type2)
		{
			track->media_type2 = track->media_type;
			if (track->media_type2 & (t_track::type_is_voice_memo|t_track::type_itunes_u))
				track->media_type2 &= ~t_track::type_audio;
		}
		{
			unsigned i;
			for (i=0; i<dohm_count; i++)
			{
				t_header_marker<identifiers::dohm> dohm;
				read_header(dohm, p_abort);
				stream_reader_memblock_ref_dop p_dohm(dohm.data.get_ptr(), dohm.data.get_size(), m_swap_endianess);
				t_uint32 type;
				p_dohm.read_lendian_auto_t(type, p_abort);

				if (type < 15 || (18 <= type && type <= 25) || (27 <= type && type <= 31) || (type == do_types::publication_id) || (type == do_types::copyright) || (type == do_types::collection_description) )
				{
					pfc::string8 dummy;
					pfc::string8 * str_u = &dummy;
					bool * b_valid = NULL;

					switch (type)
					{
					case do_types::sort_artist:
						str_u = &track->sort_artist;
						b_valid = &track->sort_artist_valid;
						break;
					case do_types::sort_album_artist:
						str_u = &track->sort_album_artist;
						b_valid = &track->sort_album_artist_valid;
						break;
					case do_types::sort_album:
						str_u = &track->sort_album;
						b_valid = &track->sort_album_valid;
						break;
					case do_types::sort_composer:
						str_u = &track->sort_composer;
						b_valid = &track->sort_composer_valid;
						break;
					case do_types::sort_show:
						str_u = &track->sort_show;
						b_valid = &track->sort_show_valid;
						break;
					case do_types::sort_title:
						str_u = &track->sort_title;
						b_valid = &track->sort_title_valid;
						break;
					case do_types::title:
						str_u = &track->title;
						b_valid = &track->title_valid;
						break;
					case do_types::artist:
						str_u = &track->artist;
						b_valid = &track->artist_valid;
						break;
					case do_types::genre:
						str_u = &track->genre;
						b_valid = &track->genre_valid;
						break;
					case do_types::album:
						str_u = &track->album;
						b_valid = &track->album_valid;
						break;
					case do_types::location:
						str_u = &track->location;
						b_valid = &track->location_valid;
						break;
					case do_types::composer:
						str_u = &track->composer;
						b_valid = &track->composer_valid;
						break;
					case do_types::comment:
						str_u = &track->comment;
						b_valid = &track->comment_valid;
						break;
					case do_types::category:
						str_u = &track->category;
						b_valid = &track->category_valid;
						break;
					case do_types::description:
						str_u = &track->description;
						b_valid = &track->description_valid;
						break;
					case do_types::collection_description:
						str_u = &track->collection_description;
						b_valid = &track->collection_description_valid;
						break;
					case do_types::publication_id:
						str_u = &track->publication_id;
						b_valid = &track->publication_id_valid;
						break;
					case do_types::grouping:
						str_u = &track->grouping;
						b_valid = &track->grouping_valid;
						break;
					case do_types::eq_setting:
						str_u = &track->eq_settings;
						b_valid = &track->eq_settings_valid;
						break;
					case do_types::filetype:
						str_u = &track->filetype;
						b_valid = &track->filetype_valid;
						break;
					case do_types::subtitle:
						str_u = &track->subtitle;
						b_valid = &track->subtitle_valid;
						break;
					case do_types::episode_number:
						str_u = &track->episode;
						b_valid = &track->episode_valid;
						break;
					case do_types::tv_network:
						str_u = &track->tv_network;
						b_valid = &track->tv_network_valid;
						break;
					case do_types::extended_content_rating:
						str_u = &track->extended_content_rating;
						b_valid = &track->extended_content_rating_valid;
						break;
					case do_types::show:
						str_u = &track->show;
						b_valid = &track->show_valid;
						break;
					case do_types::album_artist:
						str_u = &track->album_artist;
						b_valid = &track->album_artist_valid;
						break;
					case do_types::keywords:
						str_u = &track->keywords;
						b_valid = &track->keywords_valid;
						break;
					case do_types::copyright:
						str_u = &track->copyright;
						b_valid = &track->copyright_valid;
						break;
					};

					read_do_string_utf16(dohm, *str_u, p_abort);
					if (b_valid)
						*b_valid=true;

				}
				else if (type == do_types::podcast_enclosure_url || type == do_types::podcast_rss_url)
				{
					pfc::string8 str_u;
					read_do_string_utf8(dohm, str_u, p_abort);

					switch (type)
					{
					case do_types::podcast_enclosure_url:
						track->podcast_enclosure_url.set_string(str_u);
						track->podcast_enclosure_url_valid = true;
						break;
					case do_types::podcast_rss_url:
						track->podcast_rss_url.set_string(str_u);
						track->podcast_rss_url_valid = true;
						break;
					}
				}
				else if (type == do_types::chapter_data)
				{
					//track->dohm_chapter_data.append_fromptr(dohm.data.get_ptr(), dohm.data.get_size());
					track->do_chapter_data.set_size(dohm.section_size - dohm.header_size);
					m_file->read(track->do_chapter_data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
					track->chapter_data_valid = true;
				}
				else if (type == do_types::store_data)
				{
					track->do_store_data.set_size(dohm.section_size - dohm.header_size);
					m_file->read(track->do_store_data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
					track->store_data_valid = true;
				}
				else if (type == do_types::video_characteristics)
				{
					t_video_characteristics vc;
					read_lendian_auto_t(vc.width, p_abort);
					read_lendian_auto_t(vc.height, p_abort);
					read_lendian_auto_t(vc.width, p_abort);
					read_lendian_auto_t(vc.track_id, p_abort);
					read_lendian_auto_t(vc.codec, p_abort);
					read_lendian_auto_t(vc.percentage_encrypted, p_abort); //*1000
					read_lendian_auto_t(vc.bit_rate, p_abort);
					read_lendian_auto_t(vc.peak_bit_rate, p_abort);
					read_lendian_auto_t(vc.buffer_size, p_abort);
					read_lendian_auto_t(vc.profile, p_abort);
					read_lendian_auto_t(vc.level, p_abort);
					read_lendian_auto_t(vc.complexity_level, p_abort);
					read_lendian_auto_t(vc.frame_rate, p_abort); //*1000
					read_lendian_auto_t(vc.unk1, p_abort);
					m_file->read(&vc.unk2[0], sizeof(vc.unk2), p_abort);
					if (dohm.section_size - dohm.header_size > 84)
						m_file->skip_object(dohm.section_size - dohm.header_size - 84, p_abort);
					track->video_characteristics_entries.add_item(vc);
				}
				else
					m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
			}
		}
		p_out = track;
	}

	void reader::read_do_string_utf16(t_header_marker< identifiers::dohm > & dohm, pfc::string8 & p_out, abort_callback & p_abort)
	{
		WCHAR * ptr = NULL; t_size len = 0;
		read_do_string_utf16(dohm, ptr, len, p_abort);

		t_size size = pfc::stringcvt::estimate_wide_to_utf8(ptr, len);

		pfc::string_buffer buffer(p_out, size);
		pfc::stringcvt::convert_wide_to_utf8( buffer,size,ptr, len);

		//p_out = pfc::stringcvt::string_utf8_from_wide(ptr, len);
	}
	void reader::read_do_string_utf16(t_header_marker< identifiers::dohm > & dohm, pfc::array_t<WCHAR> & p_out, abort_callback & p_abort)
	{
		t_uint32 length;
		pfc::array_t<t_uint8> data;
		data.set_size(dohm.section_size - dohm.header_size);
		m_file->read(data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);

		stream_reader_memblock_ref_dop p_dohm_data(data.get_ptr(), data.get_size(), m_swap_endianess);
		p_dohm_data.skip_object(4, p_abort);
		p_dohm_data.read_lendian_auto_t(length, p_abort);
		p_dohm_data.skip_object(8, p_abort);

		p_out.set_size(length/2);
		p_dohm_data.read(p_out.get_ptr(), p_out.get_size()*sizeof(WCHAR), p_abort);
	}
	void reader::read_do_string_utf16(t_header_marker< identifiers::dohm > & dohm, WCHAR * & p_out, t_size & len, abort_callback & p_abort)
	{
		t_uint32 length;
		t_uint8 * data=NULL; t_size datalen=dohm.section_size - dohm.header_size;

		m_file->read_nobuffer(data, datalen, p_abort);

		stream_reader_memblock_ref_dop p_dohm_data(data, datalen, m_swap_endianess);
		p_dohm_data.skip_object(4, p_abort);
		p_dohm_data.read_lendian_auto_t(length, p_abort);
		p_dohm_data.skip_object(8, p_abort);

		len = length/2;
		p_dohm_data.read_nobuffer((t_uint8*&)p_out, len*sizeof(WCHAR), p_abort);
	}
	void reader::read_do_string_utf8(t_header_marker< identifiers::dohm > & dohm, pfc::string8 & p_out, abort_callback & p_abort)
	{
		pfc::array_t<t_uint8> data;
		pfc::array_t<char> data2;
		data.set_size(dohm.section_size - dohm.header_size);
		m_file->read(data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);

		stream_reader_memblock_ref_dop p_dohm_data(data.get_ptr(), data.get_size(), m_swap_endianess);

		data2.set_size(dohm.section_size - dohm.header_size);

		p_dohm_data.read(data2.get_ptr(), data2.get_size(), p_abort);
		p_out.set_string(data2.get_ptr(), data2.get_size());
	}

	void writer::write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
		const void * p_data, t_size data_size, t_uint32 header_data, abort_callback & p_abort)
	{
		m_file.write_bendian_t(identifier, p_abort);

		m_file.write_lendian_t(t_uint32(header_size) + 4*3, p_abort);
		m_file.write_lendian_t(header_data, p_abort);
		m_file.write(p_header, header_size, p_abort);
		m_file.write(p_data, data_size, p_abort);
	}
	void writer::write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
		const void * p_data, t_size data_size, abort_callback & p_abort)
	{
		write_section(identifier, p_header, header_size, p_data, data_size, (t_uint32)data_size + t_uint32(header_size) + 4*3, p_abort);
	}

	void writer::write_do(do_types::t_type type, t_uint32 val1, t_uint32 val2, const void * data, t_size data_size, abort_callback & p_abort)
	{
		stream_writer_mem dshm;
		dshm.write_lendian_t(t_uint32(type), p_abort);
		dshm.write_lendian_t(t_uint32(val1), p_abort);
		dshm.write_lendian_t(t_uint32(val2), p_abort);

		write_section(identifiers::dohm, dshm.get_ptr(), dshm.get_size(), data, data_size, p_abort);
	}

	bool writer::write_do_string_meta(do_types::t_type type,const file_info * info, const char * field, abort_callback & p_abort)
	{
		if (info->meta_exists(field))
		{
			pfc::string8_fast_aggressive buffer;
			buffer.prealloc(32);
			t_size i, count = info->meta_get_count_by_name(field);
			for (i=0; i<count; i++)
			{
				buffer << info->meta_get(field, i);
				if (i+1<count) buffer << ", ";
			}

			pfc::stringcvt::string_wide_from_utf8 str(buffer);
			write_do_string(type, str.get_ptr(), str.length(), p_abort);
			return true;
		}
		return false;
	}
	void writer::write_do_string(do_types::t_type type, const char * str, abort_callback & p_abort)
	{
		pfc::stringcvt::string_wide_from_utf8 wide(str);
		write_do_string(type, wide, wide.length(), p_abort);
	}

	void writer::write_do_string(do_types::t_type type, const WCHAR * str, t_size length, abort_callback & p_abort)
	{
		stream_writer_mem dshm;
		dshm.write_lendian_t(t_uint32(type), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		stream_writer_mem ds;
		ds.write_lendian_t(t_uint32(1), p_abort);
		t_size str_bytes = length;
		if (m_dbversion < 5) str_bytes = min (str_bytes, 500);
		str_bytes *= sizeof(wchar_t);
		ds.write_lendian_t(t_uint32(str_bytes), p_abort);
		ds.write_lendian_t(t_uint32(1), p_abort);
		ds.write_lendian_t(t_uint32(0), p_abort);
		ds.write(str, str_bytes, p_abort);

		write_section(identifiers::dohm, dshm.get_ptr(), dshm.get_size(), ds.get_ptr(), ds.get_size(), p_abort);
	}

	void writer::write_do_string_utf8(do_types::t_type type, const char * str, abort_callback & p_abort)
	{
		stream_writer_mem dshm;
		dshm.write_lendian_t(t_uint32(type), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::dohm, dshm.get_ptr(), dshm.get_size(), str, strlen(str), p_abort);
	}
}