#pragma once

#include "ipod_manager.h"

class string_descape_xml : public pfc::string8_fast_aggressive
{
public:
	string_descape_xml(const char* p_source, t_size len = pfc_infinite);
};

class string_escape_xml : public pfc::string8_fast_aggressive
{
public:
	string_escape_xml(const char* p_source, t_size len = pfc_infinite);
};

class XMLPlistParser
{
public:
	XMLPlistParser(const char * pptr) : ptr(pptr) {};
	XMLPlistParser() : ptr(NULL) {};
	void set_stream (const char * str) {ptr = str;}
	void run(ipod_info & p_out)
	{
		skip_header();
		skip_doctype();
		skip_plist();
		get_keys(p_out);
	}
	void run_artwork_v2(device_properties_t & p_out);
	void run_cfobject(cfobject::object_t::ptr_t & p_out)
	{
		skip_header();
		skip_doctype();
		skip_plist(false);
		get_cfobject(p_out);
	}
private:
	void skip_header()
	{
		if (*ptr == '<') ptr++;
		else throw exception_io_unsupported_format();
		if (*ptr == '?') ptr++;
		else throw exception_io_unsupported_format();
		while (*ptr && *ptr != '?') ptr++;
		if (*ptr == '?') ptr++;
		else throw exception_io_unsupported_format();
		if (*ptr == '>') ptr++;
		else throw exception_io_unsupported_format();
		skip_eol_and_whitespace();
	}
	void skip_doctype()
	{
		if (*ptr == '<') ptr++;
		else throw exception_io_unsupported_format();
		if (*ptr == '!') ptr++;
		else throw exception_io_unsupported_format();
		while (*ptr && *ptr != '>') ptr++;
		if (*ptr == '>') ptr++;
		else throw exception_io_unsupported_format();
		skip_eol_and_whitespace();
	}
	void skip_plist(bool b_skip_dict = true)
	{
		if (stricmp_utf8_max(ptr, "<plist", 6)) 
			throw exception_io_unsupported_format();
		ptr+=6;
		while (*ptr && *ptr != '>') ptr++;
		if (*ptr == '>') ptr++;
		else throw exception_io_unsupported_format();
		skip_eol_and_whitespace();
		if (b_skip_dict)
		{
			if (stricmp_utf8_max(ptr, "<dict>", 6))
				throw exception_io_unsupported_format();
			ptr+= 6;
			skip_eol_and_whitespace();
		}
	}
	void get_keys(ipod_info & p_out);
	class key_t
	{
	public:
		const char * m_key_name;
		t_size m_key_name_length;
		const char * m_value_type;
		t_size m_value_type_length;
		bool m_value_self_contained;
		const char * m_value;
		t_size m_value_length;

		key_t() : m_key_name(NULL), m_value_type(NULL), m_value(NULL), 
			m_key_name_length(0), m_value_type_length(0), m_value_length(0), m_value_self_contained(false) {};
	};
	static bool g_get_tag_v2(const char * & ptr, key_t & p_out);
	t_uint32 get_cftype(const key_t & p_key);
	void get_cfobject(cfobject::object_t::ptr_t & p_out)
	{
		key_t key_root;
		if (!g_get_tag_v2(ptr, key_root))
			return;

		get_cfobject_for_key(key_root, p_out);
	}
	void get_cfobject_for_key(const key_t & key, cfobject::object_t::ptr_t & p_out);
	void skip_eol_and_whitespace()
	{
		while (*ptr == '\r' || *ptr=='\n' || *ptr == ' ' || *ptr == '\t') ptr++;
	}
	static void g_skip_eol_and_whitespace(const char * & p_ptr)
	{
		while (*p_ptr == '\r' || *p_ptr=='\n' || *p_ptr == ' ' || *p_ptr == '\t') p_ptr++;
	}
	const char * ptr;
};

class XMLPlistParserFromFile : public XMLPlistParser
{
public:
	XMLPlistParserFromFile (service_ptr_t<file> & ptr, abort_callback & p_abort)
	{
		file_to_string(ptr, p_abort);
	}
	XMLPlistParserFromFile (const char * path, abort_callback & p_abort)
	{
		file::ptr ptr;
		filesystem::g_open(ptr, path, filesystem::open_mode_read, p_abort);
		file_to_string(ptr, p_abort);
	}
private:
	void file_to_string(service_ptr_t<file> & ptr, abort_callback & p_abort)
	{
		t_size size = pfc::downcast_guarded<t_size>(ptr->get_size_ex(p_abort));
		ptr->read(pfc::string_buffer(m_buffer, size), size, p_abort);
		set_stream(m_buffer.get_ptr());
	}
	pfc::string8 m_buffer;
};

class PlistParser
{
public:
	PlistParser(const void * data, t_size size)
	{
		try
		{
			abort_callback_dummy p_abort;
			bplist::reader br(p_abort);
			br.read(data, size);
			m_root_object = br.m_root_object;
		}
		catch (mmh::exception_wrong_format const &)
		{
			XMLPlistParser(pfc::string8((const char *)data, size)).run_cfobject(m_root_object);
		}
	}
	cfobject::object_t::ptr_t m_root_object;
};

class PlistParserFromFile
{
public:
	PlistParserFromFile(const char * path, abort_callback & p_abort)
	{
		file::ptr f;
		filesystem::g_open_read(f, path, p_abort);

		pfc::array_staticsize_t<t_uint8> data(pfc::downcast_guarded<t_size>(f->get_size_ex(p_abort)));
		f->read(data.get_ptr(), data.get_size(), p_abort);
		m_root_object = PlistParser(data.get_ptr(), data.get_size()).m_root_object;
	}
	cfobject::object_t::ptr_t m_root_object;
};