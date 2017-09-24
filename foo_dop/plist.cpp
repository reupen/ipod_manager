#include "main.h"

#include "plist.h"

t_filetimestamp g_filetime_to_timestamp(const LPFILETIME ft);

t_filetimestamp g_iso_timestamp_to_filetime (const char * str, t_size len = pfc_infinite)
{
	len = pfc::strlen_max(str, len);

	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));

	if (len == 4 + 1 + 2 + 1 + 2 + 1 + 8 + 1)
	{
		st.wYear = mmh::strtoul_n(str+0, 4);
		st.wMonth = mmh::strtoul_n(str+5, 2);
		st.wDay = mmh::strtoul_n(str+8, 2);
		st.wHour = mmh::strtoul_n(str+11, 2);
		st.wMinute = mmh::strtoul_n(str+14, 2);
		st.wSecond = mmh::strtoul_n(str+17, 2);
		FILETIME ft = {0};
		SystemTimeToFileTime(&st, &ft);
		return g_filetime_to_timestamp(&ft);
	}
	else return 0;
}

string_descape_xml::string_descape_xml(const char* p_source, t_size len)
	{
		len = pfc::strlen_max(p_source, len);
		const char * ptr = p_source, *start = ptr;
		while (t_size(ptr-p_source) < len)
		{
			start = ptr;
			while (t_size(ptr-p_source) < len && *ptr != '&') ptr++;
			if (ptr > start) add_string(start, ptr-start);
			if (t_size(ptr-p_source) < len && *ptr == '&')
			{
				ptr++;
				start = ptr;
				while (t_size(ptr-p_source) < len && *ptr != ';' && *ptr != ' ') ptr++;

				bool b_valid_escape = t_size(ptr-p_source) < len && *ptr == ';';

				if (b_valid_escape)
				{
					if (!strncmp(start, "lt", ptr-start)) add_string("<");
					else if (!strncmp(start, "gt", ptr-start)) add_string(">");
					else if (!strncmp(start, "amp", ptr-start)) add_string("&");
					else if (!strncmp(start, "apos", ptr-start)) add_string("'");
					else if (!strncmp(start, "quot", ptr-start)) add_string("\"");
					else b_valid_escape = false;
					ptr++;
				}

				if (!b_valid_escape)
				{
					add_byte('&');
					ptr = start;
				}
			}
		}
	}

bool g_test_xml_char_escape(char c)
{
	return c == '<' || c == '>' || c == '&' || c == '\'' || c == '\"';
}
string_escape_xml::string_escape_xml(const char* p_source, t_size len)
	{
		len = pfc::strlen_max(p_source, len);
		const char * ptr = p_source, *start = ptr;
		while (t_size(ptr-p_source) < len)
		{
			start = ptr;
			while (t_size(ptr-p_source) < len && !g_test_xml_char_escape(*ptr)) ptr++;
			if (ptr > start) add_string(start, ptr-start);
			while (t_size(ptr-p_source) < len && g_test_xml_char_escape(*ptr))
			{
				if (*ptr == '<') add_string("&lt;");
				else if (*ptr == '>') add_string("&gt;");
				else if (*ptr == '&') add_string("&amp;");
				else if (*ptr == '\'') add_string("&apos;");
				else if (*ptr == '\"') add_string("&quot;");
				ptr++;
			}
		}
	}


void XMLPlistParser::get_keys(ipod_info & p_out)
	{
		while (*ptr)
		{
			if (!stricmp_utf8_max(ptr, "</dict>", 7))
				break;
			if (stricmp_utf8_max(ptr, "<key>", 5))
				throw exception_io_unsupported_format();
			ptr+= 5;
			const char * key_start = ptr;
			t_size key_len=0;
			while (*ptr && *ptr != '<') ptr++;
			key_len=ptr-key_start;
			if (stricmp_utf8_max(ptr, "</key>", 6)) 
				throw exception_io_unsupported_format();
			ptr+=6;
			skip_eol_and_whitespace();
			if (*ptr == '<') ptr++;
			else throw exception_io_unsupported_format();
			const char * type_start = ptr;
			t_size type_len=0;
			while (*ptr && *ptr != '>' && *ptr != '/') ptr++;
			type_len=ptr-type_start;
			bool selfcontained = *ptr == '/';
			if (selfcontained) ptr++;
			if (*ptr == '>') ptr++;
			else throw exception_io_unsupported_format();
			t_size counter = selfcontained ? 0 : 1;
			const char * val_start = ptr;
			t_size val_len=0;
			while (counter)
			{
				while (*ptr && *ptr != '<') ptr++;
				val_len = ptr-val_start;
				if (*ptr == '<') ptr++;
				else throw exception_io_unsupported_format();
				bool b_closer = *ptr == '/';
				if (b_closer) ptr++;
				const char * type2_start = ptr;
				t_size type2_len=0;
				while (*ptr && *ptr != '>') ptr++;
				type2_len=ptr-type2_start;
				if (*ptr == '>') ptr++;
				else throw exception_io_unsupported_format();

				if (!stricmp_utf8_ex(type_start, type_len, type2_start, type2_len))
					counter += b_closer ? -1 : 1;
				skip_eol_and_whitespace();
			}
			if (!stricmp_utf8_max(key_start, "SerialNumber", key_len))
			{
				p_out.serial . set_string(val_start, val_len);
			}
			else if (!stricmp_utf8_max(key_start, "FamilyID", key_len))
			{
				p_out.family_valid = true;
				p_out.family_id = mmh::strtoul_n(val_start, val_len);
			}
			else if (!stricmp_utf8_max(key_start, "UpdaterFamilyID", key_len))
			{
				p_out.updater_family_id = mmh::strtoul_n(val_start, val_len);
			}
			else if (!stricmp_utf8_max(key_start, "VisibleBuildID", key_len))
				p_out.firmware . set_string(val_start, val_len);
			else if (!stricmp_utf8_max(key_start, "BatteryPollInterval", key_len))
				p_out.battery_poll_interval = mmh::strtoul_n(val_start, val_len);;
			skip_eol_and_whitespace();
		}
	}

bool XMLPlistParser::g_get_tag_v2(const char * & ptr, key_t & p_out)
	{
		if (!stricmp_utf8_max(ptr, "<key>", 5))
		{
			ptr+= 5;
			const char * & key_start = p_out.m_key_name;
			key_start = ptr;
			t_size & key_len=p_out.m_key_name_length;
			key_len = 0;
			while (*ptr && *ptr != '<') ptr++;
			key_len=ptr-key_start;
			if (stricmp_utf8_max(ptr, "</key>", 6)) 
				throw exception_io_unsupported_format();
			ptr+=6;
			g_skip_eol_and_whitespace(ptr);
		}
		if (*ptr == '<') ptr++;
		else throw exception_io_unsupported_format();
		const char * & type_start = p_out.m_value_type;
		type_start = ptr;
		t_size & type_len=p_out.m_value_type_length;
		type_len=0;
		while (*ptr && *ptr != '>' && *ptr != '/') ptr++;
		type_len=ptr-type_start;
		bool & selfcontained = p_out.m_value_self_contained;
		selfcontained = *ptr == '/';
		if (selfcontained) ptr++;
		if (*ptr == '>') ptr++;
		else throw exception_io_unsupported_format();
		t_size counter = selfcontained ? 0 : 1;
		if (!selfcontained)
			g_skip_eol_and_whitespace(ptr);
		const char * & val_start = p_out.m_value;
		val_start =ptr;
		t_size & val_len=p_out.m_value_length;
		val_len=0;
		while (counter)
		{
			while (*ptr && *ptr != '<') ptr++;
			val_len = ptr-val_start;
			if (*ptr == '<') ptr++;
			else throw exception_io_unsupported_format();
			bool b_closer = *ptr == '/';
			if (b_closer) ptr++;
			const char * type2_start = ptr;
			t_size type2_len=0;
			while (*ptr && *ptr != '>') ptr++;
			type2_len=ptr-type2_start;
			if (*ptr == '>') ptr++;
			else throw exception_io_unsupported_format();

			if (!stricmp_utf8_ex(type_start, type_len, type2_start, type2_len))
				counter += b_closer ? -1 : 1;
			g_skip_eol_and_whitespace(ptr);
		}
		g_skip_eol_and_whitespace(ptr);
		return true;
	}

t_uint32 XMLPlistParser::get_cftype(const key_t & p_key)
	{
		if (!stricmp_utf8_max(p_key.m_value_type, "dict", p_key.m_value_type_length))
			return cfobject::kTagDictionary;
		else if (!stricmp_utf8_max(p_key.m_value_type, "array", p_key.m_value_type_length))
			return cfobject::kTagArray;
		else if (!stricmp_utf8_max(p_key.m_value_type, "string", p_key.m_value_type_length))
			return cfobject::kTagUnicodeString;
		else if (!stricmp_utf8_max(p_key.m_value_type, "date", p_key.m_value_type_length))
			return cfobject::kTagDate;
		else if (!stricmp_utf8_max(p_key.m_value_type, "data", p_key.m_value_type_length))
			return cfobject::kTagData;
		else if (!stricmp_utf8_max(p_key.m_value_type, "integer", p_key.m_value_type_length))
			return cfobject::kTagInt;
		else if (!stricmp_utf8_max(p_key.m_value_type, "real", p_key.m_value_type_length))
			return cfobject::kTagReal;
		else if (p_key.m_value_self_contained && 
			(!stricmp_utf8_max(p_key.m_value_type, "true", p_key.m_value_type_length) || !stricmp_utf8_max(p_key.m_value_type, "false", p_key.m_value_type_length))
			)
			return cfobject::kTagBoolean;
		else
			return cfobject::kTagUnset;
	}

void g_strip_spaces_tabs(const char * str, t_size size, pfc::string8 & p_out)
{
	const char * ptr = str;

	while (t_size(ptr-str) < size)
	{
		const char *start = ptr;
		while (t_size(ptr-str) < size && *ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n')
			ptr++;
		if (ptr > start) p_out.add_string(start, ptr-start);
		while (t_size(ptr-str) < size && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
			ptr++;
	}

}

void XMLPlistParser::get_cfobject_for_key(const key_t & key, cfobject::object_t::ptr_t & p_out)
	{
		t_uint32 cftype = get_cftype(key);
		if (cftype != cfobject::kTagUnset)
		{
			p_out = new cfobject::object_t;
			p_out->m_type = cftype;
			p_out->m_key.set_string(pfc::stringcvt::string_wide_from_utf8(key.m_key_name, key.m_key_name_length));
			switch (cftype)
			{
			case cfobject::kTagDictionary:
				{
					pfc::string8 contents(key.m_value, key.m_value_length);
					const char * ptr = contents.get_ptr();
					while (*ptr)
					{
						cfobject::object_t::ptr_t temp = new cfobject::object_t;
						key_t key_child;
						if (!g_get_tag_v2(ptr, key_child)) break;
						get_cfobject_for_key(key_child, temp);
						cfobject::dictionary_entry_t de;
						de.m_key = new cfobject::object_t;
						de.m_key->m_type = cfobject::kTagUnicodeString;
						de.m_key->m_string.set_string(pfc::stringcvt::string_wide_from_utf8(key_child.m_key_name, key_child.m_key_name_length));
						de.m_value= temp;
						p_out->m_dictionary.add_item(de);
					}
				}
				break;
			case cfobject::kTagArray:
				{
					pfc::string8 contents(key.m_value, key.m_value_length);
					cfobject::object_t::ptr_t temp = new cfobject::object_t;
					const char * ptr = contents.get_ptr();
					while (*ptr)
					{
						key_t key_child;
						if (!g_get_tag_v2(ptr, key_child)) break;
						get_cfobject_for_key(key_child, temp);
						p_out->m_array.add_item(temp);
					}
				}
				break;
			case cfobject::kTagUnicodeString:
				{
					p_out->m_string.set_string(pfc::stringcvt::string_wide_from_utf8(string_descape_xml(key.m_value, key.m_value_length)));
				}
				break;
			case cfobject::kTagData:
				{
					pfc::string8 stripped;
					g_strip_spaces_tabs(key.m_value, key.m_value_length, stripped);					
					
					try {
					p_out->m_data.set_size(pfc::base64_decode_estimate(stripped));
					pfc::base64_decode(stripped, p_out->m_data.get_ptr());
					} catch (pfc::exception const &) 
					{
						p_out->m_data.set_size(0);
					};
				}
				break;
			case cfobject::kTagReal:
				{
					p_out->m_float = pfc::string_to_float(key.m_value, key.m_value_length);
				}
				break;
			case cfobject::kTagDate:
				{
					p_out->m_date = g_iso_timestamp_to_filetime(key.m_value, key.m_value_length);
				}
				break;
			case cfobject::kTagInt:
				{
					p_out->m_integer = mmh::strtol64_n(key.m_value, key.m_value_length);
				}
				break;
			case cfobject::kTagBoolean:
				{
					p_out->m_boolean = !stricmp_utf8_max(key.m_value_type, "true", key.m_value_type_length);
				}
				break;
			}
		}
	}

void g_get_checkpoint_artwork_format_single(cfobject::object_t::ptr_t const & AlbumArt, pfc::list_t<artwork_format_t> & p_out)
{
	t_size i, count = AlbumArt->m_array.get_count();
	for (i=0; i<count; i++)
	{
		if (!AlbumArt->m_array[i].is_valid()) break;

		if (AlbumArt->m_array[i]->m_type == cfobject::kTagDictionary)
		{

			artwork_format_t fmt;

#define getArtElemUint(elem, member) \
	if (AlbumArt->m_array[i]->m_dictionary.get_child(L#elem, elem)) \
	fmt.##member = elem->get_flat_uint32()

#define getArtElemBool(elem, member) \
	if (AlbumArt->m_array[i]->m_dictionary.get_child(L#elem, elem)) \
	fmt.##member = elem->get_bool()

#define getArtElemUintStr(elem, member) \
	if (AlbumArt->m_array[i]->m_dictionary.get_child(L#elem, elem)) \
	fmt.##member = mmh::strtoul_n(elem->m_string.get_ptr(), pfc_infinite, 0x10)

			cfobject::object_t::ptr_t FormatId, ExcludedFormats, RenderHeight, RenderWidth, BackColor, 
				AssociatedFormat, PixelFormat, OffsetAlignment, PixelOrder,
				LeftInset, TopInset, RightInset, BottomInset, Interlaced, Crop, AlignRowBytes, RowBytesAlignment,
				OriginalHeight, OriginalWidth, MinimumSize;

			if (!AlbumArt->m_array[i]->m_key.is_empty())
				fmt.m_format_id = mmh::strtoul_n(AlbumArt->m_array[i]->m_key.get_ptr(), AlbumArt->m_array[i]->m_key.length());

			getArtElemUint(FormatId, m_format_id);
			getArtElemUint(ExcludedFormats, m_excluded_formats);
			getArtElemUint(RenderHeight, m_render_height);
			getArtElemUint(RenderWidth, m_render_width);
			getArtElemUintStr(BackColor, m_back_colour);
			getArtElemUint(AssociatedFormat, m_associated_format);
			getArtElemUintStr(PixelFormat, m_pixel_format);
			fmt.m_rgb555 = fmt.m_pixel_format == 'L555';
			getArtElemBool(Interlaced, m_interlaced);
			getArtElemBool(Crop, m_crop);
			getArtElemBool(AlignRowBytes, m_align_row_bytes);
			getArtElemUint(OffsetAlignment, m_offset_alignment);
			getArtElemUint(RowBytesAlignment, m_row_byte_alignment);
			getArtElemBool(PixelOrder, m_alternate_pixel_order);
			getArtElemUint(LeftInset, m_left_inset);
			getArtElemUint(TopInset, m_top_inset);
			getArtElemUint(RightInset, m_right_inset);
			getArtElemUint(BottomInset, m_bottom_inset);
			getArtElemUint(OriginalHeight, m_original_height);
			getArtElemUint(OriginalWidth, m_original_width);
			getArtElemUint(MinimumSize, m_minimum_size);

			//exclude unsupported formats
			if (fmt.m_pixel_format == 'L565' || fmt.m_pixel_format == 'L555' || fmt.m_pixel_format == 'jpeg')
				p_out.add_item(fmt);
		}
	}
}

void g_get_checkpoint_artwork_formats(cfobject::object_t::ptr_t const & deviceInfo, device_properties_t & p_out)
{
	cfobject::object_t::ptr_t AlbumArt, ChapterImageSpecs, ImageSpecifications;
	if (deviceInfo->m_dictionary.get_child(L"AlbumArt2", AlbumArt) || deviceInfo->m_dictionary.get_child(L"AlbumArt", AlbumArt))
		g_get_checkpoint_artwork_format_single(AlbumArt, p_out.m_artwork_formats);
	if (deviceInfo->m_dictionary.get_child(L"ChapterImageSpecs2", ChapterImageSpecs) || deviceInfo->m_dictionary.get_child(L"ChapterImageSpecs", ChapterImageSpecs))
		g_get_checkpoint_artwork_format_single(ChapterImageSpecs, p_out.m_chapter_artwork_formats);
	if (deviceInfo->m_dictionary.get_child(L"ImageSpecifications2", ImageSpecifications) || deviceInfo->m_dictionary.get_child(L"ImageSpecifications", ImageSpecifications))
		g_get_checkpoint_artwork_format_single(ImageSpecifications, p_out.m_image_formats);
}

void g_get_checkpoint_device_info(cfobject::object_t::ptr_t const & checkpoint, device_properties_t & p_out)
{
	pfc::string8 msg;
	cfobject::object_t::ptr_t node, childNode;
	if (checkpoint->m_dictionary.get_child(L"DBVersion", node))
	{
		p_out.m_db_version = node->get_flat_uint32();
		msg << "DBVersion: " << p_out.m_db_version;
	}
	if (checkpoint->m_dictionary.get_child(L"FireWireGUID", node))
	{
		p_out.m_FireWireGUID = pfc::stringcvt::string_utf8_from_wide(node->m_string);
		if (msg.get_length()) msg << ", ";
		msg << "FireWireGUID: " << p_out.m_FireWireGUID;
	}
	if (checkpoint->m_dictionary.get_child(L"SQLiteDB", node))
	{
		p_out.m_SQLiteDB = node->get_bool();
		if (msg.get_length()) msg << ", ";
		msg << "SQLiteDB";
	}
	if (checkpoint->m_dictionary.get_child(L"ShadowDB", node))
	{
		p_out.m_ShadowDB = node->get_bool();
		if (msg.get_length()) msg << ", ";
		msg << "ShadowDB";
	}
	if (checkpoint->m_dictionary.get_child(L"ShadowDBVersion", node))
	{
		p_out.m_ShadowDBVersion = node->get_flat_uint32();
		if (msg.get_length()) msg << ", ";
		msg << "ShadowDBVersion: " << p_out.m_ShadowDBVersion;
	}
	if (checkpoint->m_dictionary.get_child(L"SupportsSoundCheck", node))
		p_out.m_SupportsSoundCheck = node->get_bool();

	if (checkpoint->m_dictionary.get_child(L"PlaylistFoldersSupported", node))
		p_out.m_PlaylistFoldersSupported = node->get_bool();

	if (checkpoint->m_dictionary.get_child(L"Speakable", node))
	{
		if (node->m_dictionary.get_child(L"SampleRate", childNode))
		{
			p_out.m_Speakable = true;
			p_out.m_SpeakableSampleRate = childNode->get_flat_uint32();
		}
	}

	console::formatter() << "iPod manager: Device properties: " << msg;

	g_get_checkpoint_artwork_formats(checkpoint, p_out);

	cfobject::object_t::ptr_t SQLMusicLibraryPostProcessCommands;
	if (checkpoint->m_dictionary.get_child(L"com.apple.mobile.iTunes.SQLMusicLibraryPostProcessCommands", SQLMusicLibraryPostProcessCommands))
	{
		g_get_sql_commands(SQLMusicLibraryPostProcessCommands, p_out.m_SQLMusicLibraryPostProcessCommands, p_out.m_SQLMusicLibraryUserVersion);
		console::formatter() << "iPod manager: " << p_out.m_SQLMusicLibraryPostProcessCommands.get_count() << " post process SQL commands, version " << p_out.m_SQLMusicLibraryUserVersion;
	}
}

void XMLPlistParser::run_artwork_v2(device_properties_t & p_out)
{
	cfobject::object_t::ptr_t checkpoint;
	run_cfobject(checkpoint);
	if (!checkpoint.is_valid())
		throw exception_io_unsupported_format();

	p_out.m_Initialised = true;

	g_get_checkpoint_device_info(checkpoint, p_out);
}
