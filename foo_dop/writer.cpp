#include "stdafx.h"

#include "ipod_manager.h"
#include "writer.h"

#ifdef _DEBUG
//#define _FORCE_SQLDB _DEBUG
#endif

namespace ipod
{
namespace tasks
{

void database_writer_t::write_artworkdb(ipod_device_ptr_ref_t p_ipod, const ipod::tasks::load_database_t & m_library, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	pfc::string8 database_folder;
	p_ipod->get_database_path(database_folder);

	if (m_library.m_artwork_valid)
	{
		try
		{
			pfc::string8 path;
			pfc::string8 backup_path, temp_path;

			{
				path = database_folder;
				path << p_ipod->get_path_separator_ptr() << "Artwork";
				if (!filesystem::g_exists(path, p_abort))
					filesystem::g_create_directory(path, p_abort);
				path<< p_ipod->get_path_separator_ptr() << "ArtworkDB";
				backup_path << path << ".dop.backup";
				temp_path << path << ".dop.temp";
				service_ptr_t<file> p_file;
				filesystem::g_open_write_new(p_file, temp_path, p_abort);
				photodb::writer w(p_file.get_ptr());
				w.write_dfhm(m_library.m_artwork, p_abort);
				p_file.release();
				if (filesystem::g_exists(path, p_abort))
				{
					if (filesystem::g_exists(backup_path, p_abort))
						filesystem::g_remove(backup_path, p_abort);
					filesystem::g_move(path, backup_path, p_abort);
				}
				filesystem::g_move(temp_path, path, p_abort);
			}
		}
		catch (const pfc::exception &)
		{
		};
	}
}

void database_writer_t::run(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & m_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	//profiler (writer_run);
	pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(1);
	progress_details[0].m_label = "Database:";
	progress_details[0].m_value = "iTunes";
	p_status.update_text_and_details("Saving database files", progress_details);
	m_library.rebuild_podcast_playlist();
	//p_status.update_progress_subpart_helper(0,15);
	if (m_library.m_library_playlist->id == NULL)
	{
		m_library.m_library_playlist->id = m_library.get_new_playlist_pid();
	}
	if (p_mappings.sort_playlists)
	{
		m_library.m_playlists.sort_t(t_playlist::g_compare_name);
	}
	try {
		m_library.add_system_voiceover_messages(p_ipod, p_mappings);

		//m_library.add_playlist_voiceover_title(p_ipod,p_mappings, m_library.m_library_playlist->id, "All songs", true);
		for (t_size i = 0, count = m_library.m_playlists_added.get_count(); i<count; i++)
		{
			m_library.add_playlist_voiceover_title(p_ipod,p_mappings,  m_library.m_playlists_added[i]->id, m_library.m_playlists_added[i]->name, false);
		}
		for (t_size i = 0, count = m_library.m_playlists_removed.get_count(); i<count; i++)
		{
			m_library.remove_playlist_voiceover_title(p_ipod, m_library.m_playlists_removed[i]->id);
		}
	} catch (pfc::exception const & ex) {console::formatter() << "iPod manager: Error updating playlist VoiceOver sounds: " << ex.what();}
	write_itunesdb(p_ipod, m_library, p_mappings, p_status, p_abort);

	pfc::string8 database_folder;
	p_ipod->get_database_path(database_folder);
	if (p_ipod->shuffle || p_ipod->m_device_properties.m_ShadowDB)
	{
		progress_details[0].m_value = "Shadow";
		p_status.update_text_and_details("Saving database files", progress_details);
		if (p_ipod->m_device_properties.m_ShadowDB && p_ipod->m_device_properties.m_ShadowDBVersion >= 2)
			ipod_write_shadowdb_v2(p_ipod, database_folder, m_library, p_status, p_abort);
		else //if (p_ipod->shuffle || p_ipod->m_device_properties.m_ShadowDB)
			ipod_write_shuffledb(p_ipod, database_folder, m_library, p_status, p_abort);
	}


	if (0)
	{
		/*try
		{
			pfc::string8 path;
			pfc::string8 backup_path;

			{
				path = m_path;
				path << "Photos\\Photo Database.dop";
				service_ptr_t<file> p_file;
				filesystem::g_open_write_new(p_file, path, p_abort);
				photodb::writer w(p_file.get_ptr());
				w.write_dfhm(m_library.m_photos, p_abort);
			}
		}
		catch (const pfc::exception &)
		{
		};*/
	}

	progress_details[0].m_value = "Artwork";
	p_status.update_text_and_details("Saving database files", progress_details);
	p_status.update_progress_subpart_helper(13,15 + (p_ipod->m_device_properties.m_SQLiteDB?15:0) );
	write_artworkdb(p_ipod, m_library, p_status, p_abort);

	progress_details[0].m_value = "iPod manager";
	p_status.update_text_and_details("Saving database files", progress_details);
	ipod_write_dopdb(database_folder, m_library, p_status, p_abort);

#ifndef _FORCE_SQLDB
	if (p_ipod->m_device_properties.m_SQLiteDB)
#endif
	{
		progress_details[0].m_value = "SQLite";
		p_status.update_text_and_details("Saving database files", progress_details);
		write_sqlitedb(p_ipod, m_library, p_mappings, p_status, p_abort);
		//p_status.update_progress_subpart_helper(14,15);
	}

	p_status.update_progress_subpart_helper(14 + (p_ipod->m_device_properties.m_SQLiteDB?15:0),15 + (p_ipod->m_device_properties.m_SQLiteDB?15:0));
	m_library.save_cache(p_status.get_wnd(), p_ipod, p_status, p_abort);

	p_status.update_progress_subpart_helper(15,15);

	g_drive_manager.on_device_modified(p_ipod, m_library);
	//Sleep(1000);
}

bool check_files_in_library_t::g_compare_meta(const metadb_handle_ptr & item1, const metadb_handle_ptr & item2)
{
	metadb_info_container::ptr p_info1, p_info2;
	if (item1->get_async_info_ref(p_info1) && item2->get_async_info_ref(p_info2))
	{
		pfc::string8 temp1, temp2;
		t_uint32 int1(NULL), int2(NULL);
		g_print_meta(p_info1->info(), "ARTIST", temp1);
		g_print_meta(p_info2->info(), "ARTIST", temp2);
		if (strcmp(temp1, temp2)) return false;
		g_print_meta(p_info1->info(), "TITLE", temp1);
		g_print_meta(p_info2->info(), "TITLE", temp2);
		if (strcmp(temp1, temp2)) return false;
		g_print_meta(p_info1->info(), "ALBUM", temp1);
		g_print_meta(p_info2->info(), "ALBUM", temp2);
		if (strcmp(temp1, temp2)) return false;
		int1 = g_print_meta_int(p_info1->info(), "TRACKNUMBER");
		int2 = g_print_meta_int(p_info2->info(), "TRACKNUMBER");
		if (int1 != int2) return false;
		int1 = g_print_meta_int(p_info1->info(), "DISCNUMBER");
		int2 = g_print_meta_int(p_info2->info(), "DISCNUMBER");
		if (int1 != int2) return false;
		return true;
	}
	return false;
}

void g_get_filestats(const char * path, t_filestats & p_out, abort_callback & p_abort)
{
	if (!stricmp_utf8_partial(path, "file://"))
	{
		SetLastError(0);
		HANDLE h1 = CreateFile(pfc::stringcvt::string_os_from_utf8(path+7), STANDARD_RIGHTS_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (!h1) throw exception_win32(GetLastError());

		GetFileTime(h1, NULL, NULL, (LPFILETIME)&p_out.m_timestamp);
		GetFileSizeEx(h1, (PLARGE_INTEGER)&p_out.m_size);
		CloseHandle(h1);
	}
	else
	{
		bool dummy;
		filesystem::g_get_stats(path, p_out, dummy, p_abort);
	}
}

void check_files_in_library_t::run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	p_status.update_text("Checking files");
	p_status.checkpoint();
//	p_status.update_progress_subpart_helper(0,3);
	//string_print_drive p_path(p_ipod->drive);
//	pfc::string8 database_path;
//	p_ipod->get_database_path(database_path);

	t_size count_tracks = p_library.m_tracks.get_count();
	t_size count_items = items.get_count();
	t_size i;

	m_result.set_size(count_items);
	//m_result.fill(t_result(false, );
	m_stats.set_count(count_tracks);

	mmh::Permutation permutation;
	permutation.set_size(count_tracks);

	for (i=0; i<count_tracks; i++)
	{

		m_stats[i] = filestats_invalid;
		if (p_library.m_tracks[i]->transcoded)
		{
			if (p_library.m_tracks[i]->original_filesize_valid)
				m_stats[i].m_size = p_library.m_tracks[i]->original_filesize;
			if (p_library.m_tracks[i]->original_timestamp_valid)
				m_stats[i].m_timestamp = p_library.m_tracks[i]->original_timestamp;
		}
		else
		{
			m_stats[i] = p_library.m_tracks[i]->m_runtime_filestats;
#if 0
			if (p_ipod->mobile)
			{
				m_stats[i].m_timestamp = filetime_time_from_appletime(p_library.m_tracks[i]->lastmodifiedtime);
			}
			else
			{
				try {
					g_get_filestats(p_library.m_handles[i]->get_path(), m_stats[i], p_abort);
				} catch (const exception_io &) {m_stats[i] = filestats_invalid;} //change for win32 io
			}
#endif
		}
//		p_status.update_progress_subpart_helper(i+1,count_tracks+count_items);
	}

	p_status.checkpoint();

	mmh::sort_get_permutation(m_stats, permutation, g_compare_filesize, false);

	pfc::list_const_permutation_t<t_filestats, const mmh::Permutation &> sorted_stats(m_stats, permutation);

	for (i=0; i<count_items; i++)
	{
		pfc::string8 can;
		p_ipod->get_root_path(can);
		t_uint32 index;
		if (!stricmp_utf8_max(items[i]->get_path(), can, can.length()) && ((index = p_library.m_handles.find_item(items[i]))!=pfc_infinite) )
		{
			m_result[i].have = true;
			m_result[i].index = index;
		}
		else
		{
			if (pfc::bsearch_t(count_tracks,sorted_stats,g_compare_filestats_with_filesize,items[i]->get_filesize(),index))
			{
				while (index > 0 && items[i]->get_filesize() == sorted_stats[index-1].m_size) 
					{
						index--;
					}

				do {
					t_filetimestamp t1=sorted_stats[index].m_timestamp;
					t_filestats t2 = items[i]->get_filestats();
					//g_get_filestats(items[i]->get_path(), t2, p_abort);

					//pfc::string8 msg; msg << "t1 raw: " << t1 << "; t2 raw: " << t2.m_timestamp;

					if (p_ipod->mobile)
					{
						if ((t1%10000000))
							t1 -= 10000000-(t1%10000000);
						if ((t2.m_timestamp%10000000))
							t2.m_timestamp -= 10000000-(t2.m_timestamp%10000000);
					}
					else
					{
						if ((t1%20000000))
							t1 += 20000000-(t1%20000000);
						if ((t2.m_timestamp%20000000))
							t2.m_timestamp += 20000000-(t2.m_timestamp%20000000);
					}
					//msg << "; t1 adj: " << t1 << "; t2 adj: " << t2.m_timestamp;
					//console::formatter() << "iPod manager: " << msg;
					t_uint64 hour = 60*60;
					hour *= 10000000;
					if (t1 == t2.m_timestamp || _abs64(t1-t2.m_timestamp) == hour)
					{
						m_result[i].have = g_compare_meta(items[i], p_library.m_handles[permutation[index]]);
						if (m_result[i].have)
						{
							m_result[i].index = permutation[index];
							break;
						}
					}
				}
				while (index +1 < count_tracks && items[i]->get_filesize() == sorted_stats[++index].m_size);
			}
		}
		p_status.update_progress_subpart_helper(i+1+count_tracks,count_tracks+count_items);
	}
	p_status.checkpoint();
}

}
}