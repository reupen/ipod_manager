#include "main.h"

#include "reader.h"

void ipod_write_dopdb(const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status,abort_callback & p_abort)
{
	pfc::string8 newpath;
	bool b_opened = false;
	//string_print_drive m_path(p_ipod->drive);

	try
	{
		pfc::string8 path = m_path;
		pfc::string8 backup_path;
		path << "\\iTunes\\dopdb";

		newpath << path << ".temp";

		//static_api_ptr_t<metadb> metadb_api;
		//in_metadb_sync metadb_lock;
		//tlhm
		stream_writer_mem header;

		header.write_lendian_t(dopdb::header, p_abort);
		dopdb::writer tlhm;

		t_size i, count = p_library.m_tracks.get_count();
		for (i=0; i<count; i++)
		{
			if (p_library.m_tracks[i]->original_path_valid || p_library.m_tracks[i]->last_known_path_valid
				|| p_library.m_tracks[i]->original_filesize_valid || p_library.m_tracks[i]->original_timestamp_valid
				|| p_library.m_tracks[i]->transcoded || p_library.m_tracks[i]->original_subsong )
			{
				dopdb::writer tracklist;
				tracklist.write_element(dopdb::t_track_id, p_library.m_tracks[i]->id, p_abort);
				tracklist.write_element(dopdb::t_track_dbid, p_library.m_tracks[i]->pid, p_abort);
				if (p_library.m_tracks[i]->location_valid)
					tracklist.write_element(dopdb::t_track_location, p_library.m_tracks[i]->location.get_ptr(), p_abort);
				if (p_library.m_tracks[i]->original_path_valid)
					tracklist.write_element(dopdb::t_track_original_path, p_library.m_tracks[i]->original_path.get_ptr(), p_abort);
				if (p_library.m_tracks[i]->last_known_path_valid)
					tracklist.write_element(dopdb::t_track_last_known_path, p_library.m_tracks[i]->last_known_path.get_ptr(), p_abort);
				if (p_library.m_tracks[i]->transcoded)
					tracklist.write_element(dopdb::t_track_transcoded, p_library.m_tracks[i]->transcoded, p_abort);
				if (p_library.m_tracks[i]->original_filesize_valid)
					tracklist.write_element(dopdb::t_track_original_filesize, p_library.m_tracks[i]->original_filesize, p_abort);
				if (p_library.m_tracks[i]->original_timestamp_valid)
					tracklist.write_element(dopdb::t_track_original_timestamp, p_library.m_tracks[i]->original_timestamp, p_abort);
				if (p_library.m_tracks[i]->original_subsong_valid)
					tracklist.write_element(dopdb::t_track_original_subsong, p_library.m_tracks[i]->original_subsong, p_abort);
				if (p_library.m_tracks[i]->artwork_source_size_valid)
					tracklist.write_element(dopdb::t_track_artwork_size, p_library.m_tracks[i]->artwork_source_size, p_abort);
				if (p_library.m_tracks[i]->artwork_source_sha1_valid)
					tracklist.write_element(dopdb::t_track_artwork_sha1_hash, p_library.m_tracks[i]->artwork_source_sha1, 20, p_abort);
				tlhm.write_element(dopdb::t_root_track, tracklist.get_ptr(), tracklist.get_size(), p_abort);
			}
		}

		//tlhm.write_element(dopdb::t_root_track, tracklist.get_ptr(), tracklist.get_size(), p_abort);
		header.write(tlhm.get_ptr(), tlhm.get_size(), p_abort);

		service_ptr_t<file> p_file;
		filesystem::g_open_write_new(p_file, newpath, p_abort);
		b_opened=true;

		p_file->write(header.get_ptr(), header.get_size(), p_abort);


		p_file.release();
		b_opened=false;


		backup_path << path.get_ptr() << ".backup";
		try {filesystem::g_remove(backup_path, p_abort);} catch (exception_io_not_found const &) {};

		if (filesystem::g_exists(path, p_abort))
			filesystem::g_move(path, backup_path, p_abort);
		filesystem::g_move(newpath, path, p_abort);
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
		//throw pfc::exception
		console::print
			(pfc::string_formatter() << "iPod manager: Error writing dopdb file - " << ex.what());
	}
}
