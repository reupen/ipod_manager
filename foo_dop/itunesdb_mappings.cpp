#include "main.h"

bool t_field_mappings::get_field(const metadb_handle_ptr & ptr, const file_info & info, const char * format, const char * field, pfc::string_base & p_out) const
{
	service_ptr_t<titleformat_object> to;
	if (!strlen(format) || !static_api_ptr_t<titleformat_compiler>()->compile(to, format))
		return field ? g_print_meta(info, field, p_out) : false;
	else if (ptr.is_valid())
	{
		ptr->format_title_from_external_info(info, NULL, p_out, to, NULL);
		return !p_out.is_empty();
	}
	else
	{
		to->run(&titleformat_hook_impl_file_info(make_playable_location("", 0), &info), p_out, NULL);
		return !p_out.is_empty();
	}
}

bool t_field_mappings::get_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, artist_mapping, "ARTIST", p_out);
}

bool t_field_mappings::get_sort_field(const metadb_handle_ptr & ptr, const file_info & info, const pfc::string8 & std_format, const pfc::string8 & sort_format, const char * field, pfc::string_base & p_out) const
{
	p_out.reset();
	if (!sort_format.is_empty() || std_format.is_empty())
		return get_field(ptr, info, sort_format, allow_sort_fields ? field : NULL, p_out);
	else return false;
}

bool t_field_mappings::get_sort_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_sort_field(ptr, info, artist_mapping, sort_artist_mapping, "ARTISTSORTORDER", p_out);
}
bool t_field_mappings::get_album_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, album_artist_mapping, "ALBUM ARTIST", p_out);
}
bool t_field_mappings::get_sort_album_artist(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_sort_field(ptr, info, album_artist_mapping, sort_album_artist_mapping, "ALBUMARTISTSORTORDER", p_out);
}
bool t_field_mappings::get_album(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, album, "ALBUM", p_out);
}
bool t_field_mappings::get_sort_album(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_sort_field(ptr, info, album, sort_album_mapping, "ALBUMSORTORDER", p_out);
}
bool t_field_mappings::get_title(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, title, "TITLE", p_out);
}
bool t_field_mappings::get_sort_title(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_sort_field(ptr, info, title, sort_title_mapping, "TITLESORTORDER", p_out);
}
bool t_field_mappings::get_composer(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, composer, "COMPOSER", p_out);
}
bool t_field_mappings::get_sort_composer(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_sort_field(ptr, info, composer, sort_composer_mapping, "COMPOSERSORTORDER", p_out);
}
bool t_field_mappings::get_comment(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, comment, "COMMENT", p_out);
}
bool t_field_mappings::get_genre(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, genre, "GENRE", p_out);
}
bool t_field_mappings::get_compilation(const metadb_handle_ptr & ptr, const file_info & info, pfc::string_base & p_out) const
{
	return get_field(ptr, info, compilation, "IPOD_COMPILATION", p_out);
}
t_field_mappings::t_field_mappings()
	: artist_mapping(settings::artist_mapping), 
	album_artist_mapping(settings::album_artist_mapping),
	album(settings::album_mapping),
	title(settings::title_mapping),
	soundcheck_adjustment(settings::soundcheck_adjustment), composer(settings::composer_mapping), genre(settings::genre_mapping),
	comment(settings::comment_mapping),
	//conversion_command(settings::conversion_command), 
	//conversion_extension(settings::conversion_extension),
	check_video(settings::check_video.get_static_instance().get_state()), use_ipod_sorting(settings::use_ipod_sorting),
	add_artwork(settings::add_artwork), scan_gapless(settings::add_gapless), convert_files(settings::convert_files),
	extra_filename_characters((t_size)settings::extra_filename_characters.get_static_instance().get_state_int()),
	compilation(settings::compilation_mapping), numbers_last(settings::numbers_last), use_fb2k_artwork(settings::use_fb2k_artwork),
	conversion_use_custom_thread_count(settings::conversion_use_custom_thread_count),
	conversion_custom_thread_count(settings::conversion_custom_thread_count),
	replaygain_processing_mode(settings::replaygain_processing_mode),
	date_added_mode(settings::date_added_mode), use_dummy_gapless_data(settings::use_dummy_gapless_data),
	quiet_sync(settings::quiet_sync),
	//conversion_parameters(settings::conversion_parameters), 
	artwork_sources(settings::artwork_sources), video_thumbnailer_enabled(settings::video_thumbnailer_enabled),
	reserved_diskspace((t_size)settings::reserved_diskspace.get_static_instance().get_state_int()),
	soundcheck_rgmode(settings::soundcheck_rgmode), conversion_use_bitrate_limit(settings::conversion_use_bitrate_limit),
	conversion_bitrate_limit(settings::conversion_bitrate_limit), sort_ipod_library_playlist(settings::sort_ipod_library),
	ipod_library_sort_script(settings::ipod_library_sort_script), sort_artist_mapping(settings::sort_artist_mapping),
	sort_album_artist_mapping(settings::sort_album_artist_mapping),
	sort_title_mapping(settings::sort_title_mapping),
	sort_album_mapping(settings::sort_album_mapping),
	sort_composer_mapping(settings::sort_composer_mapping),
	allow_sort_fields(settings::allow_sort_fields), voiceover_title_mapping(settings::voiceover_title_mapping), sort_playlists(settings::sort_playlists)
{
	if (settings::active_encoder < settings::encoder_list.get_count())
		m_conversion_encoder = settings::encoder_list[settings::active_encoder];

	core_api::ensure_main_thread();

	settings::conversion_temp_files_folder.get_static_instance().get_state(conversion_temp_files_folder);

	m_media_library.prealloc(64);
	static_api_ptr_t<library_manager>()->get_all_items(m_media_library);
	m_media_library.sort_by_pointer();

};
