#ifndef _DOP_BPLIST_H_
#define _DOP_BPLIST_H_

#include "cfobject.h"

namespace bplist
{
	const t_uint8 header_identifier[] = {'b','p','l','i','s','t','0','0'};

	struct footer_info
	{
		t_uint8				unused[6];
		t_uint8				offsetIntSize;
		t_uint8				objectRefSize;
		t_uint64			objectCount;
		t_uint64			topLevelObject;
		t_uint64			offsetTableOffset;
	};

	enum objectType
	{
		// Object tags (high nybble)
		kTagSimple			= 0x00,	// Null, true, false, filler, or invalid
		kTagInt				= 0x10,
		kTagReal			= 0x20,
		kTagDate			= 0x30,
		kTagData			= 0x40,
		kTagASCIIString		= 0x50,
		kTagUnicodeString	= 0x60,
		kTagUID				= 0x80,
		kTagArray			= 0xA0,
		kTagDictionary		= 0xD0,
		
		// "simple" object values
		kValueNull			= 0x00,
		kValueFalse			= 0x08,
		kValueTrue			= 0x09,
		kValueFiller		= 0x0F,
		
		kValueFullDateTag	= 0x33	// Dates are tagged with a whole byte.
	};

	typedef cfobject::dictionary_entry_t dictionary_entry_t;
	typedef cfobject::object_t object_t;

#if 0

	class object_t : public pfc::refcounted_object_root
	{
	public:
		typedef pfc::refcounted_object_ptr_t<object_t> ptr;
		t_size m_type;
		t_uint8 m_simple_value;
		t_int64 m_integer;
		double m_float;
		pfc::string_simple_t<WCHAR> m_string;
		pfc::list_t<object_t::ptr> m_array;
		pfc::list_t< class dictionary_entry_t > m_dictionary;

		object_t() : m_type(NULL), m_integer(NULL), m_float(NULL), m_simple_value(NULL) {};
	};

	class dictionary_entry_t
	{
	public:
		object_t::ptr m_key, m_value;
	};
#endif

	class reader
	{
	public:
		object_t::ptr_t m_root_object;
		reader(abort_callback & p_abort) : m_abort(p_abort) {};
		void read (service_ptr_t<file> & ptr)
		{
			m_data.set_size(pfc::downcast_guarded<t_size>(ptr->get_size_ex(m_abort)));
			ptr->read(m_data.get_ptr(), m_data.get_size(), m_abort);
			read();
		}
		void read (const char * path)
		{
			file::ptr ptr;
			filesystem::g_open(ptr, path, filesystem::open_mode_read, m_abort);
			read(ptr);
		}
		void read (const void * p_data, t_size size)
		{
			m_data.set_size(0);
			m_data.append_fromptr((t_uint8*)p_data, size);
			read();
		}

	private:
		void read ()
		{
			fbh::StreamReaderMemblock reader(m_data);

			t_uint8 header[8];
			if (reader.get_remaining() < 8)
				throw mmh::exception_wrong_format();
			reader.read(header, 8, m_abort);
			if (memcmp(header, header_identifier, 8))
				throw mmh::exception_wrong_format();
			reader.seek_ex(file::seek_from_eof, 0-sizeof(footer_info), m_abort);
			reader.read(&m_footer.unused, 6, m_abort);
			reader.read_bendian_t(m_footer.offsetIntSize, m_abort);
			reader.read_bendian_t(m_footer.objectRefSize, m_abort);
			reader.read_bendian_t(m_footer.objectCount, m_abort);
			reader.read_bendian_t(m_footer.topLevelObject, m_abort);
			reader.read_bendian_t(m_footer.offsetTableOffset, m_abort);

			m_objects.set_count(pfc::downcast_guarded<t_size>(m_footer.objectCount));

			get_object(m_footer.topLevelObject, m_root_object);
		}
		void get_object(t_uint64 index, object_t::ptr_t & p_out)
		{
			if (index >= m_objects.get_count())
				throw exception_io_unsupported_format();
			object_t::ptr_t & ptr = m_objects[pfc::downcast_guarded<t_size>(index)];
			if (!ptr.is_valid())
				read_object(index, ptr);
			p_out = ptr;
		}
		pfc::array_t<object_t::ptr_t> m_objects;
		t_uint64 read_offset(t_uint64 index)
		{
			fbh::StreamReaderMemblock reader(m_data);
			reader.seek_ex(file::seek_from_beginning, pfc::downcast_guarded<t_ssize>(m_footer.offsetTableOffset + m_footer.offsetIntSize * index), m_abort);
			return reader.read_sized_int_bendian(m_footer.offsetIntSize, m_abort);
		}
		void read_object(t_uint64 index, object_t::ptr_t & p_object)
		{
			object_t::ptr_t object = new object_t;
			fbh::StreamReaderMemblock reader(m_data);
			reader.seek_ex(file::seek_from_beginning, pfc::downcast_guarded<t_ssize>(read_offset(index)), m_abort);

			t_uint8 object_type;
			reader.read_lendian_t(object_type, m_abort);

			t_size size = (object_type & 0x0F);
			t_size size_shifted = 1<<size;

			switch ((object_type & 0xF0))
			{
				case kTagSimple:
					//reader.read_lendian_t(object->m_simple_value, m_abort);
					object->m_type = cfobject::kTagBoolean;
					object->m_boolean = (object_type & 0x0F) == kValueTrue;
					break;
				
				case kTagInt:
					object->m_type = cfobject::kTagInt;
					object->m_integer = reader.read_sized_int_bendian(size_shifted, m_abort);
					break;
					
				case kTagReal:
					object->m_type = cfobject::kTagUnset;
					break;
					
				case kTagDate:
					object->m_type = cfobject::kTagUnset;
					break;
					
				case kTagData:
					object->m_type = cfobject::kTagData;
					if (size == 0xf)
					{
						t_uint8 temp;
						reader.read_lendian_t(temp, m_abort);
						size = pfc::downcast_guarded<t_size>(reader.read_sized_int_bendian(1<<(temp&0x0f), m_abort));
					}
					object->m_data.set_size(size);
					reader.read(object->m_data.get_ptr(), size, m_abort);
					break;
					
				case kTagASCIIString:
					{
						object->m_type = cfobject::kTagUnicodeString;
						if (size == 0xf)
						{
							t_uint8 temp;
							reader.read_lendian_t(temp, m_abort);
							size = pfc::downcast_guarded<t_size>(reader.read_sized_int_bendian(1<<(temp&0x0f), m_abort));
						}
						pfc::array_t<wchar_t> string;
						string.set_size(size);
						t_size ptr = 0;
						while (ptr < size)
						{
							t_uint8 temp;
							reader.read_bendian_t(temp, m_abort);
							string[ptr]=temp;
							ptr++;
						}
						object->m_string.set_string(string.get_ptr(), string.get_size());
					}
					break;
					
				case kTagUnicodeString:
					{
						object->m_type = cfobject::kTagUnicodeString;
						if (size == 0xf)
						{
							t_uint8 temp;
							reader.read_lendian_t(temp, m_abort);
							size = pfc::downcast_guarded<t_size>(reader.read_sized_int_bendian(1<<(temp&0x0f), m_abort));
						}
						pfc::array_t<wchar_t> string;
						string.set_size(size);
						t_size ptr = 0;
						while (ptr < size)
						{
							reader.read_bendian_t(string[ptr], m_abort);
							ptr++;
						}
						object->m_string.set_string(string.get_ptr(), string.get_size());
					}
					break;
					
				case kTagUID:
					object->m_type = cfobject::kTagUnset;
					break;
					
				case kTagArray:
					{
						object->m_type = cfobject::kTagArray;
						if (size == 0xf)
						{
							t_uint8 temp;
							reader.read_lendian_t(temp, m_abort);
							size = pfc::downcast_guarded<t_size>(reader.read_sized_int_bendian(1<< (temp&0x0f), m_abort));
						}
						t_size i;
						object->m_array.set_count(size);
						for (i=0; i<size; i++)
						{
							t_uint64 indexchild = reader.read_sized_int_bendian(m_footer.objectRefSize, m_abort);
							get_object(indexchild, object->m_array[i]);
						}
					}
					break;
					
				case kTagDictionary:
					{
						object->m_type = cfobject::kTagDictionary;
						if (size == 0xf)
						{
							t_uint8 temp;
							reader.read_lendian_t(temp, m_abort);
							size = pfc::downcast_guarded<t_size>(reader.read_sized_int_bendian(1<<(temp&0x0f), m_abort));
						}
						t_size i;
						object->m_dictionary.set_count(size);
						for (i=0; i<size; i++)
						{
							t_uint64 indexchild = reader.read_sized_int_bendian(m_footer.objectRefSize, m_abort);
							get_object(indexchild, object->m_dictionary[i].m_key);
						}
						for (i=0; i<size; i++)
						{
							t_uint64 indexchild = reader.read_sized_int_bendian(m_footer.objectRefSize, m_abort);
							get_object(indexchild, object->m_dictionary[i].m_value);
						}
					}
					break;
					
				default:
					throw exception_io_data(pfc::string8 () << "Unknown bplist object type: " << object->m_type);
					break;
			}
			p_object = object;
		}
		pfc::array_t<t_uint8> m_data;
		footer_info m_footer;
		abort_callback & m_abort;
	};
};

#endif //_DOP_BPLIST_H_