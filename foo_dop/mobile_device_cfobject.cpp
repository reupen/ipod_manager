#include "stdafx.h"

#include "mobile_device_v2.h"

bool g_get_CFType_object (const in_mobile_device_api_handle_sync & api, CFTypeRef ref, cfobject::object_t::ptr_t & p_out)
{
	if (ref)
	{
		p_out = new cfobject::object_t;

		CFTypeID type = api->CFGetTypeID(ref);
		if (type == api->CFStringGetTypeID())
		{
			CFStringRef str = (CFStringRef)ref;
			t_size len = api->CFStringGetLength(str);
			pfc::array_t<WCHAR> wstr;
			wstr.fill_null();
			wstr.set_size(len+1);
			api->CFStringGetCharacters(str, CFRangeMake(0, len), (unsigned short *)wstr.get_ptr());
			wstr[len] = 0; //Bubble wrap
			p_out->m_type = cfobject::kTagUnicodeString;
			p_out->m_string.set_string(wstr.get_ptr(), len);
		}
		else if (type == api->CFNumberGetTypeID())
		{
			if (api->CFNumberIsFloatType((CFNumberRef)ref))
			{
				double buff = NULL;
				api->CFNumberGetValue((CFNumberRef)ref, kCFNumberDoubleType, &buff);
				p_out->m_type = cfobject::kTagReal;
				p_out->m_float = buff;
			}
			else
			{
				long long buff = NULL;
				api->CFNumberGetValue((CFNumberRef)ref, kCFNumberLongLongType, &buff);
				p_out->m_type = cfobject::kTagInt;
				p_out->m_integer = buff;
			}
		}
		else if (type == api->CFBooleanGetTypeID())
		{
			p_out->m_type = cfobject::kTagBoolean;
			p_out->m_boolean = api->CFBooleanGetValue((CFBooleanRef)ref) != 0;
		}
		else if (type == api->CFDictionaryGetTypeID())
		{
			p_out->m_type = cfobject::kTagDictionary;
			CFDictionaryRef dict = (CFDictionaryRef)ref;
			t_size i, count = api->CFDictionaryGetCount(dict);
			pfc::array_t<CFTypeRef> keys, values;
			keys.set_size(count);
			values.set_size(count);
			api->CFDictionaryGetKeysAndValues(dict, keys.get_ptr(), values.get_ptr());
			p_out->m_dictionary.set_size(count);
			for (i=0; i<count; i++)
			{
				g_get_CFType_object(api, keys[i], p_out->m_dictionary[i].m_key);
				g_get_CFType_object(api, values[i], p_out->m_dictionary[i].m_value);
				if (p_out->m_dictionary[i].m_value.is_valid() && p_out->m_dictionary[i].m_key.is_valid())
					p_out->m_dictionary[i].m_value->m_key = p_out->m_dictionary[i].m_key->m_string;
			}
		}
		else if (type == api->CFArrayGetTypeID())
		{
			p_out->m_type = cfobject::kTagArray;
			CFArrayRef arr = (CFArrayRef)ref;
			t_size i, count = api->CFArrayGetCount(arr);
			p_out->m_array.resize(count);
			for (i=0; i<count; i++)
			{
				CFTypeRef child = api->CFArrayGetValueAtIndex(arr, i);
				g_get_CFType_object(api, child, p_out->m_array[i]);
			}
		}
		else if (type == api->CFDataGetTypeID())
		{
			CFDataRef data = (CFDataRef)ref;
			t_size len = api->CFDataGetLength(data);

			p_out->m_data.set_size(len);
			api->CFDataGetBytes(data, CFRangeMake(0, len), p_out->m_data.get_ptr());

			p_out->m_type = cfobject::kTagData;
		}
		else return false;
	}
	else return false;

	return true;
}

void g_get_sql_commands (cfobject::object_t::ptr_t const & cfobj, pfc::string_list_impl & p_out, t_size & p_version)
{
	class t_names : public pfc::string_list_impl 
	{
	public:
		bool have_string(const char * str)
		{
			t_size i, count = get_count();
			for (i=0; i<count; i++)
			{
				if (!stricmp_utf8(str, get_item(i)))
					return true;
			}
			return false;
		}
	}
	names;

	p_version = 26; //user_version

	if (cfobj.is_valid())
	{
		{
			cfobject::object_t::ptr_t UserVersionCommandSets, CommandSet, Commands;
			if (cfobj->m_dictionary.get_child(L"UserVersionCommandSets", UserVersionCommandSets))
			{
				while (!UserVersionCommandSets->m_dictionary.get_child(pfc::stringcvt::string_wide_from_utf8(pfc::string8() << p_version), CommandSet) && p_version)
					p_version--;

				if (CommandSet.is_valid() && CommandSet->m_dictionary.get_child(L"Commands", Commands))
				{
					t_size j, count = Commands->m_array.size();
					for (j=0; j<count; j++)
						if (Commands->m_array[j].is_valid())
							names.add_item(pfc::stringcvt::string_utf8_from_wide(Commands->m_array[j]->m_string.get_ptr()));
				}
			}
		}
		{
			cfobject::object_t::ptr_t SQLCommands;

			if (cfobj->m_dictionary.get_child(L"SQLCommands", SQLCommands))
			{
				t_size j, count_names = names.get_count();
				for (j=0; j<count_names; j++)
				{
					cfobject::object_t::ptr_t Command;
					if (SQLCommands->m_dictionary.get_child(pfc::stringcvt::string_wide_from_utf8(names[j]), Command))
						p_out.add_item(pfc::stringcvt::string_utf8_from_wide(Command->m_string.get_ptr()));
				}
			}
		}
	}
}
