#include "main.h"

#include "dopdb.h"
#include "reader.h"

void ipod_read_dopdb_tracklist(fbh::StreamReaderMemblock * p_file, t_uint32 root_id, t_uint32 root_size,
	const mmh::Permutation & permutationtid,
	const mmh::Permutation & permutationdbid,
	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, pfc::array_t<t_size> > & sorted_array_tid,
	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, pfc::array_t<t_size> > & sorted_array_dbid,
	const ipod::tasks::load_database_t & p_library,
	threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	t_uint32 tid = 0; pfc::string8 location, original_path, last_known_path;
	t_uint64 dbid = 0;
	bool tid_valid = false, location_valid = false, original_path_valid = false, last_known_path_valid = false;
	bool original_filesize_valid = false, original_timestamp_valid = false, transcoded = false, dbid_valid = false, original_subsong_valid = false;
	t_uint64 original_filesize = 0, original_timestamp = 0;
	t_size original_subsong = 0;

	t_filesize artwork_source_size = NULL;
	t_uint8 artwork_source_sha1[20] = { 0 };

	bool artwork_source_size_valid = false, artwork_source_sha1_valid = false;

	t_uint32 id, size;
	t_filesize start = p_file->get_position(p_abort);
	while (p_file->get_position(p_abort) - start < root_size)
	{
		p_file->read_lendian_t(id, p_abort);
		p_file->read_lendian_t(size, p_abort);

		switch (id)
		{
			case dopdb::t_track_id:
				p_file->read_lendian_t(tid, p_abort);
				tid_valid = true;
				break;
			case dopdb::t_track_dbid:
				p_file->read_lendian_t(dbid, p_abort);
				dbid_valid = true;
				break;
			case dopdb::t_track_location:
			{
				pfc::string_buffer buff(location, size);
				p_file->read(buff, size, p_abort);
				location_valid = true;
			}
			break;
			case dopdb::t_track_original_path:
			{
				pfc::string_buffer buff(original_path, size);
				p_file->read(buff, size, p_abort);
				original_path_valid = true;
			}
			break;
			case dopdb::t_track_original_subsong:
			{
				original_subsong_valid = true;
				p_file->read_lendian_t(original_subsong, p_abort);
			}
			break;
			case dopdb::t_track_last_known_path:
			{
				pfc::string_buffer buff(last_known_path, size);
				p_file->read(buff, size, p_abort);
				last_known_path_valid = true;
			}
			break;
			case dopdb::t_track_original_filesize:
				p_file->read_lendian_t(original_filesize, p_abort);
				original_filesize_valid = true;
				break;
			case dopdb::t_track_original_timestamp:
				p_file->read_lendian_t(original_timestamp, p_abort);
				original_timestamp_valid = true;
				break;
			case dopdb::t_track_transcoded:
				p_file->read_lendian_t(transcoded, p_abort);
				break;
			case dopdb::t_track_artwork_size:
				p_file->read_lendian_t(artwork_source_size, p_abort);
				artwork_source_size_valid = true;
				break;
			case dopdb::t_track_artwork_sha1_hash:
				p_file->read(artwork_source_sha1, 20, p_abort);
				artwork_source_sha1_valid = true;
				break;
			default:
				p_file->skip_object(size, p_abort);
				break;
		};


	}
	t_size index;
	bool bydbid = false;
	if ((dbid_valid && (bydbid = p_library.m_tracks.bsearch_permutation_t(ipod::tasks::load_database_t::g_compare_track_dbid_with_dbid, dbid, permutationdbid, index))) || (tid_valid && p_library.m_tracks.bsearch_permutation_t(ipod::tasks::load_database_t::g_compare_track_id_with_id, tid, permutationtid, index)))
	{
		//index = bydbid ? index : index;
		if (!location_valid || !_stricmp(p_library.m_tracks[index]->location, location))
		{
			p_library.m_tracks[index]->original_path = original_path;
			p_library.m_tracks[index]->original_path_valid = original_path_valid;
			p_library.m_tracks[index]->last_known_path = last_known_path;
			p_library.m_tracks[index]->last_known_path_valid = last_known_path_valid;
			p_library.m_tracks[index]->original_timestamp = original_timestamp;
			p_library.m_tracks[index]->original_timestamp_valid = original_timestamp_valid;
			p_library.m_tracks[index]->original_filesize = original_filesize;
			p_library.m_tracks[index]->original_filesize_valid = original_filesize_valid;
			p_library.m_tracks[index]->original_subsong_valid = original_subsong_valid;
			p_library.m_tracks[index]->original_subsong = original_subsong;
			p_library.m_tracks[index]->transcoded = transcoded;
			p_library.m_tracks[index]->artwork_source_size_valid = artwork_source_size_valid;
			p_library.m_tracks[index]->artwork_source_size = artwork_source_size;
			p_library.m_tracks[index]->artwork_source_sha1_valid = artwork_source_sha1_valid;
			memcpy(p_library.m_tracks[index]->artwork_source_sha1, artwork_source_sha1, 20);
		}
	}
}

void ipod_read_dopdb(const char * m_path, const ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	service_ptr_t<file> _file;

	//string_print_drive m_path(p_ipod->drive);
	mmh::Permutation permutation(p_library.m_tracks.get_count()), permutationdbid(p_library.m_tracks.get_count());

	//const pfc::list_base_const_t< pfc::rcptr_t <t_track> > & const_list = p_library.m_tracks;

	mmh::sort_get_permutation(p_library.m_tracks.get_ptr(), permutation, ipod::tasks::load_database_t::g_compare_track_id, false);
	mmh::sort_get_permutation(p_library.m_tracks.get_ptr(), permutationdbid, ipod::tasks::load_database_t::g_compare_track_dbid, false);

	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, mmh::Permutation > sorted_array(p_library.m_tracks, permutation);
	//pfc::list_const_permutation_t<pfc::rcptr_t <t_track>, mmh::Permutation > sorted_arraydbid(p_library.m_tracks, permutationdbid);

	try
	{
		////p_status.update_progress_subpart_helper(0);
		pfc::string8 path = m_path;
		path << "\\" << "iTunes" << "\\" << "dopdb";
		if (filesystem::g_exists(path, p_abort))
		{
			filesystem::g_open_read(_file, path, p_abort);
			t_filesize filesize = _file->get_size_ex(p_abort);
			pfc::array_t<t_uint8> data;
			data.set_size(pfc::downcast_guarded<t_size>(filesize));
			_file->read(data.get_ptr(), data.get_size(), p_abort);
			fbh::StreamReaderMemblock stream(data.get_ptr(), data.get_size());

			fbh::StreamReaderMemblock * p_file = &stream;

			GUID guid;
			p_file->read_lendian_t(guid, p_abort);
			if (guid == dopdb::header)
			{
				t_uint32 root_id, size;
				while (!p_file->is_eof(p_abort))
				{
					//try
					//{
					p_file->read_lendian_t(root_id, p_abort);
					//} catch (const exception_io_data_truncation &) {break;}
					p_file->read_lendian_t(size, p_abort);
					switch (root_id)
					{
						case dopdb::t_root_track:
							ipod_read_dopdb_tracklist(p_file, root_id, size, permutation, permutationdbid, /*sorted_array, sorted_arraydbid, */p_library, p_status, p_abort);
							break;
						default:
							p_file->skip_object(size, p_abort);
							break;
					};
				}
			}
		}
	}
	catch (const exception_aborted &)
	{
		throw;
	}
	catch (const pfc::exception & e)
	{
		//throw pfc::exception
		console::print
			(pfc::string_formatter() << "iPod manager: Error reading dopdb - " << e.what());
	}
}
