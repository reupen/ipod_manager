#pragma once

//#define IPHONECALC_WHITELIST

class iPhoneCalc {
	typedef void (__cdecl * xgen_hash_ab_Proc)(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint8 * p_out);
	typedef void (__cdecl * xgen_hash_ab_gen_Proc)(const t_uint8 * sha1, const t_uint8 * p_udid, t_uint32 udid_length, t_uint8 * p_out);

public:
	void initialise()
	{
		if (m_library == NULL)
		{
			pfc::string8 fb2kpath;
			if (uGetModuleFileName(NULL, fb2kpath) && GetLastError() == ERROR_SUCCESS)
			{
				pfc::string8 path_iphonecalc;
				path_iphonecalc << pfc::string_directory(fb2kpath) << "\\" << "iPhoneCalc.dll";
				try 
				{
#ifdef IPHONECALC_WHITELIST
					file::ptr p_file;
					abort_callback_dummy p_abort;
					filesystem::g_open_read(p_file, path_iphonecalc, p_abort);
					t_size p_size = pfc::downcast_guarded<t_size>(p_file->get_size_ex(p_abort));
					pfc::array_staticsize_t<t_uint8> data(p_size);
					p_file->read(data.get_ptr(), data.get_size(), p_abort);
					p_file.release();
					t_uint8 digest[mmh::hash::sha1_digestsize] = {0};
					mmh::hash::sha1(data.get_ptr(), data.get_size(), digest);
					
					if (!memcmp("\x36\x41\xb5\x4e\x0e\xd9\x1c\x5b\xe8\x33\xb5\x07\x83\x8f\x99\x71\x6e\x28\x3d\xb4", digest, mmh::hash::sha1_digestsize)
					|| !memcmp("\x55\xae\x84\xfc\x03\x2d\x8f\xc7\x04\x1d\x79\x3e\xe0\xa8\xf8\x74\xe6\x5b\x65\xc0", digest, mmh::hash::sha1_digestsize))
#endif
					{
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
				catch (pfc::exception const &)
				{
				};
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