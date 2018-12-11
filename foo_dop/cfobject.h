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

		template <objectType TypeTag>
		auto& get_value_strict()
		{
			if (m_type != TypeTag) {
				throw exception_io_unsupported_format();
			}

			if constexpr (TypeTag == kTagInt)
				return m_integer;

			if constexpr (TypeTag == kTagArray)
				return m_array;

			if constexpr (TypeTag == kTagBoolean)
				return m_boolean;

			if constexpr (TypeTag == kTagDictionary)
				return m_dictionary;

			if constexpr (TypeTag == kTagUnicodeString)
				return m_string;

			if constexpr (TypeTag == kTagData)
				return m_data;

			if constexpr (TypeTag == kTagReal)
				return m_float;

			throw pfc::exception_bug_check();
		}

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

			template<objectType TypeTag>
			auto* get_child(const wchar_t* key)
			{
				object_t::ptr_t child;
				if (get_child(key, child))
					return &child->get_value_strict<TypeTag>();

				using ValuePtr = decltype(&child->get_value_strict<TypeTag>());
				return static_cast<ValuePtr>(nullptr);
			}

			/** Note: This will copy the value (unlike get_child above) */
			template<objectType TypeTag, class Default>
			auto get_child_strict_with_default(const wchar_t* key, Default&& default_value = {})
			{
				using Type = std::remove_reference_t<decltype(std::declval<object_t&>().get_value_strict<TypeTag>())>;
				std::optional<Type> default_value_as_optional{ std::forward<Default>(default_value) };
				auto child = get_child<TypeTag>(key);

				if (!child && !default_value_as_optional)
					throw exception_io_unsupported_format();

				if (!child)
					return default_value_as_optional.value();
				
				return *child;
			}

			template<objectType TypeTag>
			auto& get_child_strict(const wchar_t* key)
			{
				auto child = get_child<TypeTag>(key);
				if (!child)
					throw exception_io_unsupported_format();
				return *child;
			}

			const pfc::string_simple_t<wchar_t>& get_child_string_strict(const wchar_t * key);
			bool get_child_bool_strict(const wchar_t * key, std::optional<bool> default_value = {});
			int64_t get_child_int64_strict(const wchar_t * key, std::optional<int64_t> default_value = {});
			const std::vector<object_t::ptr_t>& get_child_array_strict(const wchar_t * key);

			dictionary() : m_sorted(false) {};
		private:
			void ensure_sorted();
			bool m_sorted;
		};

		t_size m_type;

		bool m_boolean;
		t_int64 m_integer;
		double m_float;
		pfc::string_simple_t<wchar_t> m_string;
		pfc::string_simple_t<wchar_t> m_key;
		std::vector<object_t::ptr_t> m_array;
		dictionary m_dictionary;
		pfc::array_t<std::uint8_t> m_data;
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