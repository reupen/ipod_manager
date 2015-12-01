#include "main.h"

namespace cfobject
{
	bool object_t::dictionary::get_child (const wchar_t * key, object_t::ptr_t & p_value)
	{
		ensure_sorted();
		t_size index;
		if (bsearch_t(dictionary_entry_t::g_compare_value, key, index))
			return (p_value = get_item(index).m_value).is_valid();
		return false;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, pfc::string8 & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = pfc::stringcvt::string_utf8_from_wide(child->m_string);
		return b_ret;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, bool & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = (child->get_bool());
		return b_ret;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, t_uint32 & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = (child->get_flat_uint32());
		return b_ret;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, t_uint16 & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = (child->get_flat_uint16());
		return b_ret;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, t_int32 & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = (child->get_flat_int32());
		return b_ret;
	}
	bool object_t::dictionary::get_child (const wchar_t * key, t_uint64 & p_value)
	{
		return get_child(key, (t_int64&)p_value);
	}
	bool object_t::dictionary::get_child (const wchar_t * key, t_int64 & p_value)
	{
		bool b_ret = false;
		object_t::ptr_t child;
		if (b_ret = get_child(key, child))
			p_value = (child->m_integer);
		return b_ret;
	}
	void object_t::dictionary::ensure_sorted()
	{
		if (!m_sorted)
		{
			sort_t(dictionary_entry_t::g_compare);
			m_sorted = true;
		}
	}

	void g_print_object(const object_t::ptr_t & ptr, pfc::string8 & p_out)
	{
		if (ptr.is_valid())
		{
			switch (ptr->m_type)
			{
			case kTagDictionary:
				{
					p_out << "{\r\n";
					for (t_size i = 0, count = ptr->m_dictionary.get_count(); i<count; i++)
					{
						g_print_object(ptr->m_dictionary[i].m_key, p_out);
						p_out << " : ";
						g_print_object(ptr->m_dictionary[i].m_value, p_out);
						p_out << "\r\n";
					}
					p_out << "}";
				}
				break;
			case kTagArray:
				{
					p_out << "{\r\n";
					for (t_size i = 0, count = ptr->m_array.get_count(); i<count; i++)
					{
						g_print_object(ptr->m_array[i], p_out);
						p_out << "\r\n";
					}
					p_out << "}\r\n";
				}
				break;
			case kTagUnicodeString:
				p_out << pfc::stringcvt::string_utf8_from_wide(ptr->m_string.get_ptr());
				break;
			case kTagReal:
				p_out << pfc::format_float(ptr->m_float);
				break;
			case kTagInt:
				p_out << (ptr->m_integer);
				break;
			case kTagDate:
				{
					std::basic_string<TCHAR> str;
					win32_helpers::format_date(ptr->m_date, str);
					p_out << pfc::stringcvt::string_utf8_from_os(str.data());
				}
				break;
			case kTagData:
				p_out << pfc::format_hexdump_lowercase(ptr->m_data.get_ptr(), ptr->m_data.get_size());
				break;
			case kTagBoolean:
				p_out << (ptr->m_boolean ? "true" : "false");
				break;
			};
		}
	};
	void g_export_object_to_inner_xml (const object_t::ptr_t & ptr, pfc::string8 & p_out)
	{
		if (ptr.is_valid())
		{
			switch (ptr->m_type)
			{
			case kTagDictionary:
				{
					p_out << "<dict>\n";
					for (t_size i = 0, count = ptr->m_dictionary.get_count(); i<count; i++)
					{
						if (ptr->m_dictionary[i].m_value.is_valid())
						{
							p_out << "<key>" 
								<< string_escape_xml(pfc::stringcvt::string_utf8_from_wide(ptr->m_dictionary[i].m_value->m_key))
								<< "</key>\n";
						}
						g_export_object_to_inner_xml(ptr->m_dictionary[i].m_value, p_out);
					}
					p_out << "</dict>\n";
				}
				break;
			case kTagArray:
				{
					p_out << "<array>\n";
					for (t_size i = 0, count = ptr->m_array.get_count(); i<count; i++)
					{
						g_export_object_to_inner_xml(ptr->m_array[i], p_out);
					}
					p_out << "</array>\n";
				}
				break;
			case kTagUnicodeString:
				p_out << "<string>" <<  string_escape_xml(pfc::stringcvt::string_utf8_from_wide(ptr->m_string.get_ptr())) << "</string>\n";
				break;
			case kTagReal:
				p_out << "<real>" <<  pfc::format_float(ptr->m_float) << "</real>\n";
				break;
			case kTagInt:
				p_out << "<integer>" << (ptr->m_integer) << "</integer>\n";
				break;
			case kTagDate:
				{
					FILETIME ft;
					ft.dwLowDateTime = (DWORD)(ptr->m_date);
					ft.dwHighDateTime = (DWORD)(ptr->m_date >> 32);
					SYSTEMTIME st;
					FileTimeToSystemTime(&ft, &st);

					p_out << "<date>" 
						<< pfc::format_int(st.wYear, 4) << "-"
						<< pfc::format_int(st.wMonth, 2) << "-"
						<< pfc::format_int(st.wDay, 2) << "T"
						<< pfc::format_int(st.wHour, 2) << ":"
						<< pfc::format_int(st.wMinute, 2) << ":"
						<< pfc::format_int(st.wSecond, 2) << "Z"
						<< "</date>\n";
				}
				break;
			case kTagData:
				p_out << "<data>";
				pfc::base64_encode_append(p_out, ptr->m_data.get_ptr(), ptr->m_data.get_size());
				p_out << "</data>\n";
				break;
			case kTagBoolean:
				p_out << (ptr->m_boolean ? "<true/>" : "<false/>") << "\n";
				break;
			};
		}
	}
	void g_export_object_to_xml (const object_t::ptr_t & ptr, pfc::string8 & p_out)
	{
		p_out.reset();
		p_out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
				"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
				"<plist version=\"1.0\">\n";
		g_export_object_to_inner_xml(ptr, p_out);
		p_out << "</plist>";

	}
}