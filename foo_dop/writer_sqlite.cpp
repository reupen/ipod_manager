#include "stdafx.h"

#include "ipod_manager.h"
#include "sqlite.h"
#include "writer.h"
#include "writer_sort_helpers.h"

#ifdef _DEBUG
//#define _FORCE_SQLDB _DEBUG
#endif

#ifdef _FORCE_SQLDB
void g_get_sql_commands (cfobject::object_t::ptr_t const & cfobj, pfc::string_list_impl & p_out, t_size & p_version);
void g_get_checkpoint_artwork_formats(cfobject::object_t::ptr_t const & deviceInfo, device_properties_t & p_out);
#endif

class string_escape_url : public pfc::string8_fast_aggressive
{
public:
	string_escape_url(const char* p_source, t_size len = pfc_infinite)
	{
		len = pfc::strlen_max(p_source, len);
		prealloc(len);
		const char * ptr = p_source, *start = ptr;
		while (t_size(ptr-p_source) < len)
		{
			start = ptr;
			while (*ptr != '%' && *ptr != 0x20 && t_size(ptr-p_source) < len) ptr++;
			if (ptr > start) add_string(start, ptr-start);
			if (t_size(ptr-p_source) < len && *ptr == '%')
			{
				add_string("%25");
				ptr++;
			}
			else if (t_size(ptr-p_source) < len && *ptr == 0x20)
			{
				add_string("%20");
				ptr++;
			}
		}
	}

};

class string_escape_sql : public pfc::string8_fast_aggressive
{
public:
	string_escape_sql(const char* p_source, t_size len = pfc_infinite)
	{
		len = pfc::strlen_max(p_source, len);
		prealloc(len);
		const char * ptr = p_source, *start = ptr;
		while (t_size(ptr-p_source) < len)
		{
			start = ptr;
			while (*ptr != '\'' && t_size(ptr-p_source) < len) ptr++;
			if (ptr > start) add_string(start, ptr-start);
			if (t_size(ptr-p_source) < len && *ptr == '\'')
			{
				add_string("''");
				ptr++;
			}
		}
	}

};

class string_escape_quote_sql : public pfc::string8_fast_aggressive
{
public:
	string_escape_quote_sql(const char* p_source, t_size len = pfc_infinite)
	{
		add_byte('\'');
		len = pfc::strlen_max(p_source, len);
		const char * ptr = p_source, *start = ptr;
		while (t_size(ptr-p_source) < len)
		{
			start = ptr;
			while (*ptr != '\'' && t_size(ptr-p_source) < len) ptr++;
			if (ptr > start) add_string(start, ptr-start);
			if (t_size(ptr-p_source) < len && *ptr == '\'')
			{
				add_string("''");
				ptr++;
			}
		}
		add_byte('\'');
	}

};

t_uint32 sqldbtime_from_itunesdbtime (t_uint32 time, bool b_from_local = true)
{
	if ( time == 0) return 0;
	t_uint32 time2 = 0;
	if (b_from_local)
	{
		t_filetimestamp ft = filetime_time_from_appletime(time);
		time2 = apple_time_from_filetime(ft, false);
	}
	else
		time2 = time;
	if (time2 < 3061152000) return 0;
	return time2 - 3061152000;
}

const char * g_librarydb_tables[] = 
{
	"CREATE TABLE db_info (pid INTEGER NOT NULL, primary_container_pid INTEGER, media_folder_url TEXT, audio_language INTEGER, subtitle_language INTEGER, genius_cuid TEXT, bib BLOB, rib BLOB, PRIMARY KEY (pid))",
	"CREATE TABLE item (pid INTEGER NOT NULL, revision_level INTEGER, media_kind INTEGER DEFAULT 0, is_song INTEGER DEFAULT 0, is_audio_book INTEGER DEFAULT 0, is_music_video INTEGER DEFAULT 0, is_movie INTEGER DEFAULT 0, is_tv_show INTEGER DEFAULT 0, is_home_video INTEGER DEFAULT 0, is_ringtone INTEGER DEFAULT 0, is_tone INTEGER DEFAULT 0, is_voice_memo INTEGER DEFAULT 0, is_book INTEGER DEFAULT 0, is_rental INTEGER DEFAULT 0, is_itunes_u INTEGER DEFAULT 0, is_digital_booklet INTEGER DEFAULT 0, is_podcast INTEGER DEFAULT 0, date_modified INTEGER DEFAULT 0, date_backed_up INTEGER DEFAULT 0, year INTEGER DEFAULT 0, content_rating INTEGER DEFAULT 0, content_rating_level INTEGER DEFAULT 0, is_compilation INTEGER, is_user_disabled INTEGER DEFAULT 0, remember_bookmark INTEGER DEFAULT 0, exclude_from_shuffle INTEGER DEFAULT 0, part_of_gapless_album INTEGER DEFAULT 0, chosen_by_auto_fill INTEGER DEFAULT 0, artwork_status INTEGER, artwork_cache_id INTEGER DEFAULT 0, start_time_ms REAL DEFAULT 0, stop_time_ms REAL DEFAULT 0, total_time_ms REAL DEFAULT 0, total_burn_time_ms REAL, track_number INTEGER DEFAULT 0, track_count INTEGER DEFAULT 0, disc_number INTEGER DEFAULT 0, disc_count INTEGER DEFAULT 0, bpm INTEGER DEFAULT 0, relative_volume INTEGER, eq_preset TEXT, radio_stream_status TEXT, genius_id INTEGER DEFAULT 0, genre_id INTEGER DEFAULT 0, category_id INTEGER DEFAULT 0, album_pid INTEGER DEFAULT 0, artist_pid INTEGER DEFAULT 0, composer_pid INTEGER DEFAULT 0, title TEXT, artist TEXT, album TEXT, album_artist TEXT, composer TEXT, sort_title TEXT, sort_artist TEXT, sort_album TEXT, sort_album_artist TEXT, sort_composer TEXT, title_order INTEGER, artist_order INTEGER, album_order INTEGER, genre_order INTEGER, composer_order INTEGER, album_artist_order INTEGER, album_by_artist_order INTEGER, series_name_order INTEGER, comment TEXT, grouping TEXT, description TEXT, description_long TEXT, collection_description TEXT, copyright TEXT, PRIMARY KEY (pid))",
	"CREATE TABLE avformat_info (item_pid INTEGER NOT NULL, sub_id INTEGER NOT NULL DEFAULT 0, audio_format INTEGER, bit_rate INTEGER DEFAULT 0, channels INTEGER DEFAULT 0, sample_rate REAL DEFAULT 0, duration INTEGER, gapless_heuristic_info INTEGER, gapless_encoding_delay INTEGER, gapless_encoding_drain INTEGER, gapless_last_frame_resynch INTEGER, analysis_inhibit_flags INTEGER, audio_fingerprint INTEGER, volume_normalization_energy INTEGER, PRIMARY KEY (item_pid,sub_id))",
	"CREATE TABLE video_info (item_pid INTEGER NOT NULL, has_alternate_audio INTEGER, has_subtitles INTEGER, characteristics_valid INTEGER, has_closed_captions INTEGER, is_self_contained INTEGER, is_compressed INTEGER, is_anamorphic INTEGER, is_hd INTEGER, season_number INTEGER, audio_language INTEGER, audio_track_index INTEGER, audio_track_id INTEGER, subtitle_language INTEGER, subtitle_track_index INTEGER, subtitle_track_id INTEGER, series_name TEXT, sort_series_name TEXT, episode_id TEXT, episode_sort_id INTEGER, network_name TEXT, extended_content_rating TEXT, movie_info TEXT, PRIMARY KEY (item_pid))",
	"CREATE TABLE video_characteristics (item_pid INTEGER, sub_id INTEGER DEFAULT 0, track_id INTEGER, height INTEGER, width INTEGER, depth INTEGER, codec INTEGER, frame_rate REAL, percentage_encrypted REAL, bit_rate INTEGER, peak_bit_rate INTEGER, buffer_size INTEGER, profile INTEGER, level INTEGER, complexity_level INTEGER, UNIQUE (item_pid,sub_id,track_id))",
	"CREATE TABLE store_info (item_pid INTEGER NOT NULL, store_kind INTEGER, date_purchased INTEGER DEFAULT 0, date_released INTEGER DEFAULT 0, account_id INTEGER, key_versions INTEGER, key_platform_id INTEGER, key_id INTEGER, key_id2 INTEGER, store_item_id INTEGER, artist_id INTEGER, composer_id INTEGER, genre_id INTEGER, playlist_id INTEGER, storefront_id INTEGER, store_link_id INTEGER, relevance REAL, popularity REAL, xid TEXT, flavor TEXT, PRIMARY KEY (item_pid))",
	"CREATE TABLE store_link (id INTEGER NOT NULL, url TEXT, PRIMARY KEY (id))",
	"CREATE TABLE podcast_info (item_pid INTEGER NOT NULL, date_released INTEGER DEFAULT 0, external_guid TEXT, feed_url TEXT, feed_keywords TEXT, PRIMARY KEY (item_pid))",
	"CREATE TABLE container (pid INTEGER NOT NULL, distinguished_kind INTEGER, date_created INTEGER, date_modified INTEGER, name TEXT, name_order INTEGER, parent_pid INTEGER, media_kinds INTEGER, workout_template_id INTEGER, is_hidden INTEGER, smart_is_folder INTEGER, smart_is_dynamic INTEGER, smart_is_filtered INTEGER, smart_is_genius INTEGER, smart_enabled_only INTEGER, smart_is_limited INTEGER, smart_limit_kind INTEGER, smart_limit_order INTEGER, smart_evaluation_order INTEGER, smart_limit_value INTEGER, smart_reverse_limit_order INTEGER, smart_criteria BLOB, description TEXT, PRIMARY KEY (pid))",
	"CREATE TABLE item_to_container (item_pid INTEGER, container_pid INTEGER, physical_order INTEGER, shuffle_order INTEGER)",
	"CREATE TABLE container_seed (container_pid INTEGER NOT NULL, item_pid INTEGER NOT NULL, seed_order INTEGER DEFAULT 0, UNIQUE (container_pid,item_pid))",
	"CREATE TABLE album (pid INTEGER NOT NULL, kind INTEGER, artwork_status INTEGER, artwork_item_pid INTEGER, artist_pid INTEGER, user_rating INTEGER, name TEXT, name_order INTEGER, all_compilations INTEGER, feed_url TEXT, season_number INTEGER, PRIMARY KEY (pid))",
	//"CREATE TABLE item_to_album (item_pid INTEGER, album_pid INTEGER)",
	"CREATE TABLE artist (pid INTEGER NOT NULL, kind INTEGER, artwork_status INTEGER, artwork_album_pid INTEGER, name TEXT, name_order INTEGER, sort_name TEXT, PRIMARY KEY (pid))",
	"CREATE TABLE composer (pid INTEGER NOT NULL, name TEXT, name_order INTEGER, sort_name TEXT, PRIMARY KEY (pid))",
	"CREATE TABLE location_kind_map (id INTEGER NOT NULL, kind TEXT NOT NULL, PRIMARY KEY (id), UNIQUE (kind))",
	"CREATE TABLE genre_map (id INTEGER NOT NULL, genre TEXT NOT NULL, genre_order INTEGER DEFAULT 0, PRIMARY KEY (id), UNIQUE (genre))",
	"CREATE TABLE category_map (id INTEGER NOT NULL, category TEXT NOT NULL, PRIMARY KEY (id), UNIQUE (category))",
	//"CREATE TABLE item_to_artist (item_pid INTEGER, artist_pid INTEGER)",
	//"CREATE TABLE item_to_composer (item_pid INTEGER, composer_pid INTEGER)",
	"CREATE TABLE version_info (id INTEGER PRIMARY KEY, major INTEGER, minor INTEGER, compatibility INTEGER DEFAULT 0, update_level INTEGER DEFAULT 0, device_update_level INTEGER DEFAULT 0, platform INTEGER DEFAULT 0)"
};

const char * g_dynamic_tables[] = 
{
	"CREATE TABLE item_stats (item_pid INTEGER NOT NULL, has_been_played INTEGER DEFAULT 0, date_played INTEGER DEFAULT 0, play_count_user INTEGER DEFAULT 0, play_count_recent INTEGER DEFAULT 0, date_skipped INTEGER DEFAULT 0, skip_count_user INTEGER DEFAULT 0, skip_count_recent INTEGER DEFAULT 0, bookmark_time_ms REAL, bookmark_time_ms_common REAL, user_rating INTEGER DEFAULT 0, user_rating_common INTEGER DEFAULT 0, rental_expired INTEGER DEFAULT 0, PRIMARY KEY (item_pid))",
	"CREATE TABLE container_ui (container_pid INTEGER NOT NULL, play_order INTEGER DEFAULT 0, is_reversed INTEGER DEFAULT 0, album_field_order INTEGER DEFAULT 0, repeat_mode INTEGER DEFAULT 0, shuffle_items INTEGER DEFAULT 0, has_been_shuffled INTEGER DEFAULT 0, PRIMARY KEY (container_pid))",
	"CREATE TABLE rental_info (item_pid INTEGER NOT NULL, rental_date_started INTEGER DEFAULT 0, rental_duration INTEGER DEFAULT 0, rental_playback_date_started INTEGER DEFAULT 0, rental_playback_duration INTEGER DEFAULT 0, is_demo INTEGER DEFAULT 0, PRIMARY KEY (item_pid))"
};

const char * g_extras_tables[] = 
{
	"CREATE TABLE chapter (item_pid INTEGER NOT NULL, data BLOB, PRIMARY KEY (item_pid))",
	"CREATE TABLE lyrics (item_pid INTEGER NOT NULL, checksum INTEGER, lyrics TEXT, PRIMARY KEY (item_pid))"
};

const char * g_location_tables[] = 
{
	"CREATE TABLE location (item_pid INTEGER NOT NULL, sub_id INTEGER NOT NULL DEFAULT 0, base_location_id INTEGER DEFAULT 0, location_type INTEGER, location TEXT, extension INTEGER, kind_id INTEGER DEFAULT 0, date_created INTEGER DEFAULT 0, file_size INTEGER DEFAULT 0, file_creator INTEGER, file_type INTEGER, num_dir_levels_file INTEGER, num_dir_levels_lib INTEGER, PRIMARY KEY (item_pid,sub_id))",
	"CREATE TABLE base_location (id INTEGER NOT NULL, path TEXT, PRIMARY KEY (id))"
};

void g_set_up_itdb_pragma(sqlite_database::ptr & p_db, t_size page_size = 1024)
{
	p_db->exec("PRAGMA journal_mode=MEMORY;");
	p_db->exec(pfc::string8() << "PRAGMA page_size=" << page_size << ";");
	p_db->exec("PRAGMA auto_vacuum=1;");
	//p_db->exec("PRAGMA synchronous=OFF;");
}

void g_set_up_itdb_tables(sqlite_database::ptr & p_db, const char ** commands, t_size count)
{
	t_size i;
	for (i=0; i<count; i++)
		p_db->exec(commands[i]);
}

t_uint8 g_translate_limit_order(t_uint8 order)
{
	t_uint8 result;
	switch ( order )
	{
	case 2:
		result = 2;
		break;
	case 3:
		result = 5;
		break;
	case 4:
		result = 6;
		break;
	case 26:
		result = 42;
		break;
	case 5:
		result = 7;
		break;
	case 35:
		result = 50;
		break;
	case 18:
		result = 24;
		break;
	case 6:
		result = 8;
		break;
	case 7:
		result = 9;
		break;
	case 8:
		result = 10;
		break;
	case 9:
		result = 11;
		break;
	case 10:
		result = 12;
		break;
	case 22:
		result = 28;
		break;
	case 11:
		result = 13;
		break;
	case 12:
		result = 14;
		break;
	case 13:
		result = 15;
		break;
	case 19:
		result = 25;
		break;
	case 14:
		result = 16;
		break;
	case 15:
		result = 21;
		break;
	case 16:
		result = 22;
		break;
	case 17:
		result = 23;
		break;
	case 20:
		result = 26;
		break;
	case 21:
		result = 27;
		break;
	case 33:
		result = 48;
		break;
	case 34:
		result = 49;
		break;
	case 23:
		result = 29;
		break;
	case 38:
		result = 53;
		break;
	case 24:
		result = 39;
		break;
	case 25:
		result = 40;
		break;
	case 27:
		result = 17;
		break;
	case 28:
		result = 20;
		break;
	case 29:
		result = 44;
		break;
	case 30:
		result = 45;
		break;
	case 31:
		result = 46;
		break;
	case 36:
		result = 52;
		break;
	case 37:
		result = 51;
		break;
	case 39:
		result = 54;
		break;
	case 40:
		result = 63;
		break;
	default:
		result = 1;
		break;
	}
	return result;
}

int g_translate_audio_format(int a1)
{
	int result = NULL;

	switch ( a1 )
	{
	case 1:
		result = 1;
		break;
	case 2:
		result = 2;
		break;
	case 10:
		result = 101;
		break;
	case 11:
		result = 201;
		break;
	case 12:
		result = 301;
		break;
	case 20:
		result = 102;
		break;
	case 21:
		result = 202;
		break;
	case 22:
		result = 302;
		break;
	case 30:
		result = 103;
		break;
	case 31:
		result = 203;
		break;
	case 32:
		result = 303;
		break;
	case 40:
		result = 401;
		break;
	case 41:
		result = 402;
		break;
	case 42:
		result = 403;
		break;
	case 43:
		result = 404;
		break;
	case 44:
		result = 405;
		break;
	case 45:
		result = 406;
		break;
	case 50:
		result = 501;
		break;
	case 51:
		result = 502;
		break;
	case 52:
		result = 503;
		break;
	case 53:
		result = 504;
		break;
	case 54:
		result = 505;
		break;
	case 55:
		result = 506;
		break;
	case 60:
		result = 601;
		break;
	default:
		result = 0;
		break;
	}
	return result;
}

bool g_track_has_store_info(t_track::ptr const & p_track)
{
	return
		p_track->account_id_primary
		|| p_track->account_id_secondary
		|| p_track->store_drm_key_versions
		|| p_track->key_platform_id
		|| p_track->store_item_id
		|| p_track->store_genre_id
		|| p_track->store_artist_id
		|| p_track->store_composer_id
		|| p_track->store_playlist_id
		|| p_track->store_front_id
		|| p_track->datereleased
		|| p_track->date_purchased
		|| p_track->store_kind;
	//store_link
}

class sort_orders_generator_t
{
public:
	template <class Tlist, class TsortFunc>
	void generate_order(t_size count_elements, Tlist & sort_entries, pfc::array_t<t_size> & p_out, TsortFunc sortFunc)
	{
		t_size i, j = 1;
		mmh::Permutation perm_temp(count_elements);
		mmh::sort_get_permutation(sort_entries, perm_temp, sortFunc, false); 
		p_out.set_count(count_elements);
		//mmh::InversePermutation perminv(perm_temp);
		for (i=0; i<count_elements; i++)
		{
			p_out[perm_temp[i]] = j*100;
			if (i+1 < count_elements && sortFunc(sort_entries[perm_temp[i]], sort_entries[perm_temp[i+1]])) j++;
		}
	}
	void run ( ipod::tasks::load_database_t & m_library)
	{
		pfc::list_t<t_sort_entry> sort_entries;
		t_size i, count_tracks = m_library.m_tracks.get_count(), count_playlists = m_library.m_playlists.get_count();

		sort_entries.set_count(count_tracks);
		m_container_order.set_count(count_playlists);

		for (i=0; i<count_tracks; i++)
		{
			sort_entries[i].set_from_track(false, *m_library.m_tracks[i]); //"ipod_sorting" handled from sort fields
		}

		ipod_sort_helpers_t<true> sorter;
		generate_order(count_tracks, sort_entries, m_title_order, sorter.g_compare_title);
		generate_order(count_tracks, sort_entries, m_artist_order, sorter.g_compare_artist);
		generate_order(count_tracks, sort_entries, m_album_order, sorter.g_compare_album);
		generate_order(count_tracks, sort_entries, m_genre_order, sorter.g_compare_genre);
		generate_order(count_tracks, sort_entries, m_composer_order, sorter.g_compare_composer);
		generate_order(count_tracks, sort_entries, m_album_artist_order, sorter.g_compare_album_artist);
		generate_order(count_tracks, sort_entries, m_series_name_order, sorter.g_compare_show);
		generate_order(count_tracks, sort_entries, m_album_or_show_order, sorter.g_compare_album_or_show);
		generate_order(count_playlists, m_library.m_playlists, m_container_order, t_playlist::g_compare_name);

	}
	pfc::array_t<t_size> m_title_order, m_artist_order, m_album_order, m_genre_order, m_composer_order, m_album_artist_order, m_series_name_order, m_album_or_show_order, m_container_order;
};


class sqlite_register_functions_scope
{
	typedef sqlite_register_functions_scope TSelf;
public:
	sqlite_register_functions_scope(sqlite_database::ptr p_db, ipod_device_ptr_ref_t p_ipod)
	{
		m_database = p_db;
		m_mobile_device = p_ipod->mobile_device;
		sqlite3_create_function(*p_db, "iPhoneSortKey", 1, SQLITE_UTF16, this, iPhoneSortKey, NULL, NULL);
		sqlite3_create_function(*p_db, "iPhoneSortSection", 1, SQLITE_ANY, this, iPhoneSortSection, NULL, NULL);
	}
	~sqlite_register_functions_scope()
	{
	}
private:
	static void iPhoneSortKey(sqlite3_context* p_context, int argc, sqlite3_value** argv)
	{
		try 
		{
			TSelf * p_this = (TSelf *)sqlite3_user_data(p_context);
			if (!p_this || argc != 1 || !p_this->m_mobile_device.is_valid() || !p_this->m_mobile_device->m_icu_context.m_coll) throw pfc::exception("");

			const wchar_t * p_string = (const wchar_t *)sqlite3_value_text16(argv[0]);
			t_size p_string_length = sqlite3_value_bytes16(argv[0])/2;
			pfc::array_t<t_uint8> blob;
			p_this->m_mobile_device->sync_get_iPhoneSortKey(p_string, p_string_length, blob);
			sqlite3_result_blob(p_context, blob.get_ptr(), blob.get_size(), SQLITE_TRANSIENT);
		} 
		catch (pfc::exception const & ex) 
		{
			sqlite3_result_error_code(p_context, SQLITE_ERROR);
		}

	}
	static void iPhoneSortSection(sqlite3_context* p_context, int argc, sqlite3_value** argv)
	{
		try 
		{
			TSelf * p_this = (TSelf *)sqlite3_user_data(p_context);
			if (!p_this || argc != 1 || !p_this->m_mobile_device.is_valid() || !p_this->m_mobile_device->m_icu_context.m_coll) throw pfc::exception("");

			const t_uint8 * p_blob = (const t_uint8 *)sqlite3_value_blob(argv[0]);
			t_size p_blob_length = sqlite3_value_bytes(argv[0]);
			t_int64 result = p_this->m_mobile_device->sync_get_iPhoneSortSection(p_blob, p_blob_length);
			sqlite3_result_int64(p_context, result);
		} 
		catch (pfc::exception const & ex) 
		{
			sqlite3_result_error_code(p_context, SQLITE_ERROR);
		}

	}
	mobile_device_handle::ptr m_mobile_device;
	sqlite_database::ptr m_database;
};

void ipod::tasks::database_writer_t::write_sqlitedb(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & m_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	//if (p_ipod->m_device_properties.m_SQLiteDB)
	{
		pfc::hires_timer timer;
		timer.start();

		pfc::string8 base, tempbase;
		p_ipod->get_database_path(base);
		base << p_ipod->get_path_separator_ptr() << "iTunes" << p_ipod->get_path_separator_ptr() << "iTunes Library.itlp";

		try {filesystem::g_create_directory(base, p_abort);} catch (exception_io_already_exists const &) {};

		base << p_ipod->get_path_separator_ptr();
		uGetTempPath(tempbase);

		try 
		{
			sqlite_handle sqlh;
			sqlite_database::ptr locationsdb, librarydb, dynamicdb, extrasdb;

			pfc::string8_fast_aggressive query;
			query.prealloc(1024);

			t_size i=NULL, count=NULL;

			try { filesystem::g_remove(pfc::string8() << tempbase << "Locations.itdb.dop.temp", p_abort); } catch (const exception_io_not_found) {};
			try { filesystem::g_remove(pfc::string8() << tempbase << "Library.itdb.dop.temp", p_abort); } catch (const exception_io_not_found) {};
			try { filesystem::g_remove(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp", p_abort); } catch (const exception_io_not_found) {};
			try { filesystem::g_remove(pfc::string8() << tempbase << "Extras.itdb.dop.temp", p_abort); } catch (const exception_io_not_found) {};

			sqlh.open_ex(pfc::string8() << tempbase << "Locations.itdb.dop.temp", SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE|SQLITE_OPEN_READWRITE, locationsdb);
			sqlh.open_ex(pfc::string8() << tempbase << "Library.itdb.dop.temp", SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE|SQLITE_OPEN_READWRITE, librarydb);
			sqlh.open_ex(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp", SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE|SQLITE_OPEN_READWRITE, dynamicdb);
			sqlh.open_ex(pfc::string8() << tempbase << "Extras.itdb.dop.temp", SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE|SQLITE_OPEN_READWRITE, extrasdb);

			g_set_up_itdb_pragma(librarydb, 4096);
			g_set_up_itdb_pragma(dynamicdb);
			g_set_up_itdb_pragma(extrasdb, 4096);
			g_set_up_itdb_pragma(locationsdb);

			locationsdb->exec("BEGIN TRANSACTION");
			librarydb->exec("BEGIN TRANSACTION");
			dynamicdb->exec("BEGIN TRANSACTION");
			extrasdb->exec("BEGIN TRANSACTION");

			g_set_up_itdb_tables(librarydb, g_librarydb_tables, tabsize(g_librarydb_tables));
			g_set_up_itdb_tables(dynamicdb, g_dynamic_tables, tabsize(g_dynamic_tables));
			g_set_up_itdb_tables(extrasdb, g_extras_tables, tabsize(g_extras_tables));
			g_set_up_itdb_tables(locationsdb, g_location_tables, tabsize(g_location_tables));

			pfc::string8 rootFolder;
			p_ipod->get_database_folder(rootFolder);

			pfc::string8 media_path; media_path << rootFolder << "/Music";
			pfc::string8 ringtones_path; ringtones_path << rootFolder << "/Ringtones";
			pfc::string8 podcasts_path; podcasts_path << "Podcasts";
			pfc::string8 purchases_path; purchases_path << "Purchases";
			t_size media_path_len = strlen(media_path);
			t_size ringtones_path_len = strlen(ringtones_path);
			t_size podcasts_path_len = strlen(podcasts_path);
			t_size purchases_path_len = strlen(purchases_path);

			locationsdb->exec(pfc::string8() << "INSERT INTO base_location (id,path) VALUES (1, '" << media_path <<"')");
			locationsdb->exec(pfc::string8() << "INSERT INTO base_location (id,path) VALUES (4, '" << podcasts_path <<"')");
			locationsdb->exec(pfc::string8() << "INSERT INTO base_location (id,path) VALUES (6, '" << ringtones_path <<"')");
			locationsdb->exec(pfc::string8() << "INSERT INTO base_location (id,path) VALUES (7, '" << purchases_path <<"')");

			/** DB INFO */

			query.reset();
			query << "INSERT INTO db_info (pid,primary_container_pid,audio_language,subtitle_language) VALUES (" //"%lld,%d,%d)"
				<< (t_int64)m_library.pid << ","
				<< (t_int64)m_library.m_library_playlist->id << ","
				<< (t_int16)m_library.audio_language << ","
				<< (t_int16)m_library.subtitle_language
				<< ")";
			librarydb->exec(query);

			sort_orders_generator_t sort_orders;
			sort_orders.run(m_library);

			/** INSERT PLAYLISTS */
			{
				t_size j, count_entries = m_library.m_tracks.get_count(), count_library_entries = m_library.m_tracks.get_count();

				count = m_library.m_playlists.get_count();

				query.reset();

				query << "INSERT INTO container (pid,name,name_order,distinguished_kind,media_kinds,date_created,date_modified,parent_pid,workout_template_id,smart_is_folder,is_hidden) VALUES ("
					<< (t_int64)m_library.m_library_playlist->id << ","
					<< "'" << string_escape_sql(m_library.m_library_playlist->name) << "',"
					<< "100" << ","
					<< "0" << ","
					<< "1" << ","
					<< "0" << ","
					<< "0" << ","
					<< "0" << ","
					<< "0" << ","
					<< "0" << ","
					<< "1" << ")";

				librarydb->exec(query);

				query.reset();

				t_uint16 repeat_mode = 0;
				if (m_library.m_library_playlist->repeat_mode == 2)
					repeat_mode = 2;
				else if (m_library.m_library_playlist->repeat_mode == 3)
					repeat_mode = 1;

				query << "INSERT INTO container_ui (container_pid,play_order,is_reversed,album_field_order,repeat_mode,shuffle_items,has_been_shuffled) VALUES ("
					<< (t_int64)m_library.m_library_playlist->id << ","
					<< g_translate_limit_order(m_library.m_library_playlist->sort_order) << ","
					<< m_library.m_library_playlist->sort_direction << ","
					<< g_translate_limit_order(m_library.m_library_playlist->album_field_order) << ","
					<< repeat_mode << ","
					<< m_library.m_library_playlist->shuffle_items << ","
					<< m_library.m_library_playlist->has_been_shuffled 
					<< ")";

				dynamicdb->exec(query);

				mmh::Permutation permutation_tid(m_library.m_tracks.get_count());
				mmh::sort_get_permutation(m_library.m_tracks.get_ptr(), permutation_tid, m_library.g_compare_track_id, false);

				for (j=0; j< count_entries; j++)
				{
					pfc::rcptr_t<t_track> & track = m_library.m_tracks[j];
					query.reset();
					query << "INSERT INTO item_to_container (item_pid,container_pid,physical_order) VALUES ("
						<< (t_int64)track->pid << ","
						<< (t_int64)m_library.m_library_playlist->id << ","
						<< j << ")";
					librarydb->exec(query);
				}

				for (i=0; i<count; i++)
				{

					query.reset();

					query << "INSERT INTO container (pid,name,name_order,distinguished_kind,media_kinds,date_created,date_modified,parent_pid,workout_template_id,smart_is_folder,is_hidden) VALUES ("
						<< (t_int64)m_library.m_playlists[i]->id << ","
						<< "'" << string_escape_sql(m_library.m_playlists[i]->name) << "',"
						<< (sort_orders.m_container_order[i] + 100) << ","
						<< "0" << ","
						<< "1" << "," //media_kinds ??
						<< sqldbtime_from_itunesdbtime(m_library.m_playlists[i]->timestamp) << ","
						<< sqldbtime_from_itunesdbtime(m_library.m_playlists[i]->date_modified) << ","
						<< (t_int64)m_library.m_playlists[i]->parentid << ","
						<< m_library.m_playlists[i]->workout_template_id << ","
						<< m_library.m_playlists[i]->folder_flag << ","
						<< m_library.m_playlists[i]->is_master
						<< ")";

					librarydb->exec(query);

					query.reset();

					t_uint16 repeat_mode = 0;
					if (m_library.m_playlists[i]->repeat_mode == 2)
						repeat_mode = 2;
					else if (m_library.m_playlists[i]->repeat_mode == 3)
						repeat_mode = 1;

					query << "INSERT INTO container_ui (container_pid,play_order,is_reversed,album_field_order,repeat_mode,shuffle_items,has_been_shuffled) VALUES ("
						<< (t_int64)m_library.m_playlists[i]->id << ","
						<< g_translate_limit_order(m_library.m_playlists[i]->sort_order) << ","
						<< m_library.m_playlists[i]->sort_direction << ","
						<< g_translate_limit_order(m_library.m_playlists[i]->album_field_order) << ","
						<< repeat_mode << ","
						<< m_library.m_playlists[i]->shuffle_items << ","
						<< m_library.m_playlists[i]->has_been_shuffled 
						<< ")";

					dynamicdb->exec(query);

					if (m_library.m_playlists[i]->smart_data_valid)
					{
						t_uint8 limit_kind = 0, limit_value = m_library.m_playlists[i]->smart_playlist_data.limit_value;
						if (m_library.m_playlists[i]->smart_playlist_data.limit_type >= 1 && m_library.m_playlists[i]->smart_playlist_data.limit_type <= 4)
						{
							limit_kind = m_library.m_playlists[i]->smart_playlist_data.limit_type -1;
						}
						else
							limit_value = 62;



						query.reset();
						query << "UPDATE container SET smart_is_dynamic="  << m_library.m_playlists[i]->smart_playlist_data.live_update 
							<< ",smart_is_filtered=" <<  m_library.m_playlists[i]->smart_playlist_data.live_update 
							<< ",smart_is_genius=" << m_library.m_playlists[i]->smart_playlist_data.unk4
							<< ",smart_enabled_only=" << m_library.m_playlists[i]->smart_playlist_data.match_checked_only
							<< ",smart_is_limited=" << m_library.m_playlists[i]->smart_playlist_data.check_limits
							<< ",smart_limit_kind=" << limit_kind
							<< ",smart_limit_value=" << limit_value
							<< ",smart_limit_order=" << g_translate_limit_order(m_library.m_playlists[i]->smart_playlist_data.limit_sort)
							<< ",smart_reverse_limit_order=" << m_library.m_playlists[i]->smart_playlist_data.reverse_limit_sort
							<< " WHERE pid=" << (t_int64)m_library.m_playlists[i]->id;
						librarydb->exec(query);
						query.reset();

						if (m_library.m_playlists[i]->smart_rules_valid)
						{
							stream_writer_mem slst;
							itunesdb::writer::g_write_smart_playlist_rules_content(m_library.m_playlists[i]->smart_playlist_rules, slst, abort_callback_dummy());
							query << "UPDATE container SET smart_criteria=X'" << pfc::format_hexdump(slst.get_ptr(), slst.get_size(), "")
								<< "' WHERE pid=" << (t_int64)m_library.m_playlists[i]->id;
							librarydb->exec(query);
						}
					}

					count_entries = m_library.m_playlists[i]->items.get_count();
					for (t_size k = 0, j=0; j<count_entries; j++)
					{
						//t_int64 pid = m_library.m_playlists[i]->items[j].item_pid;
						t_size index;
						if (pfc::bsearch_permutation_t(count_library_entries, m_library.m_tracks, m_library.g_compare_track_id_with_id, 
							m_library.m_playlists[i]->items[j].track_id, permutation_tid, index))
						{
							pfc::rcptr_t<t_track> & track = m_library.m_tracks[index];
							query.reset();
							query << "INSERT INTO item_to_container (item_pid,container_pid,physical_order) VALUES ("
								<< (t_int64)track->pid << ","
								<< (t_int64)m_library.m_playlists[i]->id << ","
								<< k << ")";
							librarydb->exec(query);
							++k;
						}
					}
				}

			}

			/** INSERT TRACKS */

			t_size count_artists = m_library.m_artist_list.get_count();
			t_size count_albums = m_library.m_album_list.m_master_list.get_count();

			mmh::Permutation perm_artists(count_artists);
			mmh::Permutation perm_albums(count_albums);

			mmh::sort_get_permutation(m_library.m_artist_list.get_ptr(), perm_artists, t_artist::g_compare_id, false);
			mmh::sort_get_permutation(m_library.m_album_list.m_master_list.get_ptr(), perm_albums, t_album::g_compare_id, false);			

			for (i=0; i<count_albums; i++)
			{
				t_album::ptr album = m_library.m_album_list.m_master_list[i];
				query.reset();
				query << "INSERT INTO album (pid,kind,artwork_status,artwork_item_pid,all_compilations,user_rating,name_order,season_number,name,feed_url) VALUES ("
					<< (t_int64)album->pid << ","
					<< album->kind << ","
					<< album->artwork_status << ","
					<< (t_int64)album->artwork_item_pid << ","
					<< album->all_compilations << ","
					<< album->user_rating << ","
					<< "0" << ","
					<< album->season_number << ","
					<< "'" << string_escape_sql(album->kind == ai_types::tv_show ? album->show : album->album) << "',"
					<< "'" << string_escape_sql(album->podcast_url) <<"'"
					<< ")";
				librarydb->exec(query);
			}

			for (i=0; i<count_artists; i++)
			{
				query.reset();
				query << "INSERT INTO artist (pid,kind,artwork_status,artwork_album_pid,name_order,name,sort_name) VALUES ("
					<< (t_int64)m_library.m_artist_list[i]->pid << ","
					<< m_library.m_artist_list[i]->type << ","
					<< m_library.m_artist_list[i]->artwork_status << ","
					<< (t_int64)m_library.m_artist_list[i]->artwork_album_pid << ","
					<< "0" ","
					<< "'" << string_escape_sql(m_library.m_artist_list[i]->artist) << "',"
					<< "?"
					//<< "'" << string_escape_sql(m_library.m_artist_list[i]->sort_artist_valid ? m_library.m_artist_list[i]->sort_artist : m_library.m_artist_list[i]->artist) << "'" 
					<< ")";
				
				sqlite_statement stmt;
				stmt.prepare(librarydb, query, query.length());

				{
					pfc::string_simple_t<WCHAR> temp; 
					if (m_library.m_artist_list[i]->sort_artist_valid) 
					{
						g_get_sort_string_for_sorting(m_library.m_artist_list[i]->sort_artist, temp, false);
					}
					else 
					{
						g_get_sort_string_for_sorting(m_library.m_artist_list[i]->artist, temp, false);
					}
					stmt.autobind_text(temp.get_ptr(), temp.length(), SQLITE_TRANSIENT);
				}

				stmt.finalise();

				//librarydb->exec(query);
			}

			t_size count_library_entries = m_library.m_tracks.get_count();
			pfc::array_staticsize_t<t_uint32> genre_ids(count_library_entries), composer_ids(count_library_entries);

			{
				t_size counter = 0;
				mmh::Permutation pgenre(count_library_entries), pcomposer(count_library_entries);

				mmh::sort_get_permutation(m_library.m_tracks.get_ptr(), pgenre, t_track::g_compare_genre, false);
				mmh::sort_get_permutation(m_library.m_tracks.get_ptr(), pcomposer, t_track::g_compare_composer, false);

				for (i=0; i<count_library_entries; i++)
				{
					if (i==0 || t_track::g_compare_genre(m_library.m_tracks[pgenre[i-1]], m_library.m_tracks[pgenre[i]]))
					{
						if (m_library.m_tracks[pgenre[i]]->genre_valid && m_library.m_tracks[pgenre[i]]->genre.length())
						{
							counter++;
							query.reset();
							query << "INSERT INTO genre_map (id,genre) VALUES (" << counter << ",'" << string_escape_sql(m_library.m_tracks[pgenre[i]]->genre) << "')";
							librarydb->exec(query);
						}
					}
					if (m_library.m_tracks[pgenre[i]]->genre_valid && m_library.m_tracks[pgenre[i]]->genre.length())
					{
						genre_ids[pgenre[i]] = counter;
					}
					else genre_ids[pgenre[i]] = 0;
				}

				counter = 0;

				for (i=0; i<count_library_entries; i++)
				{
					if (i==0 || t_track::g_compare_composer(m_library.m_tracks[pcomposer[i-1]], m_library.m_tracks[pcomposer[i]]))
					{
						if (m_library.m_tracks[pcomposer[i]]->composer_valid && m_library.m_tracks[pcomposer[i]]->composer.length())
						{
							counter++;
							query.reset();
							query << "INSERT INTO composer (pid,name,sort_name) VALUES (" << counter << ",'" 
								<< string_escape_sql(m_library.m_tracks[pcomposer[i]]->composer) << "','" 
								<< string_escape_sql(m_library.m_tracks[pcomposer[i]]->sort_composer_valid ? m_library.m_tracks[pcomposer[i]]->sort_composer : m_library.m_tracks[pcomposer[i]]->composer) << "')";
							librarydb->exec(query);
						}
					}
					if (m_library.m_tracks[pcomposer[i]]->composer_valid && m_library.m_tracks[pcomposer[i]]->composer.length())
					{
						composer_ids[pcomposer[i]] = counter;
					}
					else composer_ids[pcomposer[i]] = 0;
				}

			}

			count = m_library.m_tracks.get_count();
			for (i=0; i<count; i++)
			{
				pfc::rcptr_t<t_track> & track = m_library.m_tracks[i];
				if (/*!track->dshm_type_6 &&*/ (track->media_type & 0xC61010) == 0)
				{
					t_uint32 media_type;
					if (track->media_type2)
						media_type = track->media_type2;
					else
					{
						media_type = track->media_type;
						if (media_type & (t_track::type_is_voice_memo|t_track::type_itunes_u))
							media_type &= ~t_track::type_audio;
					}
					int is_song = (media_type & t_track::type_audio) ? 1 : 0;
					int is_audio_book = (media_type & t_track::type_audiobook) ? 1 : 0;
					int is_music_video = (media_type & t_track::type_music_video) ? 1 : 0;
					int is_movie = (media_type & t_track::type_video) ? 1 : 0;
					int is_tv_show = (media_type & t_track::type_tv_show) ? 1 : 0;
					int is_ringtone = (media_type & t_track::type_ringtone) ? 1 : 0;
					int is_voice_memo = (media_type & t_track::type_is_voice_memo) ? 1 : 0;
					int is_podcast = (media_type & t_track::type_podcast) ? 1 : 0;
					int is_rental = (media_type & t_track::type_rental) ? 1 : 0;
					int is_itunes_u = (media_type & t_track::type_itunes_u) ? 1 : 0;
					int is_digital_booklet = (media_type & t_track::type_digital_booklet) ? 1 : 0;
					int is_book = (media_type & t_track::type_book) ? 1 : 0;

					t_size index = pfc_infinite;
					t_int64 artist_pid = 0, album_pid = 0;
					if (pfc::bsearch_permutation_t(count_artists, m_library.m_artist_list, t_artist::g_compare_id_value, track->artist_id, perm_artists, index))
						artist_pid = m_library.m_artist_list[index]->pid;
					if (pfc::bsearch_permutation_t(count_albums, m_library.m_album_list.m_master_list, t_album::g_compare_id_value, track->album_id, perm_albums, index))
						album_pid = m_library.m_album_list.m_master_list[index]->pid;

#define INSERT_ITEM_STRING(x) \
	if (track->##x##_valid) librarydb->exec(pfc::string8() << "UPDATE item SET " << #x << "='" << string_escape_sql(track->##x) << "' WHERE pid=" << (t_int64)track->pid)

#define INSERT_ITEM_STRING2(x,y) \
	if (track->##x##_valid) librarydb->exec(pfc::string8() << "UPDATE item SET " << y << "='" << string_escape_sql(track->##x) << "' WHERE pid=" << (t_int64)track->pid)

#define INSERT_ITEM_STRING_EX(d,t,p,x,y) \
	if (track->##x##_valid) d->exec(pfc::string8() << "UPDATE " t " SET " << y << "='" << string_escape_sql(track->##x) << "' WHERE "p"=" << (t_int64)track->pid)

#define INSERT_ITEM_SORT_STRING_EX(d,t,p,x,y) \
	if (track->##x##_valid) d->exec(pfc::string8() << "UPDATE " t " SET " << y << "='" << string_escape_sql(track->sort_##x##_valid ? track->sort_##x : track->##x) << "' WHERE "p"=" << (t_int64)track->pid)

#define ITEM_SET_STRING(x) \
	if (track->##x##_valid) inner_query << (inner_query.length() ? "," : "") << #x << "='" << string_escape_sql(track->##x) << "'"

#define ITEM_SET_STRING2(x,y) \
	if (track->##x##_valid) inner_query << (inner_query.length() ? "," : "") << y << "='" << string_escape_sql(track->##x) << "'"

#define ITEM_SET_STRING_BIND(x) \
	if (track->##x##_valid) {stmt.autobind_text(track->##x, track->##x##.get_length());}

#define ITEM_SET_SORT_STRING_BIND(x) \
	{pfc::string_simple_t<WCHAR> temp;  if (track->sort_##x##_valid) {g_get_sort_string_for_sorting(track->sort_##x, temp, false);} else if (track->##x##_valid) {g_get_sort_string_for_sorting(track->##x, temp, false);} stmt.autobind_text(temp.get_ptr(), temp.length(), SQLITE_TRANSIENT);}

#define ITEM_SET_STRING_TOBIND(x) \
	if (track->##x##_valid) query << ",?"

#define ITEM_SET_SORT_STRING_TOBIND(x) \
	if (track->sort_##x##_valid || track->##x##_valid) query << ",?"

#define ITEM_SET_STRING_TOBINDF(x) \
	if (track->##x##_valid) query << ","#x

#define ITEM_SET_SORT_STRING_TOBINDF(x) \
	if (track->sort_##x##_valid || track->##x##_valid) query << ",sort_"#x

#define ITEM_SET_STRING_TOBINDF2(x,y) \
	if (track->##x##_valid) query << ","#y


					pfc::string_list_impl ExtCR;
					pfc::splitStringSimple_toList(ExtCR, '|', track->extended_content_rating);
					t_uint32 CRLevel = ExtCR.get_count() >=3 ? mmh::strtoul_n(ExtCR[2], pfc_infinite) : 0;
					query.reset();

					query << "INSERT INTO item (pid,media_kind,is_song,is_audio_book,is_music_video,is_movie,is_tv_show,is_ringtone,is_voice_memo,is_book,is_podcast,is_rental,is_itunes_u,is_digital_booklet,"
						"date_modified,year,content_rating,content_rating_level,is_compilation,is_user_disabled,remember_bookmark,exclude_from_shuffle,part_of_gapless_album,chosen_by_auto_fill,artwork_status,"
						"artwork_cache_id,start_time_ms,stop_time_ms,total_time_ms,track_number,track_count,disc_number,disc_count,bpm,"
						"relative_volume,genius_id,album_pid,artist_pid"
						",composer_pid,genre_id,title_order,artist_order,album_order,genre_order,composer_order,album_artist_order,series_name_order";
					//",title,artist,album,album_artist,composer,sort_title,sort_artist,sort_album,sort_album_artist,comment,grouping,description,description_long,eq_preset";

					ITEM_SET_STRING_TOBINDF(title);
					ITEM_SET_STRING_TOBINDF(artist);
					ITEM_SET_STRING_TOBINDF(album);
					ITEM_SET_STRING_TOBINDF(album_artist);
					ITEM_SET_STRING_TOBINDF(composer);
					ITEM_SET_SORT_STRING_TOBINDF(title);
					ITEM_SET_SORT_STRING_TOBINDF(artist);
					ITEM_SET_SORT_STRING_TOBINDF(album);
					ITEM_SET_SORT_STRING_TOBINDF(album_artist);
					ITEM_SET_SORT_STRING_TOBINDF(composer);
					ITEM_SET_STRING_TOBINDF(comment);
					ITEM_SET_STRING_TOBINDF(grouping);
					ITEM_SET_STRING_TOBINDF2(subtitle, "description");
					ITEM_SET_STRING_TOBINDF2(description, "description_long");
					ITEM_SET_STRING_TOBINDF2(collection_description, "collection_description");
					ITEM_SET_STRING_TOBINDF(copyright);
					ITEM_SET_STRING_TOBINDF2(eq_settings, "eq_preset");

					query <<
						") VALUES ("
						<< (t_int64)track->pid << ","
						<< media_type << ","
						<< is_song<< "," 
						<< is_audio_book<< "," 
						<<is_music_video<< "," 
						<<is_movie<< "," 
						<<is_tv_show<< "," 
						<<is_ringtone<< "," 
						<<is_voice_memo<< "," 
						<<is_book<< "," 
						<<is_podcast<< "," 
						<<is_rental << "," 
						<<is_itunes_u << "," 
						<< is_digital_booklet << ","
						<< sqldbtime_from_itunesdbtime(track->lastmodifiedtime) << ","
						<< track->year << ","
						<< track->content_rating << ","
						<< CRLevel << ","
						<< track->compilation << ","
						<< track->checked << ","
						<< track->remember_playback_position << ","
						<< track->skip_on_shuffle << ","
						<< (track->gapless_album !=0) << ","
						<< (track->chosen_by_auto_fill !=0) << ","
						<< track->artwork_flag << ","
						<< (track->mhii_id) << ","
						<< track->starttime << ","
						<< track->stoptime << ","
						<< track->length << ","
						<< track->tracknumber << ","
						<< track->totaltracks << ","
						<< track->discnumber << ","
						<< track->totaldiscs << ","
						<< track->bpm << ","
						<< track->volume << ","
						<< (t_int64)track->genius_id << ","
						<< album_pid << ","
						<< artist_pid

						<< ","
						<< composer_ids[i] << ","
						<< genre_ids[i]
					<< ","

						<< sort_orders.m_title_order[i] << ","
						<< sort_orders.m_artist_order[i] << ","
						<< sort_orders.m_album_order[i] << ","
						<< sort_orders.m_genre_order[i] << ","
						<< sort_orders.m_composer_order[i] << ","
						<< sort_orders.m_album_artist_order[i] << ","
						<< sort_orders.m_series_name_order[i];

#if 1
					ITEM_SET_STRING_TOBIND(title);
					ITEM_SET_STRING_TOBIND(artist);
					ITEM_SET_STRING_TOBIND(album);
					ITEM_SET_STRING_TOBIND(album_artist);
					ITEM_SET_STRING_TOBIND(composer);
					ITEM_SET_SORT_STRING_TOBIND(title);
					ITEM_SET_SORT_STRING_TOBIND(artist);
					ITEM_SET_SORT_STRING_TOBIND(album);
					ITEM_SET_SORT_STRING_TOBIND(album_artist);
					ITEM_SET_SORT_STRING_TOBIND(composer);
					ITEM_SET_STRING_TOBIND(comment);
					ITEM_SET_STRING_TOBIND(grouping);
					ITEM_SET_STRING_TOBIND(subtitle);
					ITEM_SET_STRING_TOBIND(description);
					ITEM_SET_STRING_TOBIND(copyright);
					ITEM_SET_STRING_TOBIND(eq_settings);
					//<< "?,?,?,?,?,?,?,?,?,?,?,?,?,?"
#else
					<< string_escape_quote_sql(track->title) << ","
						<< string_escape_quote_sql(track->artist) << ","
						<< string_escape_quote_sql(track->album) << ","
						<< string_escape_quote_sql(track->album_artist) << ","
						<< string_escape_quote_sql(track->composer) << ","
						<< string_escape_quote_sql(track->sort_title) << ","
						<< string_escape_quote_sql(track->sort_artist) << ","
						<< string_escape_quote_sql(track->sort_album) << ","
						<< string_escape_quote_sql(track->sort_album_artist) << ","
						<< string_escape_quote_sql(track->comment) << ","
						<< string_escape_quote_sql(track->grouping) << ","
						<< string_escape_quote_sql(track->subtitle) << ","
						<< string_escape_quote_sql(track->description) << ","
						<< string_escape_quote_sql(track->eq_settings)// << ","
#endif

						query << ")";

#if 1
					sqlite_statement stmt;
					stmt.prepare(librarydb, query, query.length());
					ITEM_SET_STRING_BIND(title);
					ITEM_SET_STRING_BIND(artist);
					ITEM_SET_STRING_BIND(album);
					ITEM_SET_STRING_BIND(album_artist);
					ITEM_SET_STRING_BIND(composer);
					ITEM_SET_SORT_STRING_BIND(title);
					ITEM_SET_SORT_STRING_BIND(artist);
					ITEM_SET_SORT_STRING_BIND(album);
					ITEM_SET_SORT_STRING_BIND(album_artist);
					ITEM_SET_SORT_STRING_BIND(composer);
					ITEM_SET_STRING_BIND(comment);
					ITEM_SET_STRING_BIND(grouping);
					ITEM_SET_STRING_BIND(subtitle);
					ITEM_SET_STRING_BIND(description);
					ITEM_SET_STRING_BIND(collection_description);
					ITEM_SET_STRING_BIND(copyright);
					ITEM_SET_STRING_BIND(eq_settings);
					stmt.finalise();
#else
					librarydb->exec(query);
#endif

					query.reset();

					if (album_pid)
					{
						query << "UPDATE album SET artist_pid=" << artist_pid << ",name_order=" << sort_orders.m_album_or_show_order[i] <<" WHERE pid=" << album_pid;
						librarydb->exec(query);
						query.reset();
					}

					if (artist_pid)
					{
						query << "UPDATE artist SET name_order=" << sort_orders.m_album_artist_order[i] <<" WHERE pid=" << artist_pid;
						librarydb->exec(query);
						query.reset();
					}

					if (composer_ids[i])
					{
						query << "UPDATE composer SET name_order=" << sort_orders.m_composer_order[i] <<" WHERE pid=" << composer_ids[i];
						librarydb->exec(query);
						query.reset();
					}

					if (genre_ids[i])
					{
						query << "UPDATE genre_map SET genre_order=" << sort_orders.m_genre_order[i]/100 <<" WHERE id=" << genre_ids[i];
						librarydb->exec(query);
						query.reset();
					}


					t_uint32 location_type = hm4(F,I,L,E), base_type = 1;
					pfc::string8 path = track->location;
					path.replace_byte(':','/');
					const char * file = path;
					if (*path == '/') file++;
					if (!stricmp_utf8_max(file, media_path, media_path_len))
						file += media_path_len;
					else if (!stricmp_utf8_max(file, ringtones_path, ringtones_path_len))
					{
						file += ringtones_path_len;
						base_type = 6;
					}
					else if (!stricmp_utf8_max(file, podcasts_path, podcasts_path_len))
					{
						file += podcasts_path_len;
						base_type = 4;
					}
					else if (!stricmp_utf8_max(file, purchases_path, purchases_path_len))
					{
						file += purchases_path_len;
						base_type = 7;
					}

					if (*file == '/') file++;

					t_uint32 extension = 0;
					pfc::string_extension ext(path);
					string_upper extupper(ext);
					if (extupper.length() == 3)
						extension = extupper[0]<<24 | extupper[1] <<16 | extupper[2] << 8 | ' ';

					query.reset();
					query << "INSERT INTO location (item_pid,sub_id,base_location_id,location_type,location,extension,date_created,file_size)VALUES ("
						<< (t_int64)track->pid << ","
						<< "0" << "," 
						<< base_type << ","
						<< location_type << ","
						//<< track->filetype << ","
						<< "'" << string_escape_sql(file) << "',"
						<< extension << ","
						<< sqldbtime_from_itunesdbtime(track->dateadded) << ","
						<< (track->filesize_64 ? track->filesize_64 : track->size)
						<< ")";

					locationsdb->exec(query);

					query.reset();
					query << "INSERT INTO avformat_info (item_pid,sub_id,audio_format,bit_rate,sample_rate,duration,gapless_heuristic_info,"
						"gapless_encoding_delay,gapless_encoding_drain,gapless_last_frame_resynch,analysis_inhibit_flags,audio_fingerprint,volume_normalization_energy) VALUES ("
						<< (t_int64)track->pid << ","
						<< "0" << ","
						<< g_translate_audio_format(track->audio_format) << ","
						<< track->bitrate << ","
						<< (track->samplerate) << ","
						<< track->samplecount << ","
						<< track->gapless_heuristic_info << ","
						<< track->encoder_delay << ","
						<< track->encoder_padding << ","
						<< track->resync_frame_offset << ","
						<< "0" << ","
						<< track->audio_fingerprint << ","
						<< track->soundcheck
						<< ")";

					librarydb->exec(query);

					if (track->video_flag)
					{
						query.reset();

						query << "INSERT INTO video_info (item_pid,has_alternate_audio,has_subtitles,characteristics_valid,has_closed_captions,"
							"is_self_contained,is_compressed,is_anamorphic,is_hd,episode_sort_id,season_number,audio_language,audio_track_index,"
							"audio_track_id,subtitle_language,subtitle_track_index,subtitle_track_id) VALUES ("
							<< (t_int64)track->pid << ","
							<< track->has_alternate_audio_and_closed_captions << ","
							<< track->has_subtitles << ","
							<< track->characteristics_valid << ","
							<< track->has_alternate_audio_and_closed_captions << ","
							<< (track->is_self_contained == 0 ?1:0) << ","
							<< track->is_compressed << ","
							<< track->is_anamorphic << ","
							<< track->is_hd << ","
							<< track->episode_number << ","
							<< track->season_number << ","
							<< (t_int16)track->audio_language << ","
							<< track->audio_track_index << ","
							<< track->audio_track_id << ","
							<< (t_int16)track->subtitle_language << ","
							<< track->subtitle_track_index << ","
							<< track->subtitle_track_id 
							<< ")";

						librarydb->exec(query);

						INSERT_ITEM_STRING_EX(librarydb,"video_info","item_pid",show,"series_name");
						INSERT_ITEM_SORT_STRING_EX(librarydb,"video_info","item_pid",show,"sort_series_name");
						INSERT_ITEM_STRING_EX(librarydb,"video_info","item_pid",tv_network,"network_name");
						INSERT_ITEM_STRING_EX(librarydb,"video_info","item_pid",episode,"episode_id");
						INSERT_ITEM_STRING_EX(librarydb,"video_info","item_pid",extended_content_rating,"extended_content_rating");
					}

					query.reset();

					if (track->podcast_flag)
					{
						query << "INSERT INTO podcast_info (item_pid, date_released, external_guid, feed_url, feed_keywords) VALUES ("
							<< (t_int64)track->pid << ","
							<< sqldbtime_from_itunesdbtime(track->datereleased, false) << ","
							<< string_escape_quote_sql(track->podcast_enclosure_url) << ","
							<< string_escape_quote_sql(track->podcast_rss_url) << ","
							<< string_escape_quote_sql(track->keywords) 
							<< ")";

						librarydb->exec(query);

						query.reset();
					}

					if (g_track_has_store_info(track))
					{
						query << "INSERT INTO store_info (item_pid,store_kind,date_purchased,date_released,store_item_id,account_id,key_versions,"
							"key_platform_id,key_id,key_id2,artist_id,composer_id,genre_id,playlist_id,storefront_id) VALUES ("
							<< (t_int64)track->pid << ","
							<< track->store_kind << ","
							<< sqldbtime_from_itunesdbtime(track->date_purchased) << "," //FIXME
							<< sqldbtime_from_itunesdbtime(track->datereleased) << "," //FIXME
							<< (t_int64)track->store_item_id << ","
							<< (t_int64)(track->account_id_primary ? track->account_id_primary : track->account_id_secondary) << ","
							<< track->store_drm_key_versions << ","
							<< track->key_platform_id << ","
							<< (t_int64)track->key_id << ","
							<< (t_int64)track->key_id2 << ","
							<< (t_int64)track->store_artist_id << ","
							<< (t_int64)track->store_composer_id << ","
							<< (t_int64)track->store_genre_id << ","
							<< (t_int64)track->store_playlist_id << ","
							<< (t_int64)track->store_front_id
							<< ")";

						librarydb->exec(query);

						query.reset();
					}

					if (track->lyrics_flag)
					{
						query << "INSERT INTO lyrics (item_pid,checksum) VALUES (" << (t_int64)track->pid << "," << (t_int32)track->lyrics_crc << ")";
						extrasdb->exec(query);
						query.reset();
					}
					if (track->chapter_data_valid && track->do_chapter_data.get_size() > 12)
					{
						query << "INSERT INTO chapter (item_pid,data) VALUES (" << (t_int64)track->pid << ",?)";
						sqlite_statement stmt;
						stmt.prepare(extrasdb, query, query.length());
						stmt.autobind_blob(track->do_chapter_data.get_ptr(), track->do_chapter_data.get_size());
						stmt.finalise();
						query.reset();
					}
				query.reset();

				query << "INSERT INTO item_stats(item_pid,has_been_played,date_played,play_count_user,play_count_recent,date_skipped,"
					"skip_count_user,skip_count_recent,bookmark_time_ms,bookmark_time_ms_common,user_rating,user_rating_common) VALUES ("
					<< (t_int64)track->pid << ","
					<< (t_uint32)(track->played_marker == 1 ? 1 : 0) << ","
					<< sqldbtime_from_itunesdbtime(track->lastplayedtime) << ","
					<< track->playcount << ","
					<< track->playcount2 << ","
					<< sqldbtime_from_itunesdbtime(track->last_skipped) << ","
					<< track->skip_count_user << ","
					<< track->skip_count_recent << ","
					<< track->bookmarktime << ","
					<< track->bookmark_time_ms_common << ","
					<< track->rating << ","
					<< track->rating
					<< ")";

				dynamicdb->exec(query);
				}
			}


			librarydb->exec("INSERT OR REPLACE INTO version_info (ROWID, major, minor, compatibility, update_level, platform) VALUES (1, 1, 55, 0, 0, 2)");
			librarydb->exec(pfc::string8() <<  "PRAGMA user_version = " << 26);

			librarydb->exec("END TRANSACTION");
			locationsdb->exec("END TRANSACTION");
			dynamicdb->exec("END TRANSACTION");
			extrasdb->exec("END TRANSACTION");

			locationsdb.release();
			dynamicdb.release();
			extrasdb.release();

			librarydb->exec(pfc::string8 () << "ATTACH " << string_escape_quote_sql(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp") << " AS " << "Dynamic");
			librarydb->exec(pfc::string8 () << "ATTACH " << string_escape_quote_sql(pfc::string8() << tempbase << "Locations.itdb.dop.temp") << " AS " << "Locations");
			librarydb->exec(pfc::string8 () << "ATTACH " << string_escape_quote_sql(pfc::string8() << tempbase << "Extras.itdb.dop.temp") << " AS " << "Extras");

			sqlite_register_functions_scope p_sqlite_register_functions_scope(librarydb, p_ipod);

			//try {
			//librarydb->exec("BEGIN TRANSACTION");

#ifdef _FORCE_SQLDB
			if (!p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands.get_count())
			{
				abort_callback_impl noabort;
				file::ptr f;
				cfobject::object_t::ptr_t root;
				{
					filesystem::g_open_read(f, "i:\\SQLMusicLibraryPostProcessCommands.plist", noabort);
					pfc::array_staticsize_t<t_uint8> d(pfc::downcast_guarded<t_uint32>(f->get_size_ex(noabort)));
					f->read(d.get_ptr(), d.get_size(), noabort);
					g_get_plist_cfobject((char *)d.get_ptr(), root);
					g_get_sql_commands(root, p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands, p_ipod->m_device_properties.m_SQLMusicLibraryUserVersion);
				}

			}
#endif

			librarydb->exec("BEGIN TRANSACTION");

			count = p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands.get_count();
			for (i=0; i<count; i++)
			{
#ifdef _DEBUG
				//console::formatter() << "iPod manager: Executing: " << p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands[i];
#endif
				try {
					librarydb->exec(p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands[i]);
				}
				catch (pfc::exception const & ex)
				{
#ifdef _DEBUG
					console::formatter() << "iPod manager: Warning - SQL post process command failed: " << ex.what() << "; Command: " << p_ipod->m_device_properties.m_SQLMusicLibraryPostProcessCommands[i];
#else
					console::formatter() << "iPod manager: Warning - SQL post process command failed: " << ex.what();
#endif
				}
			}

			librarydb->exec("END TRANSACTION");

			librarydb->exec("BEGIN TRANSACTION");

			librarydb->exec("PRAGMA journal_mode=DELETE");
			librarydb->exec("PRAGMA Locations.journal_mode=DELETE");
			librarydb->exec("PRAGMA Dynamic.journal_mode=DELETE");
			librarydb->exec("PRAGMA Extras.journal_mode=DELETE");

			librarydb->exec("END TRANSACTION");

			/*librarydb->exec("PRAGMA locking_mode=NORMAL");
			librarydb->exec("PRAGMA Locations.locking_mode=NORMAL");
			librarydb->exec("PRAGMA Dynamic.locking_mode=NORMAL");
			librarydb->exec("PRAGMA Extras.locking_mode=NORMAL");*/

			librarydb->exec(pfc::string8() << "DETACH " << "Locations");
			librarydb->exec(pfc::string8() << "DETACH " << "Dynamic");
			librarydb->exec(pfc::string8() << "DETACH " << "Extras");

			//librarydb->exec("END TRANSACTION");
			//}
			//catch (pfc::exception const & ex)
			//{
			//	console::formatter() << "iPod manager: SQL post process command failed: " << ex.what();
			//}

		}
		catch (pfc::exception const & ex)
		{
			abort_callback_dummy noabort;
			try {filesystem::g_remove(pfc::string8() << tempbase << "Library.itdb.dop.temp", noabort); } catch (pfc::exception const &) {};
			try {filesystem::g_remove(pfc::string8() << tempbase << "Extras.itdb.dop.temp", noabort); } catch (pfc::exception const &) {};
			try {filesystem::g_remove(pfc::string8() << tempbase << "Locations.itdb.dop.temp", noabort); } catch (pfc::exception const &) {};
			try {filesystem::g_remove(pfc::string8() << tempbase << "Locations.itdb.cbk.dop.temp", noabort); } catch (pfc::exception const &) {};
			try {filesystem::g_remove(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp", noabort); } catch (pfc::exception const &) {};

			throw pfc::exception(pfc::string8() << "SQLite command failed: " << ex.what());
		}

		try
		{
			abort_callback_dummy dummy_aborter;

			auto cbk_data = calculate_cbk(p_ipod, pfc::string8() << tempbase << "Locations.itdb.dop.temp");

			file::ptr cbk_file;
			filesystem::g_open_write_new(cbk_file, pfc::string8() << tempbase << "Locations.itdb.cbk.dop.temp", dummy_aborter);
			cbk_file->write(cbk_data.get_ptr(), cbk_data.get_size(), dummy_aborter);

			cbk_file.release();

			try {filesystem::g_remove(pfc::string8() << base << "Library.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_remove(pfc::string8() << base << "Extras.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_remove(pfc::string8() << base << "Locations.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_remove(pfc::string8() << base << "Locations.itdb.cbk.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_remove(pfc::string8() << base << "Dynamic.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};

			filesystem::g_copy(pfc::string8() << tempbase << "Library.itdb.dop.temp", pfc::string8() << base << "Library.itdb.dop.temp", dummy_aborter);
			filesystem::g_copy(pfc::string8() << tempbase << "Extras.itdb.dop.temp", pfc::string8() << base << "Extras.itdb.dop.temp", dummy_aborter);
			filesystem::g_copy(pfc::string8() << tempbase << "Locations.itdb.dop.temp", pfc::string8() << base << "Locations.itdb.dop.temp", dummy_aborter);
			filesystem::g_copy(pfc::string8() << tempbase << "Locations.itdb.cbk.dop.temp", pfc::string8() << base << "Locations.itdb.cbk.dop.temp", dummy_aborter);
			filesystem::g_copy(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp", pfc::string8() << base << "Dynamic.itdb.dop.temp", dummy_aborter);

			filesystem::g_remove(pfc::string8() << tempbase << "Library.itdb.dop.temp", dummy_aborter);
			filesystem::g_remove(pfc::string8() << tempbase << "Extras.itdb.dop.temp", dummy_aborter);
			filesystem::g_remove(pfc::string8() << tempbase << "Locations.itdb.dop.temp", dummy_aborter);
			filesystem::g_remove(pfc::string8() << tempbase << "Locations.itdb.cbk.dop.temp", dummy_aborter);
			filesystem::g_remove(pfc::string8() << tempbase << "Dynamic.itdb.dop.temp", dummy_aborter);

			try {filesystem::g_move(pfc::string8() << base << "Library.itdb", pfc::string8() << base << "Library.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_move(pfc::string8() << base << "Extras.itdb", pfc::string8() << base << "Extras.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_move(pfc::string8() << base << "Locations.itdb", pfc::string8() << base << "Locations.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_move(pfc::string8() << base << "Locations.itdb.cbk", pfc::string8() << base << "Locations.itdb.cbk.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};
			try {filesystem::g_move(pfc::string8() << base << "Dynamic.itdb", pfc::string8() << base << "Dynamic.itdb.dop.backup", dummy_aborter); } catch (exception_io_not_found const &) {};

			filesystem::g_move(pfc::string8() << base << "Library.itdb.dop.temp", pfc::string8() << base << "Library.itdb", dummy_aborter);
			filesystem::g_move(pfc::string8() << base << "Extras.itdb.dop.temp", pfc::string8() << base << "Extras.itdb", dummy_aborter);
			filesystem::g_move(pfc::string8() << base << "Locations.itdb.dop.temp", pfc::string8() << base << "Locations.itdb", dummy_aborter);
			filesystem::g_move(pfc::string8() << base << "Locations.itdb.cbk.dop.temp", pfc::string8() << base << "Locations.itdb.cbk", dummy_aborter);
			filesystem::g_move(pfc::string8() << base << "Dynamic.itdb.dop.temp", pfc::string8() << base << "Dynamic.itdb", dummy_aborter);
		}
		catch (pfc::exception const & ex)
		{
			throw pfc::exception(pfc::string8() << "" << ex.what());
		}
		console::formatter() << "iPod manager: Generate SQLite database completed in " << timer.query() << " s";
	}

}

pfc::array_staticsize_t<t_uint8> ipod::tasks::database_writer_t::calculate_cbk(ipod_device_ptr_ref_t p_ipod, const char* locations_itdb_path)
{
	const t_size cbk_version = p_ipod->m_device_properties.m_db_version >= 5 ? 3 : 2;
	const t_size cbk_size = cbk_version >= 3 ? 57 : 46;

	abort_callback_dummy dummy_aborter;

	file::ptr p_locationsdb;
	filesystem::g_open_read(p_locationsdb, locations_itdb_path, dummy_aborter);

	t_size fs = pfc::downcast_guarded<t_size>(p_locationsdb->get_size_ex(dummy_aborter));
	if (fs % 1024)
		throw exception_io("Locations.itdb is malformed");
	t_size i, pagecount = fs / 1024;
	pfc::array_staticsize_t<t_uint8> cbk_data(cbk_size + 20 + mmh::hash::sha1_digestsize*pagecount);
	pfc::array_staticsize_t<t_uint8> itdb_data(fs);

	p_locationsdb->read(itdb_data.get_ptr(), itdb_data.get_size(), dummy_aborter);

	for (i = 0; i<pagecount; i++)
		mmh::hash::sha1(&itdb_data[1024 * i], 1024, &cbk_data[cbk_size + 20 + mmh::hash::sha1_digestsize*i]);
	mmh::hash::sha1(cbk_data.get_ptr() + cbk_size + 20, mmh::hash::sha1_digestsize*pagecount, &cbk_data[cbk_size]);

	pfc::string8 struid;
	p_ipod->get_deviceuniqueid(struid);
	t_size udid_length = struid.get_length() / 2;

	pfc::array_t<t_uint8> uid, hash72;
	uid.set_size(udid_length);
	uid.fill_null();
	for (i = 0; i<udid_length; i++)
	{
		//if (!struid[i*2] || !struid[i*2 + 1]) break;
		uid[i] = (unsigned char)mmh::strtoul64_n(&struid[i * 2], 2, 0x10);
	}

	if (cbk_version == 3)
	{
		if (m_iPhoneCalc.is_valid_gen())
			m_iPhoneCalc.xgen_hash_ab_gen(cbk_data.get_ptr() + cbk_size, uid.get_ptr(), udid_length, cbk_data.get_ptr());
		else if (m_iPhoneCalc.is_valid() && udid_length == 20)
			m_iPhoneCalc.xgen_hash_ab(cbk_data.get_ptr() + cbk_size, uid.get_ptr(), cbk_data.get_ptr());
		else
			memset(cbk_data.get_ptr(), 0, cbk_size);
	}
	else
		itunescrypt::cbk_generate_for_sha1(cbk_data.get_ptr() + cbk_size, uid.get_ptr(), cbk_data.get_ptr());
	
	return cbk_data;
}
