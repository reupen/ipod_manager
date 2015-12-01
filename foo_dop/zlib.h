#pragma once

#include "..\zlib-1.2.5\zlib.h"

#if 0
#define ZLGPA(x, y) _##x = (x##_Proc)GetProcAddress(y, #x)
#define SafeZLGPA(x, y) if (!(ZLGPA(x,y))) throw pfc::exception(pfc::string8() << "Failed to locate function " << #x)
#define ZLMP(x) x##_Proc _##x
//#define ZLMPI(x) x = NULL

typedef int (__cdecl * deflateInit__Proc)(z_streamp strm, int level, const char *version, int stream_size);
typedef int (__cdecl * inflateInit__Proc)(z_streamp strm, const char *version, int stream_size);
typedef int (__cdecl * deflate_Proc)(z_streamp strm, int flush);
typedef int (__cdecl * deflateEnd_Proc)(z_streamp strm);
typedef int (__cdecl * inflate_Proc)(z_streamp strm, int flush);
typedef int (__cdecl * inflateEnd_Proc)(z_streamp strm);

class zlib_handle
{
public:
	zlib_handle()
	{
		if (!(m_library = LoadLibrary(L"zlib1.dll")))
			throw pfc::exception(pfc::string8() << "Failed to load zlib1.dll - " << format_win32_error(GetLastError()));

		SafeZLGPA(deflateInit_, m_library);
		SafeZLGPA(inflateInit_, m_library);
		SafeZLGPA(inflate, m_library);
		SafeZLGPA(deflate, m_library);
		SafeZLGPA(deflateEnd, m_library);
		SafeZLGPA(inflateEnd, m_library);

	}
	~zlib_handle()
	{
		if (m_library)
			FreeLibrary(m_library);
	}
	ZLMP(deflateInit_);
	ZLMP(inflateInit_);
	ZLMP(deflate);
	ZLMP(inflate);
	ZLMP(deflateEnd);
	ZLMP(inflateEnd);
	int _deflateInit(z_streamp strm, int level);
	int _inflateInit(z_streamp strm);
	//bool valid() {return m_library != NULL;}
	//void ensure_valid() {if (!valid()) throw pfc::exception(pfc::string8() << "Failed to load zlib1.dll - " << format_win32_error(m_error));}
private:
	HINSTANCE m_library;
};
#endif

class zlib_stream
{
public:
	zlib_stream() 
	{
		memset(&m_zstream, 0, sizeof(m_zstream));
	}
	/** throws exception on error */
	void compress_singlerun(t_uint8 const * p_buffer, t_size const p_buffer_size,
		pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & m_output)
	{
		const t_size buffer_size = min(p_buffer_size, 1024*1024);

		pfc::array_staticsize_t<t_uint8> output_buffer(buffer_size);
		m_output.prealloc(p_buffer_size);

		int ret = Z_OK, flush = Z_NO_FLUSH;
		t_size have = 0;

		/* allocate deflate state */
		m_zstream.zalloc = Z_NULL;
		m_zstream.zfree = Z_NULL;
		m_zstream.opaque = Z_NULL;

		ret = deflateInit(&m_zstream, Z_BEST_SPEED);
		if (ret != Z_OK)
			throw pfc::exception("zlib: deflateInit error");

		/* compress until end of file */
		do {

			m_zstream.avail_in = p_buffer_size;
			flush = true ? Z_FINISH : Z_NO_FLUSH;
			m_zstream.next_in = const_cast<t_uint8*>(p_buffer);

			/* run deflate() on input until output buffer not full, finish
			compression if all of source has been read in */
			do {

				m_zstream.avail_out = buffer_size;
				m_zstream.next_out = output_buffer.get_ptr();

				ret = deflate(&m_zstream, flush);    /* no bad return value */
				//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

				have = buffer_size - m_zstream.avail_out;
				m_output.append_fromptr(output_buffer.get_ptr(), buffer_size-m_zstream.avail_out);

			} while (m_zstream.avail_out == 0);
			assert(m_zstream.avail_in == 0);     /* all input will be used */


			/* done when last data in file processed */
		} while (flush != Z_FINISH);
		//assert(ret == Z_STREAM_END);        /* stream will be complete */

		/* clean up and return */
		deflateEnd(&m_zstream);
	}
	/** throws exception on error */
	void decompress_singlerun(t_uint8 const * p_buffer, t_size const p_buffer_size, 
		pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> & m_output)
	{
		const t_size buffer_size = min(p_buffer_size, 1024*1024);

		pfc::array_staticsize_t<t_uint8> output_buffer(buffer_size);
		m_output.prealloc(p_buffer_size*2);

		int ret = Z_OK;
		t_size have = 0;

		/* allocate inflate state */
		m_zstream.zalloc = Z_NULL;
		m_zstream.zfree = Z_NULL;
		m_zstream.opaque = Z_NULL;
		m_zstream.avail_in = 0;
		m_zstream.next_in = Z_NULL;
		ret = inflateInit(&m_zstream);
		if (ret != Z_OK)
			throw pfc::exception("zlib: inflateInit error");

		/* decompress until deflate stream ends or end of file */
		do
		{
			m_zstream.avail_in = p_buffer_size;

			if (m_zstream.avail_in == 0)
				break;
			m_zstream.next_in = const_cast<t_uint8*>(p_buffer);

			/* run inflate() on input until output buffer not full */
			do {

				m_zstream.avail_out = buffer_size;
				m_zstream.next_out = output_buffer.get_ptr();

				ret = inflate(&m_zstream, Z_NO_FLUSH);
				//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				switch (ret)
				{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					{
						inflateEnd(&m_zstream);
						throw pfc::exception("zlib: inflate error");
					}
				}

				m_output.append_fromptr(output_buffer.get_ptr(), buffer_size-m_zstream.avail_out);
				have = p_buffer_size - m_zstream.avail_out;

			} while (m_zstream.avail_out == 0);


			/* done when inflate() says it's done */
		} while (ret != Z_STREAM_END);


		/* clean up and return */
		inflateEnd(&m_zstream);
	
	}

	//pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> m_output;
private:
	z_stream m_zstream;
	//zlib_handle m_zlib;
};
