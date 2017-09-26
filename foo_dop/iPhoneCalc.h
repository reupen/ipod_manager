#pragma once

class iPhoneCalc {
	typedef void (__cdecl * xgen_hash_ab_Proc)(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint8 * p_out);
	typedef void (__cdecl * xgen_hash_ab_gen_Proc)(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint32 udid_length, t_uint8 * p_out);

public:
	void initialise()
	{
		if (m_library == NULL)
		{
			pfc::string8 current_module_path;
			if (uGetModuleFileName(mmh::get_current_instance(), current_module_path) && GetLastError() == ERROR_SUCCESS)
			{
				pfc::string8 path_iphonecalc;
				path_iphonecalc << pfc::string_directory(current_module_path) << "\\" << "iPhoneCalc.dll";
				m_library = LoadLibraryEx(pfc::stringcvt::string_wide_from_utf8(path_iphonecalc).get_ptr(), NULL, NULL);
				if (m_library != NULL)
				{
					m_xgen_hash_ab = (xgen_hash_ab_Proc)GetProcAddress(m_library, "xgen_hash_ab"); 
					m_xgen_hash_ab_gen = (xgen_hash_ab_gen_Proc)GetProcAddress(m_library, "xgen_hash_ab_gen"); 
					if (m_xgen_hash_ab == NULL && m_xgen_hash_ab_gen == NULL)
					{
						release();
					}
				}
			}
		}
	}
	void release() 
	{
		if (m_library != NULL)
		{
			FreeLibrary(m_library);
			m_library = NULL;
		}
		m_xgen_hash_ab = NULL;
		m_xgen_hash_ab_gen = NULL;
	}
	bool is_valid() {return m_library && m_xgen_hash_ab;}
	bool is_valid_gen() {return m_library && m_xgen_hash_ab_gen;}
	void xgen_hash_ab(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint8 * p_out) {m_xgen_hash_ab(sha1, p_udid, p_out);}
	void xgen_hash_ab_gen(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint32 udid_length, t_uint8 * p_out) {m_xgen_hash_ab_gen(sha1, p_udid, udid_length, p_out);}
	~iPhoneCalc() {release();}
	iPhoneCalc() : m_library(NULL), m_xgen_hash_ab(NULL), m_xgen_hash_ab_gen(NULL) {};
private:
	HINSTANCE m_library;
	xgen_hash_ab_Proc m_xgen_hash_ab;
	xgen_hash_ab_gen_Proc m_xgen_hash_ab_gen;
};