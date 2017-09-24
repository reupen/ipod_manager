#pragma once

#include "reader.h"
#include "results.h"

void g_convert_file (metadb_handle_ptr src, const char * dst, const char * cmd, abort_callback & p_abort);
bool g_get_artwork_for_track (metadb_handle_ptr & p_track, album_art_data_ptr & p_out, const t_field_mappings & p_mappings, bool b_absolute_only, abort_callback & p_abort);
album_art_extractor_instance_ptr g_get_album_art_extractor_instance(const char * path, abort_callback & p_abort);

class ipod_add_files
{
public:
	class t_result
	{
	public:
		bool b_added;
		t_uint32 index;

		t_result() : b_added(false), index(0) {};
	};
	void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort);
	//pfc::string_list_impl m_error_list;
	pfc::string_list_impl m_added_items;

	pfc::list_t<results_viewer::result_t> m_errors;

	pfc::list_t<t_result, pfc::alloc_fast_aggressive> m_results;

	ipod_add_files()// : m_artwork_script("folder.jpg")
	{
		core_api::ensure_main_thread();
		m_artwork_script = settings::artwork_sources;
	};
private:
	pfc::string8 m_artwork_script;
};

bool g_is_file_supported(metadb_handle * ptr);
void g_load_info(HWND wnd, const pfc::list_base_const_t<metadb_handle_ptr> & p_list, threaded_process_v2_t & p_status);
void g_copy_file(const char * src, const char * dst, checkpoint_base * p_checkpoint, abort_callback & p_abort);
void g_downmix_51ch_to_stereo(audio_chunk * chunk);
bool g_is_ext_supported(const char * ext);
bool g_is_file_supported(const char * path);
bool g_is_file_supported(metadb_handle * ptr);
void g_set_filetimestamp(const char * path, t_filetimestamp & p_out);
t_uint32 g_get_directory_child_count(const char * path, abort_callback & p_abort);
t_filetimestamp g_filetime_to_timestamp(const LPFILETIME ft);
t_filetimestamp g_string_to_timestamp(const char * str);

