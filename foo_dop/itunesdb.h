#ifndef _ITUNESDB_H_DOP_
#define _ITUNESDB_H_DOP_

#include "config.h"

//#define LOAD_LIBRARY_INDICES

#define hm(a,b) \
	((#@a)|(#@b)<<8|'h'<<16|'m'<<24)

#define hm2(a,b,c,d) \
	((#@a)|(#@b)<<8|(#@c)<<16|(#@d)<<24)

#define hm3(a,b,c,d) \
	const t_uint32 a##b##c##d = ((#@d)|(#@c)<<8|(#@b)<<16|(#@a)<<24)

#define hm4(a,b,c,d) \
	((#@d)|(#@c)<<8|(#@b)<<16|(#@a)<<24)

t_uint32 apple_time_from_filetime(t_filetimestamp filetime, bool b_local = true);
t_filetimestamp filetime_time_from_appletime(t_uint32 appletime, bool b_convert_to_utc = true);

bool g_print_meta(const file_info & info, const char * field, pfc::string_base & p_out);
bool g_print_meta_noblanks(const file_info & info, const char * field, pfc::string_base & p_out);
t_uint32 g_print_meta_int(const file_info & info, const char * field);

class t_field_mappings
{
public:
	pfc::string8
		artist_mapping,
		album_artist_mapping,
		album,
		title,
		composer,
		genre,
		compilation,
		comment,
		sort_artist_mapping,
		sort_album_artist_mapping,
		sort_title_mapping,
		sort_album_mapping,
		sort_composer_mapping,
		voiceover_title_mapping,
		conversion_temp_files_folder
		;
	t_int32 soundcheck_adjustment;
	pfc::string8 conversion_command,conversion_extension,conversion_parameters;
	pfc::string8 artwork_sources;
	t_size reserved_diskspace;

	t_size extra_filename_characters;
	t_size soundcheck_rgmode;
	bool check_video;
	bool use_ipod_sorting;
	bool add_artwork, scan_gapless, convert_files, numbers_last, use_fb2k_artwork, conversion_use_bitrate_limit;
	bool conversion_use_custom_thread_count;
	unsigned conversion_custom_thread_count, conversion_bitrate_limit;
	t_uint8 replaygain_processing_mode;
	bool use_dummy_gapless_data, quiet_sync, video_thumbnailer_enabled, allow_sort_fields, sort_playlists;
	//bool check_if_files_changed;
	unsigned date_added_mode;
	bool sort_ipod_library_playlist;
	pfc::string8 ipod_library_sort_script;

	metadb_handle_list_t<pfc::alloc_fast_aggressive> m_media_library;

	settings::conversion_preset_t m_conversion_encoder;

	bool get_field(const metadb_handle_ptr & ptr, const file_info & info, const char * format, const char * field, pfc::string_base & p_out) const;
	bool get_sort_field(const metadb_handle_ptr & ptr, const file_info & info, const pfc::string8 & std_format, const pfc::string8 & sort_format, const char * field, pfc::string_base & p_out) const;

	bool get_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_sort_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_sort_album_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_album_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_album(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_sort_album(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_title(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_sort_title(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_genre(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_composer(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_sort_composer(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_comment(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	bool get_compilation(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const;
	t_field_mappings();
};

class itunesprefs 
{
public:
	bool m_voiceover_enabled;

	itunesprefs() : m_voiceover_enabled(true) {};
};

namespace shareddb
{
	template <const t_uint32 t_identifier>
	class t_header_marker_v1
	{
	public:
		t_uint32 identifier;
		t_uint32 header_size;
		/** Interpretation depends on section */
		t_uint32 section_size;
		pfc::array_t<t_uint8> data;

		void verify_identifier()
		{
			if (t_identifier != identifier)
			{
				t_uint32 wantu = t_identifier;
				pfc::string8 want((char*)(&wantu), 4);
				pfc::stringcvt::string_utf8_from_codepage got(pfc::stringcvt::codepage_iso_8859_1, (char*)(&identifier), 4);
				throw exception_io_unsupported_format(pfc::string8() << "Invalid format; expected header marker \"" << want.get_ptr() << "\" got \"" << got.get_ptr() << "\"");
			}
		}
	};
	template <const t_uint32 t_identifier>
	class t_header_marker
	{
	public:
		t_uint32 identifier;
		t_uint32 header_size;
		/** Interpretation depends on section */
		t_uint32 section_size;
		class memblock_ref_t
		{
		public:
			t_uint8 * m_data;
			t_size m_size;

			memblock_ref_t() : m_data(NULL), m_size(NULL) {};

			t_uint8 * get_ptr() const {return m_data;}
			t_size get_size() const {return m_size;}

			//t_uint8 operator [](t_size index) {return m_data[index];}
		};
		memblock_ref_t data;

		void verify_identifier()
		{
			if (t_identifier != identifier)
			{
				t_uint32 wantu = t_identifier;
				pfc::string8 want((char*)(&wantu), 4);
				pfc::stringcvt::string_utf8_from_codepage got(pfc::stringcvt::codepage_iso_8859_1, (char*)(&identifier), 4);
				throw exception_io_unsupported_format(pfc::string8() << "Invalid format; expected header marker \"" << want.get_ptr() << "\" got \"" << got.get_ptr() << "\"");
			}
		}
	};
};

namespace itunesdb
{
	class chapter_entry 
	{
	public:
		t_uint32 m_start_position; /** ms */
		pfc::string8 m_title, m_url_title, m_url;

		bool m_ploc_valid;
		t_uint32 m_ploc1, m_ploc2, m_ploc3, m_ploc4;

		pfc::array_t<t_uint8> m_image_data;

		chapter_entry() : m_start_position(0), m_ploc1(0), m_ploc2(0), m_ploc3(0), m_ploc4(0), m_ploc_valid(false) {};

		static int g_compare_position_value(const chapter_entry & p1, t_uint32 p2) {return pfc::compare_t(p1.m_start_position, p2);}
	};

	class chapter_list : public pfc::list_t<chapter_entry>
	{
	public:
		t_uint32 m_hedr_1, m_hedr_2;
		chapter_list() : m_hedr_1(0), m_hedr_2(0) {};
	};

	using namespace shareddb;
	namespace identifiers
	{
		const t_uint32
			dbhm = hm(d,b),
			tihm = hm(t,i),
			dshm = hm(d,s),
			pyhm = hm(p,y),
			pihm = hm(p,i),
			plhm = hm(p,l),
			dohm = hm(d,o),
			tlhm = hm(t,l),
			ophm = hm(o,p),
			pdhm = hm(p,d),
			alhm = hm(a,l),
			aihm = hm(a,i),
			ilhm = hm(i,l),
			iihm = hm(i,i);
	}

	namespace ai_types
	{
		enum {
		song = 2, //and movies
		podcast = 3,
		tv_show = 4
		//5 == ?
		};
	};

	enum t_dataset_type
	{
		dataset_tracklist = 1,
		dataset_playlistlist = 2,
		dataset_playlistlist_v2 = 3,
		dataset_albumlist = 4,
		dataset_specialplaylists = 5,
		dataset_tracklist2 = 6,
		dataset_artistlist = 8,
		dataset_genius_cuid = 9,
		dataset_tracklist3 = 0xa,
	};

	namespace do_types
	{
		enum t_type
		{
			title=1,
			location=2,
			album=3,
			artist=4,
			genre=5,
			filetype=6,
			eq_setting=7, //eq_preset
			comment=8,
			category=9,
			radio_stream_status=10,
			composer=12,
			grouping=13,
			description=14, //description_long

			podcast_enclosure_url=15,
			podcast_rss_url=16,

			chapter_data=17,
			subtitle=18, //description
			show=19,
			episode_number=20, //episode_id
			tv_network=21,

			album_artist=22,
			sort_artist=23,
			keywords=24, //podcast_feed_keywords

			extended_content_rating=25,
			movie_info=26,

			sort_title=27,
			sort_album=28,
			sort_album_artist=29,
			sort_composer=30,
			sort_show=31,
			video_characteristics=32,
			publication_id=37,

			copyright=39,
			store_data=40, //bplist
			collection_description=41,

			smart_playlist_data = 50,
			smart_playlist_rules = 51,
			library_index = 52,
			library_index_letters = 53,
			itunes_columns_info=100,
			itunes_data_102=102,
			position=100,

			album_list_album = 200,
			album_list_artist = 201, //album artist, falling back to artist
			album_list_album_artist = 202, //album artist only
			album_list_podcast_url = 203,
			album_list_show = 204,

			artist_list_artist = 300,
			artist_list_sort_artist = 301,

			playilst_item_podcast_title=1,
			playilst_item_podcast_sort_title=2,
		};
	};

	namespace library_index_types
	{
		enum t_type
		{
			title = 0x03,
			album_disc_tracknumber_title = 0x04,
			artist_album_disc_tracknumber_title = 0x05,
			genre_artist_album_disc_tracknumber_title = 0x07,
			composer_title = 0x12,
			show_episode_1=29,
			show_episode_2=30,
			episode=31,
			albumartist_artist_album_disc_tracknumber_title = 35,
			artist_album_disc_tracknumber = 36
		};
	}

	class t_album : public pfc::refcounted_object_root
	{
	public:
		typedef t_album our_type;
		typedef pfc::refcounted_object_ptr_t<our_type> ptr;

		t_uint32 id; //16
		t_uint64 pid; //20
		t_uint8 kind; //28
		t_uint8 all_compilations;
		t_uint8 artwork_status;
		t_uint8 unk1;
		t_uint64 artwork_item_pid; //32
		t_uint32 user_rating; //40 
		t_uint32 season_number; //44

		t_uint64 artist_pid;

		t_uint64 temp_track_pid;
		t_uint32 temp_media_type;

		bool artist_valid;
		bool album_valid;
		bool podcast_url_valid;
		bool show_valid;
		bool album_artist_strict_valid;

		pfc::string8 artist,
			album_artist_strict,
			album,
			podcast_url,
			show;

		t_album() : id(0), pid(0), kind(0), all_compilations(0), artwork_status(0), unk1(0),
			artwork_item_pid(0), season_number(0), user_rating(0),
			artist_pid(NULL), temp_track_pid(NULL), temp_media_type(0),
			artist_valid(false), album_valid(false), podcast_url_valid(false), show_valid(false), album_artist_strict_valid(false) {};

		static int g_compare_album_mixed(const ptr & i1, const ptr & i2);
		static int g_compare_album_mixed_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);
		static int g_compare_song(const ptr & i1, const ptr & i2)
		{
			int ret = 0;
			ret = stricmp_utf8(i1->artist, i2->artist);
			if (!ret)
				ret = stricmp_utf8(i1->album, i2->album);
			if (!ret)
				ret = pfc::compare_t(i1->temp_track_pid, i2->temp_track_pid);
			return ret;
		}
		static int g_compare_show(const ptr & i1, const ptr & i2)
		{
			int ret = 0;
			ret = stricmp_utf8(i1->show, i2->show);
			if (!ret)
				ret = pfc::compare_t(i1->season_number, i2->season_number);
			return ret;
		}
		static int g_compare_podcast(const ptr & i1, const ptr & i2)
		{
			int ret = 0;
			ret = stricmp_utf8(i1->podcast_url, i2->podcast_url);
			return ret;
		}
		static int g_compare_id(const ptr & i1, const ptr & i2)
		{
			return pfc::compare_t(i1->id, i2->id);
		}
		static int g_compare_id_value(const ptr & i1, const t_uint32 & i2)
		{
			return pfc::compare_t(i1->id, i2);
		}
		static int g_compare_pid(const ptr & i1, const ptr & i2)
		{
			return pfc::compare_t(i1->pid, i2->pid);
		}
		static int g_compare_pid_value(const ptr & i1, const t_uint64 & i2)
		{
			return pfc::compare_t(i1->pid, i2);
		}

		static int g_compare_artist_pid(const ptr & i1, const ptr & i2)
		{
			return pfc::compare_t(i1->artist_pid, i2->artist_pid);
		}
		static int g_compare_artist_pid_value(const ptr & i1, const t_uint64 & i2)
		{
			return pfc::compare_t(i1->artist_pid, i2);
		}

		static int g_compare_song_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);
		static int g_compare_other_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);
		static int g_compare_show_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);
		static int g_compare_podcast_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);
	};

	class t_artist : public pfc::refcounted_object_root
	{
	public:
		typedef t_artist our_type;
		typedef pfc::refcounted_object_ptr_t<our_type> ptr;

		t_uint32 id;
		t_uint64 pid;
		t_uint8 type;
		t_uint8 artwork_status;
		t_uint8 unk1;
		t_uint8 unk2;
		t_uint64 artwork_album_pid;

		t_uint32 temp_season_number;

		bool artist_valid;
		bool sort_artist_valid;

		pfc::string8 artist,
			sort_artist, temp_show;

		t_uint64 temp_track_pid;

		t_artist() : id(0), pid(0), type(0), unk1(0), unk2(0), artwork_status(0),artwork_album_pid(0),
			artist_valid(false), sort_artist_valid(false), temp_season_number(0), temp_track_pid(0) {};

		static int g_compare_id(const ptr & i1, const ptr & i2)
		{
			return pfc::compare_t(i1->id, i2->id);
		}
		static int g_compare_id_value(const ptr & i1, const t_uint32 & i2)
		{
			return pfc::compare_t(i1->id, i2);
		}
		static int g_compare_pid(const ptr & i1, const ptr & i2)
		{
			return pfc::compare_t(i1->pid, i2->pid);
		}
		static int g_compare_standard(const ptr & i1, const ptr & i2)
		{
			int ret = 0;
			ret = stricmp_utf8(i1->artist, i2->artist);
			if (ret == 0)
				ret = stricmp_utf8(i1->temp_show, i2->temp_show);
			if (ret == 0)
				ret = pfc::compare_t(i1->temp_season_number, i2->temp_season_number);
			return ret;
		}
		static int g_compare_standard_track(const ptr & i1, const pfc::rcptr_t<class t_track> & i2);

		static int g_compare_album (const ptr & i1, const t_album::ptr & i2)
		{
			int ret = 0;
			ret = stricmp_utf8(i1->artist, i2->album_artist_strict_valid ? i2->album_artist_strict : i2->artist);
			return ret;
		}
	};

	class t_album_list
	{
	public:
		pfc::list_t<t_album::ptr> m_normal;
		pfc::list_t<t_album::ptr> m_tv_shows;
		pfc::list_t<t_album::ptr> m_podcasts;
		pfc::list_t<t_album::ptr> m_master_list;

		bool have_pid(t_uint64 pid)
		{
			t_size i, count = m_master_list.get_count();
			for (i=0; i<count; i++)
				if (m_master_list[i]->pid == pid) return true;
			return false;
		}
	};

	class t_artist_list : public pfc::list_t<t_artist::ptr>
	{
	public:
		bool have_pid(t_uint64 pid)
		{
			t_size i, count = get_count();
			for (i=0; i<count; i++)
				if ((*this)[i]->pid == pid) return true;
			return false;
		}
	};

	class t_video_characteristics
	{
	public:
		t_uint32 height;
		t_uint32 width;
		t_uint32 track_id;
		t_uint32 codec;
		t_uint32 percentage_encrypted; //*1000
		t_uint32 bit_rate;
		t_uint32 peak_bit_rate;
		t_uint32 buffer_size;
		t_uint32 profile;
		t_uint32 level;
		t_int32 complexity_level;
		t_uint32 frame_rate; //*1000
		t_uint32 unk1;
		t_uint32 unk2[7];

		t_video_characteristics() : height(0), width(0), track_id(0), codec(0), percentage_encrypted(0), bit_rate(0),
			peak_bit_rate(0), buffer_size(0), profile(0), level(0), complexity_level(0), frame_rate(0), unk1(0)
		{
			memset(&unk2, 0, sizeof(unk2));
		}
	};

	class t_track
	{
	public:
		typedef pfc::rcptr_t<t_track> ptr;

		t_uint32 id;
		t_uint32 visible;
		t_uint32 tracknumber;
		t_uint32 totaltracks;
		t_uint32 year;
		t_uint8 type1;
		t_uint8 type2;
		t_uint8 compilation;
		t_uint8 rating;

		t_uint32 size; //bytes
		t_uint32 length; //ms - corrected for gapless?

		t_uint32 bitrate;
		t_uint16 unk8; //*0x10000
		t_uint16 samplerate; //*0x10000
		t_uint32 volume;
		t_uint32 starttime;
		t_uint32 stoptime;

		t_uint32 discnumber;
		t_uint32 totaldiscs;

		t_uint32 lastmodifiedtime; //number of seconds since 00:00:00 UTC, January 1, 1904.
		t_uint32 userid;
		t_uint32 playcount;
		t_uint32 playcount2;
		t_uint32 lastplayedtime;
		t_uint32 bookmarktime;
		t_uint32 datereleased;
		t_uint32 dateadded;
		t_uint32 soundcheck;

		t_uint64 pid;
		t_uint8 checked;
		t_uint8 application_rating;
		t_uint16 bpm;
		t_uint16 artwork_count;
		t_uint32 artwork_size;

		t_uint16 unk9;
		t_uint32 unk11;
		t_uint16 audio_format;
		t_uint8 content_rating, unk12;
		t_uint32 store_drm_key_versions;

		t_uint32 skip_count_user;
		t_uint32 skip_count_recent;
		t_uint32 last_skipped;

		enum t_artwork_flag
		{
			has_artwork = 1,
			no_artwork = 2
		};
		t_uint8 artwork_flag;
		t_uint8 skip_on_shuffle;
		t_uint8 remember_playback_position;
		t_uint8 podcast_flag;

		t_uint64 dbid2;

		t_uint8 lyrics_flag;
		t_uint8 video_flag;
		t_uint8 played_marker;
		t_uint8 unk37;

		t_uint32 bookmark_time_ms_common;
		t_uint32 encoder_delay;
		t_uint64 samplecount;
		//t_uint32 unk24;
		t_uint32 lyrics_crc;
		t_uint32 encoder_padding;
		t_uint32 gapless_heuristic_info; // 1 if gapless accurate, 0x02000003 if not, 0 if no gapless info present

		enum t_media_type
		{
			//type_audio_video = 0,
			type_audio = 1<<0,
			type_video = 1<<1,  //movie
			type_podcast = 1<<2,
			type_video_podcast = type_podcast|type_video,
			type_audiobook = 1<<3,
			type_music_video = 1<<5,
			type_tv_show = 1<<6,
			type_ringtone = 1<<14,
			type_rental = 1<<15,
			type_digital_booklet = 1<<16,
			type_is_voice_memo = 1<<20,
			type_itunes_u = 1<<21,
			type_book = (1<<22)|(1<<23), //either, difference? EPUB, PDF, ...?
		};
		t_uint32 media_type;
		t_uint32 season_number;
		t_uint32 episode_number;
		t_uint32 date_purchased;
		t_uint32 unk32;
		t_uint32 unk33;
		t_uint32 unk34;
		t_uint32 unk35;
		t_uint32 unk36;


		t_uint32 unk40;
		t_uint64 resync_frame_offset; //64-bit ?
		t_uint16 unk43_1; //default 1
		t_uint8 gapless_album;
		t_uint8 unk43_2;
		t_uint8 unk44[20];
		//t_uint32 unk45;
		//t_uint32 unk46;
		//t_uint32 unk47;
		//t_uint32 unk48;
		t_uint32 unk49;
		t_uint32 unk50;
		t_uint32 album_id;
		t_uint32 unk52;
		t_uint32 unk53;
		t_uint64 filesize_64;
		t_uint32 unk56;
		t_uint32 unk57;
		t_uint32 unk58;
		t_uint32 unk59;
		t_uint32 unk60;
		t_uint8 is_self_contained;
		t_uint8 is_compressed;
		t_uint8 unk61_1;
		t_uint8 unk61_2;
		t_uint32 unk62;
		t_uint32 unk63;
		t_uint32 unk64;
		t_uint32 audio_fingerprint;
		t_uint32 unk66;
		t_uint32 mhii_id;
		t_uint32 unk68;
		t_uint8 unk69_1;
		t_uint8 unk69_2;
		t_uint8 unk69_3;
		t_uint8 is_anamorphic;
		t_uint32 unk70;
		t_uint8 unk71_1;
		t_uint8 unk71_2;
		t_uint8 has_alternate_audio_and_closed_captions;
		t_uint8 has_subtitles;
		t_uint16 audio_language;
		t_uint16 audio_track_index;
		t_uint32 audio_track_id;
		t_uint16 subtitle_language;
		t_uint16 subtitle_track_index;
		t_uint32 subtitle_track_id;

		t_uint32 unk76;
		t_uint32 unk77;
		t_uint32 unk78;
		t_uint32 unk79;
		t_uint8 characteristics_valid;
		t_uint8 unk80_1;
		t_uint8 is_hd;
		t_uint8 store_kind;
		t_uint64 account_id_primary;
		t_uint64 account_id_secondary;
		t_uint64 key_id, key_id2;

		t_uint64 store_item_id;
		t_uint64 store_genre_id;
		t_uint64 store_artist_id;
		t_uint64 store_composer_id;
		t_uint64 store_playlist_id;
		t_uint64 store_front_id;
		t_uint32 artist_id;

		t_uint64 genius_id;
		t_uint32 key_platform_id;
		t_uint32 unk100;
		t_uint32 unk101;
		t_uint32 media_type2;
		t_uint32 unk102;
		t_uint32 unk103;
		t_uint32 unk104;
		t_uint32 unk105;
		t_uint8 unk106_1;
		t_uint8 chosen_by_auto_fill;
		t_uint8 unk106_3;
		t_uint8 unk106_4;
		t_uint32 unk107;
		t_uint32 unk108;
		t_uint32 unk109[10];

		bool sort_artist_valid;
		pfc::string8 sort_artist;
		bool sort_album_artist_valid;
		pfc::string8 sort_album_artist;
		bool sort_album_valid;
		pfc::string8 sort_album;
		bool sort_show_valid;
		pfc::string8 sort_show;
		bool sort_title_valid;
		pfc::string8 sort_title;
		bool sort_composer_valid;
		pfc::string8 sort_composer;

		bool title_valid;
		pfc::string8 title;
		bool artist_valid;
		pfc::string8 artist;
		bool album_valid;
		pfc::string8 album;
		bool genre_valid;
		pfc::string8 genre;
		bool location_valid;
		pfc::string8 location;

		bool composer_valid;
		pfc::string8 composer;

		bool comment_valid;
		pfc::string8 comment;
		bool category_valid;
		pfc::string8 category;
		bool grouping_valid;
		pfc::string8 grouping;
		bool description_valid;
		pfc::string8 description;
		bool collection_description_valid;
		pfc::string8 collection_description;
		bool publication_id_valid;
		pfc::string8 publication_id;

		bool eq_settings_valid;
		pfc::string8 eq_settings;

		bool podcast_rss_url_valid;
		pfc::string8 podcast_rss_url;
		bool podcast_enclosure_url_valid;
		pfc::string8 podcast_enclosure_url;

		bool filetype_valid;
		pfc::string8 filetype;
		bool subtitle_valid;
		pfc::string8 subtitle;

		bool show_valid;
		pfc::string8 show;
		bool episode_valid;
		pfc::string8 episode;
		bool tv_network_valid;
		pfc::string8 tv_network;

		bool extended_content_rating_valid;
		pfc::string8 extended_content_rating;

		bool album_artist_valid;
		pfc::string8 album_artist;
		bool keywords_valid;
		pfc::string8 keywords;
		bool copyright_valid;
		pfc::string8 copyright;

		bool chapter_data_valid, store_data_valid;
		itunesdb::chapter_list m_chapter_list;
		//pfc::array_t<t_uint8> dohm_chapter_data;
		pfc::array_t<t_uint8> do_chapter_data, do_store_data;

		pfc::list_t<t_video_characteristics> video_characteristics_entries;

		/** Dop data start */
		pfc::string8 original_path,
			last_known_path;

		bool original_filesize_valid, original_timestamp_valid, transcoded;
		t_uint64 original_filesize, original_timestamp;

		t_size original_subsong;

		bool original_path_valid, last_known_path_valid, original_subsong_valid;

		t_filesize artwork_source_size;
		t_uint8 artwork_source_sha1[20];

		bool artwork_source_size_valid, artwork_source_sha1_valid;

		bool dshm_type_6, dshm_type_6_is_new;

		/** Run-time data */
		t_filestats m_runtime_filestats;

		t_track() : id(0), visible(1), tracknumber(0), year(0), rating(0), size(0), length(0), totaltracks(0),
			bitrate(0), samplerate(0), unk8(0), discnumber(0), totaldiscs(0), lastmodifiedtime(0), title_valid(false), 
			original_subsong(0), original_subsong_valid(false),
			album_valid(false), artist_valid(false), genre_valid(false), location_valid(false), pid(0),
			artwork_count(0), artwork_size(0), userid(0), playcount(0), playcount2(0), dateadded(0),
			lastplayedtime(0), bookmarktime(0), datereleased(0), store_drm_key_versions(0), composer_valid(false),
			soundcheck(0), unk9(0), audio_format(0), type1(0), type2(0), compilation(0),
			content_rating(0), unk12(0), skip_count_user(0), skip_count_recent(0), last_skipped(0), artwork_flag(no_artwork), skip_on_shuffle(0),
			remember_playback_position(0), podcast_flag(0), dbid2(0), lyrics_flag(0), video_flag(0),
			played_marker(2), unk37(0), bookmark_time_ms_common(0), encoder_delay(0), samplecount(0), /*unk24(0),*/ lyrics_crc(0),
			encoder_padding(0), gapless_heuristic_info(0), media_type(1), 
			season_number(0), episode_number(0), date_purchased(0), unk32(0), unk33(0), unk34(0), unk35(0),
			unk36(0), comment_valid(false), category_valid(false), grouping_valid(false),
			description_valid(false), podcast_rss_url_valid(false), podcast_enclosure_url_valid(false),
			chapter_data_valid(false), eq_settings_valid(false), starttime(0), stoptime(0),
			volume(0), unk11(0), checked(0), application_rating(0), bpm(0), filetype_valid(false),
			subtitle_valid(false), tv_network_valid(false), episode_valid(false), show_valid(false),
			unk40(0), resync_frame_offset(0), unk43_1(1), unk43_2(0), gapless_album(0),
			/*unk44(0), unk45(0), unk46(0), unk47(0), unk48(0),*/ unk49(0), album_id(0), 
			unk50(0), unk52(0), unk53(0), filesize_64(0), unk56(0), unk57(0), unk58(0), 
			unk59(0), unk60(0), dshm_type_6(false), dshm_type_6_is_new(false), extended_content_rating_valid(false),
			
			unk76(0),unk77(0),unk78(0),unk79(0),characteristics_valid(0),unk80_1(0),is_hd(0),store_kind(0),
			account_id_primary(0),account_id_secondary(0),
			key_id(0),key_id2(0),store_item_id(0),store_genre_id(0),
			store_composer_id(0),store_artist_id(0),store_playlist_id(0),store_front_id(0),

			artist_id(0),

			album_artist_valid(false), keywords_valid(false),
			original_path_valid(false), last_known_path_valid(false), original_filesize_valid(false),
			original_timestamp_valid(false), original_filesize(0), original_timestamp(0), transcoded(false),
			is_self_contained(0), is_compressed(0), unk61_1(0), unk61_2(0),
			unk62(0), unk63(0), unk64(0), audio_fingerprint(0), unk66(0), mhii_id(0), unk68(0),
			unk69_1(0x20), unk69_2(0), unk69_3(0), is_anamorphic(0), unk70(0), unk71_1(0), unk71_2(0), 
			has_alternate_audio_and_closed_captions(0), has_subtitles(0), audio_language(0), audio_track_index(0),
			audio_track_id(0), subtitle_language(0), subtitle_track_index(0), subtitle_track_id(0),
			sort_artist_valid(false), sort_album_artist_valid(false),
			sort_album_valid(false), sort_show_valid(false),
			sort_title_valid(false), sort_composer_valid(false), m_runtime_filestats(filestats_invalid),
			artwork_source_size_valid(false), artwork_source_sha1_valid(false),
			artwork_source_size(filesize_invalid),
			genius_id(0),
			key_platform_id(0),
			unk100(0),
			unk101(0),
			media_type2(1),
			unk102(0),
			unk103(0), unk104(0),unk105(0),unk106_1(0),chosen_by_auto_fill(0),unk106_3(0),unk106_4(0),unk107(0),unk108(0),copyright_valid(false),
			collection_description_valid(false), store_data_valid(false), publication_id_valid(false)
		{
			memset(unk44, 0,  sizeof(unk44));
			memset(unk109, 0,  sizeof(unk109));
			memset(artwork_source_sha1, 0,  sizeof(artwork_source_sha1));
		};
		metadb_handle_ptr create_source_handle() 
		{
			metadb_handle_ptr ret; 
			if (original_path_valid) 
			{
				static_api_ptr_t<metadb>()->handle_create(ret, make_playable_location(original_path, original_subsong));
			}
			return ret;
		}
		//void copy_to_file_info(file_info & p_out);
		//void set_from_metadb_handle_locked(const metadb_handle_ptr & ptr, const t_field_mappings & p_mappings);
		void set_from_metadb_handle(const metadb_handle_ptr & ptr, const metadb_handle_ptr & p_original_file, const t_field_mappings & p_mappings);
		void set_from_metadb_handle(const metadb_handle_ptr & p_original_file, const playable_location & ptr, const file_info & info, const t_filestats & stats, const t_field_mappings & p_mappings);
		//void set_from_metadb_handle_locked_v2(const metadb_handle_ptr & ptr, const chapter_list & p_chapters);
		void set_gapless_info(t_uint32 p_encoder_delay, t_uint32 p_encoder_padding, t_uint32 p_sync_frame_offset)
		{
			encoder_delay = p_encoder_delay;
			encoder_padding = p_encoder_padding;
			resync_frame_offset = p_sync_frame_offset;
			gapless_heuristic_info = 0x1;
		}
		t_filetimestamp get_timestamp()
		{
			t_uint64 diff = 303;
			diff *= 365 * 24 * 60 * 60;
			diff += 24 * 3 * 24 * 60 * 60;
			diff *= 10000000;
			t_filetimestamp ret;
			ret = 10000000 * (t_uint64)lastmodifiedtime  + diff;
			return ret;
		}

		static bool is_media_type_video(t_uint32 media_type)
		{
			return ((media_type & t_track::type_video) || (media_type & t_track::type_tv_show) || (media_type & t_track::type_music_video));
		}

		static int g_compare_album_id (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return pfc::compare_t(item1->album_id, item2->album_id);
		}
		static int g_compare_album_id_value (const pfc::rcptr_t<t_track> & item1, const t_uint32 item2)
		{
			return pfc::compare_t(item1->album_id, item2);
		}
		static int g_compare_artist_id (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return pfc::compare_t(item1->artist_id, item2->artist_id);
		}
		static int g_compare_artist_id_value (const pfc::rcptr_t<t_track> & item1, const t_uint32 item2)
		{
			return pfc::compare_t(item1->artist_id, item2);
		}
		static int g_compare_album (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return stricmp_utf8(item1->album, item2->album);
		}
		static int g_compare_podcast_group (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			int ret = stricmp_utf8(item1->album, item2->album);
			return ret;
		}
		static int g_compare_podcast (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			int ret = g_compare_podcast_group(item1, item2);
			if (ret == 0)
				ret = -pfc::compare_t(item1->datereleased, item2->datereleased);
			if (ret == 0)
				ret = -pfc::compare_t(item1->discnumber, item2->discnumber);
			if (ret == 0)
				ret = -pfc::compare_t(item1->tracknumber, item2->tracknumber);
			return ret;
		}
		static int g_compare_genre (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return stricmp_utf8(item1->genre, item2->genre);
		}
		static int g_compare_composer (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return stricmp_utf8(item1->composer, item2->composer);
		}
		static int g_compare_artist (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return stricmp_utf8(item1->artist, item2->artist);
		}
		static int g_compare_title (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return stricmp_utf8(item1->title, item2->title);
		}
		static int g_compare_date_added_descending (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return -pfc::compare_t(item1->dateadded, item2->dateadded);
		}
		static int g_compare_unsigned_pid (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return pfc::compare_t((t_uint64)item1->pid, (t_uint64)item2->pid);
		}
		static int g_compare_signed_pid (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return pfc::compare_t((t_int64)item1->pid, (t_int64)item2->pid);
		}
		static int g_compare_last_played_descending (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return -pfc::compare_t(item1->lastplayedtime, item2->lastplayedtime);
		}
		static int g_compare_play_count_descending (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return -pfc::compare_t(item1->playcount, item2->playcount);
		}
		static int g_compare_rating_descending (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
		{
			return -pfc::compare_t(item1->rating, item2->rating);
		}
	};

	class t_playlist_entry
	{
	public:
		t_uint8 unk0;
		t_uint8 is_podcast_group;
		t_uint8 is_podcast_group_expanded;
		t_uint8 podcast_group_name_flags; //0x80 = name only, 0x81 = name + sort name
		t_uint32 group_id;
		t_uint32 track_id;
		t_uint32 timestamp;
		/** Parent Group */
		t_uint32 podcast_group;
		t_uint32 unk1;
		t_uint32 unk2;
		t_uint64 item_pid;
		pfc::string8 podcast_title;
		bool podcast_title_valid;

		pfc::string8 podcast_sort_title;
		bool podcast_sort_title_valid;

		t_uint32 position;
		bool position_valid;

		static int g_compare_position(const t_playlist_entry & item1, const t_playlist_entry & item2)
		{
			return pfc::compare_t<t_uint32>(item1.position_valid? item1.position:0, item2.position_valid?item2.position:0);
		}

		t_playlist_entry()
			: unk0(0), is_podcast_group(0), is_podcast_group_expanded(0), podcast_group_name_flags(0),
			group_id(0), track_id(0), timestamp(0), podcast_group(0),
			podcast_title_valid(false), position(0), position_valid(false), podcast_sort_title_valid(false),
			unk1(0), unk2(0), item_pid(0)
		{};
	};

	//enum t_podcast_grouping_flag
	//{
		//normal=0,
		//podcast_group = 0x100
	//};
	namespace smart_playlist_fields
	{
		enum t_smart_playlist_fields
		{
			title=0x02, //string
			album=0x03, //string
			artist=0x04, //string
			bitrate=0x05, //integer
			sample_rate=0x06, //integer
			year=0x07, //integer
			genre=0x08, //string
			kind=0x09, //string
			date_modified=0x0a, //timestamp
			track_number=0x0b, //integer
			size=0x0c, //integer
			time=0x0d, //integer
			comment=0x0e, //string
			date_added=0x10, //timestamp
			composer=0x12, //string
			play_count=0x16, //integer
			last_played=0x17, //timestamp
			disc_number=0x18, //integer
			rating=0x19, //integer (multiply by 20 for stars/rating)
			compilation=0x1f, //integer
			bpm=0x23, //integer
			grouping=0x27, //string (see special note)
			playlist=0x28, //integer - the playlist id number (see special note)
			description=0x36, //string
			category=0x37, //string
			podcast=0x39, //integer
			video_kind=0x3c, //logic integer, works on mediatype
			tv_show=0x3e, //string
			season_number=0x3f, //integer
			skip_count=0x44, //integer
			last_skipped=0x45, //timestamp
			album_artist=0x47, //string
			sort_album=0x4f,
			sort_album_artist=0x51,
			sort_title=0x4e,
			sort_artist=0x50,
			sort_show=0x53,
			sort_composer=0x52,
		};
	}
	class t_smart_playlist_data
	{
	public:
		t_uint8 live_update;
		t_uint8 check_rules;
		t_uint8 check_limits;
		t_uint8 limit_type;
		t_uint8 limit_sort;
		t_uint8 unk1, unk2, unk3;
		t_uint32 limit_value;
		t_uint8 match_checked_only;
		t_uint8 reverse_limit_sort;
		t_uint8 unk4, unk5;
		t_smart_playlist_data():live_update(1), check_rules(1), check_limits(0), limit_type(3), limit_sort(2), unk1(0), unk2(0),
			unk3(0), limit_value(0), match_checked_only(0), reverse_limit_sort(0), unk4(0), unk5(0) {};
	};

	class t_smart_playlist_rule
	{
	public:
		t_uint32 field;
		t_uint32 action;
		t_uint32 child_ruleset_flag;
		t_uint32 unk0[10];
		pfc::string_simple_t<WCHAR> string;
		t_uint64 from_value;
		t_int64 from_date;
		t_uint64 from_units;
		t_uint64 to_value;
		t_int64 to_date;
		t_uint64 to_units;

		pfc::array_t<t_uint8> subrule;

		t_uint32 unk1,unk2,unk3,unk4,unk5;
		t_smart_playlist_rule() : field(0), action(0), from_value(0), from_date(0), from_units(1)
			, to_value(0), to_date(0), to_units(1), unk1(0), unk2(0), unk3(0), unk4(0), unk5(0),
			child_ruleset_flag(0) {memset(&unk0, 0, sizeof(unk0));};
	};

	class t_smart_playlist_rules
	{
	public:
		t_uint32 unk1;
		t_uint32 rule_operator;
		pfc::list_t<t_smart_playlist_rule> rules;
		t_smart_playlist_rules():unk1(0), rule_operator(0) {};
	};

	namespace playlist_sort_orders
	{
		enum playlist_sort_order_t //also smart limit sorts
		{
			manual=1,
			random=2,
			title=3,
			album=4,
			artist=5,
			bitrate=6,
			genre=7,
			kind=8,
			date_modified=9,
			track_number=10,
			size=11,
			time=12,
			year=13,
			sample_rate=14,
			comment=15,
			date_added=16,
			equalizer=17,
			composer=18,
			unk2=19,
			play_count=20,
			last_played=21,
			disc_number=22,
			rating=23,
			release_date=24,
			bpm=25,
			grouping=26,
			category=27,
			description=28,
			show=29,
			season=30,
			episode_number=31,
		};
	};

	class t_playlist 
	{
	public:
		typedef pfc::rcptr_t<t_playlist> ptr;

		t_uint32 timestamp; //date_created
		t_uint64 id;//pid
		t_uint32 sort_order;

		t_uint8 is_master, //is_hidden
			shuffle_items, has_been_shuffled, unk3;

		t_uint32 unk4;
		t_uint16 repeat_mode;
		t_uint8 podcast_flag;
		t_uint8 folder_flag; //smart_is_folder
		t_uint64 parentid; //parent_pid

		t_uint32 album_field_order, 
			workout_template_id, //workout_template_id
			unk6, 
			unk7, 
			unk8, 
			unk9;
		t_uint8 unk12, unk13, unk11;
		t_uint8 sort_direction;
		t_uint32 unk14, date_modified;

		pfc::string8 name;
		pfc::list_t<t_playlist_entry> items;

		bool smart_rules_valid;
		bool smart_data_valid;
		//pfc::array_t<t_uint8> dohm_smart_rules;
		//pfc::array_t<t_uint8> do_smart_rules;
		//pfc::array_t<t_uint8> dohm_smart_data;
		//pfc::array_t<t_uint8> do_smart_data;
		t_smart_playlist_rules smart_playlist_rules;
		t_smart_playlist_data smart_playlist_data;

		bool column_data_valid, itunes_data_102_valid;
		pfc::array_t<t_uint8> dohm_column_data;
		pfc::array_t<t_uint8> do_column_data, do_itunes_data_102;

		bool is_special_playlist;

#ifdef LOAD_LIBRARY_INDICES
		class t_library_index
		{
		public:
			t_uint32 type;
			pfc::array_t<t_uint32> indices;
		};

		pfc::list_t<t_library_index> m_library_indices;
#endif
		void remove_track_by_id(t_uint32 id)
		{
			t_size i, count = items.get_count();
			if (count)
				for (i=count; i>0; i--)
					if (!items[i-1].is_podcast_group && items[i-1].track_id == id)
					{
						bool valid = items[i-1].position_valid;
						t_uint32 position = items[i-1].position;
						items.remove_by_idx(i-1);
						t_size j, count2 = items.get_count();
						for (j=0; j<count2; j++)
							if (items[j].position_valid && items[j].position > position)
								items[j].position--;
					}
		}

		static int g_compare_pid (const t_playlist::ptr &p1, const t_playlist::ptr &p2) {return pfc::compare_t(p1->id, p2->id);}
		static int g_compare_pid_by_value (const t_playlist::ptr &p1, const t_uint64 &p2) {return pfc::compare_t(p1->id, p2);}
		static int g_compare_name (const t_playlist::ptr &p1, const t_playlist::ptr &p2) 
		{
			return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, pfc::stringcvt::string_wide_from_utf8_fast(p1->name), -1, pfc::stringcvt::string_wide_from_utf8_fast(p2->name), -1) - 2;
		}

		t_playlist() : timestamp(0), id(0), sort_order(1), smart_rules_valid(false),repeat_mode(1),
			smart_data_valid(false), is_master(0), shuffle_items(0), has_been_shuffled(0), unk3(0), unk4(0), podcast_flag(0),
			column_data_valid(false), parentid(NULL), folder_flag(0), sort_direction(0),
			unk12(0), workout_template_id(0), unk6(0), unk7(0), unk8(0), unk9(0), album_field_order(0), unk11(0), unk13(0),
			unk14(0), date_modified(0), is_special_playlist(false), itunes_data_102_valid(false)
			{};
	};
	class t_onthego_playlist 
	{
	public:
		pfc::array_t<t_uint32> items;
	};

	struct t_play_count_entry
	{
		t_uint32 play_count;
		t_uint32 last_played;
		t_uint32 bookmark_position;
		t_uint32 rating;
		t_uint8 unk1, play_state, unk2, unk3;
		t_uint32 skip_count;
		t_uint32 last_skipped;

		t_play_count_entry()
			: play_count(0), last_played(0), bookmark_position(-1), rating(-1), unk1(0), play_state(0),
			unk2(0), unk3(0), skip_count(0), last_skipped(0)
		{};
	};

	class reader
	{
	protected:
		fbh::StreamReaderMemblock* m_file;
		bool m_swap_endianess;
	public:
		template<typename T> void read_bendian_auto_t(T& p_object,abort_callback & p_abort)
		{
			if (!m_swap_endianess)
				m_file->read_bendian_t(p_object, p_abort);
			else
				m_file->read_lendian_t(p_object, p_abort);
		}
		template<typename T> void read_lendian_auto_t(T& p_object,abort_callback & p_abort)
		{
			if (m_swap_endianess)
				m_file->read_bendian_t(p_object, p_abort);
			else
				m_file->read_lendian_t(p_object, p_abort);
		}
		template <t_uint32 id>
		void read_header(t_header_marker<id> & p_out, abort_callback & p_abort)
		{
			read_bendian_auto_t(p_out.identifier, p_abort);
			p_out.verify_identifier();

			read_lendian_auto_t(p_out.header_size, p_abort);
			read_lendian_auto_t(p_out.section_size, p_abort);
			//p_out.data.set_size(p_out.header_size - sizeof(t_uint32)*3);
			//m_file->read(p_out.data.get_ptr(), p_out.header_size - sizeof(t_uint32)*3, p_abort);
			p_out.data.m_size = p_out.header_size - sizeof(t_uint32)*3;
			m_file->read_nobuffer(p_out.data.m_data, p_out.data.m_size, p_abort);
		}
		void read_pyhm(t_header_marker< identifiers::pyhm > & p_header, pfc::rcptr_t< itunesdb::t_playlist > & p_out, abort_callback & p_abort);
		void read_tihm(t_header_marker< identifiers::tihm > & p_header, pfc::rcptr_t <t_track> & p_out, abort_callback & p_abort);
		void read_aihm(t_header_marker< identifiers::aihm > & p_header, itunesdb::t_album::ptr & p_out, abort_callback & p_abort);
		void read_iihm(t_header_marker< identifiers::iihm > & p_header, itunesdb::t_artist::ptr & p_out, abort_callback & p_abort);

		void read_do_string_utf16(t_header_marker< identifiers::dohm > & dohm, WCHAR * & p_out, t_size & len, abort_callback & p_abort);
		void read_do_string_utf16(t_header_marker< identifiers::dohm > & p_header, pfc::string8 & p_out, abort_callback & p_abort);
		void read_do_string_utf16(t_header_marker< identifiers::dohm > & p_header, pfc::array_t<WCHAR> & p_out, abort_callback & p_abort);
		void read_do_smart_playlist_data(t_header_marker< identifiers::dohm > & p_header, t_smart_playlist_data & p_out, abort_callback & p_abort);
		void read_do_smart_playlist_rules(t_header_marker< identifiers::dohm > & p_header, t_smart_playlist_rules & p_out, abort_callback & p_abort);

		/**
		* Podcast URLs 
		*
		* Apparently, these people are not sure if it is UTF-8 or ASCII or ISO-8859-1 ...
		*/
		void read_do_string_utf8(t_header_marker< identifiers::dohm > & p_header, pfc::string8 & p_out, abort_callback & p_abort);
		itunesdb::reader(fbh::StreamReaderMemblock* p_file, bool p_swap_endianess = false)
			: m_file(p_file), m_swap_endianess(p_swap_endianess)
		{};
	};

	class stream_reader_memblock_ref_dop : public fbh::StreamReaderMemblock
	{
		bool m_swap_endianess;
	public:
		template<typename T> void read_bendian_auto_t(T& p_object,abort_callback & p_abort)
		{
			if (!m_swap_endianess)
				read_bendian_t(p_object, p_abort);
			else
				read_lendian_t(p_object, p_abort);
		}
		template<typename T> void read_lendian_auto_t(T& p_object,abort_callback & p_abort)
		{
			if (m_swap_endianess)
				read_bendian_t(p_object, p_abort);
			else
				read_lendian_t(p_object, p_abort);
		}
		void read_string_utf16_raw_bendian_auto_t(wchar_t * p_out, t_size length,abort_callback & p_abort)
		{
			for (t_size k=0; k<length; k++)
				read_bendian_auto_t(p_out[k], p_abort);
		}
		void read_string_utf16_raw_bendian_auto_t(t_size length, pfc::string8 & p_out,abort_callback & p_abort)
		{
			pfc::array_staticsize_t<wchar_t> data(length);
			read_string_utf16_raw_bendian_auto_t(data.get_ptr(), data.get_size(), p_abort);
			p_out.set_string(pfc::stringcvt::string_utf8_from_wide(data.get_ptr(), data.get_size()));
		}
		template <typename TInt>
		void read_string_utf16_sized_bendian_auto_t(pfc::string8 & p_out,abort_callback & p_abort)
		{
			TInt length;
			read_bendian_auto_t(length, p_abort);
			read_string_utf16_raw_bendian_auto_t(length, p_out, p_abort);
		}
		stream_reader_memblock_ref_dop(const void * p_data,t_size p_data_size, bool p_swap_endianess = false)
			: m_swap_endianess(p_swap_endianess), fbh::StreamReaderMemblock(p_data, p_data_size) {};
	};

	class stream_writer_memblock_dop : public stream_writer_memblock
	{
		bool m_swap_endianess;
	public:
		template<typename T> void write_bendian_auto_t(T p_object,abort_callback & p_abort)
		{
			if (!m_swap_endianess)
				write_bendian_t(p_object, p_abort);
			else
				write_lendian_t(p_object, p_abort);
		}
		template<typename T> void write_lendian_auto_t(T p_object,abort_callback & p_abort)
		{
			if (m_swap_endianess)
				write_bendian_t(p_object, p_abort);
			else
				write_lendian_t(p_object, p_abort);
		}
		void write_string_utf16_raw_bendian_auto_t(const wchar_t * p_string, t_size length,abort_callback & p_abort)
		{
			for (t_size k=0; k<length; k++)
				write_bendian_auto_t(p_string[k], p_abort);
		}
		t_size write_string_utf16_raw_bendian_auto_t(const char * p_string,t_size length,abort_callback & p_abort)
		{
			pfc::stringcvt::string_wide_from_utf8 wstr(p_string,length);
			t_size wlen = wstr.length();
			write_string_utf16_raw_bendian_auto_t(wstr.get_ptr(), wlen, p_abort);
			return wlen;
		}
		template <typename TInt>
		t_size write_string_utf16_sized_bendian_auto_t(const char * p_string,TInt length,abort_callback & p_abort)
		{
			write_bendian_auto_t(length, p_abort);
			return sizeof(TInt) + write_string_utf16_raw_bendian_auto_t(p_string,length, p_abort);
		}
		stream_writer_memblock_dop(bool p_swap_endianess = false)
			: m_swap_endianess(p_swap_endianess) {};
	};

	class writer
	{
	protected:
		stream_writer & m_file;
		bool m_swap_endianess;
		t_uint32 m_dbversion;
	public:
		template<typename T> void write_bendian_auto_t(T p_object,abort_callback & p_abort)
		{
			if (!m_swap_endianess)
				m_file.write_bendian_t(p_object, p_abort);
			else
				m_file.write_lendian_t(p_object, p_abort);
		}
		template<typename T> void write_lendian_auto_t(T p_object,abort_callback & p_abort)
		{
			if (m_swap_endianess)
				m_file.write_bendian_t(p_object, p_abort);
			else
				m_file.write_lendian_t(p_object, p_abort);
		}
		void write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
			const void * p_data, t_size data_size, t_uint32 header_data, abort_callback & p_abort);
		void write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
			const void * p_data, t_size data_size, abort_callback & p_abort);

		void write_do(do_types::t_type type, t_uint32 val1, t_uint32 val2, const void * data, t_size data_size, abort_callback & p_abort);
		bool write_do_string_meta(do_types::t_type type,const file_info * info, const char * field, abort_callback & p_abort);
		void write_do_string(do_types::t_type type, const char * str, abort_callback & p_abort);
		void write_do_string(do_types::t_type type, const WCHAR * str, t_size length, abort_callback & p_abort);
		void write_do_string_utf8(do_types::t_type type, const char * str, abort_callback & p_abort);
		void write_do_smart_playlist_data(const t_smart_playlist_data & p_data, abort_callback & p_abort);
		static void g_write_smart_playlist_rules_content(const t_smart_playlist_rules & p_rules, stream_writer_mem & ds, abort_callback & p_abort);
		void write_do_smart_playlist_rules(const t_smart_playlist_rules & p_rules, abort_callback & p_abort);
		writer(stream_writer & p_file, t_uint32 p_dbversion = 0, bool p_swap_endianess = false) 
			: m_file(p_file), m_swap_endianess(p_swap_endianess), m_dbversion(p_dbversion)
		{};
		/*writer() 
			: m_file(m_data)
			{};*/
	};

};



t_uint32 round_float(double f);
t_uint32 g_print_meta_int(const file_info & info, const char * field);
t_uint32 g_print_meta_int_n(const file_info & info, const char * field, t_size n);
bool g_print_meta_single(const file_info & info, const char * field, pfc::string_base & p_out);

#endif //_ITUNESDB_H_DOP_