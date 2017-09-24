#include "stdafx.h"

#include "itunesdb.h"

namespace itunesdb {
	void reader::read_pyhm(t_header_marker<identifiers::pyhm> & p_header, pfc::rcptr_t< itunesdb::t_playlist > & p_out, abort_callback & p_abort)
	{
		stream_reader_memblock_ref_dop pyhm(p_header.data.get_ptr(), p_header.data.get_size(), m_swap_endianess);
		t_uint32 count_do, count_pi;
		//t_uint16 count_string_dohm;
		p_out = pfc::rcnew_t< itunesdb::t_playlist >();
		pyhm.read_lendian_auto_t(count_do, p_abort); //12
		pyhm.read_lendian_auto_t(count_pi, p_abort); //16
		pyhm.read_lendian_auto_t(p_out->is_master, p_abort); //20
		pyhm.read_lendian_auto_t(p_out->shuffle_items, p_abort); //21
		pyhm.read_lendian_auto_t(p_out->has_been_shuffled, p_abort); //22
		pyhm.read_lendian_auto_t(p_out->unk3, p_abort); //23
		pyhm.read_lendian_auto_t(p_out->timestamp, p_abort); //24
		pyhm.read_lendian_auto_t(p_out->id, p_abort); //28
		pyhm.read_lendian_auto_t(p_out->unk4, p_abort); //36
		pyhm.read_lendian_auto_t(p_out->repeat_mode, p_abort); //40
		pyhm.read_lendian_auto_t(p_out->podcast_flag, p_abort); //42
		pyhm.read_lendian_auto_t(p_out->folder_flag, p_abort); //43
		pyhm.read_lendian_auto_t(p_out->sort_order, p_abort); //44

		try
		{
			pyhm.read_lendian_auto_t(p_out->parentid, p_abort); //48
			pyhm.read_lendian_auto_t(p_out->workout_template_id, p_abort); //56
			pyhm.read_lendian_auto_t(p_out->unk6, p_abort); //60
			pyhm.read_lendian_auto_t(p_out->unk7, p_abort); //64
			pyhm.read_lendian_auto_t(p_out->unk8, p_abort); //68
			pyhm.read_lendian_auto_t(p_out->unk9, p_abort); //72
			pyhm.read_lendian_auto_t(p_out->album_field_order, p_abort); //76
			pyhm.read_lendian_auto_t(p_out->unk11, p_abort); //80
			pyhm.read_lendian_auto_t(p_out->unk12, p_abort); //81
			pyhm.read_lendian_auto_t(p_out->unk13, p_abort); //82
			pyhm.read_lendian_auto_t(p_out->sort_direction, p_abort); //83
			pyhm.read_lendian_auto_t(p_out->unk14, p_abort); //84
			pyhm.read_lendian_auto_t(p_out->date_modified, p_abort); //88
		}
		catch (exception_io_data_truncation const &) {};

		//t_filesize start = m_file->get_position(p_abort);

		unsigned i;
		for (i = 0; i<count_do; i++)
		{
			t_header_marker<identifiers::dohm> dohm;
			read_header(dohm, p_abort);
			stream_reader_memblock_ref_dop p_do(dohm.data.get_ptr(), dohm.data.get_size(), m_swap_endianess);
			t_uint32 type;
			p_do.read_lendian_auto_t(type, p_abort);

			if (type == do_types::title)
			{
				pfc::string8 str_u;
				read_do_string_utf16(dohm, str_u, p_abort);
				p_out->name.set_string(str_u);
			}
			else if (type == do_types::library_index)
			{
#ifndef LOAD_LIBRARY_INDICES
				m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
#else
				pfc::array_t<t_uint8> dohm_data;
				pfc::array_t<t_uint8> do_data;
				dohm_data.append_fromptr(dohm.data.get_ptr(), dohm.data.get_size());
				do_data.set_size(dohm.section_size - dohm.header_size);
				m_file->read(do_data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
				stream_reader_memblock_ref_dop p_data(do_data.get_ptr(), do_data.get_size());
				t_uint32 temp;
				t_playlist::t_library_index li;
				p_data.read_lendian_auto_t(li.type, p_abort);
				//console::formatter() << pfc::format_hex(temp);
				//console::formatter fmt;
				p_data.read_lendian_auto_t(temp, p_abort);
				li.indices.set_count(temp);
				//fmt << temp << ": ";
				p_data.skip(4 * 10, p_abort);

				t_size j;
				for (j = 0; j<temp; j++)
					p_data.read_lendian_auto_t(li.indices[j], p_abort);
				p_out->m_library_indices.add_item(li);
#endif 
			}
			else if (type == do_types::smart_playlist_data)
			{
				read_do_smart_playlist_data(dohm, p_out->smart_playlist_data, p_abort);
				p_out->smart_data_valid = true;
			}
			else if (type == do_types::smart_playlist_rules)
			{
				read_do_smart_playlist_rules(dohm, p_out->smart_playlist_rules, p_abort);
				p_out->smart_rules_valid = true;
			}
			else if (type == do_types::itunes_columns_info)
			{
				p_out->dohm_column_data.append_fromptr(dohm.data.get_ptr(), dohm.data.get_size());
				p_out->do_column_data.set_size(dohm.section_size - dohm.header_size);
				m_file->read(p_out->do_column_data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
				p_out->column_data_valid = true;
			}
			else if (type == do_types::itunes_data_102)
			{
				p_out->do_itunes_data_102.set_size(dohm.section_size - dohm.header_size);
				m_file->read(p_out->do_itunes_data_102.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
				p_out->itunes_data_102_valid = true;
			}
			else
				m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
		}
		p_out->items.set_count(count_pi);

		pfc::array_t<t_uint32> positions;
		positions.set_size(count_pi);

		for (i = 0; i<count_pi; i++)
		{
			t_header_marker<identifiers::pihm> pihm;
			read_header(pihm, p_abort);
			stream_reader_memblock_ref_dop p_pihm(pihm.data.get_ptr(), pihm.data.get_size(), m_swap_endianess);

			t_uint32 pi_count_do;
			p_pihm.read_lendian_auto_t(pi_count_do, p_abort); //12
			p_pihm.read_lendian_auto_t(p_out->items[i].unk0, p_abort); //16
			p_pihm.read_lendian_auto_t(p_out->items[i].is_podcast_group, p_abort); //17
			p_pihm.read_lendian_auto_t(p_out->items[i].is_podcast_group_expanded, p_abort); //18
			p_pihm.read_lendian_auto_t(p_out->items[i].podcast_group_name_flags, p_abort); //19
			p_pihm.read_lendian_auto_t(p_out->items[i].group_id, p_abort); //20
			p_pihm.read_lendian_auto_t(p_out->items[i].track_id, p_abort); //24
			p_pihm.read_lendian_auto_t(p_out->items[i].timestamp, p_abort); //28
			p_pihm.read_lendian_auto_t(p_out->items[i].podcast_group, p_abort); //32

			try
			{
				p_pihm.read_lendian_auto_t(p_out->items[i].unk1, p_abort); //36
				p_pihm.read_lendian_auto_t(p_out->items[i].unk2, p_abort); //40
				p_pihm.read_lendian_auto_t(p_out->items[i].item_pid, p_abort); //44
			}
			catch (exception_io_data_truncation const &) {};

			unsigned j;
			for (j = 0; j<pi_count_do; j++)
			{
				t_header_marker<identifiers::dohm> dohm;
				read_header(dohm, p_abort);
				stream_reader_memblock_ref_dop p_do(dohm.data.get_ptr(), dohm.data.get_size(), m_swap_endianess);
				t_uint32 type;
				p_do.read_lendian_auto_t(type, p_abort);
				if (type == 100)
				{
					pfc::array_t<t_uint8> data;
					data.set_size(dohm.section_size - dohm.header_size);
					m_file->read(data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
					stream_reader_memblock_ref_dop p_do2(data.get_ptr(), data.get_size(), m_swap_endianess);
					p_do2.read_lendian_auto_t(p_out->items[i].position, p_abort);

					p_out->items[i].position_valid = true;

					//supposedly numbers can be skipped etc.
					//if (position && (position - 1 < count_pi))
					//	p_out->items[position-1] = pi_id;
				}
				else if (type == do_types::playilst_item_podcast_title)
				{
					pfc::string8 str_u;
					read_do_string_utf16(dohm, p_out->items[i].podcast_title, p_abort);
					p_out->items[i].podcast_title_valid = true;
				}
				else if (type == do_types::playilst_item_podcast_sort_title)
				{
					pfc::string8 str_u;
					read_do_string_utf16(dohm, p_out->items[i].podcast_sort_title, p_abort);
					p_out->items[i].podcast_sort_title_valid = true;
				}
				else
					m_file->skip_object(dohm.section_size - dohm.header_size, p_abort);
			}

		}

		//t_filesize end = m_file->get_position(p_abort);
		//m_file->skip(p_header.section_size - p_header.header_size - (end-start), p_abort);
	}
	void reader::read_do_smart_playlist_data(t_header_marker< identifiers::dohm > & p_header, t_smart_playlist_data & p_out, abort_callback & p_abort)
	{
		//stream_reader_memblock_ref_dop p_do(p_header.data.get_ptr(), p_header.data.get_size());
		pfc::array_t<t_uint8> data;
		data.set_size(p_header.section_size - p_header.header_size);
		m_file->read(data.get_ptr(), p_header.section_size - p_header.header_size, p_abort);

		stream_reader_memblock_ref_dop p_do(data.get_ptr(), data.get_size(), m_swap_endianess);
		p_do.read_lendian_auto_t(p_out.live_update, p_abort); //0 //smart_is_dynamic
		p_do.read_lendian_auto_t(p_out.check_rules, p_abort); //1 //smart_is_filtered
		p_do.read_lendian_auto_t(p_out.check_limits, p_abort); //2 smart_is_limited
		p_do.read_lendian_auto_t(p_out.limit_type, p_abort); //3 smart_limit_kind
		p_do.read_lendian_auto_t(p_out.limit_sort, p_abort); //4 smart_limit_order
		p_do.read_lendian_auto_t(p_out.unk1, p_abort); //5
		p_do.read_lendian_auto_t(p_out.unk2, p_abort); //6
		p_do.read_lendian_auto_t(p_out.unk3, p_abort); //7
		p_do.read_lendian_auto_t(p_out.limit_value, p_abort); //8 smart_limit_value
		p_do.read_lendian_auto_t(p_out.match_checked_only, p_abort); //12 smart_enabled_only
		p_do.read_lendian_auto_t(p_out.reverse_limit_sort, p_abort); //13 smart_reverse_limit_order
		p_do.read_lendian_auto_t(p_out.unk4, p_abort); //14 smart_is_genius
		p_do.read_lendian_auto_t(p_out.unk5, p_abort); //15
	}

	void reader::read_do_smart_playlist_rules(t_header_marker< identifiers::dohm > & p_header, t_smart_playlist_rules & p_out, abort_callback & p_abort)
	{
		//stream_reader_memblock_ref_dop p_do(p_header.data.get_ptr(), p_header.data.get_size());
		pfc::array_t<t_uint8> data, data2;
		data.set_size(p_header.section_size - p_header.header_size);
		m_file->read(data.get_ptr(), p_header.section_size - p_header.header_size, p_abort);

		stream_reader_memblock_ref_dop p_do(data.get_ptr(), data.get_size(), m_swap_endianess);
		t_uint32 identifier, count;
		p_do.read_lendian_auto_t(identifier, p_abort);
		if (identifier != hm2(S, L, s, t))
			throw exception_io_unsupported_format("Unexpected smart playlist rule identifier");
		p_do.read_bendian_auto_t(p_out.unk1, p_abort);
		p_do.read_bendian_auto_t(count, p_abort);
		p_do.read_bendian_auto_t(p_out.rule_operator, p_abort);
		p_do.skip_object(120, p_abort);
		for (; count; count--)
		{
			t_smart_playlist_rule rule;
			p_do.read_bendian_auto_t(rule.field, p_abort);
			p_do.read_bendian_auto_t(rule.action, p_abort);
			p_do.read_bendian_auto_t(rule.child_ruleset_flag, p_abort);
			for (t_size i = 0, count = tabsize(rule.unk0); i<count; i++) //40 bytes
				p_do.read_bendian_auto_t(rule.unk0[i], p_abort);
			//			p_do.skip(40, p_abort);
			t_uint32 length;
			p_do.read_bendian_auto_t(length, p_abort);
			if (rule.action & (1 << 24))
			{
				pfc::array_t<WCHAR> str;
				str.set_size(length / 2);
				p_do.read(str.get_ptr(), str.get_size() * 2, p_abort);
				t_size i;
				for (i = 0; i<length / 2; i++)
				{
					byte_order::order_be_to_native_t(str[i]);
				}
				rule.string.set_string(str.get_ptr(), str.get_size());
			}
			else if (rule.child_ruleset_flag == 0)
			{
				data2.set_size(length);
				p_do.read(data2.get_ptr(), length, p_abort);
				stream_reader_memblock_ref_dop p_rule_data(data2.get_ptr(), data2.get_size(), m_swap_endianess);
				p_rule_data.read_bendian_auto_t(rule.from_value, p_abort);
				p_rule_data.read_bendian_auto_t(rule.from_date, p_abort);
				p_rule_data.read_bendian_auto_t(rule.from_units, p_abort);
				p_rule_data.read_bendian_auto_t(rule.to_value, p_abort);
				p_rule_data.read_bendian_auto_t(rule.to_date, p_abort);
				p_rule_data.read_bendian_auto_t(rule.to_units, p_abort);
				p_rule_data.read_bendian_auto_t(rule.unk1, p_abort);
				p_rule_data.read_bendian_auto_t(rule.unk2, p_abort);
				p_rule_data.read_bendian_auto_t(rule.unk3, p_abort);
				p_rule_data.read_bendian_auto_t(rule.unk4, p_abort);
				p_rule_data.read_bendian_auto_t(rule.unk5, p_abort);
			}
			else if (rule.child_ruleset_flag == 0x01000000)
			{
				rule.subrule.set_size(length);
				p_do.read(rule.subrule.get_ptr(), length, p_abort);
			}
			p_out.rules.add_item(rule);
		}
	}
	void writer::write_do_smart_playlist_data(const t_smart_playlist_data & data, abort_callback & p_abort)
	{
		stream_writer_mem dshm, ds;
		dshm.write_lendian_t(t_uint32(do_types::smart_playlist_data), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		ds.write_lendian_t(data.live_update, p_abort);
		ds.write_lendian_t(data.check_rules, p_abort);
		ds.write_lendian_t(data.check_limits, p_abort);
		ds.write_lendian_t(data.limit_type, p_abort);
		ds.write_lendian_t(data.limit_sort, p_abort);
		ds.write_lendian_t(data.unk1, p_abort);
		ds.write_lendian_t(data.unk2, p_abort);
		ds.write_lendian_t(data.unk3, p_abort);
		ds.write_lendian_t(data.limit_value, p_abort);
		ds.write_lendian_t(data.match_checked_only, p_abort);
		ds.write_lendian_t(data.reverse_limit_sort, p_abort);
		ds.write_lendian_t(data.unk4, p_abort);
		ds.write_lendian_t(data.unk5, p_abort);

		t_uint32 padding = 14;
		for (; padding; padding--)
			ds.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::dohm, dshm.get_ptr(), dshm.get_size(), ds.get_ptr(), ds.get_size(), p_abort);
	}
	void writer::g_write_smart_playlist_rules_content(const t_smart_playlist_rules & rules, stream_writer_mem & ds, abort_callback & p_abort)
	{
		ds.write_lendian_t(hm2(S, L, s, t), p_abort);
		ds.write_bendian_t(rules.unk1, p_abort);
		t_uint32 i, count = rules.rules.get_count();
		ds.write_bendian_t(count, p_abort);
		ds.write_bendian_t(rules.rule_operator, p_abort);

		t_uint32 padding = 30;
		for (; padding; padding--)
			ds.write_lendian_t(t_uint32(0), p_abort);
		for (i = 0; i<count; i++)
		{
			ds.write_bendian_t(rules.rules[i].field, p_abort);
			ds.write_bendian_t(rules.rules[i].action, p_abort);
			ds.write_bendian_t(rules.rules[i].child_ruleset_flag, p_abort);
			for (t_size j = 0, count = tabsize(rules.rules[i].unk0); j<count; j++) //40 bytes
				ds.write_bendian_t(rules.rules[i].unk0[j], p_abort);
			if (rules.rules[i].action & (1 << 24))
			{
				t_uint32 len = min(254, rules.rules[i].string.length() * 2);
				ds.write_bendian_t(len, p_abort);
				pfc::array_t<WCHAR> buffer;
				buffer.append_fromptr(rules.rules[i].string.get_ptr(), len / sizeof(WCHAR));
				t_size j;
				for (j = 0; j<len / sizeof(WCHAR); j++)
					byte_order::order_native_to_be_t(buffer[j]);
				ds.write(buffer.get_ptr(), len, p_abort);
			}
			else if (rules.rules[i].child_ruleset_flag == 0)
			{
				t_uint32 len = 0x44;
				ds.write_bendian_t(len, p_abort);
				ds.write_bendian_t(rules.rules[i].from_value, p_abort);
				ds.write_bendian_t(rules.rules[i].from_date, p_abort);
				ds.write_bendian_t(rules.rules[i].from_units, p_abort);
				ds.write_bendian_t(rules.rules[i].to_value, p_abort);
				ds.write_bendian_t(rules.rules[i].to_date, p_abort);
				ds.write_bendian_t(rules.rules[i].to_units, p_abort);
				ds.write_bendian_t(rules.rules[i].unk1, p_abort);
				ds.write_bendian_t(rules.rules[i].unk2, p_abort);
				ds.write_bendian_t(rules.rules[i].unk3, p_abort);
				ds.write_bendian_t(rules.rules[i].unk4, p_abort);
				ds.write_bendian_t(rules.rules[i].unk5, p_abort);
			}
			else
			{
				t_uint32 len = rules.rules[i].subrule.get_size();
				ds.write_bendian_t(len, p_abort);
				ds.write(rules.rules[i].subrule.get_ptr(), len, p_abort);
			}
		}
	}
	void writer::write_do_smart_playlist_rules(const t_smart_playlist_rules & rules, abort_callback & p_abort)
	{
		stream_writer_mem dshm, ds;
		dshm.write_lendian_t(t_uint32(do_types::smart_playlist_rules), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		dshm.write_lendian_t(t_uint32(0), p_abort);
		g_write_smart_playlist_rules_content(rules, ds, p_abort);
		write_section(identifiers::dohm, dshm.get_ptr(), dshm.get_size(), ds.get_ptr(), ds.get_size(), p_abort);
	}

}