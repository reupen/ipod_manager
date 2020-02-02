#include "stdafx.h"

#include "vendored/mp3_utils.h"

struct mpeg_header
{
	t_uint8 mpeg_version; // 0=2, 1=1
	t_uint16 layer; //1=3, 2=2, 3=1
	t_uint32 bitrate;
	t_uint16 samplerate;
	t_uint8 padding;
	t_uint32 calc_size()
	{
		t_uint32 ret = 144 * bitrate * 1000 / samplerate + padding;
		return ret;
	}
};

void read_mpeg_header (service_ptr_t<file> & file, mpeg_header & out, abort_callback & p_abort)
{
	//const t_uin32 syncmask = (1<<0)|(1<<0)|(1<<0)|(1<<0)|(1<<0)|(1<<0)|(1<<0)|(1<<0)|(1<<0)
	t_uint32 header;
	file->read_bendian_t(header, p_abort);
	if ( (header & 0xfff00000) != 0xfff00000) throw exception_io_unsupported_format();
	out.mpeg_version = (header & (1<<19)) >> 19;
	if (!out.mpeg_version) throw exception_io_unsupported_format();
	out.layer = (header & ((1<<18)|(1<<17))) >> 17;
	if (out.layer != 1) throw exception_io_unsupported_format();
	//t_uint32 nool = (1<<15)|(1<<14)|(1<<13)|(1<<12);
	//t_uint32 numpty = (header & nool);
	t_uint16 bitrate = ((header & ((1<<15)|(1<<14)|(1<<13)|(1<<12))) >> 12);
	if (bitrate ==0) throw exception_io_unsupported_format();
	else if (bitrate <=5) out.bitrate = bitrate*8 + 24;
	else if (bitrate <=9) out.bitrate = 5*8 + (bitrate-5) * 16 + 24;
	else if (bitrate <=13) out.bitrate = 5*8 + (9-5)*16 + (bitrate-9) * 32 + 24;
	else if (bitrate ==14) out.bitrate = 320;
	else throw exception_io_unsupported_format();
	t_uint16 sr = (header & ((1<<11)|(1<<10))) >> 10;
	if (sr==0) out.samplerate = 44100;
	else if (sr==1) out.samplerate = 48000;
	else if (sr==2) out.samplerate = 32000;
	else throw exception_io_unsupported_format();
	out.padding = (header & (1<<9)) >> 9;
}

t_uint32 g_get_gapless_sync_frame_mp3(service_ptr_t<file> p_file, abort_callback & p_abort)
{
	pfc::list_t<t_filesize> offsets;
	t_filesize skipped=0;
	try 
	{
		tag_processor_id3v2::g_skip(p_file, skipped, p_abort);

		while (!p_file->is_eof(p_abort))
		{
			t_filesize start = p_file->get_position(p_abort);
			mpeg_header header;
			read_mpeg_header(p_file, header, p_abort);
			p_file->skip_object(header.calc_size()-4, p_abort);
			offsets.add_item(start);
		}
	}
	catch (const pfc::exception &) {};
	return offsets.get_count() > 8 ? (t_uint32)(offsets[offsets.get_count()-8] -skipped) : 0;
}

t_filesize g_skip_null(service_ptr_t<file> & p_file, abort_callback & p_abort)
{
	t_filesize ret=0;
	t_uint8 byte = 0;
	bool bread=false;
	do
	{
		p_file->read_lendian_t(byte, p_abort);
		bread=true;
		if (byte==0) ret++;
	}
	while (byte == 0);
	if (bread)
		p_file->seek_ex(-1, file::seek_from_current, p_abort);
	return ret;
}

t_filesize g_search_mpeg_frame(service_ptr_t<file> & p_file, abort_callback & p_abort)
{
	t_filesize ret=0;
	t_uint16 bytes = 0;
	bool bread=false;
	do
	{
		p_file->read_lendian_t(bytes, p_abort);
		ret+=2;
		bread=true;
		p_file->seek_ex(-1, file::seek_from_current, p_abort);
		ret--;
	}
	while ((bytes & 0x70ff) != 0x70ff);
	if (bread)
	{
		p_file->seek_ex(-1, file::seek_from_current, p_abort);
		ret--;
	}
	return ret;
}

t_filesize g_get_gapless_sync_frame_mp3_v2(service_ptr_t<file> p_file, abort_callback & p_abort)
{
	pfc::list_t<t_filesize> offsets;
	t_filesize skippedid3=0, skipped=0;
	//double scanned_length = 0;
	try 
	{
		//skipped += g_skip_null(p_file, p_abort);
		tag_processor_id3v2::g_skip(p_file, skippedid3, p_abort);
		skipped += skippedid3;
		skipped += g_search_mpeg_frame(p_file, p_abort);

		skipped = p_file->get_position(p_abort);

		while (!p_file->is_eof(p_abort))
		{
			t_filesize start = p_file->get_position(p_abort);
			t_uint8 header[4];
			p_file->read(header, sizeof(header), p_abort);
			mp3_utils::TMPEGFrameInfo headerinfo;
			bool valid = mp3_utils::ParseMPEGFrameHeader(headerinfo, header);
			if (!valid || headerinfo.m_bytes < 4) break;
			p_file->skip_object(headerinfo.m_bytes-4, p_abort);
			offsets.add_item(start);
			//scanned_length += audio_math::samples_to_time(headerinfo.m_duration, headerinfo.m_sample_rate);
		}
	}
	catch (const pfc::exception &) {};
	return offsets.get_count() > 8 ? (offsets[offsets.get_count()-8] -skipped) : 0;
}

t_filesize g_get_gapless_sync_frame_mp3(const char * path, abort_callback & p_abort)
{
	service_ptr_t<file> file;
	filesystem::g_open_read(file, path, p_abort);
	return g_get_gapless_sync_frame_mp3(file, p_abort);
}

t_filesize g_get_gapless_sync_frame_mp3_v2(const char * path, abort_callback & p_abort)
{
	//try
	//{
	service_ptr_t<file> file, fileCached;
	filesystem::g_open_read(file, path, p_abort);
	file_cached::g_create(fileCached, file, p_abort, 8*1024*1024);
	return g_get_gapless_sync_frame_mp3_v2(fileCached, p_abort);
	//} catch (const pfc::exception &) {return 0;}
}