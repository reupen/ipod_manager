#include "stdafx.h"

#include "chapter.h"
#include "file_adder.h"
#include "file_adder_conversion.h"
#include "ipod_manager.h"
#include "gapless_scanner.h"
#include "mp4.h"
#include "speech.h"

bool g_get_album_art_extractor_interface(service_ptr_t<album_art_extractor> & out,const char * path)
{
	service_enum_t<album_art_extractor> e; album_art_extractor::ptr ptr;
	pfc::string_extension ext(path);
	while(e.next(ptr)) 
	{
		if (ptr->is_our_path(path,ext)) 
		{
			out = ptr; return true;
		}
	}
	return false;
}

album_art_extractor_instance_ptr g_get_album_art_extractor_instance(const char * path, abort_callback & p_abort)
{
	album_art_extractor::ptr api;
	if (g_get_album_art_extractor_interface(api, path))
	{
		return api->open(NULL, path, p_abort);
	}
	throw exception_album_art_not_found();
}

bool g_get_artwork_for_track (metadb_handle_ptr & p_track, album_art_data_ptr & p_out, const t_field_mappings & p_mappings, bool b_absolute_only, abort_callback & p_abort)
{
	pfc::string8 path;

	p_track->format_title_legacy(NULL, path, p_mappings.artwork_sources, NULL);
	bool b_found = false;
	if (path.length())
	{

		const char * image_extensions[] = {"jpg", "jpeg", "gif", "bmp", "png"};

		t_size i, count = tabsize(image_extensions);

		bool b_absolute = path.find_first(':') != pfc_infinite || path.get_ptr()[0] == '\\';

		pfc::string8 realPath;
		if (b_absolute)
			realPath = path;
		else
			realPath << pfc::string_directory(p_track->get_path()) << "\\" << path;

		bool b_search = realPath.find_first('*') != pfc_infinite || realPath.find_first('?') != pfc_infinite;
		bool b_search_matched = false;

		if (b_search && (!b_absolute_only || b_absolute))
		{
			const char * pMainPath = realPath;
			if (!stricmp_utf8_partial(pMainPath, "file://"))
				pMainPath += 7;
			puFindFile pSearcher = uFindFirstFile(pfc::string8() << pMainPath << ".*");
			pfc::string8 searchPath = realPath;
			realPath.reset();
			if (pSearcher)
			{
				do
				{
					const char * pResult = pSearcher->GetFileName();
					for (i=0; i<count; i++)
					{
						if (!stricmp_utf8(pfc::string_extension(pResult), image_extensions[i]))
						{
							realPath << pfc::string_directory(searchPath) << "\\" << pResult;
							b_search_matched = true;
							break;
						}
					}
				}
				while (!b_search_matched && pSearcher->FindNext());
				delete pSearcher;
			}
		}

		if ((!b_search || b_search_matched) && (!b_absolute_only || b_absolute))
		{

			//if (!filesystem::g_is_remote_or_unrecognized(canPath))
			{
				file::ptr file;
				if (b_search)
				{
					pfc::string8 canPath;
					filesystem::g_get_canonical_path(realPath, canPath);
					if (!filesystem::g_is_remote_or_unrecognized(canPath))
						filesystem::g_open(file, canPath, filesystem::open_mode_read, p_abort);
				}
				else
				{
					for (i=0; i<count; i++)
					{
						pfc::string8 canPath;
						try
						{
							filesystem::g_get_canonical_path(pfc::string8() << realPath << "." << image_extensions[i], canPath);
							if (!filesystem::g_is_remote_or_unrecognized(canPath))
							{
								filesystem::g_open(file, canPath, filesystem::open_mode_read, p_abort);
								break;
							}
						}
						catch (exception_io const &)
						{
						};
					}
				}
				if (file.is_valid())
				{
					service_ptr_t<album_art_data_impl> ptr = new service_impl_t<album_art_data_impl>;
					ptr->from_stream(file.get_ptr(), pfc::downcast_guarded<t_size>(file->get_size_ex(p_abort)), p_abort);
					b_found = true;
					p_out = ptr;
				}
			}
		}
	}

	if (!b_found)
	{
		try
		{
			if (p_mappings.use_fb2k_artwork)
			{
				if (b_absolute_only)
				{
					// "should limit to embedded only". we hope.
					album_art_extractor_instance_ptr api  = g_get_album_art_extractor_instance(p_track->get_path(), p_abort);
					p_out = api->query(album_art_ids::cover_front, p_abort);
					b_found = true;
				}
				else
				{
					album_art_extractor_instance_v2::ptr artwork_api;
					artwork_api = static_api_ptr_t<album_art_manager_v2>()->open(pfc::list_single_ref_t<metadb_handle_ptr>(p_track), pfc::list_single_ref_t<GUID>(album_art_ids::cover_front), p_abort);
					p_out = artwork_api->query(album_art_ids::cover_front, p_abort);
					b_found = true;
				}
			}
		}
		catch (const exception_album_art_not_found &)
		{
		}
	}

	return b_found;
}

void ipod_add_files::run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	p_status.checkpoint();
	pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(2);
	string_format_metadb_handle_for_progress track_formatter;
	progress_details[0].m_label = "Item:";
	progress_details[0].m_value = "Processing...";
	progress_details[1].m_label = "Remaining:";
	progress_details[1].m_value << "Processing...";
	{
		p_status.update_text_and_details(pfc::string8() << "Copying files", progress_details);
	}
	////p_status.update_progress_subpart_helper(0);
	//p_status.force_update();

	bool sixg = (p_ipod->is_6g_format());

	pfc::string8 database_folder, root_path;
	p_ipod->get_database_path(database_folder);
	p_ipod->get_root_path(root_path);

	t_size i, count = items.get_count();
	pfc::string8 dest_base = database_folder;
	dest_base << p_ipod->get_path_separator_ptr() << "Music"; //"Dop Music\\";

	if (!filesystem::g_exists(dest_base, p_abort))
		filesystem::g_create_directory(dest_base, p_abort);

	const t_size max_directory_number = 49;

	pfc::array_t<t_uint32> dir_counts;
	mmh::Permutation permutation_dir_counts;
	dir_counts.set_size(max_directory_number+1);
	permutation_dir_counts.set_size(max_directory_number+1);
	for (i=0; i<max_directory_number+1; i++)
	{
		permutation_dir_counts[i] = i;
		pfc::string8 dir = dest_base;
		dir << p_ipod->get_path_separator_ptr() << "F" << pfc::format_uint(i, 2);
		if (filesystem::g_exists(dir, p_abort))
			dir_counts[i] = g_get_directory_child_count(dir, p_abort);
		else
			dir_counts[i] = 0;

	}

	mmh::sort_get_permutation(dir_counts, permutation_dir_counts, pfc::compare_t<t_uint32,t_uint32>, false);

	t_size dir_position = 0;

	m_results.set_count(count);


	pfc::string8_fastalloc dst;
	t_size count_start = p_library.m_tracks.get_count();
	//metadb_handle_list_t<pfc::alloc_fast_aggressive> handles_sent;
	//handles_sent.prealloc(count);
	//pfc::list_t<file_info_impl, pfc::alloc_fast_aggressive> infos;
	//infos.prealloc(count);
	//pfc::ptr_list_t<const file_info, pfc::list_t<const file_info *, pfc::alloc_fast_aggressive> > infos_ptr;
	//infos_ptr.prealloc(count);
	//pfc::list_t<t_filestats, pfc::alloc_fast_aggressive> stats;
	//stats.prealloc(count);
	try {
		filesystem::g_create_directory(dest_base, p_abort);
	}
	catch (exception_io_already_exists &) {}
	//pfc::list



	bit_array_bittable mask(count);
	mmh::Permutation order(count);
	mmh::Permutation identities(count);
	t_size count_nodups=0;
	if (count>0)
	{

		mmh::sort_get_permutation(items, order, pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>, false);
		//items.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());

		t_size n;
		//bool found = false;
		//for(n=0;n<count;n++)
		//	order_inverse[order[n]] = n;
		for(n=0;n<count;n++)
		{
			t_size start = n;
			while (n+1 < count && items.get_item(order[n]) == items.get_item(order[n+1]))
			{ 
				n++;
				mask.set(order[n],true);
				identities[order[n]] = order[start];
			}
			count_nodups++;
			/*
			if (items.get_item(order[n])==items.get_item(order[n+1]))
			{
			//found = true;
			mask.set(order[n+1],true);
			}
			else
			count_nodups++;*/
		}
	}
	//if (count_nodups) count_nodups++;

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	t_uint32 appletime_added = apple_time_from_filetime(g_filetime_to_timestamp(&ft));

	t_uint32 last_tid;
	t_uint64 last_dbid;
	p_library.get_next_ids(last_tid, last_dbid);
	mmh::UIntegerNaturalFormatter text_count(count_nodups);

	class processing_entry_t
	{
	public:
		pfc::rcptr_t<t_track> m_track;
		metadb_handle_ptr m_source;
		metadb_handle_ptr m_destination;
		file_info_impl m_info_source;
		bool m_toadd;
		bool m_transcode_pending;
		bool m_transcode_succeeded;
		processing_entry_t() : m_toadd(false), m_transcode_pending(false), m_transcode_succeeded(false) {};
	};

	pfc::array_t<processing_entry_t> processing_data;
	processing_data.set_count(count);

	//metadb_hint_list::ptr p_hint_list = static_api_ptr_t<metadb_io_v2>()->create_hint_list();
	
	metadb_handle_list p_hint_handles;
	pfc::list_t<file_info_impl> p_hint_info;
	pfc::list_t<t_filestats> p_hint_stats;
	pfc::ptr_list_t<const file_info> p_hint_info_ptrs;
	mmh::GenRand gen_rand;

	t_size j, counter=0, count_added=0, progress_index=0;
	try
	{
		for (j=0; j<count; j++)
		{
			i=j;
			if (!mask[i])
			{
				counter++;
				try
				{

					if (last_tid == pfc_infinite)
						throw pfc::exception("Gave up looking for suitable track tid");
					//if (last_dbid == 0xffffffffffffffff)
					//	throw pfc::exception("Gave up looking for suitable track dbid");

					dst.reset();
					metadb_handle_ptr ptr;
					pfc::string8 can;
					p_ipod->get_root_path(can);

#if 0
					service_ptr_t<chapterizer> p_chapters;
					chapter_list_impl chapters;

					if (1/*items[i]->get_subsong_index()*/)
					{
						if (!chapterizer::g_find(p_chapters, items[i]->get_path(), p_abort))
							p_chapters.release();
						else
							p_chapters->get_chapters(items[i]->get_path(), chapters, p_abort);
					}
#endif

					//console::formatter() << can << " " << items[i]->get_path();

					bool b_to_convert = false;
					bool source_ipod=!stricmp_utf8_max(items[i]->get_path(), can, can.length());
					file_info_impl & info = processing_data[i].m_info_source;
					items[i]->get_info_async(info);
					const char * reffile = info.info_get("referenced_file");
					pfc::string_extension extension(items[i]->get_path());
					bool mp4 = !stricmp_utf8(extension, "mp4");
					bool m4r = !stricmp_utf8(extension, "m4r");
					bool m4v = !stricmp_utf8(extension, "m4v");
					bool m4a = !stricmp_utf8(extension, "m4a");
					//bool video = false;
					t_uint32 mediatype = 0;
					{
						const char * kindstr = info.meta_get("MEDIA KIND", 0);
						if (kindstr)
						{
							if (!stricmp_utf8(kindstr, "music"))
								mediatype = t_track::type_audio;
							else if (!stricmp_utf8(kindstr, "movie"))
								mediatype = t_track::type_video;
							else if (!stricmp_utf8(kindstr, "tv show"))
								mediatype = t_track::type_tv_show;
							else if (!stricmp_utf8(kindstr, "music video"))
								mediatype = t_track::type_music_video;
							else if (!stricmp_utf8(kindstr, "audiobook"))
								mediatype = t_track::type_audiobook;
						}
					}
					if (!mediatype)
					{
						if (mp4 && p_mappings.check_video)
						{
							if (g_check_mp4_type(items[i]->get_path(), p_abort))
								mediatype = t_track::type_video;
						}
						else if (m4v)
							mediatype = t_track::type_video;
						else if (m4r)
							mediatype = t_track::type_ringtone;
					}
					bool b_is_video = (mediatype & (t_track::type_music_video|t_track::type_tv_show|t_track::type_video)) != 0;
					bool b_supported = g_is_file_supported(items[i].get_ptr()) && info.info_get_int("samplerate") <= 48000;
					if (items[i]->get_subsong_index() && (b_supported || (reffile && g_is_file_supported(reffile)) ))
						throw pfc::exception("Files with chapters are not currently supported by foo_dop");
					if (!b_supported)
					{
						if (source_ipod || !p_mappings.convert_files)
							throw pfc::exception("File type is not supported by the iPod");
						else
							b_to_convert = true;
					}
					else
					{
						if (!source_ipod && !b_is_video && p_mappings.conversion_use_bitrate_limit && info.info_get_bitrate() > p_mappings.conversion_bitrate_limit)
						{
							b_to_convert = true;
						}
					}

					if (!b_to_convert)
					{
						//mmh::UIntegerNaturalFormatter text_remaining(count_nodups-progress_index);
						progress_details[0].m_value = track_formatter.run(items[j]);
						progress_details[1].m_value = pfc::string8() << count_nodups-progress_index-1;
						p_status.update_text_and_details(pfc::string8() << "Copying " << text_count << " file"  << (text_count.is_plural() ? "s" : ""), progress_details);
					}

					if (b_to_convert && (p_mappings.m_conversion_encoder.m_executable.is_empty() || !g_is_ext_supported(p_mappings.m_conversion_encoder.m_file_extension)))
						throw pfc::exception("Invalid conversion settings");

					processing_data[i].m_source = items[i];

					if (!source_ipod)
					{
						dst << dest_base.get_ptr() <<  p_ipod->get_path_separator_ptr() << "F" << pfc::format_uint(permutation_dir_counts[dir_position], 2);

						if (!filesystem::g_exists(dst, p_abort))
							filesystem::g_create_directory(dst, p_abort);

						pfc::string8 dstfilename_utf8, dstext;
						if (b_to_convert)
						{
							uStringLower(dstext, p_mappings.m_conversion_encoder.m_file_extension);
							dstfilename_utf8 = pfc::string_filename(items[i]->get_path());
						}
						else
						{
							uStringLower(dstext, pfc::string_extension(items[i]->get_path()));
							dstfilename_utf8 = pfc::string_filename(items[i]->get_path());
						}
						if ((mediatype == 0 || mediatype == t_track::type_audio) && !strcmp(dstext, "mp4"))
							dstext = "m4a";
						dstfilename_utf8 << "." << dstext;
						pfc::string8 dstfilename;
						{
							//pfc::string8 dstfilename = pfc::stringcvt::string_codepage_from_utf8(pfc::stringcvt::codepage_ascii, dstfilename_utf8.get_ptr());
							pfc::stringcvt::string_wide_from_utf8 str_utf16(dstfilename_utf8);
							char * replacement = "_";
							int size_ascii = WideCharToMultiByte(20127, NULL, str_utf16.get_ptr(), str_utf16.length(), NULL, NULL, replacement, NULL);
							if (!size_ascii)
								throw exception_win32(GetLastError());

							pfc::array_t<char> str_ascii;
							str_ascii.set_size(size_ascii+1);
							str_ascii.fill_null();
							int ret = WideCharToMultiByte(20127, NULL, str_utf16.get_ptr(), str_utf16.length(), str_ascii.get_ptr(), size_ascii, replacement, NULL);
							if (!ret)
								throw exception_win32(GetLastError());
							dstfilename.set_string(str_ascii.get_ptr(), size_ascii);
						}
						dstfilename.fix_filename_chars();
						dstfilename.replace_byte('%','_');
						if (dstfilename[0] == ' ') dstfilename.set_char(0, '_');
						pfc::string_filename filename(dstfilename);
						pfc::string_extension ext(dstfilename);
						const t_size mobile_extra =  (p_ipod->mobile ? 2 : 0);
						if (dstfilename.length() > /*31*/ 4 + 4 + p_mappings.extra_filename_characters - mobile_extra)
						{
							filename.truncate(p_mappings.extra_filename_characters + 4 + 4 - 1 - ext.length()/*31 - 1 - ext.length()*/ - mobile_extra);
							dstfilename = filename;
							dstfilename << "." << ext;
						}
						pfc::string8 dsttemp; dsttemp << dst.get_ptr() << p_ipod->get_path_separator_ptr() << dstfilename;
						if (filesystem::g_exists(dsttemp, p_abort))
						{
							t_uint32 index = 0;
							const auto file_name_len = p_mappings.extra_filename_characters + 4 + 4 - 1 - ext.length() - mobile_extra;
							filename.truncate(file_name_len - 3);
							do {
								dsttemp.reset();
								dsttemp << dst.get_ptr() << p_ipod->get_path_separator_ptr();
								if (index < 5) {
									dsttemp << filename.get_ptr() << "(" << index << ")";
								} else if (index < 10) {
									const auto num_rand_bytes = file_name_len / 2;
									pfc::array_staticsize_t<uint8_t> rand_bytes(num_rand_bytes);
									gen_rand.run(rand_bytes.get_ptr(), num_rand_bytes);
									dsttemp << pfc::format_hexdump_ex(rand_bytes.get_ptr(), num_rand_bytes, "");
								} else {
									throw pfc::exception("Gave up looking for suitable filename");
								}
								dsttemp << "." << ext;
								index++;
							}
							while (filesystem::g_exists(dsttemp, p_abort));

						}
						dst = dsttemp;
#ifdef DOP_USE_SOME_WIN32_IO
						BOOL b_abort = FALSE;
						SetLastError(0);
						pfc::string8 dsrc, ddst;
						filesystem::g_get_display_path(items[i]->get_path(), dsrc);
						filesystem::g_get_display_path(dst, ddst);
						if (!CopyFileEx(pfc::stringcvt::string_os_from_utf8(dsrc),
							pfc::stringcvt::string_os_from_utf8(ddst), NULL, NULL, &b_abort,
							COPY_FILE_FAIL_IF_EXISTS))
							throw exception_win32(GetLastError());
						m_added_items.add_item(dst);
#else
						//blah, filesystem::g_copy sucks
						bool dummy;
						t_filestats stats_source, stats_dest = filestats_invalid;
						filesystem::g_get_stats(items[i]->get_path(), stats_source, dummy, p_abort);
						if (b_to_convert)
						{
							filesystem::g_open_write_new(file::ptr(), dst, p_abort);
							processing_data[i].m_transcode_pending = true;
							/*try
							{
							g_convert_file(items[i], dst, p_mappings.conversion_command, p_abort);
							}
							catch (const pfc::exception& ex)
							{
							throw pfc::exception(pfc::string_formatter() << "Conversion failed - " << ex.what());
							}*/
						}
						else
						{
							drive_space_info_t spaceinfo;
							p_ipod->get_capacity_information(spaceinfo);
							if ((t_sfilesize)spaceinfo.m_freespace - (t_sfilesize)items[i]->get_filesize() <= ((t_sfilesize)p_mappings.reserved_diskspace * (t_sfilesize)spaceinfo.m_capacity) / 1000)
								throw pfc::exception(pfc::string8 () << "Reserved disk space limit exceeded (" << "Capacity: " << spaceinfo.m_capacity << "; Free: " << spaceinfo.m_freespace << "; File To Copy: " << items[i]->get_filesize() << "; Reserved 0.1%s: " << p_mappings.reserved_diskspace << ")");
							g_copy_file(items[i]->get_path(), dst, &p_status, p_abort);
							g_set_filetimestamp(dst, stats_source.m_timestamp);
							try {
								filesystem::g_get_stats(dst, stats_dest, dummy, p_abort);
							} catch (pfc::exception const &) {};
							m_added_items.add_item(dst);
						}
#endif
						dir_counts[permutation_dir_counts[dir_position]]++;
						if (dir_position + 1 < max_directory_number + 1)
						{
							if (dir_counts[permutation_dir_counts[dir_position]] > dir_counts[permutation_dir_counts[dir_position+1]])
								dir_position++;
							else
								dir_position = 0;
						}
						else if (dir_position == max_directory_number && dir_counts[permutation_dir_counts[dir_position]] > dir_counts[permutation_dir_counts[dir_position-1]])
							dir_position=0;
						static_api_ptr_t<metadb>()->handle_create(ptr, make_playable_location(dst, 0/*items[i]->get_subsong_index()*/));
						if (!b_to_convert && stats_dest != filestats_invalid)
						{
							//p_hint_list->add_hint(ptr, info, stats_dest, true);
							p_hint_handles.add_item(ptr);
							p_hint_info.add_item(info);
							p_hint_stats.add_item(stats_dest);
						}
					}
					else
					{
						if (strlen(items[i]->get_path()) > 55 + root_path.get_length() - 1   - 31 + 4 + 4 + p_mappings.extra_filename_characters)
							throw pfc::exception("Path is too long");
						if (p_library.m_handles.have_item(items[i]))
							throw pfc::exception("File already in database");
						if (strcmp(pfc::stringcvt::string_codepage_from_utf8(pfc::stringcvt::codepage_ascii, items[i]->get_path()), items[i]->get_path()))
							throw pfc::exception("Filename contains invalid characters");
						dst = items[i]->get_path();
						ptr = items[i];
					}

					pfc::rcptr_t<t_track> track = pfc::rcnew_t<t_track>();
					{
						if (mp4 || m4a)
						{
							try {
							g_get_itunes_chapters_mp4(items[i]->get_path(), track->m_chapter_list, p_abort);
							itunesdb::chapter_writer(&stream_writer_memblock_ref(track->do_chapter_data)).write(track->m_chapter_list, p_abort);
							track->chapter_data_valid = true;
							}
							catch (pfc::exception & ex)
							{
								m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), items[i], pfc::string8() << "Could not read chapters: " << ex.what()));
							}

						}

						if (mediatype)
						{
							track->media_type = mediatype;
							track->media_type2 = mediatype;
							//if ((mediatype & t_track::type_tv_show) || (mediatype & t_track::type_video))
							//	track->remember_playback_position = true;
						}

						const char * start = dst.get_ptr();
						t_size fs = 0;
						if (p_ipod->mobile)
						{
							if (!stricmp_utf8_max(dst,"applemobiledevice://", 20))
								fs = 20;
						}
						else
						{
							fs = !stricmp_utf8_max(dst,"file://", 7) ? 7 : 0;
						}
						t_size ps = dst.find_first(':', fs);
						if (ps!=pfc_infinite)  start+=ps+1;
						track->location = start;
						track->location.replace_char('\\', ':');
						track->location.replace_char('/', ':');
						track->location_valid = true;
						if (!source_ipod)
						{
							track->original_path = items[i]->get_path();
							track->original_path_valid = true;
							track->original_subsong = items[i]->get_subsong_index();
							track->original_subsong_valid = true;

							track->original_timestamp = items[i]->get_filetimestamp();
							track->original_filesize = items[i]->get_filesize();
							track->original_filesize_valid = true;
							track->original_timestamp_valid = true;
							if (b_to_convert)
							{
								track->transcoded = true;
							}
						}
						processing_data[i].m_toadd = true;
						processing_data[i].m_track = track;
						processing_data[i].m_destination = ptr;

					}
					if (!b_to_convert)
						progress_index++;
					p_status.checkpoint();

				}
				catch (const exception_aborted &) {throw;}
				catch (const pfc::exception & ex)
				{
					m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), items[i], pfc::string8() << "Failed to add file to iPod: " << ex.what()));
				}

			}
			//else
				

			p_status.update_progress_subpart_helper(progress_index,count_nodups*3);
		}
		{
			t_size convcount=0;
			j=0;
			pfc::array_t<conversion_entry_t> entries;
			pfc::array_t<t_size> source_indices;
			for (i=0; i<count; i++)
			{
				if (processing_data[i].m_transcode_pending)
					convcount++;
			}
			if (convcount)
			{
				//p_status.update_text("Encoding files...");
				entries.set_count (convcount);
				source_indices.set_count (convcount);
				for (i=0; i<count; i++)
				{
					if (processing_data[i].m_transcode_pending)
					{
						entries[j].m_source = processing_data[i].m_source;
						entries[j].m_destination = processing_data[i].m_destination->get_path();
						entries[j].m_destination_handle = processing_data[i].m_destination;
						entries[j].m_info.copy_meta(processing_data[i].m_info_source);
						source_indices[j] = i;
						j++;
					}
				}
				conversion_manager_t p_converter;
				t_size threadCount = p_mappings.conversion_use_custom_thread_count ? settings::conversion_custom_thread_count : std::thread::hardware_concurrency();
				if (threadCount<1) threadCount =1;
				p_converter.initialise(entries, p_mappings,threadCount);
				p_converter.run(p_ipod, p_mappings, p_status, progress_index, count_nodups*3, p_abort);

				for (j=0; j<convcount; j++)
				{
					if (!p_converter.get_index_succeeded(j))
					{
						m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), items[source_indices[j]], pfc::string8() << "Failed to add file to iPod: " << "Conversion failed - " << p_converter.get_index_error(j)));

					}
					else
					{
						processing_data[source_indices[j]].m_transcode_succeeded = true;
						m_added_items.add_item(processing_data[source_indices[j]].m_destination->get_path());
					}
				}
			}
		}
	}
	catch (exception_aborted const &) {};
	for (i=0; i<count; i++)
	{
		if (processing_data[i].m_toadd)
		{
			if (processing_data[i].m_transcode_pending && !processing_data[i].m_transcode_succeeded)
			{
				if (processing_data[i].m_destination.is_valid())
				{
					try 
					{
						filesystem::g_remove(processing_data[i].m_destination->get_path(), abort_callback_dummy());
					}
					catch (pfc::exception const &) 
					{
					};
				}
			}
			else if (!processing_data[i].m_transcode_pending || processing_data[i].m_transcode_succeeded)
			{
				metadb_handle_list updateitems;
				updateitems.add_item(processing_data[i].m_destination);

				if (processing_data[i].m_transcode_pending)
				{
					file_info_impl info2;
					processing_data[i].m_destination->get_info_async(info2);
					processing_data[i].m_track->set_from_metadb_handle(items[i], processing_data[i].m_destination->get_location(), info2, processing_data[i].m_destination->get_filestats(), p_mappings);
				}
				else
					processing_data[i].m_track->set_from_metadb_handle(items[i], items[i]->get_location(), processing_data[i].m_info_source, items[i]->get_filestats(), p_mappings);

				m_results[i].b_added = true;
				count_added++;
				if (p_mappings.date_added_mode == 0)
				{
					processing_data[i].m_track->dateadded = appletime_added;
				}
				else
				{
					pfc::string8 str;
					processing_data[i].m_source->format_title_legacy(NULL, str, "[%added%]", NULL); //Local Time!!
					if (str.length())
					{
						processing_data[i].m_track->dateadded = apple_time_from_filetime(g_string_to_timestamp(str), false);
					}
					else
						processing_data[i].m_track->dateadded = 0;
				}
				processing_data[i].m_track->id = ++last_tid;
				processing_data[i].m_track->pid = p_library.get_new_track_pid();//++last_dbid;
				processing_data[i].m_track->dbid2 = processing_data[i].m_track->pid;
				m_results[i].index = p_library.m_tracks.add_item(processing_data[i].m_track);
				p_library.m_handles.add_item(processing_data[i].m_destination);
				itunesdb::t_playlist_entry pe;
				pe.position_valid = true;
				pe.position = m_results[i].index+1;
				pe.track_id = processing_data[i].m_track->id;
				p_library.m_library_playlist->items.add_item(pe);
				//handles_sent.add_item(processing_data[i].m_destination);
			}
		}
	}
	for (i=0; i<count; i++)
	{
		if (mask[i])
		{
			m_results[i].b_added = m_results[identities[i]].b_added;
			m_results[i].index = m_results[identities[i]].index;
		}
	}
	//p_hint_list->on_done();
	for (t_size i=0, count = p_hint_info.get_count(); i<count; i++)
		p_hint_info_ptrs.add_item(&p_hint_info.get_ptr()[i]);
	static_api_ptr_t<metadb_io>()->hint_multi_async(p_hint_handles, p_hint_info_ptrs, p_hint_stats, bit_array_true());
	p_library.repopulate_albumlist();
	p_status.checkpoint();
	if (p_ipod->m_device_properties.m_ShadowDBVersion == 2 && p_ipod->m_device_properties.m_Speakable)
	{
		try	{
		p_status.update_text("Generating VoiceOver sounds");
		pfc::string8 tracksVoicePath;
		p_ipod->get_database_path(tracksVoicePath);
		tracksVoicePath << p_ipod->get_path_separator_ptr() << "Speakable";
		try { filesystem::g_create_directory(tracksVoicePath, p_abort); } catch (exception_io_already_exists const &) {};
		tracksVoicePath << p_ipod->get_path_separator_ptr() << "Tracks";
		try { filesystem::g_create_directory(tracksVoicePath, p_abort); } catch (exception_io_already_exists const &) {};
		tracksVoicePath << p_ipod->get_path_separator_ptr();

		titleformat_object::ptr to_track;
		static_api_ptr_t<titleformat_compiler>()->compile_safe(to_track, p_mappings.voiceover_title_mapping.length() ? p_mappings.voiceover_title_mapping : "Unknown");
		
		coinitialise_scope coinit(COINIT_MULTITHREADED);

		sapi pSAPI;
		if (pSAPI.is_valid())
		{
			for (i=0; i<count; i++)
			{
				if (!mask[i] && m_results[i].b_added)
				{
					pfc::string8 path, text;
					path << tracksVoicePath << pfc::format_hex(p_library.m_tracks[m_results[i].index]->pid, 16) << ".wav";

					items[i]->format_title(NULL, text, to_track, NULL);

					if (text.is_empty()) text = "Unknown";

					try {
						drive_space_info_t spaceinfo;
						p_ipod->get_capacity_information(spaceinfo);
						if ((t_sfilesize)spaceinfo.m_freespace <= ((t_sfilesize)p_mappings.reserved_diskspace * (t_sfilesize)spaceinfo.m_capacity) / 1000)
							throw pfc::exception(pfc::string8 () << "Reserved disk space limit exceeded (" << "Capacity: " << spaceinfo.m_capacity << "; Free: " << spaceinfo.m_freespace << "; Reserved 0.1%s: " << p_mappings.reserved_diskspace << ")");
						pSAPI.run_mapped(text, p_ipod->m_device_properties.m_SpeakableSampleRate, path);
					}
					catch (pfc::exception const & ex) 
					{
						m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), items[i], pfc::string8() << "Failed to generate VoiceOver sound for track: " << ex.what()));
					}
				}
			}
		}
		}
		catch (pfc::exception const & ex) 
		{
			m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), metadb_handle_ptr(), pfc::string8() << "Error generating VoiceOver sounds: " << ex.what()));
		}
	}
	p_status.checkpoint();
	if (p_mappings.add_artwork /*&& m_artwork_script.get_length()*/ && p_ipod->m_device_properties.m_artwork_formats.get_count())
	{
		progress_details[0].m_value = "Processing...";
		progress_details[1].m_value = "Processing...";
		p_status.update_text_and_details("Copying artwork", progress_details);
		pfc::string8 empty_album_artist, empty_artist, empty_album;
		{
			file_info_impl empty_info;
			p_mappings.get_artist(metadb_handle_ptr(), empty_info, empty_artist); //deal with "(None)" or "?" field remappings
			p_mappings.get_album(metadb_handle_ptr(), empty_info, empty_album);
		}
		t_size count_tracks = p_library.m_tracks.get_count();
		mmh::Permutation permutation_album_grouping;
		permutation_album_grouping.set_count(count_tracks);
		mmh::sort_get_permutation(p_library.m_tracks, permutation_album_grouping, ipod::tasks::load_database_t::g_compare_track_album_id, false);
		counter=0;
		pfc::rcptr_t<video_thumbailer_t> p_video_thumbailer;
		for (j=0; j<count; j++)
		{
			i=j;//order[j];
			if (!mask[i] && m_results[i].b_added)
			{
				counter++;
				{
					//mmh::UIntegerNaturalFormatter text_remaining(count_added-counter);
					progress_details[0].m_value = track_formatter.run(items[j]);
					progress_details[1].m_value = pfc::string8() << count_added-counter;
					p_status.update_text_and_details(pfc::string8() << "Copying artwork for " << text_count << " file" << (text_count.is_plural() ? "s" : ""), progress_details);
				}
				bool b_album_track = p_library.m_tracks[m_results[i].index]->media_type == t_track::type_audio && strcmp(p_library.m_tracks[m_results[i].index]->album, empty_album) != 0 && p_library.m_tracks[m_results[i].index]->album.length();
				t_size k = permutation_album_grouping.find_item(m_results[i].index); //should never be pfc_infinite
				/*if (k && p_library.m_tracks[i]->album_id == p_library.m_tracks[permutation_album_grouping[k-1]]->album_id))
				{
				if (((!p_library.m_tracks[i]->album_artist_valid && strcmp(p_library.m_tracks[i]->artist, empty_artist)) || strcmp(p_library.m_tracks[i]->album_artist, empty_album_artist)) && )
				b_prev_same_group = true;
				}*/

				try
				{
					album_art_data_ptr artwork_data;
					g_get_artwork_for_track(items[i], artwork_data, p_mappings, false, p_abort);
					if (!artwork_data.is_valid() && p_library.m_tracks[m_results[i].index]->video_flag && p_mappings.video_thumbnailer_enabled)
					{
						const char * path = items[i]->get_path();
						if (!stricmp_utf8_max(path, "file://", 7))
						{
							if (!p_video_thumbailer.is_valid())
								p_video_thumbailer = pfc::rcnew_t<video_thumbailer_t>();
							p_video_thumbailer->create_video_thumbnail(path+7, artwork_data);
						}
					}
						if (artwork_data.is_valid())
						{
							pfc::rcptr_t<itunesdb::t_track> p_track = p_library.m_tracks[m_results[i].index] ;
							p_track->artwork_source_size = artwork_data->get_size();
							p_track->artwork_source_size_valid = true;
							mmh::hash::sha1((const t_uint8*)artwork_data->get_ptr(), artwork_data->get_size(), p_track->artwork_source_sha1);
							p_track->artwork_source_sha1_valid = true;
							bool b_processed = false;
							if (sixg && b_album_track)
							{
								t_size l = k;
								if (l)
									while (--l && !b_processed && p_library.m_tracks[m_results[i].index]->album_id == p_library.m_tracks[permutation_album_grouping[l]]->album_id)
									{
										pfc::rcptr_t<itunesdb::t_track> p_album_track = p_library.m_tracks[permutation_album_grouping[l]];
										if (p_album_track->artwork_cache_id && (!p_album_track->artwork_source_sha1_valid || !memcmp(p_track->artwork_source_sha1, p_album_track->artwork_source_sha1, mmh::hash::sha1_digestsize)))
										{
											p_library.m_tracks[m_results[i].index]->artwork_cache_id = p_album_track->artwork_cache_id;
											p_library.m_tracks[m_results[i].index]->artwork_count = 1;
											p_library.m_tracks[m_results[i].index]->artwork_flag = 1;
											p_library.m_tracks[m_results[i].index]->artwork_size = p_album_track->artwork_size;
											b_processed = true;
										}
										//l--;
									}
									l=k;
									if (l<count_tracks)
										while (++l < count_tracks && !b_processed && p_library.m_tracks[m_results[i].index]->album_id == p_library.m_tracks[permutation_album_grouping[l]]->album_id)
										{
											pfc::rcptr_t<itunesdb::t_track> p_album_track = p_library.m_tracks[permutation_album_grouping[l]];
											if (p_album_track->artwork_cache_id && (!p_album_track->artwork_source_sha1_valid || !memcmp(p_track->artwork_source_sha1, p_album_track->artwork_source_sha1, mmh::hash::sha1_digestsize)))
											{
												p_library.m_tracks[m_results[i].index]->artwork_cache_id = p_album_track->artwork_cache_id;
												p_library.m_tracks[m_results[i].index]->artwork_count = 1;
												p_library.m_tracks[m_results[i].index]->artwork_flag = 1;
												p_library.m_tracks[m_results[i].index]->artwork_size = p_album_track->artwork_size;
												b_processed = true;
											}
										}
							}
							bool b_incRef = true;
							if (!b_processed)
							{
								if (!p_library.m_artwork_valid)
									p_library.m_artwork.initialise_artworkdb(p_ipod);
								p_library.m_artwork_valid=true;
								//pfc::dynamic_assert(p_library.m_artwork_valid, "No ArtworkDB found");
								b_incRef = p_library.m_artwork.add_artwork_v3(p_ipod, p_library.m_tracks[m_results[i].index]->media_type, p_library.m_tracks[m_results[i].index]->pid, p_library.m_tracks[m_results[i].index]->artwork_cache_id, artwork_data, count_added, p_abort, &p_library.m_tracks[m_results[i].index]->m_chapter_list);
								p_library.m_tracks[m_results[i].index]->artwork_count = 1;
								p_library.m_tracks[m_results[i].index]->artwork_flag = 1;
								p_library.m_tracks[m_results[i].index]->artwork_size = artwork_data->get_size();
								b_processed = true;
							}// else console::formatter() << artwork;
							if (sixg && b_processed && b_incRef)
							{
								t_size ii_index;
								if (p_library.m_artwork.find_by_image_id(ii_index, p_library.m_tracks[m_results[i].index]->artwork_cache_id))
								{
									p_library.m_artwork.image_list[ii_index].refcount++;
									p_library.m_artwork.image_list[ii_index].unk8 = 1;
								}
							}

						
					}
					p_status.checkpoint();
				}
				catch (const exception_aborted &)
				{
					try {
						p_library.m_artwork.finalise_add_artwork_v2(p_ipod, abort_callback_dummy());
					} catch (pfc::exception &) {};
					throw;
				}
				catch (const pfc::exception & ex)
				{
					m_errors.add_item(results_viewer::result_t(metadb_handle_ptr(), items[i], pfc::string8() << "Failed to add album art for track: " << ex.what()));
					//pfc::string8_fast_aggressive err; err << "Failed to add album art for track: " <<  items[i]->get_path() << "; Reason: " <<ex.what();
					//m_error_list.add_item(err);
				}
			}
			p_status.update_progress_subpart_helper(j+1+count,count*3);
		}
		p_library.m_artwork.finalise_add_artwork_v2(p_ipod, abort_callback_dummy());
	}
	//p_library.m_handles.add_items(handles_sent);
	if (p_mappings.scan_gapless)
	{
		//p_status.update_text("Sending files (determining gapless information) ...");
		counter=0;
		for (j=0; j<count; j++)
		{
			i=j;//order[j];
			if (!mask[i] && m_results[i].b_added)
			{
				counter++;
				{
					//mmh::UIntegerNaturalFormatter text_remaining(count_added-counter);
					progress_details[0].m_value = track_formatter.run(items[j]);
					progress_details[1].m_value = pfc::string8() << count_added-counter;
					p_status.update_text_and_details(pfc::string8() << "Updating gapless information for " << text_count << " file" << (text_count.is_plural() ? "s" : ""), progress_details);
				}
				itunesdb::t_track & track = *p_library.m_tracks[m_results[i].index];
				metadb_handle_ptr ptr = track.transcoded ? p_library.m_handles[m_results[i].index] : items[i];

				try
				{
					ipod::tasks::gapless_scanner_t::g_scan_gapless(NULL, track, ptr, p_mappings.use_dummy_gapless_data, p_abort);
				}
				catch (pfc::exception & ex)
				{
					m_errors.add_item(results_viewer::result_t(p_library.m_handles[m_results[i].index], items[i], pfc::string8() << "Failed to add gapless data for file: " << ex.what()));
				}
				p_status.checkpoint();
			}
			p_status.update_progress_subpart_helper(j+1+count*2,count*3);
		}
	}

	//}
	//	t_size count_end = p_library.m_tracks.get_count();
	//	p_library.glue_items(count_start);
	//static_api_ptr_t<metadb_io> metadb_io;
	//metadb_io->hint_multi_async(handles_sent,infos_ptr,stats,bit_array_val(true));
}
