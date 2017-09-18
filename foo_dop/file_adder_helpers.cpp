#include "main.h"

bool g_is_ext_supported(const char * ext)
{
	return (!stricmp_utf8(ext, "WAV") || !stricmp_utf8(ext, "MP4") || !stricmp_utf8(ext, "M4A")
		|| !stricmp_utf8(ext, "MP3") || !stricmp_utf8(ext, "M4B") || !stricmp_utf8(ext, "M4P")
		|| !stricmp_utf8(ext, "AA") || !stricmp_utf8(ext, "M4V") || !stricmp_utf8(ext, "MOV")) || !stricmp_utf8(ext, "M4R");
}

bool g_is_file_supported(const char * path)
{
	pfc::string_extension ext(path);
	return g_is_ext_supported(ext);
}

bool g_is_file_supported(metadb_handle * ptr)
{
	return g_is_file_supported(ptr->get_path());
}

void g_set_filetimestamp(const char * path, t_filetimestamp & p_out)
{
	pfc::string8 f1u;
	filesystem::g_get_display_path(path, f1u);
	pfc::stringcvt::string_os_from_utf8 f1(f1u);
	SetLastError(0);
	HANDLE h1 = CreateFile(f1, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (!h1) throw exception_win32(GetLastError());

	//SetLastError(0);
	//if (!
	SetFileTime(h1, NULL, NULL, (LPFILETIME)&p_out);
	//	)
	//	throw exception_win32(GetLastError());
	CloseHandle(h1);
}

t_uint32 g_get_directory_child_count(const char * path, abort_callback & p_abort)
{
	class directory_callback_count : public directory_callback
	{
	public:
		bool on_entry(filesystem * p_owner, abort_callback & p_abort, const char * p_url, bool p_is_subdirectory, const t_filestats & p_stats)
		{
			p_abort.check();
			if (!p_is_subdirectory) m_count++;
			return true;
		}
		t_uint32 m_count;
		directory_callback_count() : m_count(0) {};
	};
	directory_callback_count callback;
	pfc::string8 can;
	filesystem::g_get_canonical_path(path, can);
	try {
		filesystem::g_list_directory(can, callback, p_abort);
	}
	catch (const exception_io & blah)
	{
		console::formatter() << "iPod manager: Error scanning directory \"" << path << "\" " << blah.what();
	};
	return callback.m_count;
}

void g_copy_file(const char * src, const char * dst, checkpoint_base * p_checkpoint, abort_callback & p_abort)
{
	service_ptr_t<file> r_src, r_dst;
	t_filesize size;

	filesystem::g_open(r_src, src, filesystem::open_mode_read, p_abort);
	size = r_src->get_size_ex(p_abort);
	filesystem::g_open(r_dst, dst, filesystem::open_mode_write_new, p_abort);

	file_mobile_device::ptr r_dstm;
	if (p_checkpoint && r_dst->service_query_t(r_dstm))
		r_dstm->set_checkpoint(p_checkpoint);

	if (size > 0) {
		try {
			file::g_transfer_object(r_src, r_dst, size, p_abort);
		}
		catch (...) {
			r_dst.release();
			try { filesystem::g_remove(dst, abort_callback_impl()); }
			catch (...) {}
			throw;
		}
	}
}

void g_load_info(HWND wnd, const pfc::list_base_const_t<metadb_handle_ptr> & p_list, threaded_process_v2_t & p_status)
{
	//p_status.update_progress_subpart_helper(0,1);
	p_status.checkpoint();
	p_status.update_text("Waiting for file info read to complete");
	service_ptr_t<t_main_thread_scan_file_info> p_info_loader = new service_impl_t<t_main_thread_scan_file_info>
		(p_list, metadb_io::load_info_check_if_changed, wnd);
	static_api_ptr_t<main_thread_callback_manager> p_main_thread;
	p_main_thread->add_callback(p_info_loader);
	if (!p_info_loader->m_signal.wait_for(-1))
		throw pfc::exception("File info reading timeout!");
	if (p_info_loader->m_ret == metadb_io::load_info_aborted)
		throw pfc::exception("File info read was aborted");
	p_status.checkpoint();
}

bool is_latin_number(char c)
{
	return c >= '0' && c <= '9';
}

t_filetimestamp g_filetime_to_timestamp(const LPFILETIME ft)
{
	ULARGE_INTEGER large;
	large.HighPart = ft->dwHighDateTime;
	large.LowPart = ft->dwLowDateTime;

	return large.QuadPart;
}

t_filetimestamp g_string_to_timestamp(const char * str)
{
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));

	const char * ptr = str, *start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 4) ptr++;
	st.wYear = mmh::strtoul_n(start, ptr - start);
	while (*ptr && !is_latin_number(*ptr)) ptr++;
	start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 2) ptr++;
	st.wMonth = mmh::strtoul_n(start, ptr - start);
	while (*ptr && !is_latin_number(*ptr)) ptr++;
	start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 2) ptr++;
	st.wDay = mmh::strtoul_n(start, ptr - start);
	while (*ptr && !is_latin_number(*ptr)) ptr++;
	start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 2) ptr++;
	st.wHour = mmh::strtoul_n(start, ptr - start);
	while (*ptr && !is_latin_number(*ptr)) ptr++;
	start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 2) ptr++;
	st.wMinute = mmh::strtoul_n(start, ptr - start);
	while (*ptr && !is_latin_number(*ptr)) ptr++;
	start = ptr;

	while (*ptr && is_latin_number(*ptr) && (ptr - start) < 2) ptr++;
	st.wSecond = mmh::strtoul_n(start, ptr - start);

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	return g_filetime_to_timestamp(&ft);
}

t_filetimestamp g_iso_timestamp_string_to_filetimestamp(const char * p_str, bool b_read_time)
{
	//2010-06-08T12:00:00Z
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	const char * ptr = p_str;
	t_size len = strlen(ptr);

	if (ptr + 4 - p_str <= len)
		st.wYear = mmh::strtoul_n(ptr, 4);
	ptr += 4;

	if (ptr + 1 - p_str <= len && !is_latin_number(*ptr))
		ptr++;

	if (ptr + 2 - p_str <= len)
		st.wMonth = mmh::strtoul_n(ptr, 2);
	ptr += 2;

	if (ptr + 1 - p_str <= len && !is_latin_number(*ptr))
		ptr++;

	if (ptr + 2 - p_str <= len)
		st.wDay = mmh::strtoul_n(ptr, 2);
	ptr += 2;

	bool b_time = false;

	if (ptr + 1 - p_str <= len && *ptr == 'T')
	{
		b_time = true;
		ptr++;
	}

	if (b_time && b_read_time)
	{

		if (ptr + 2 - p_str <= len)
			st.wHour = mmh::strtoul_n(ptr, 2);
		ptr += 2;

		if (ptr + 1 - p_str <= len && !is_latin_number(*ptr))
			ptr++;

		if (ptr + 2 - p_str <= len)
			st.wMinute = mmh::strtoul_n(ptr, 2);
		ptr += 2;

		if (ptr + 1 - p_str <= len && !is_latin_number(*ptr))
			ptr++;

		if (ptr + 2 - p_str <= len)
			st.wSecond = mmh::strtoul_n(ptr, 2);
		ptr += 2;

		bool b_utc = (ptr + 1 - p_str <= len && *ptr == 'Z');

		if (!b_utc)
		{
			SYSTEMTIME stLocal = st;
			TzSpecificLocalTimeToSystemTime(NULL, &stLocal, &st);
		}
	}
	FILETIME ft = { 0 };
	SystemTimeToFileTime(&st, &ft);
	return g_filetime_to_timestamp(&ft);
}

