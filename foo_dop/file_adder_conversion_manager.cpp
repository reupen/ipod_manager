#include "stdafx.h"

#include "file_adder_conversion.h"

void conversion_manager_t::initialise(const pfc::array_t<conversion_entry_t>& entries, const t_field_mappings & p_mappings, t_size thread_count)
{
	t_uint8 replaygain_processing_mode = p_mappings.replaygain_processing_mode;
	t_size i, count = entries.get_count();

	m_command = p_mappings.m_conversion_encoder;
	m_threads.set_count(thread_count);
	m_entries.set_count(count);

	m_permutation.set_count(count);
	m_inverse_permutation.set_count(count);
	m_mask_flush_items.resize(count);
	//m_mask_move_processed.resize(count);

	{
		pfc::list_t<pfc::string8> sort_entries;
		bit_array_bittable mask_album_valid(count);
		sort_entries.set_count(count);
		for (i = 0; i<count; i++)
		{
			pfc::string8 temp;
			bool b_artist_valid = false, b_album_valid = false;
			if (!(b_artist_valid = g_print_meta_noblanks(entries[i].m_info, "ALBUM ARTIST", sort_entries[i])))
				b_artist_valid = g_print_meta_noblanks(entries[i].m_info, "ARTIST", sort_entries[i]);
			b_album_valid = g_print_meta_noblanks(entries[i].m_info, "ALBUM", temp);
			sort_entries[i] << " - " << temp;
			mask_album_valid.set(i, b_artist_valid && b_album_valid);
		}
		mmh::sort_get_permutation(sort_entries.get_ptr(), m_permutation, stricmp_utf8, false);
		mmh::InversePermutation invperm(m_permutation);
		m_inverse_permutation = invperm;
		pfc::list_permutation_t<pfc::string8> permentries(sort_entries, m_permutation.get_ptr(), m_permutation.get_count());
		for (i = 0; i<count; i++)
		{
			//console::formatter() << "sorted index: " << invperm[i] << " permidx: " << i;
			if (!mask_album_valid[m_permutation[i]] || i + 1 == count || stricmp_utf8(permentries[i], permentries[i + 1]))
			{
				m_mask_flush_items.set(i, true);
				//console::formatter() << "setting flush index: " << invperm[i] << " permidx: " << i;
			}
		}
	}

	if (replaygain_processing_mode == settings::replaygain_processing_scan_after_encoding)
	{
		try
		{
			m_replaygain_api = standard_api_create_t<replaygain_scanner_entry>();
		}
		catch (const exception_service_not_found &)
		{
		};
	}

	for (i = 0; i<count; i++)
	{
		try
		{
			pfc::string8 tempFolder = p_mappings.conversion_temp_files_folder, tempFile;
			if (!tempFolder.length() && !uGetTempPath(tempFolder))
				throw pfc::exception("uGetTempPath failed");
			char last_char = tempFolder.is_empty() ? 0 : tempFolder[tempFolder.get_length() - 1];
			if (last_char != '\\' && last_char != '/')
				tempFolder.add_byte('\\');
			DWORD attribs = uGetFileAttributes(tempFolder);
			if (attribs == INVALID_FILE_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY))
				throw pfc::exception("Invalid conversion temporary files folder");
			tempFile << tempFolder << "dop" << pfc::format_hex(i + 1, 8) << ".dop.tmp." << pfc::string_extension(entries[i].m_destination);
			//if (!uGetTempFileName(tempFolder, "dop", i+1, tempFile))
			//	throw pfc::exception("uGetTempFileName failed");
			//tempFile << ".dop." << pfc::string_extension(entries[i].m_destination);
			m_entries[i].m_temporary_destination = tempFile;
			m_entries[i].m_source = entries[i].m_source;
			m_entries[i].m_destination = entries[i].m_destination;
			m_entries[i].m_destination_handle = entries[i].m_destination_handle;
			m_entries[i].m_info = entries[i].m_info;
		}
		catch (pfc::exception & ex)
		{
			m_entries[i].m_early_fail = true;
			m_entries[i].m_error = ex.what();
		}

	}
	for (i = 0; i<count; i++)
	{
		try
		{
			if (!m_entries[i].m_early_fail)
				filesystem::g_remove(m_entries[i].m_temporary_destination, abort_callback_impl());
		}
		catch (const exception_io_not_found &)
		{
		}
		catch (pfc::exception & ex)
		{
			m_entries[i].m_succeeded = false;
			m_entries[i].m_error = ex.what();
		}
	}


	for (i = 0; i<thread_count; i++)
	{
		m_threads[i].set_command(m_command);
		m_threads[i].set_replaygain_data(replaygain_processing_mode, (t_uint8)p_mappings.soundcheck_rgmode, m_replaygain_api);
	}
}

void conversion_manager_t::flush_filemoves(conversion_filemover_thread_t & p_mover, const bit_array_var & mask_flush, const bit_array_var & mask_processed)
{
	bool b_need_move = false;
	t_size count = count = m_entries.get_count();
	do {
		replaygain_result::ptr album_replaygain;
		b_need_move = false;
		t_size start = m_move_pointer, ptr = start;
		while (ptr < count && mask_processed[m_permutation[ptr]])
		{
			if (mask_flush[ptr])
			{
				ptr++;
				b_need_move = true;
				break;
			}
			ptr++;
		}
		if (b_need_move)
		{
			t_size i;
			//console::formatter() << "merging album gain";
			for (i = start; i<ptr; i++)
			{
				//console::formatter() << m_inverse_permutation[i] << " " << i << " " << m_permutation[i];
				if (m_entries[m_permutation[i]].m_replaygain_result.is_valid())
					album_replaygain = album_replaygain.is_valid() ? album_replaygain->merge(m_entries[m_permutation[i]].m_replaygain_result) : m_entries[m_permutation[i]].m_replaygain_result;
			}
			for (i = start; i<ptr; i++)
			{
				if (m_entries[m_permutation[i]].m_succeeded_to_temp_file)
				{
					//console::formatter() << "requesting move of: " << m_permutation[i];
					p_mover.request(m_permutation[i], m_entries[m_permutation[i]].m_replaygain_result, album_replaygain);
				}
			}
			m_move_pointer = ptr;
		}
	} while (b_need_move && m_move_pointer < count && mask_processed[m_permutation[m_move_pointer]]);
}

void conversion_manager_t::run(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, t_size progress_start, t_size progress_range, abort_callback & p_abort)
{
	t_size i, count = m_entries.get_count();
	t_size index = 0, threadcount = m_threads.get_count();
	t_size thread_index = 0;

	string_format_metadb_handle_for_progress track_formatter;

	threadcount = min(threadcount, count);

	conversion_filemover_thread_t p_mover;
	{
		pfc::array_t<conversion_filemover_entry_t> moverentries;
		moverentries.set_count(count);
		for (i = 0; i<count; i++)
		{
			moverentries[i].m_source << "file://" << m_entries[i].m_temporary_destination;
			moverentries[i].m_destination = m_entries[i].m_destination;
			moverentries[i].m_destination_handle = m_entries[i].m_destination_handle;
			moverentries[i].m_info = m_entries[i].m_info;
		}
		p_mover.initialise(p_ipod, p_mappings.reserved_diskspace, moverentries, &p_status);
	}

	p_mover.create_thread();

	pfc::array_staticsize_t<pfc::string8> progress_filenames(threadcount);

	while (thread_index < threadcount)
	{
		//thread_index = index%threadcount;
		if (!m_entries[index].m_early_fail)
		{
			m_threads[thread_index].initialise(m_permutation[index], m_entries[m_permutation[index]].m_source, m_entries[m_permutation[index]].m_temporary_destination, &p_status);
			m_threads[thread_index].create_thread();
			progress_filenames[thread_index] = track_formatter.run(m_entries[m_permutation[index]].m_source);
			thread_index++;
		}
		index++;
		if (index >= count) break;
	}

	threadcount = min(threadcount, thread_index);

	if (threadcount)
	{
		{
			mmh::UIntegerNaturalFormatter text_remaining(count), text_count(progress_range / 3);

			pfc::array_t<threaded_process_v2_t::detail_entry> progress_details;
			for (t_size pfindex = 0, pfcount = progress_filenames.get_size(); pfindex<pfcount; pfindex++)
			{
				if (!progress_filenames[pfindex].is_empty())
					progress_details.append_single(threaded_process_v2_t::detail_entry("Item:", progress_filenames[pfindex]));
			}
			progress_details.append_single(threaded_process_v2_t::detail_entry("Remaining:", pfc::string8() << count - index));

			p_status.update_text_and_details(pfc::string8() << "Copying " << text_count << " file" << (text_count.is_plural() ? "s" : "") << " - encoding", progress_details);
		}

		pfc::array_t< HANDLE > waitobjects;
		waitobjects.set_count(threadcount);

		for (thread_index = 0; thread_index<threadcount; thread_index++)
			waitobjects[thread_index] = m_threads[thread_index].get_thread();

		DWORD res = WaitForMultipleObjectsEx(threadcount, waitobjects.get_ptr(), FALSE, pfc_infinite, FALSE);

		t_size progress_index = 0;

		bit_array_bittable mask_processed(count);


		while (res >= WAIT_OBJECT_0 && res < WAIT_OBJECT_0 + threadcount && !p_abort.is_aborting())
		{
			thread_index = res - WAIT_OBJECT_0;
			m_threads[thread_index].wait_for_and_release_thread();
			m_entries[m_threads[thread_index].m_index].m_succeeded_to_temp_file = m_threads[thread_index].m_succeeded;
			m_entries[m_threads[thread_index].m_index].m_error = m_threads[thread_index].m_error;
			m_entries[m_threads[thread_index].m_index].m_replaygain_result = m_threads[thread_index].m_replaygain_result;
			mask_processed.set(m_threads[thread_index].m_index, true);
			progress_filenames[thread_index].reset();
			//console::formatter() << "flushing: " << m_threads[thread_index].m_index;

			//if (m_threads[thread_index].m_succeeded)
			{
				flush_filemoves(p_mover, m_mask_flush_items, mask_processed);
				//p_mover.request(m_threads[thread_index].m_index);
			}
			if (index < count)
			{
				m_threads[thread_index].initialise(m_permutation[index], m_entries[m_permutation[index]].m_source, m_entries[m_permutation[index]].m_temporary_destination, &p_status);
				progress_filenames[thread_index] = track_formatter.run(m_entries[m_permutation[index]].m_source);
				index++;
				m_threads[thread_index].create_thread();
				waitobjects[thread_index] = m_threads[thread_index].get_thread();
				res = WaitForMultipleObjectsEx(threadcount, waitobjects.get_ptr(), FALSE, pfc_infinite, FALSE);
			}

			{
				progress_index++;
				mmh::UIntegerNaturalFormatter text_remaining(count - progress_index), text_count(progress_range / 3);
				pfc::array_t<threaded_process_v2_t::detail_entry> progress_details;
				for (t_size pfindex = 0, pfcount = progress_filenames.get_size(); pfindex<pfcount; pfindex++)
				{
					if (!progress_filenames[pfindex].is_empty())
						progress_details.append_single(threaded_process_v2_t::detail_entry("Item:", progress_filenames[pfindex]));
				}
				progress_details.append_single(threaded_process_v2_t::detail_entry("Remaining:", pfc::string8() << count - progress_index - min(threadcount, count - progress_index)));
				p_status.update_text_and_details(pfc::string8() << "Copying " << text_count << " file" << (text_count.is_plural() ? "s" : "") << " - encoding", progress_details);
				p_status.update_progress_subpart_helper(progress_start + index, progress_range);
			}

			if (index >= count) break;
		}
		for (thread_index = 0; thread_index<threadcount; thread_index++)
		{
			if (m_threads[thread_index].is_thread_open())
			{
				m_threads[thread_index].wait_for_and_release_thread();
				progress_filenames[thread_index].reset();
				{
					progress_index++;
					if (progress_index < count)
					{
						pfc::array_t<threaded_process_v2_t::detail_entry> progress_details;
						for (t_size pfindex = 0, pfcount = progress_filenames.get_size(); pfindex<pfcount; pfindex++)
						{
							if (!progress_filenames[pfindex].is_empty())
								progress_details.append_single(threaded_process_v2_t::detail_entry("Item:", progress_filenames[pfindex]));
						}
						progress_details.append_single(threaded_process_v2_t::detail_entry("Remaining:", pfc::string8() << count - progress_index - min(threadcount, count - progress_index)));

						mmh::UIntegerNaturalFormatter text_remaining(count - progress_index), text_count(progress_range / 3);
						p_status.update_text_and_details(pfc::string8() << "Copying " << text_count << " file" << (text_count.is_plural() ? "s" : "") << " - encoding", progress_details);
					}
				}
				m_entries[m_threads[thread_index].m_index].m_succeeded_to_temp_file = m_threads[thread_index].m_succeeded;
				m_entries[m_threads[thread_index].m_index].m_error = m_threads[thread_index].m_error;
				m_entries[m_threads[thread_index].m_index].m_replaygain_result = m_threads[thread_index].m_replaygain_result;
				mask_processed.set(m_threads[thread_index].m_index, true);
				//console::formatter() << "flushing: " << m_threads[thread_index].m_index;
				//if (m_threads[thread_index].m_succeeded)
				//p_mover.request(m_threads[thread_index].m_index);
				flush_filemoves(p_mover, m_mask_flush_items, mask_processed);
			}
		}
	}
	p_status.update_progress_subpart_helper(progress_start + count, progress_range);

	p_mover.exit();
	p_mover.wait_for_and_release_thread();

	for (i = 0; i<count; i++)
	{
		if (m_entries[i].m_succeeded_to_temp_file)
		{
			m_entries[i].m_succeeded = p_mover.get_index_succeeded(i);
			m_entries[i].m_error = p_mover.get_index_error(i);
		}
		//console::formatter() << i << " " << m_entries[i].m_succeeded_to_temp_file << " " << m_entries[i].m_succeeded;
	}
	/*for (i=0; i<count; i++)
	{
	try
	{
	if (m_entries[i].m_succeeded_to_temp_file)
	{
	filesystem::g_remove(pfc::string8() << "file://" << m_entries[i].m_temporary_destination, abort_callback_dummy());
	}
	}
	catch (pfc::exception & ex)
	{
	}
	}*/
}
