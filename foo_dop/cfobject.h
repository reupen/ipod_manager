#ifndef _DOP_CFOBJ_H_
#define _DOP_CFOBJ_H_

namespace cfobject
{
	enum objectType
	{
		kTagUnset,
		kTagBoolean,
		kTagInt,
		kTagReal,
		kTagDate,
		kTagData,
		kTagUnicodeString,
		kTagArray,
		kTagDictionary,
	};
	class object_t : public pfc::refcounted_object_root
	{
	public:
		typedef pfc::refcounted_object_ptr_t<object_t> ptr_t;

		class dictionary_entry_t
		{
		public:
			object_t::ptr_t m_key, m_value;

			static int g_compare (const dictionary_entry_t & i1, const dictionary_entry_t & i2)
			{
				return _wcsicmp (i1.m_key.is_valid() ? i1.m_key->m_string.get_ptr() : L"", i2.m_key.is_valid() ? i2.m_key->m_string.get_ptr() : L"");
			}
			static int g_compare_value (const dictionary_entry_t & i1, const wchar_t * i2)
			{
				return _wcsicmp (i1.m_key.is_valid() ? i1.m_key->m_string.get_ptr() : L"", i2);
			}
		};

		class dictionary : public pfc::list_t < class dictionary_entry_t >
		{
		public:

			bool get_child (const wchar_t * key, object_t::ptr_t & p_value);
			bool get_child (const wchar_t * key, pfc::string8 & p_value);
			bool get_child (const wchar_t * key, bool & p_value);
			bool get_child (const wchar_t * key, t_uint16 & p_value);
			bool get_child (const wchar_t * key, t_uint32 & p_value);
			bool get_child (const wchar_t * key, t_int32 & p_value);
			bool get_child (const wchar_t * key, t_uint64 & p_value);
			bool get_child (const wchar_t * key, t_int64 & p_value);

			dictionary() : m_sorted(false) {};
		private:
			void ensure_sorted();
			bool m_sorted;
		};

		t_size m_type;

		bool m_boolean;
		t_int64 m_integer;
		double m_float;
		pfc::string_simple_t<WCHAR> m_string;
		pfc::string_simple_t<WCHAR> m_key;
		pfc::list_t<object_t::ptr_t> m_array;
		dictionary m_dictionary;
		pfc::array_t<t_uint8> m_data;
		t_filetimestamp m_date;

		bool get_bool() {return m_boolean || m_integer;}
		t_int32 get_flat_int32() {return m_integer < 0 ? (t_int32)m_integer : (t_int32)(t_uint32)m_integer;}
		t_int32 get_flat_uint32() {return m_integer < 0 ? (t_uint32)(t_int32)m_integer : (t_uint32)m_integer;}
		t_int32 get_flat_uint16() {return m_integer < 0 ? (t_uint16)(t_int16)m_integer : (t_uint16)m_integer;}

		object_t() : m_type(kTagUnset), m_integer(NULL), m_float(NULL), m_boolean(false), m_date(filetimestamp_invalid) {};
	};


	typedef object_t::dictionary_entry_t dictionary_entry_t;

	void g_print_object(const object_t::ptr_t & ptr, pfc::string8 & p_out);
	void g_export_object_to_xml (const object_t::ptr_t & ptr, pfc::string8 & p_out);
}

#endif