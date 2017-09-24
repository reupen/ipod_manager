#include "stdafx.h"

#include "file_adder.h"
#include "file_adder_conversion.h"

#define SQRTHALF          0.70710678118654752440084436210485

void g_downmix_51ch_to_stereo(audio_chunk * chunk)
{
	if (chunk->get_channels() == 6)
	{
		UINT n, samples = chunk->get_sample_count();
		pfc::array_staticsize_t<audio_sample> temp(samples * 2);
		audio_sample * p_temp = temp.get_ptr(), *data = chunk->get_data();
		for (n = 0; n<samples; n++)
		{
			p_temp[n << 1] = data[0] + data[2] * SQRTHALF + data[4] * SQRTHALF + data[3];
			p_temp[(n << 1) + 1] = data[1] + data[2] * SQRTHALF + data[5] * SQRTHALF + data[3];
			data += 6;
		}
		chunk->set_data(p_temp, samples, 2, chunk->get_srate());
	}
}

/*
void g_downmix_4ch_to_stereo(audio_chunk * chunk)
{
if (chunk->get_channels() == 4)
{
audio_sample div = 1.0 / 2.0;
UINT n,samples = chunk->get_sample_count();
audio_sample * p_temp = temp.check_size(samples*2), * data = chunk->get_data();
for(n=0;n<samples;n++)
{
p_temp[n<<1]	 = data[0] + data[2] * SQRTHALF + data[4] * SQRTHALF + data[3];
p_temp[(n<<1)+1] = data[1] + data[2] * SQRTHALF + data[5] * SQRTHALF + data[3];
data += 6;
}
chunk->set_data(p_temp,samples,2,chunk->get_srate());
}
}*/


bool g_replaygain_scan_file(const char * path, const replaygain_scanner::ptr & api, replaygain_result::ptr & p_out, abort_callback & p_abort)
{
	try
	{
		service_ptr_t<input_decoder> decoder;
		file::ptr p_file, p_fileCached;
		filesystem::g_open_read(p_file, path, p_abort);
		file_cached::g_create(p_fileCached, p_file, p_abort, 512 * 1024);

		input_entry::g_open_for_decoding(decoder, p_fileCached, path, p_abort);
		decoder->initialize(0, input_flag_no_seeking | input_flag_no_looping, p_abort);
		audio_chunk_impl chunk;
		while (decoder->run(chunk, p_abort))
		{
			api->process_chunk(chunk);
		}
		p_out = api->finalize();
		return true;
	}
	catch (const pfc::exception &)
	{
		return false;
	}
}

void g_convert_file_v2(metadb_handle_ptr src, const char * dst_win32, const settings::conversion_preset_t & p_encoder_settings, t_uint8 replaygain_processing_mode, t_uint8 replaygain_gain_mode, abort_callback & p_abort)
{
	//const GUID guid_dsp_downmix_51_to_stereo = {0x866233BB, 0xC466, 0x43E6, {0xAF, 0xC4, 0x8B, 0x7B, 0xE5, 0x63, 0xD9, 0x12}};
	//abort_callback_impl p_abort;
	service_ptr_t<input_decoder> decoder;
	file::ptr p_file, p_fileCached;
	try {
		filesystem::g_open_read(p_file, src->get_path(), p_abort);
		file_cached::g_create(p_fileCached, p_file, p_abort, 512 * 1024);
	}
	catch (exception_io_no_handler_for_path &) {};

	bool b_apply_gain = replaygain_processing_mode == settings::replaygain_processing_apply_before_encoding;
	audio_sample scale = 1.0;
	if (b_apply_gain)
	{
		metadb_info_container::ptr p_info;
		if (src->get_info_ref(p_info))
		{
			replaygain_info rginfo = p_info->info().get_replaygain();
			float gain = 0.0;
			if (replaygain_gain_mode == 0 && rginfo.is_track_gain_present())
				gain = rginfo.m_track_gain;
			else if (rginfo.is_album_gain_present())
				gain = rginfo.m_album_gain;
			else if (replaygain_gain_mode == 1 && rginfo.is_track_gain_present())
				gain = rginfo.m_track_gain;
			if (gain != 0.0)
				scale = pow(10.0, gain / 20.0);
		}
	}

	input_entry::g_open_for_decoding(decoder, p_fileCached, src->get_path(), p_abort);
	file_info_impl info;
	decoder->get_info(src->get_subsong_index(), info, p_abort);
	t_int64 samples = info.info_get_length_samples();
	t_int64 channels = info.info_get_int("channels");
	if (channels == 0) channels = 1;
	t_uint32 bps = (t_uint32)info.info_get_int("bitspersample");
	if (bps == 0) bps = 32;
	t_uint32 encoder_max_bpx = p_encoder_settings.get_max_bps();
	t_uint32 target_bps = min(bps, encoder_max_bpx);
	if (channels != 1 && channels != 2 && channels != 6)
		throw pfc::exception(pfc::string8() << channels << " channel files are not supported");

	HANDLE encinrd = INVALID_HANDLE_VALUE, encinwr = INVALID_HANDLE_VALUE;

	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		if (!CreatePipe(&encinrd, &encinwr, &saAttr, 0))
			throw pfc::exception("CreatePipe failed");
	}
	SetHandleInformation(encinwr, HANDLE_FLAG_INHERIT, 0);

	pfc::string8 cmd, cmd2;
	p_encoder_settings.get_command(cmd);
	t_size dpos = pfc::string_find_first(cmd, "%d");

	cmd2.add_string(cmd, dpos);
	if (dpos != pfc_infinite)
	{
		cmd2 << "\"" << dst_win32 << "\"";
		cmd2.add_string(cmd + dpos + 2, pfc_infinite);
	}

	pfc::stringcvt::string_os_from_utf8 oscmd(cmd2);

	pfc::array_t<TCHAR> cmdline; //buffer must be read-write!
	cmdline.append_fromptr(oscmd.get_ptr(), oscmd.length());
	cmdline.append_single(0);

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	static_api_ptr_t<audio_postprocessor> processor;
	memset(&piProcInfo, 0, sizeof(PROCESS_INFORMATION));

	memset(&siStartInfo, 0, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = INVALID_HANDLE_VALUE;
	siStartInfo.hStdOutput = INVALID_HANDLE_VALUE;
	siStartInfo.hStdInput = encinrd;
	siStartInfo.wShowWindow = SW_HIDE;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	BOOL b_createprocess_ret = FALSE;

	try {
		if (b_createprocess_ret = CreateProcess(NULL,
			cmdline.get_ptr(),     // command line
			NULL,          // process security attributes 
			NULL,          // primary thread security attributes 
			TRUE,          // handles are inherited 
			BELOW_NORMAL_PRIORITY_CLASS,             // creation flags 
			NULL,          // use parent's environment 
			NULL,          // use parent's current directory 
			&siStartInfo,  // STARTUPINFO pointer 
			&piProcInfo))  // receives PROCESS_INFORMATION 
		{
			CloseHandle(encinrd);
			encinrd = INVALID_HANDLE_VALUE;
			CloseHandle(piProcInfo.hThread);

			DWORD p_written;

			t_int64 sample_count = 0;
			if (p_encoder_settings.m_encoder_requires_accurate_length)
			{
				decoder->initialize(src->get_subsong_index(), input_flag_no_seeking | input_flag_no_looping, p_abort);
				audio_chunk_impl chunk;
				while (decoder->run(chunk, p_abort))
					sample_count += chunk.get_data_size();
			}
			else sample_count = -1;

			decoder->initialize(src->get_subsong_index(), input_flag_no_seeking | input_flag_no_looping, p_abort);

			bool b_header_written = false;

			dsp::ptr resampler;
			dsp_chunk_list_impl resampler_chunks;

			audio_chunk_impl chunk;
			mem_block_container_impl_t<pfc::alloc_fast_aggressive> chunk2;
			while (true)
			{
				bool b_decoded = false;
				if (!(b_decoded = decoder->run(chunk, p_abort)))
				{
					if (resampler.is_valid())
					{
						resampler->run(&resampler_chunks, metadb_handle_ptr(), dsp::FLUSH);
						if (!resampler_chunks.get_count())
							break;
					}
					else break;
				}
				t_uint32 orig_channels = chunk.get_channels();
				if (orig_channels == 6)
					g_downmix_51ch_to_stereo(&chunk);
				if (!b_header_written)
				{
					t_uint32 samplerate = chunk.get_sample_rate();
					if (samplerate > 48000)
					{
						if (!resampler_entry::g_create(resampler, chunk.get_sample_rate(), 48000, 1.0))
							throw pfc::exception(pfc::string8() << "Could not create resampler (" << chunk.get_sample_rate() << " Hz -> 48000 Hz)");
						samplerate = 48000;
					}

					riff_header_writer_t riff_header(samplerate, target_bps, chunk.get_channels(), sample_count / orig_channels);
					WriteFile(encinwr, riff_header.get_data_ptr(), riff_header.get_data_size(), &p_written, NULL);
					b_header_written = true;
				}
				if (resampler.is_valid())
				{
					//resampler_chunks.remove_all();
					if (b_decoded)
					{
						resampler_chunks.add_chunk(&chunk);
						resampler->run(&resampler_chunks, metadb_handle_ptr(), NULL);
					}
					while (resampler_chunks.get_count())
					{
						audio_chunk * pChunk = resampler_chunks.get_item(0);
						if (pChunk)
						{
							if (target_bps == 32 && !b_apply_gain)
								WriteFile(encinwr, pChunk->get_data(), pChunk->get_data_size()*sizeof(audio_sample), &p_written, NULL);
							else
							{
								processor->run(*pChunk, chunk2, target_bps, target_bps, false, scale);
								WriteFile(encinwr, chunk2.get_ptr(), chunk2.get_size(), &p_written, NULL);
							}
						}
						resampler_chunks.remove_by_idx(0);
					}
				}
				else
				{
					if (target_bps == 32 && !b_apply_gain)
						WriteFile(encinwr, chunk.get_data(), chunk.get_data_size()*sizeof(audio_sample), &p_written, NULL);
					else
					{
						processor->run(chunk, chunk2, target_bps, target_bps, false, scale);
						WriteFile(encinwr, chunk2.get_ptr(), chunk2.get_size(), &p_written, NULL);
					}
				}
			};

			CloseHandle(encinwr);
			encinwr = INVALID_HANDLE_VALUE;
			WaitForSingleObjectEx(piProcInfo.hProcess, pfc_infinite, FALSE);
			DWORD ExitCode = NULL;
			BOOL bGetExitRet = GetExitCodeProcess(piProcInfo.hProcess, &ExitCode);
			CloseHandle(piProcInfo.hProcess);
			piProcInfo.hProcess = INVALID_HANDLE_VALUE;

			if (!bGetExitRet)
				throw pfc::exception("GetExitCodeProcess failed");
			if (ExitCode)
				throw pfc::exception(pfc::string8() << "Unexpected process exit code " << pfc::format_hex(ExitCode, 8) << "h");
		}
		else
		{
			DWORD err = GetLastError();
			throw pfc::exception(pfc::string_formatter() << "Failed to start encoder process - " << format_win32_error(err) << " (" << pfc::format_hex(err) << "h)");
		}
	}
	catch (const pfc::exception &)
	{
		if (encinwr != INVALID_HANDLE_VALUE)
		{
			CloseHandle(encinwr);
			encinwr = INVALID_HANDLE_VALUE;
		}
		if (encinrd != INVALID_HANDLE_VALUE)
		{
			CloseHandle(encinrd);
			encinrd = INVALID_HANDLE_VALUE;
		}
		abort_callback_dummy dummy;
		try {
			if (b_createprocess_ret)
			{
				if (piProcInfo.hProcess != INVALID_HANDLE_VALUE)
				{
					WaitForSingleObjectEx(piProcInfo.hProcess, 10 * 1000, FALSE);
					CloseHandle(piProcInfo.hProcess);
					piProcInfo.hProcess = INVALID_HANDLE_VALUE;
				}
				try {
					filesystem::g_remove(pfc::string8() << "file://" << dst_win32, dummy);
				}
				catch (const pfc::exception &)
				{
				};
			}
		}
		catch (const pfc::exception &) {};
		throw;
	}
	if (!filesystem::g_exists(dst_win32, p_abort))
		throw pfc::exception("Encoder failure (target file was not created)");

}

DWORD conversion_thread_t::on_thread()
{
	try
	{
		if (m_checkpoint) m_checkpoint->checkpoint();
		g_convert_file_v2(m_source, m_temporary_destination, m_command, m_replaygain_processing_mode, m_replaygain_gain_mode, abort_callback_dummy());
		m_succeeded = true;
		if (m_replaygain_api.is_valid())
			g_replaygain_scan_file(m_temporary_destination, m_replaygain_api->instantiate(), m_replaygain_result, abort_callback_dummy());
	}
	catch (const pfc::exception & ex)
	{
		m_error = ex.what();
	}
	return 0;
}

t_main_thread_tagger::t_main_thread_tagger(const pfc::list_base_const_t<metadb_handle_ptr>& p_list, const pfc::list_base_const_t<const file_info*>& p_infos, HWND p_parent_window, t_uint32 flags)
	: m_list(p_list), m_parent_window(p_parent_window), m_flags(flags)
{
	m_signal.create(true, false);
	t_size i, count = p_infos.get_count();
	m_infos.set_count(count);
	for (i = 0; i<count; i++)
	{
		m_infos[i] = *p_infos[i];
		m_infos_ptrs.add_item(&m_infos[i]);
	}
}

void t_main_thread_tagger::on_task_completion(unsigned taskid, unsigned p_code)
{
	if (taskid == 0)
	{
		static_api_ptr_t<metadb_io_v2> api;
		api->update_info_async(m_list, new service_impl_t<file_info_filter_impl>(m_list, m_infos_ptrs), m_parent_window, m_flags, completion_notify_create(this, 1));
	}
	else if (taskid == 1)
	{
		m_ret = metadb_io_v2::t_update_info_state(p_code);
		m_signal.set_state(true); //this deletes us!
	}
}

void t_main_thread_read_info_no_cache::on_task_completion(unsigned taskid, unsigned p_code)
{
	if (taskid == 1)
	{
		m_ret = metadb_io_v2::t_load_info_state(p_code);
		m_signal.set_state(true); //this deletes us!
	}
}

riff_header_writer_t::riff_header_writer_t(t_uint32 samplerate, t_uint32 bps, t_uint32 channel_count, t_int64 sample_count)
{
	if (channel_count > 2)
	{
		throw pfc::exception_not_implemented();
#if 0
		m_riff_wfex.id0 = 'RIFF';
		m_riff_wfex.id1 = 'fmt ';
		m_riff_wfex.type0 = 'WAVE';
		m_riff_wfex.id2 = 'data';
		m_riff_wfex.headersize = sizeof(m_riff_wf_ex.wfext);
		m_riff_wfex.filesize = -1;
		m_riff_wfex.datasize = -1;
		m_riff_wfex.wfext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		m_riff_wfex.wfext.Format.cbSize = 22;
		m_riff_wfex.wfext.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
#endif
	}
	else
	{
		m_riff_wf.id0 = 'FFIR';
		m_riff_wf.id1 = ' tmf';
		m_riff_wf.type0 = 'EVAW';
		m_riff_wf.id2 = 'atad';
		m_riff_wf.headersize = sizeof(m_riff_wf.wft);
		if (sample_count < 0 || (sample_count * channel_count * (bps / 8)) >(0xffffffff - sizeof(m_riff_wf)))
		{
			m_riff_wf.filesize = -1;
			m_riff_wf.datasize = -1;
		}
		else
		{
			m_riff_wf.datasize = (t_uint32)sample_count * channel_count * (bps / 8);
			m_riff_wf.filesize = m_riff_wf.datasize + sizeof(m_riff_wf);
		}
		m_riff_wf.wft.wf.nSamplesPerSec = samplerate;
		m_riff_wf.wft.wf.nChannels = channel_count;
		m_riff_wf.wft.wBitsPerSample = bps;
		m_riff_wf.wft.wf.nBlockAlign = (m_riff_wf.wft.wf.nChannels*m_riff_wf.wft.wBitsPerSample) / 8;
		m_riff_wf.wft.wf.nAvgBytesPerSec = (m_riff_wf.wft.wf.nBlockAlign*m_riff_wf.wft.wf.nSamplesPerSec);
		m_riff_wf.wft.wf.wFormatTag = (bps > 24 ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM);
	}
}

void conversion_filemover_thread_t::initialise(ipod_device_ptr_cref_t p_ipod, t_size reserved_diskspace, pfc::array_t<conversion_filemover_entry_t>& entries, checkpoint_base * p_checkpoint)
{
	m_checkpoint = p_checkpoint;
	m_ipod = p_ipod;
	m_reserved_diskspace = reserved_diskspace;
	t_size i, count = entries.get_count();
	m_entries.set_count(count);
	for (i = 0; i<count; i++)
	{
		m_entries[i].m_source = entries[i].m_source;
		m_entries[i].m_destination = entries[i].m_destination;
		m_entries[i].m_destination_handle = entries[i].m_destination_handle;
		m_entries[i].m_info = entries[i].m_info;
	}
	m_pending_request.create(false, false);
	m_exit.create(false, false);
}

DWORD conversion_filemover_thread_t::on_thread()
{
	HANDLE events[2] = { m_pending_request.get(), m_exit.get() };
	while (true)
	{
		DWORD ret = WaitForMultipleObjectsEx(tabsize(events), events, FALSE, pfc_infinite, FALSE);
		if (ret == WAIT_OBJECT_0 + 1)
			return 0;
		else if (ret == WAIT_OBJECT_0)
		{
			bool b_got_entry = false;

			do
			{
				b_got_entry = false;
				waiting_entry_t entry;
				{
					insync(m_sync);
					t_size count = m_waiting_entries.get_count();
					if (count)
					{
						entry = m_waiting_entries[count - 1];
						m_waiting_entries.remove_by_idx(count - 1);
						b_got_entry = true;
					}
				}
				if (b_got_entry)
				{
					try
					{
						if (m_checkpoint) m_checkpoint->checkpoint();
						static_api_ptr_t<main_thread_callback_manager> p_main_thread;
						pfc::string8 newTemp = m_entries[entry.index].m_source;
						//console::formatter() << "moving " << newTemp << " to " << m_entries[entry.index].m_destination;
						//newTemp << ".dop." << pfc::string_extension(m_entries[index].m_destination);
						{
							static_api_ptr_t<metadb> metadb_api;
							static_api_ptr_t<metadb_io_v3> metadbio_api;

							//filesystem::g_move(m_entries[index].m_source, newTemp, abort_callback_impl());

							if (entry.trackgain.is_valid())
							{
								m_entries[entry.index].m_info.info_set_replaygain_track_gain(entry.trackgain->get_gain());
								m_entries[entry.index].m_info.info_set_replaygain_track_peak(entry.trackgain->get_peak());
							}

							if (entry.albumgain.is_valid())
							{
								m_entries[entry.index].m_info.info_set_replaygain_album_gain(entry.albumgain->get_gain());
								m_entries[entry.index].m_info.info_set_replaygain_album_peak(entry.albumgain->get_peak());
							}

							{
								//fixup lyrics
								pfc::string_extension ext(newTemp);
								if (!stricmp_utf8(ext, "mp3"))
								{
									if (m_entries[entry.index].m_info.meta_exists("LYRICS")
										&& !m_entries[entry.index].m_info.meta_exists("UNSYNCED LYRICS"))
									{
										m_entries[entry.index].m_info.meta_set("UNSYNCED LYRICS", m_entries[entry.index].m_info.meta_get("LYRICS", 0));
										m_entries[entry.index].m_info.meta_remove_field("LYRICS");
									}
								}
								else if (!stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a"))
								{
									if (m_entries[entry.index].m_info.meta_exists("UNSYNCED LYRICS")
										&& !m_entries[entry.index].m_info.meta_exists("LYRICS"))
									{
										m_entries[entry.index].m_info.meta_set("LYRICS", m_entries[entry.index].m_info.meta_get("UNSYNCED LYRICS", 0));
										m_entries[entry.index].m_info.meta_remove_field("UNSYNCED LYRICS");
									}
								}

							}

							metadb_handle_ptr handle;
							metadb_api->handle_create(handle, make_playable_location(newTemp, 0));
							metadb_handle_list handles;
							pfc::ptr_list_t<const file_info> infos;
							handles.add_item(handle);
							infos.add_item(&m_entries[entry.index].m_info);

							service_ptr_t<t_main_thread_tagger> p_tagger = new service_impl_t<t_main_thread_tagger>
								(handles, infos, core_api::get_main_window(), metadb_io_v2::op_flag_delay_ui | metadb_io_v2::op_flag_no_errors | metadb_io_v2::op_flag_background);
							p_main_thread->add_callback(p_tagger);
							p_tagger->m_signal.wait_for(-1);


						}
						drive_space_info_t spaceinfo;
						m_ipod->get_capacity_information(spaceinfo);
						t_filestats stats; bool blah;
						filesystem::g_get_stats(newTemp, stats, blah, abort_callback_dummy());

						if ((t_sfilesize)spaceinfo.m_freespace - (t_sfilesize)stats.m_size <= ((t_sfilesize)m_reserved_diskspace * (t_sfilesize)spaceinfo.m_capacity) / 1000)
							throw pfc::exception(pfc::string8() << "Reserved disk space limit exceeded (" << "Capacity: " << spaceinfo.m_capacity << "; Free: " << spaceinfo.m_freespace << "; File To Copy: " << stats.m_size << "; Reserved 0.1%s: " << m_reserved_diskspace << ")");

						g_copy_file(newTemp, m_entries[entry.index].m_destination, m_checkpoint, abort_callback_dummy());

						try
						{
							filesystem::g_remove(newTemp, abort_callback_dummy());
						}
						catch (pfc::exception &)
						{
						}
						if (1)
						{
							metadb_handle_list handles;
							handles.add_item(m_entries[entry.index].m_destination_handle);
							service_ptr_t<t_main_thread_read_info_no_cache> p_reader = new service_impl_t<t_main_thread_read_info_no_cache>
								(handles, core_api::get_main_window(), metadb_io_v2::op_flag_delay_ui | metadb_io_v2::op_flag_no_errors | metadb_io_v2::op_flag_background);
							p_main_thread->add_callback(p_reader);
							p_reader->m_signal.wait_for(-1);
						}
						m_entries[entry.index].m_succeeded = true;
					}
					catch (pfc::exception & ex)
					{
						try
						{
							filesystem::g_remove(m_entries[entry.index].m_source, abort_callback_dummy());
						}
						catch (pfc::exception &)
						{
						}
						m_entries[entry.index].m_succeeded = false;
						m_entries[entry.index].m_error = ex.what();
					}
				}
			} while (b_got_entry);
		}
		else return ret;
	}
	return 0;
}

