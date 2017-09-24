#pragma once

#include "itunesdb.h"

namespace itunesdb {

	class chapter_reader : public itunesdb::reader
	{
	public:
		class atom
		{
		public:
			t_uint32 size;
			t_uint32 type;
			t_uint32 data0;
			t_uint32 children_count;
			t_uint32 unk0;

			class memblock_ref_t
			{
			public:
				t_uint8 * m_data;
				t_size m_size;

				memblock_ref_t() : m_data(NULL), m_size(NULL) {};

				t_uint8 * get_ptr() const {return m_data;}
				t_size get_size() const {return m_size;}

				//t_uint8 operator [](t_size index) {return m_data[index];}
			};
			memblock_ref_t data;

			void verify_identifier(t_uint32 type_wanted)
			{
				if (type_wanted != type)
				{
					t_uint32 wantu = type_wanted;
					pfc::string8 want((char*)(&wantu), 4);
					pfc::stringcvt::string_utf8_from_codepage got(pfc::stringcvt::codepage_iso_8859_1, (char*)(&type), 4);
					throw exception_io_unsupported_format(pfc::string8() << "Invalid format; expected chapter atom type \"" << want.get_ptr() << "\" got \"" << got.get_ptr() << "\"");
				}
			}
		};
		void read_atom(atom & p_out, abort_callback & p_abort)
		{
			read_bendian_auto_t(p_out.size, p_abort);
			if (p_out.size < 20) throw exception_io_unsupported_format("Invalid chapter atom size");
			read_bendian_auto_t(p_out.type, p_abort);
			read_bendian_auto_t(p_out.data0, p_abort);
			read_bendian_auto_t(p_out.children_count, p_abort);
			read_bendian_auto_t(p_out.unk0, p_abort);
			p_out.data.m_size = p_out.size - 20;
			m_file->read_nobuffer(p_out.data.m_data, p_out.data.m_size, p_abort);
		}
		void read_chapter(const atom & parent, chapter_entry & p_out, abort_callback & p_abort)
		{
			p_out.m_start_position = parent.data0;
			fbh::StreamReaderMemblock parent_data(parent.data.get_ptr(), parent.data.get_size());
			chapter_reader parent_reader(&parent_data);
			for (t_size j=0; j< parent.children_count; j++)
			{
				atom child;
				parent_reader.read_atom(child, p_abort);
				
				stream_reader_memblock_ref_dop child_data(child.data.get_ptr(), child.data.get_size());
				switch (child.type)
				{
				case 'name':
					child_data.read_string_utf16_sized_bendian_auto_t<t_uint16>(p_out.m_title, p_abort);
					break;
				case 'urlt':
					child_data.read_string_utf16_sized_bendian_auto_t<t_uint16>(p_out.m_url_title, p_abort);
					break;
				case 'url ':
					child_data.read_string_ex(p_out.m_url, child.size-20, p_abort);
					break;
				case 'ploc':
					child_data.read_bendian_auto_t(p_out.m_ploc1, p_abort);
					child_data.read_bendian_auto_t(p_out.m_ploc2, p_abort);
					child_data.read_bendian_auto_t(p_out.m_ploc3, p_abort);
					child_data.read_bendian_auto_t(p_out.m_ploc4, p_abort);
					p_out.m_ploc_valid = true;
					break;
				};
			}
		}
		void read(chapter_list & p_out, abort_callback & p_abort)
		{
			m_file->skip_object(12, p_abort);
			atom sean;
			read_atom(sean, p_abort);
			sean.verify_identifier('sean');

			fbh::StreamReaderMemblock sean_data(sean.data.get_ptr(), sean.data.get_size());
			chapter_reader sean_reader(&sean_data);

			for (t_size i=0; i< sean.children_count; i++)
			{
				atom child;
				sean_reader.read_atom(child, p_abort);
				switch (child.type)
				{
				case 'chap':
					{
						chapter_entry chapter;
						read_chapter(child, chapter, p_abort);
						p_out.add_item(chapter);
					}
					break;
				case 'hedr':
					{
						fbh::StreamReaderMemblock child_data(child.data.get_ptr(), child.data.get_size());
						chapter_reader child_reader(&child_data);
						child_reader.read_bendian_auto_t(p_out.m_hedr_1, p_abort);
						child_reader.read_bendian_auto_t(p_out.m_hedr_2, p_abort);
					}
					break;
				};
			}

		}
		chapter_reader(fbh::StreamReaderMemblock * p_file) : itunesdb::reader(p_file) {};
	};


	class chapter_writer : public itunesdb::writer
	{
	public:
		void write_atom(t_uint32 type, t_uint32 data0, t_uint32 children_count, t_uint8* p_data, t_size p_data_size, abort_callback & p_abort)
		{
			write_bendian_auto_t(t_uint32(p_data_size + 20), p_abort);
			write_bendian_auto_t(type, p_abort);
			write_bendian_auto_t(data0, p_abort);
			write_bendian_auto_t(children_count, p_abort);
			write_bendian_auto_t(0, p_abort);
			m_file.write(p_data, p_data_size, p_abort);
		}
		void write_chapter(const chapter_entry & p_out, abort_callback & p_abort)
		{
			stream_writer_memblock_dop p_chapter_memwriter;
			chapter_writer p_chapter_writer(&p_chapter_memwriter);

			t_size child_count = 0;
			if (!p_out.m_title.is_empty())
			{
				stream_writer_memblock_dop p_name_data;
				p_name_data.write_string_utf16_sized_bendian_auto_t(p_out.m_title, pfc::downcast_guarded<t_uint16>(p_out.m_title.get_length()), p_abort);
				p_chapter_writer.write_atom('name', 1, 0, p_name_data.m_data.get_ptr(), p_name_data.m_data.get_size(), p_abort);
				++child_count;
			}
			if (!p_out.m_url_title.is_empty())
			{
				stream_writer_memblock_dop p_name_data;
				p_name_data.write_string_utf16_sized_bendian_auto_t(p_out.m_url_title, pfc::downcast_guarded<t_uint16>(p_out.m_url_title.get_length()), p_abort);
				p_chapter_writer.write_atom('urlt', 1, 0, p_name_data.m_data.get_ptr(), p_name_data.m_data.get_size(), p_abort);
				++child_count;
			}
			if (!p_out.m_url.is_empty())
			{
				stream_writer_memblock_dop p_name_data;
				p_name_data.write(p_out.m_url, p_out.m_url.length(), p_abort);
				p_chapter_writer.write_atom('url ', 1, 0, p_name_data.m_data.get_ptr(), p_name_data.m_data.get_size(), p_abort);
				++child_count;
			}
			if (p_out.m_ploc_valid)
			{
				stream_writer_memblock_dop p_name_data;
				p_name_data.write_bendian_auto_t(p_out.m_ploc1, p_abort);
				p_name_data.write_bendian_auto_t(p_out.m_ploc2, p_abort);
				p_name_data.write_bendian_auto_t(p_out.m_ploc3, p_abort);
				p_name_data.write_bendian_auto_t(p_out.m_ploc4, p_abort);
				p_chapter_writer.write_atom('ploc', 1, 0, p_name_data.m_data.get_ptr(), p_name_data.m_data.get_size(), p_abort);
				++child_count;
			}

			write_atom('chap', p_out.m_start_position, child_count, p_chapter_memwriter.m_data.get_ptr(), p_chapter_memwriter.m_data.get_size(), p_abort);
		}
		void write(const chapter_list & p_out, abort_callback & p_abort)
		{
			stream_writer_memblock_dop p_chapter_memwriter;
			chapter_writer p_chapter_writer(&p_chapter_memwriter);

			for (t_size i=0, count=p_out.get_count(); i<count; i++)
			{
				p_chapter_writer.write_chapter(p_out[i], p_abort);
			}

			stream_writer_memblock_dop p_hedr_memwriter;
			chapter_writer p_hedr_writer(&p_hedr_memwriter);
			p_hedr_writer.write_bendian_auto_t(p_out.m_hedr_1, p_abort);
			p_hedr_writer.write_bendian_auto_t(p_out.m_hedr_2, p_abort);
			p_chapter_writer.write_atom('hedr', 1, 0, p_hedr_memwriter.m_data.get_ptr(), p_hedr_memwriter.m_data.get_size(), p_abort);

			write_bendian_auto_t(t_uint32(0), p_abort);
			write_bendian_auto_t(t_uint32(0), p_abort);
			write_bendian_auto_t(t_uint32(0), p_abort);

			write_atom('sean', 0, p_out.get_count()+1, p_chapter_memwriter.m_data.get_ptr(), p_chapter_memwriter.m_data.get_size(), p_abort);
		}
		chapter_writer(stream_writer * p_file) : itunesdb::writer(*p_file) {};
	};
}