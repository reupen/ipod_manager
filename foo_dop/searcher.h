#ifndef _DOP_SEARCH_H_
#define _DOP_SEARCH_H_

class find_songs_in_library
{
public:
	class t_entry
	{
	public:
		pfc::string8 artist;pfc::string8 title; pfc::string8 album;
		static int g_compare_title (const t_entry & item1, const t_entry & item2)
		{
			return stricmp_utf8(item1.title, item2.title);
		}
		static int g_compare_artist (const t_entry & item1, const t_entry & item2)
		{
			return stricmp_utf8(item1.artist, item2.artist);
		}
		static int g_compare_album (const t_entry & item1, const t_entry & item2)
		{
			return stricmp_utf8(item1.album, item2.album);
		}
	};
	static int g_compare_title_string (const t_entry & item1, const char * item2)
	{
		return stricmp_utf8(item1.title, item2);
	}
	static int g_compare_album_string (const t_entry & item1, const char * item2)
	{
		return stricmp_utf8(item1.album, item2);
	}
	static int g_compare_artist_string (const t_entry & item1, const char * item2)
	{
		return stricmp_utf8(item1.artist, item2);
	}

	void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort);
	struct t_result
	{
		bool have;
		t_uint32 index;
		t_result() : have(false), index(0) {};
	};
	pfc::array_t<t_result> m_result;
	pfc::list_t<t_filestats> m_stats;
};

#endif