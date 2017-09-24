#pragma once

struct speech_map
{
	const char * from, * to;
	speech_map() : from(NULL), to(NULL) {};
	speech_map(const char * p_from, const char * p_to) : from(p_from), to(p_to) {};
	static int g_compare (const speech_map & p1, const speech_map & p2)
	{
		return stricmp_utf8(p1.from, p2.from);
	}
	static int g_compare_by_value (const speech_map & p1, const char * p2)
	{
		return stricmp_utf8(p1.from, p2);
	}
};
class speech_string_preprocessor
{
public:
	void run(const char * p_source, pfc::string8 & p_out)
	{
		const char * start = p_source, *ptr = start;

		while (*(start = ptr))
		{
			while (*ptr && !test_char(*ptr)) ptr++;
			if (ptr > start)
			{
				t_size index;
				if (m_maps.bsearch_t(speech_map::g_compare_by_value, pfc::string8(start, ptr-start), index))
					p_out.add_string(m_maps[index].to);
				else
					p_out.add_string(start, ptr-start);

			}
			start = ptr;
			while (*ptr && test_char(*ptr)) ptr++;
			if (ptr > start)
				p_out.add_string(start, ptr-start);
		}

	}
	static bool test_char(char c) {return c == ' ' || c == ',' || c == '(' || c == ')';}
	speech_string_preprocessor();
	pfc::list_t<speech_map> m_maps;
};
class sapi 
{
public:

	bool is_valid() const {return m_valid;}
	void run_mapped (const char * text, unsigned samplerate, const char * path);
	void run (const char * text, unsigned samplerate, const char * path);
	sapi() : m_valid(false)
	{
		HRESULT hr = m_SpVoice.instantiate(CLSID_SpVoice);
		if (SUCCEEDED(hr)) m_valid = true;
		else console::formatter() << "Failed to instantiate ISpVoice: " << format_win32_error(hr);
		//_check_hresult(hr);
	}
private:
	void _check_hresult (HRESULT hr) {if (FAILED(hr)) throw pfc::exception(pfc::string8() << "SAPI error: " << format_win32_error(hr));}
	mmh::ComPtr<ISpVoice> m_SpVoice;
	bool m_valid;
	speech_string_preprocessor m_preprocessor;
};
