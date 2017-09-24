#include "main.h"

#include "ipod_manager.h"
#include "writer_sort_helpers.h"

bool is_blank(const char * str)
{
	if (str == NULL || *str == NULL) return true;
	while (*str == ' ') str++;
	if (*str == NULL) return true;
	return false;
}

bool g_print_meta(const file_info & info, const char * field, pfc::string_base & p_out)
{
	p_out.reset();
	if (info.meta_exists(field))
	{
		t_size i, count = info.meta_get_count_by_name(field);
		for (i = 0; i<count; i++)
		{
			p_out << info.meta_get(field, i);
			if (i + 1<count) p_out << ", ";
		}
		return true;
	}
	return false;
}

bool g_print_meta_single(const file_info & info, const char * field, pfc::string_base & p_out)
{
	p_out.reset();
	if (info.meta_exists(field))
	{
		p_out << info.meta_get(field, 0);
		return true;
	}
	return false;
}

bool g_print_meta_noblanks(const file_info & info, const char * field, pfc::string_base & p_out)
{
	p_out.reset();
	if (info.meta_exists(field))
	{
		t_size i, count = info.meta_get_count_by_name(field);
		for (i = 0; i<count; i++)
		{
			const char * value = info.meta_get(field, i);
			if (!is_blank(value))
			{
				p_out << value;
				if (i + 1<count) p_out << ", ";
			}
		}
		return p_out.length() > 0;
	}
	return false;
}

t_uint32 round_float(double f)
{
	return t_uint32(f >= -0.5 ? f + 0.5 : f - 0.5f);
}

t_uint32 g_print_meta_int(const file_info & info, const char * field)
{
	t_uint32 ret = 0;
	if (info.meta_exists(field))
	{
		ret = strtoul(info.meta_get(field, 0), NULL, 10);
	}
	return ret;
}

t_uint32 g_print_meta_int_n(const file_info & info, const char * field, t_size n)
{
	t_uint32 ret = 0;
	if (info.meta_exists(field))
	{
		ret = mmh::strtoul_n(info.meta_get(field, 0), n, 10);
	}
	return ret;
}

BOOL
FileTimeToLocalFileTime2(
	__in  CONST FILETIME *lpFileTime,
	__out LPFILETIME lpLocalFileTime
	)
{
	SYSTEMTIME stUTC, stLocal;
	memset(&stUTC, 0, sizeof(stUTC));
	memset(&stLocal, 0, sizeof(stLocal));

	FileTimeToSystemTime(lpFileTime, &stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
	return SystemTimeToFileTime(&stLocal, lpLocalFileTime);
}

BOOL
LocalFileTimeToFileTime2(
	__in  CONST FILETIME *lpLocalFileTime,
	__out LPFILETIME lpFileTime
	)
{
	SYSTEMTIME stUTC, stLocal;
	memset(&stUTC, 0, sizeof(stUTC));
	memset(&stLocal, 0, sizeof(stLocal));

	FileTimeToSystemTime(lpLocalFileTime, &stLocal);
	TzSpecificLocalTimeToSystemTime(NULL, &stLocal, &stUTC);
	return SystemTimeToFileTime(&stUTC, lpFileTime);
}

t_uint32 apple_time_from_filetime(t_filetimestamp filetime_src, bool b_local)
{
	if (filetime_src == filetimestamp_invalid) return 0;
	t_filetimestamp filetime = filetime_src;
	if (b_local)
		FileTimeToLocalFileTime2((LPFILETIME)&filetime_src, (LPFILETIME)&filetime);
	t_uint64 diff = 303;
	diff *= 365 * 24 * 60 * 60;
	diff += 24 * 3 * 24 * 60 * 60;
	diff *= 10000000;

	t_uint32 ret;
	ret = t_uint32((filetime - diff) / 10000000);
	return ret;
}

t_filetimestamp filetime_time_from_appletime(t_uint32 appletime, bool b_convert_to_utc)
{
	if (appletime == 0) return 0;
	t_uint64 diff = 303;
	diff *= 365 * 24 * 60 * 60;
	diff += 24 * 3 * 24 * 60 * 60;

	t_filetimestamp ret = appletime, rets;
	ret += diff;
	ret *= 10000000;

	rets = ret;
	if (b_convert_to_utc)
	{
		LocalFileTimeToFileTime2((LPFILETIME)&ret, (LPFILETIME)&rets);
	}

	return rets;
}

bool g_get_sort_string_for_ipod(const char * str, pfc::string8 & p_out, bool ipod_sorting)
{
	while (*str == ' ') str++;
	const char * p_source = str;
	if (ipod_sorting && !stricmp_utf8_max(str, "The ", 4))
		str += 4;
	else if (ipod_sorting && !stricmp_utf8_max(str, "A ", 2))
		str += 2;
	p_out = str;
	//if (str != p_source)
	//	(p_out << ", ").add_string(p_source, str-p_source);

	return str != p_source;
}

void g_get_sort_string_for_sorting(const char * str, pfc::string_simple_t<WCHAR> & p_out, bool ipod_sorting)
{
	pfc::string_simple_t<WCHAR> temp = pfc::stringcvt::string_wide_from_utf8(str);

	const wchar_t * p_start = temp.get_ptr(), *ptr = p_start;

	ptr += ipod_sort_helpers_t<true>::g_get_first_character_index(temp.get_ptr());
	if (ipod_sorting && !wcsicmp_ex(ptr, 4, L"The ", 4))
		ptr += 4;
	else if (ipod_sorting && !wcsicmp_ex(ptr, 2, L"A ", 2))
		ptr += 2;

	pfc::array_t<wchar_t> temp1;

	temp1.append_fromptr(ptr, wcslen(ptr));

	if (ptr != p_start)
	{
		temp1.append_fromptr(L", ", 2);
		temp1.append_fromptr(p_start, ptr - p_start);
	}

	p_out.set_string(temp1.get_ptr(), temp1.get_size());
}

bool g_test_sort_string(const char * str)
{
	while (*str == ' ') str++;
	if (!stricmp_utf8_max(str, "The ", 4))
		return true;
	else if (!stricmp_utf8_max(str, "A ", 2))
		return true;
	return false;
}
