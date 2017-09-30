#include "stdafx.h"

#include "helpers.h"
#include "itunesdb.h"
#include "writer_sort_helpers.h"

namespace itunesdb {
	int t_artist::g_compare_standard_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		//ret = stricmp_utf8(i1->artist, i2->album_artist_valid ? i2->album_artist : i2->artist);
		if (!i2->album_artist_valid && !i2->artist && !i2->show_valid)
			ret = pfc::compare_t(i1->temp_track_pid, i2->pid);
		else
		{
			ret = stricmp_utf8(i1->artist, i2->album_artist_valid ? i2->album_artist : i2->artist);
			if (ret == 0)
				ret = stricmp_utf8(i1->temp_show, i2->show);
			if (ret == 0)
				ret = pfc::compare_t(i1->temp_season_number, i2->season_number);
		}
		return ret;
	}
	int t_album::g_compare_album_mixed(const ptr & i1, const ptr & i2)
	{
		int ret = 0;
		ret = pfc::compare_t(i1->temp_media_type, i2->temp_media_type);
		if (!ret)
		{
			switch (i1->temp_media_type)
			{
				case itunesdb::t_track::type_audio:
				case itunesdb::t_track::type_music_video:
					ret = stricmp_utf8(i1->artist, i2->artist);
					if (!ret)
						ret = stricmp_utf8(i1->album, i2->album);
					break;
				case itunesdb::t_track::type_podcast:
					ret = stricmp_utf8(i1->album, i2->album);
					break;
				case itunesdb::t_track::type_tv_show:
					ret = stricmp_utf8(i1->show, i2->show);
					if (!ret)
						ret = pfc::compare_t(i1->season_number, i2->season_number);
					break;
				default:
					ret = pfc::compare_t(i1->temp_track_pid, i2->temp_track_pid);
					break;
			};
		}
		return ret;
	}
	int t_album::g_compare_album_mixed_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		ret = pfc::compare_t(i1->temp_media_type, i2->media_type2);
		if (!ret)
		{
			switch (i1->temp_media_type)
			{
				case itunesdb::t_track::type_audio:
				case itunesdb::t_track::type_music_video:
					ret = stricmp_utf8(i1->artist, i2->album_artist_valid ? i2->album_artist : i2->artist);
					if (!ret)
						ret = stricmp_utf8(i1->album, i2->album);
					break;
				case itunesdb::t_track::type_podcast:
					ret = stricmp_utf8(i1->album, i2->album);
					break;
				case itunesdb::t_track::type_tv_show:
					ret = stricmp_utf8(i1->show, i2->show);
					if (!ret)
						ret = pfc::compare_t(i1->season_number, i2->season_number);
					break;
				default:
					ret = pfc::compare_t(i1->temp_track_pid, i2->pid);
					break;
			};
		}
		return ret;
	}
	int t_album::g_compare_song_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		ret = stricmp_utf8(i1->artist, i2->album_artist_valid ? i2->album_artist : i2->artist);
		if (!ret)
			ret = stricmp_utf8(i1->album, i2->album);
		return ret;
	}
	int t_album::g_compare_other_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		ret = stricmp_utf8(i1->artist, i2->album_artist_valid ? i2->album_artist : i2->artist);
		if (!ret)
			ret = stricmp_utf8(i1->album, i2->album);
		if (!ret)
			ret = pfc::compare_t(i1->temp_track_pid, i2->pid);
		return ret;
	}
	int t_album::g_compare_show_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		ret = stricmp_utf8(i1->show, i2->show);
		if (!ret)
			ret = pfc::compare_t(i1->season_number, i2->season_number);
		return ret;
	}
	int t_album::g_compare_podcast_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2)
	{
		int ret = 0;
		ret = stricmp_utf8(i1->album, i2->album);
		return ret;
	}


	void t_track::set_from_metadb_handle(const metadb_handle_ptr & ptr, const metadb_handle_ptr & p_original_file, const t_field_mappings & p_mappings)
	{
		file_info_impl info;
		if (ptr->get_info_async(info))
		{
			set_from_metadb_handle(p_original_file, ptr->get_location(), info, ptr->get_filestats(), p_mappings);
		}
	}

	void t_track::set_from_metadb_handle(const metadb_handle_ptr & p_original_file, const playable_location & ptr, const file_info & info, const t_filestats & stats, const t_field_mappings & p_mappings)
	{
		//const file_info * info = NULL;
		//if (ptr.get_info_async_locked(info))
		{
			if (info.meta_get_count() || !title_valid || !title.length())
			{
				title_valid = p_mappings.get_title(p_original_file, info, title) && title.length();;
				if (!title_valid)
				{
					title_valid = true;
					title = pfc::string_filename(ptr.get_path());
				}
			}

			pfc::string_extension ext(ptr.get_path());
			t_uint8 mp3 = stricmp_utf8(ext, "mp3") ? 0 : 1;
			bool mp4 = !stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a");

			if (info.meta_get_count())
			{
				artist_valid = p_mappings.get_artist(p_original_file, info, artist);
				sort_artist_valid = p_mappings.get_sort_artist(p_original_file, info, sort_artist) && strcmp(sort_artist, artist);
				album_valid = p_mappings.get_album(p_original_file, info, album);
				sort_album_valid = p_mappings.get_sort_album(p_original_file, info, sort_album) && strcmp(sort_album, album);
				//title_valid = g_print_meta(info, "TITLE", title);
				sort_title_valid = p_mappings.get_sort_title(p_original_file, info, sort_title) && strcmp(sort_title, title);
				composer_valid = p_mappings.get_composer(p_original_file, info, composer);
				sort_composer_valid = p_mappings.get_sort_composer(p_original_file, info, sort_composer) && strcmp(sort_composer, composer);
				category_valid = g_print_meta(info, "CATEGORY", category);
				album_artist_valid = p_mappings.get_album_artist(p_original_file, info, album_artist);
				//g_print_meta(info, "ALBUM ARTIST", album_artist);
				sort_album_artist_valid = p_mappings.get_sort_album_artist(p_original_file, info, sort_album_artist) && strcmp(sort_album_artist, album_artist);
				comment_valid = p_mappings.get_comment(p_original_file, info, comment);//g_print_meta(info, "COMMENT", comment);
				eq_settings_valid = g_print_meta(info, "EQ PRESET", eq_settings);
				description_valid = g_print_meta(info, "DESCRIPTION", description);// || g_print_meta(info, "COMMENT", description);
				grouping_valid = g_print_meta(info, "GROUPING", grouping);
				podcast_enclosure_url_valid = g_print_meta(info, "PODCAST ENCLOSURE URL", podcast_enclosure_url);
				podcast_rss_url_valid = g_print_meta(info, "PODCAST RSS URL", podcast_rss_url);
				genre_valid = p_mappings.get_genre(p_original_file, info, genre);
				year = g_print_meta_int_n(info, "DATE", 4);
				tracknumber = g_print_meta_int(info, "TRACKNUMBER");
				{
					if (info.meta_exists("RELEASE DATE"))
						datereleased = apple_time_from_filetime(g_iso_timestamp_string_to_filetimestamp(info.meta_get("RELEASE DATE", 0), true), true);
					else if (info.meta_exists("DATE"))
						datereleased = apple_time_from_filetime(g_iso_timestamp_string_to_filetimestamp(info.meta_get("DATE", 0), true), true);
				}
				{
					if (p_original_file.is_valid() && p_original_file->is_info_loaded_async() && p_mappings.m_media_library.bsearch_by_pointer(p_original_file) != pfc_infinite)
					{
						pfc::string8 temp;
						titleformat_object::ptr to;
						static_api_ptr_t<titleformat_compiler>()->compile_force(to, "[%rating%]");
						p_original_file->format_title_from_external_info(info, NULL, temp, to, NULL);
						if (!temp.is_empty())
							rating = mmh::strtoul_n(temp.get_ptr(), pfc_infinite) * 20;

						static_api_ptr_t<titleformat_compiler>()->compile_force(to, "[%play_count%]");
						p_original_file->format_title_from_external_info(info, NULL, temp, to, NULL);
						if (temp.length())
							play_count_user = mmh::strtoul_n(temp.get_ptr(), pfc_infinite);

						static_api_ptr_t<titleformat_compiler>()->compile_force(to, "[%last_played_timestamp%]");
						p_original_file->format_title_from_external_info(info, NULL, temp, to, NULL);
						if (temp.length())
							lastplayedtime = apple_time_from_filetime(mmh::strtoul64_n(temp.get_ptr(), pfc_infinite));
					}
					else
						rating = g_print_meta_int(info, "RATING");
				}
				bpm = g_print_meta_int(info, "BPM");
				totaltracks = g_print_meta_int(info, "TOTALTRACKS");
				discnumber = g_print_meta_int(info, "DISCNUMBER");
				totaldiscs = g_print_meta_int(info, "TOTALDISCS");
				subtitle_valid = g_print_meta(info, "SUBTITLE", subtitle);
				tv_network_valid = g_print_meta(info, "NETWORK", tv_network);
				extended_content_rating_valid = g_print_meta_single(info, "ITUNEXTC", extended_content_rating);
				show_valid = g_print_meta(info, "SHOW", show);
				sort_show_valid = p_mappings.allow_sort_fields && g_print_meta(info, "SHOWSORTORDER", sort_show) && strcmp(sort_show, show);
				episode_valid = g_print_meta(info, "EPISODEID", episode);
				season_number = g_print_meta_int(info, "SEASONNUMBER");
				episode_sort_id = g_print_meta_int(info, "EPISODENUMBER");

				bool id3v2 = false;
				if (mp3 && info.info_exists("tagtype"))
				{
					const char * tagtype = info.info_get("tagtype");
					const char * ptrtagtype = tagtype;
					while (*ptrtagtype)
					{
						const char * start = ptrtagtype;
						while (*ptrtagtype && *ptrtagtype != '|') ptrtagtype++;
						if (ptrtagtype > start && !stricmp_utf8_ex("id3v2", pfc_infinite, start, min(ptrtagtype - start, 5)))
						{
							id3v2 = true;
							break;
						}
						while (*ptrtagtype == '|') ptrtagtype++;
					}

				}

				lyrics_flag = ((id3v2 && info.meta_exists("UNSYNCED LYRICS")) || (mp4 && info.meta_exists("LYRICS"))) ? 1 : 0;
				if (lyrics_flag && !lyrics_checksum)
					lyrics_checksum = ~0;
			}

			bitrate = (t_uint32)info.info_get_bitrate();
			const auto sample_rate_raw = info.info_get_int("samplerate");
			samplerate = t_uint32(sample_rate_raw)*0x10000;
			samplerate_float = static_cast<float>(sample_rate_raw);
			length = round_float(info.get_length()*1000.0);
			channel_count = t_uint16(info.info_get_int("channels"));

			if (!filetype_valid && info.info_exists("codec"))
			{
				const char * codec = info.info_get("codec");
				filetype_valid = true;
				if (!stricmp_utf8(codec, "MP3"))
					filetype = "MPEG audio file";
				else if (!stricmp_utf8(codec, "AAC"))
					filetype = "AAC audio file";
				else if (!stricmp_utf8(codec, "ALAC"))
					filetype = "Apple Lossless audio file";
				else if (!stricmp_utf8(ext, "WAV"))
					filetype = "WAV audio file";
				else
					filetype_valid = false;
			}

			bool m4b = !stricmp_utf8(ext, "m4b");
			bool aa = !stricmp_utf8(ext, "aa");

			if (m4b || aa)
			{
				media_type = t_track::type_audiobook;
			}

			{
				const char * kindstr = info.meta_get("MEDIA KIND", 0);
				const char * genrestr = info.meta_get("GENRE", 0);
				bool b_itunespodcast = info.meta_exists("iTUNESPODCAST");
				if (kindstr)
				{
					if (!stricmp_utf8(kindstr, "music"))
						media_type = t_track::type_audio;
					else if (!stricmp_utf8(kindstr, "movie"))
						media_type = t_track::type_video;
					else if (!stricmp_utf8(kindstr, "tv show"))
						media_type = t_track::type_tv_show;
					else if (!stricmp_utf8(kindstr, "music video"))
						media_type = t_track::type_music_video;
					else if (!stricmp_utf8(kindstr, "audiobook"))
						media_type = t_track::type_audiobook;
					else if (!stricmp_utf8(kindstr, "ringtone"))
						media_type = t_track::type_ringtone;
					else if (!stricmp_utf8(kindstr, "podcast"))
						media_type = video_flag ? t_track::type_video_podcast : t_track::type_podcast;

					media_type2 = media_type;
				}
				else if (genrestr && !stricmp_utf8(genrestr, "podcast") || b_itunespodcast)
				{
					media_type = video_flag ? t_track::type_video_podcast : t_track::type_podcast;
					media_type2 = media_type;
				}
			}
			if ((media_type & (t_track::type_video | t_track::type_tv_show | t_track::type_music_video)))
				video_flag = 1;
			else if ((media_type & (t_track::type_audiobook | t_track::type_audio | t_track::type_podcast | t_track::type_ringtone)))
				video_flag = 0;

			podcast_flag = (media_type & t_track::type_podcast) ? 1 : 0;

			if (stats.m_timestamp != filetimestamp_invalid)
				lastmodifiedtime = apple_time_from_filetime(stats.m_timestamp);
			file_size_32 = (t_uint32)stats.m_size;

			bool vbr =
				((info.info_exists("extrainfo") && !stricmp_utf8(info.info_get("extrainfo"), "VBR"))
					|| (info.info_exists("codec_profile") && strstr(info.info_get("codec_profile"), "VBR")))
				? true : false;
			type1 = (t_uint8(mp3 && vbr ? 1 : 0));
			type2 = (t_uint8(mp3));

			bool aac = info.info_exists("codec") && !stricmp_utf8(info.info_get("codec"), "AAC");
			unk9 = (aac || mp3) ? 0xffff : ((media_type & t_track::type_audiobook) ? 0x1 : 0);
			audio_format = mp3 ? 0xc : (aac ? 0x33 : ((m4b || aa) ? 0x29 : 0));

			if (info.meta_exists("IPOD_REMEMBER_PLAYBACK_POSITION") && !(media_type & t_track::type_audiobook))
				remember_playback_position = _atoi64(info.meta_get("IPOD_REMEMBER_PLAYBACK_POSITION", 0)) != 0;
			else if ((media_type & t_track::type_audiobook) && !(m4b || aa))
				remember_playback_position = true;
			else if (podcast_flag)
				remember_playback_position = true;
			else if (media_type & (t_track::type_tv_show | t_track::type_video))
				remember_playback_position = true;

			if (info.meta_exists("IPOD_SKIP_WHEN_SHUFFLING") && !(media_type & t_track::type_audiobook))
				skip_on_shuffle = _atoi64(info.meta_get("IPOD_SKIP_WHEN_SHUFFLING", 0)) != 0;
			else if ((media_type & t_track::type_audiobook) && !(m4b || aa))
				skip_on_shuffle = true;
			else if (podcast_flag)
				skip_on_shuffle = true;

			pfc::string8 comp_str;
			p_mappings.get_compilation(p_original_file, info, comp_str);
			is_compilation = _atoi64(comp_str.get_ptr()) != 0;

			bool b_valid = true;
			float db_adjustment = 0;
			if (p_mappings.soundcheck_rgmode == 0 && info.get_replaygain().is_track_gain_present())
				db_adjustment = info.get_replaygain().m_track_gain;
			else if (info.get_replaygain().is_album_gain_present())
				db_adjustment = info.get_replaygain().m_album_gain;
			else if (p_mappings.soundcheck_rgmode == 1 && info.get_replaygain().is_track_gain_present())
				db_adjustment = info.get_replaygain().m_track_gain;
			else b_valid = false;

#if 0
			if (info.get_replaygain().is_album_gain_present())
				db_adjustment = info.get_replaygain().m_album_gain;
			else if (info.get_replaygain().is_track_gain_present())
				db_adjustment = info.get_replaygain().m_track_gain;
			else b_valid = false;
#endif
			if (b_valid)
			{
				db_adjustment += p_mappings.soundcheck_adjustment;
				float scheck_float = 1000.0f * pow(10.0f, (-0.1f * db_adjustment));
				//scheck_float = scheck_float >= -0.5f ? scheck_float + 0.5f : scheck_float - 0.5f;
				volume_normalisation_energy = pfc::rint32(scheck_float);
			}

			samplecount = (t_uint64)info.info_get_length_samples();

			{
				bool b_use_ipod_sorting = p_mappings.use_ipod_sorting;
				//bool b_preserve_sort_fields = podcast_flag != 0;

				//if (!b_preserve_sort_fields)
				{
					if (b_use_ipod_sorting && !sort_title_valid && title_valid && g_test_sort_string(title))
					{
						g_get_sort_string_for_ipod(title.get_ptr(), sort_title, true);
						sort_title_valid = true;
					}
					//else sort_title_valid = false;

					if (b_use_ipod_sorting && !sort_album_valid && album_valid && g_test_sort_string(album))
					{
						g_get_sort_string_for_ipod(album.get_ptr(), sort_album, true);
						sort_album_valid = true;
					}
					//else sort_album_valid = false;

					if (b_use_ipod_sorting && !sort_artist_valid && artist_valid && g_test_sort_string(artist))
					{
						g_get_sort_string_for_ipod(artist.get_ptr(), sort_artist, true);
						sort_artist_valid = true;
					}
					//else sort_artist_valid = false;

					if (b_use_ipod_sorting && !sort_album_artist_valid && album_artist_valid && g_test_sort_string(album_artist))
					{
						g_get_sort_string_for_ipod(album_artist.get_ptr(), sort_album_artist, true);
						sort_album_artist_valid = true;
					}
					//else sort_album_artist_valid = false;

					if (b_use_ipod_sorting && !sort_composer_valid && composer_valid && g_test_sort_string(composer))
					{
						g_get_sort_string_for_ipod(composer.get_ptr(), sort_composer, true);
						sort_composer_valid = true;
					}
					//else sort_composer_valid = false;

					if (b_use_ipod_sorting && !sort_show_valid && show_valid && g_test_sort_string(show))
					{
						g_get_sort_string_for_ipod(show.get_ptr(), sort_show, true);
						sort_show_valid = true;
					}
					//else sort_show_valid = false;
				}
			}
		}


	}
}