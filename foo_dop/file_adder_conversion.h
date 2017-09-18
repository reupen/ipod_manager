#pragma once

class t_main_thread_tagger : public main_thread_callback
{
public:
	t_main_thread_tagger(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,
		const pfc::list_base_const_t<const file_info *> & p_infos,
		HWND p_parent_window, t_uint32 flags);;
	void on_task_completion(unsigned taskid, unsigned p_code);

	virtual void callback_run()
	{
		static_api_ptr_t<metadb_io_v2> api;
		api->load_info_async(m_list, metadb_io::load_info_default, m_parent_window, m_flags, completion_notify_create(this, 0));
	}
	metadb_io_v2::t_update_info_state m_ret;
	win32_event m_signal;
	const pfc::list_base_const_t<metadb_handle_ptr> & m_list;
	pfc::list_t<file_info_impl> m_infos;
	pfc::ptr_list_t<const file_info> m_infos_ptrs;
	HWND m_parent_window;
	t_uint32 m_flags;
};

class t_main_thread_read_info_no_cache : public main_thread_callback
{
public:
	t_main_thread_read_info_no_cache(const pfc::list_base_const_t<metadb_handle_ptr> & p_list,
		HWND p_parent_window, t_uint32 flags)
		: m_list(p_list), m_parent_window(p_parent_window), m_flags(flags)
	{
		m_signal.create(true, false);
	};
	void on_task_completion(unsigned taskid, unsigned p_code);

	virtual void callback_run()
	{
		static_api_ptr_t<metadb_io_v2> api;
		api->load_info_async(m_list, metadb_io::load_info_force, m_parent_window, m_flags, completion_notify_create(this, 1));
	}
	metadb_io_v2::t_load_info_state m_ret;
	win32_event m_signal;
	const pfc::list_base_const_t<metadb_handle_ptr> & m_list;
	HWND m_parent_window;
	t_uint32 m_flags;
};

class t_main_thread_get_info : public main_thread_callback
{
public:
	t_main_thread_get_info(const metadb_handle_ptr & ptr)
		: m_ptr(ptr)
	{
		m_signal.create(true, false);
	};
	virtual void callback_run()
	{
		m_ret = m_ptr->get_info_async(m_info);
		m_signal.set_state(true); //this deletes us!
	}
	bool m_ret;
	file_info_impl m_info;
	win32_event m_signal;
	metadb_handle_ptr m_ptr;
};


struct t_riff_header
{
	t_uint32 id0;
	t_uint32 filesize;
	t_uint32 type0;
	t_uint32 id1;
	t_uint32 headersize;
	struct t_riff_wave
	{
		WORD        wFormatTag;         /* format type */
		WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
		DWORD       nSamplesPerSec;     /* sample rate */
		DWORD       nAvgBytesPerSec;    /* for buffer estimation */
		WORD        nBlockAlign;        /* block size of data */
		WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	} header;
	t_uint32 id2;
	t_uint32 datasize;
};

struct t_riff_header_wf
{
	t_uint32 id0;
	t_uint32 filesize;
	t_uint32 type0;
	t_uint32 id1;
	t_uint32 headersize;
	pcmwaveformat_tag wft;
	t_uint32 id2;
	t_uint32 datasize;
};

struct t_riff_header_wfextensible
{
	t_uint32 id0;
	t_uint32 filesize;
	t_uint32 type0;
	t_uint32 id1;
	t_uint32 headersize;
	WAVEFORMATEXTENSIBLE wfext;
	t_uint32 id2;
	t_uint32 datasize;
};

class riff_header_writer_t
{
public:
	riff_header_writer_t(t_uint32 samplerate, t_uint32 bps, t_uint32 channel_count, t_int64 sample_count = -1);
	void * get_data_ptr() { return &m_riff_wf; }
	t_size get_data_size() { return m_riff_wf.wft.wf.wFormatTag == WAVE_FORMAT_EXTENSIBLE ? sizeof(m_riff_wfex) : sizeof(m_riff_wf); }
private:
	union
	{
		t_riff_header_wfextensible m_riff_wfex;
		t_riff_header_wf m_riff_wf;
	};
};



class conversion_thread_t : public mmh::Thread
{
public:
	conversion_thread_t() : m_replaygain_processing_mode(0), m_replaygain_gain_mode(0), m_checkpoint(NULL) {};

	void set_command(const settings::conversion_preset_t & str)
	{
		m_command = str;
	}
	void set_replaygain_data(t_uint8 mode, t_uint8 gain_mode, const replaygain_scanner_entry::ptr & api)
	{
		m_replaygain_processing_mode = mode;
		m_replaygain_gain_mode = gain_mode;
		m_replaygain_api = api;
	}
	void initialise(t_size index, const metadb_handle_ptr & src, const char * dst, checkpoint_base * p_checkpoint)
	{
		m_source = src;
		m_temporary_destination = dst;
		m_index = index;
		m_succeeded = false;
		m_checkpoint = p_checkpoint;
	}
	DWORD on_thread();
	t_size m_index;
	pfc::string8 m_error;
	bool m_succeeded;
	replaygain_result::ptr m_replaygain_result;
private:
	metadb_handle_ptr m_source;
	pfc::string8 m_temporary_destination;
	settings::conversion_preset_t m_command;
	t_uint8 m_replaygain_processing_mode, m_replaygain_gain_mode;
	service_ptr_t<replaygain_scanner_entry> m_replaygain_api;
	checkpoint_base * m_checkpoint;
};
class conversion_filemover_entry_t
{
public:
	pfc::string8 m_source;
	pfc::string8 m_destination;
	metadb_handle_ptr m_destination_handle;
	file_info_impl m_info;
};
class completion_notify_event : public completion_notify, public win32_event
{
public:
	completion_notify_event() : m_code(NULL)
	{
		create(false, false);
	}
	virtual void on_completion(unsigned p_code)
	{
		m_code = p_code;
		set_state(true);
	}
	t_size m_code;
};

class conversion_filemover_thread_t : public mmh::Thread
{
public:
	conversion_filemover_thread_t() : m_checkpoint(NULL) {};

	bool get_index_succeeded(t_size index) { return m_entries[index].m_succeeded; }
	const char * get_index_error(t_size index) { return m_entries[index].m_error; }
	class entry_t : public conversion_filemover_entry_t
	{
	public:
		bool m_succeeded;
		pfc::string8 m_error;
		entry_t() : m_succeeded(false) {};
	};
	void initialise(ipod_device_ptr_cref_t p_ipod, t_size reserved_diskspace, pfc::array_t<conversion_filemover_entry_t> & entries, checkpoint_base * p_checkpoint);
	void exit()
	{
		m_exit.set_state(true);
	}
	class waiting_entry_t
	{
	public:
		t_size index;
		replaygain_result::ptr trackgain;
		replaygain_result::ptr albumgain;
	};
	void request(t_size index, replaygain_result::ptr trackgain, replaygain_result::ptr albumgain)
	{
		{
			insync(m_sync);
			waiting_entry_t temp;
			temp.index = index;
			temp.trackgain = trackgain;
			temp.albumgain = albumgain;
			m_waiting_entries.add_item(temp);
		}
		m_pending_request.set_state(true);
	}
	DWORD on_thread();
private:
	win32_event m_pending_request;
	win32_event m_exit;
	pfc::list_t<waiting_entry_t> m_waiting_entries;
	critical_section m_sync;
	pfc::array_t<entry_t> m_entries;
	ipod_device_ptr_t m_ipod;
	t_size m_reserved_diskspace;
	checkpoint_base * m_checkpoint;
};
class conversion_entry_t
{
public:
	metadb_handle_ptr m_source;
	pfc::string8 m_destination;
	metadb_handle_ptr m_destination_handle;
	file_info_impl m_info;
};
class conversion_manager_t
{
	class entry_t
	{
	public:
		metadb_handle_ptr m_source;
		pfc::string8 m_temporary_destination;
		pfc::string8 m_destination;
		metadb_handle_ptr m_destination_handle;
		file_info_impl m_info;
		bool m_succeeded;
		bool m_early_fail;
		bool m_succeeded_to_temp_file;
		pfc::string8 m_error;
		replaygain_result::ptr m_replaygain_result;

		entry_t() : m_succeeded(false), m_early_fail(false), m_succeeded_to_temp_file(false) {};
	};
	bit_array_bittable m_mask_flush_items;//, m_mask_move_processed;
	mmh::Permutation m_permutation, m_inverse_permutation;
	t_size m_move_pointer;
public:
	conversion_manager_t() : m_move_pointer(0), m_mask_flush_items(0) {};

	bool  get_index_succeeded(t_size index) { return m_entries[index].m_succeeded; }
	const char *  get_index_error(t_size index) { return m_entries[index].m_error; }
	void initialise(const pfc::array_t<conversion_entry_t> & entries, const t_field_mappings & p_mappings, t_size thread_count);
	void flush_filemoves(conversion_filemover_thread_t & p_mover, const bit_array_var & mask_flush, const bit_array_var & mask_processed);
	void run(ipod_device_ptr_cref_t p_ipod, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, t_size progress_start, t_size progress_range, abort_callback & p_abort);
private:
	settings::conversion_preset_t m_command;
	pfc::array_t<entry_t> m_entries;
	pfc::array_t<conversion_thread_t> m_threads;
	service_ptr_t<replaygain_scanner_entry> m_replaygain_api;
};

void g_convert_file_v2(metadb_handle_ptr src, const char * dst_win32, const settings::conversion_preset_t & p_encoder_settings, t_uint8 replaygain_processing_mode, t_uint8 replaygain_gain_mode, abort_callback & p_abort);
