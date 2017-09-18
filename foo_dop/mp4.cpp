#include "main.h"

#define box(a,b,c,d) \
	((#@d)|(#@c)<<8|(#@b)<<16|(#@a)<<24)

#define decbox(a,b,c,d) \
	const t_uint32 a##b##c##d = box(a,b,c,d)

decbox(m,o,o,v);
decbox(t,r,a,k);
decbox(m,d,i,a);
decbox(m,i,n,f);
decbox(v,m,h,d);
decbox(s,m,h,d);
decbox(s,t,b,l);
decbox(s,t,s,d);
decbox(e,s,d,s);
decbox(m,p,4,a);
decbox(s,t,t,s);

decbox(u,d,t,a);
decbox(m,e,t,a);
decbox(i,l,s,t);
decbox(c,h,p,l);

class t_box 
{
public:
	t_box() : size32(0), type(0), size(0), version(0), flags(0) {};
	t_uint32 size32;
	t_uint32 type;
	t_uint64 size;
	
	//FullBox
	t_uint8 version;
	t_uint32 flags;

	pfc::array_t<t_uint8> m_data;

	void readbox(stream_reader * p_file, t_filesize p_sizehint, abort_callback & p_abort)
	{
		p_file->read_bendian_t(size32, p_abort);
		p_file->read_bendian_t(type, p_abort);
		if (size32 == 1)
			p_file->read_bendian_t(size, p_abort);
		else if (size32)
			size = size32;
		else size = p_sizehint;
	}
	void readbox(service_ptr_t<file> & p_file, abort_callback & p_abort)
	{
		readbox(p_file.get_ptr(), p_file->get_remaining(p_abort), p_abort);
	}
	void readfullbox(stream_reader * p_file, abort_callback & p_abort)
	{
		t_uint32 data;
		p_file->read_bendian_t(data, p_abort);
		version = (data & 0xff000000) >> 24;
		flags = (data & 0x00ffffff);
		//t_box::read(p_file, p_abort);
		//p_file->skip(4, p_abort);
	}
	void readfullbox(file::ptr & p_file, abort_callback & p_abort)
	{
		readfullbox(p_file.get_ptr(), p_abort);
	}
	void skip (service_ptr_t<file> & p_file, abort_callback & p_abort)
	{
		p_file->skip_object(size - sizeof(t_uint32)*2 - (size32==1 ? sizeof(t_uint64) : 0), p_abort);
	}
	t_filesize get_data_size()
	{
		return size - sizeof(t_uint32)*2 - (size32==1 ? sizeof(t_uint64) : 0);
	}
	t_filesize get_size()
	{
		return size;
	}
	//void readboxfull(service_ptr_t<file> & p_file, abort_callback & p_abort)
	//{
	//	readbox(p_file, p_abort);
	//	p_file->skip(4, p_abort);
	//}
	void readdata(stream_reader * p_file, abort_callback & p_abort)
	{
		m_data.set_size(pfc::downcast_guarded<t_size>(get_data_size()));
		p_file->read(m_data.get_ptr(), m_data.get_size(), p_abort);
	}
};


class t_box2 : public t_box
{
public:
	pfc::array_t<t_uint8> m_data;

	void readdata(stream_reader * p_file, abort_callback & p_abort)
	{
		m_data.set_size(pfc::downcast_guarded<t_size>(get_data_size()));
		p_file->read(m_data.get_ptr(), m_data.get_size(), p_abort);
	}
	void readbox(stream_reader * p_file, t_filesize p_sizehint, abort_callback & p_abort)
	{
		t_box::readbox(p_file, p_sizehint, p_abort);
		readdata(p_file, p_abort);
	}
};

class mp4_box : public t_box
{
public:
	pfc::array_t<t_uint8> m_data;

	void readdata(stream_reader * p_file, t_filesize p_sizehint, abort_callback & p_abort)
	{
		m_data.set_size(pfc::downcast_guarded<t_size>(get_data_size()));
		p_file->read(m_data.get_ptr(), m_data.get_size(), p_abort);
	}
};

class mp4_reader
{
public:
	t_box m_box;
	t_size m_box_data_read;
	//mp4_box m_box;
	//mp4_reader() : m_box_position_start(0) {};

	void readfullbox(t_uint8 max_version = 0)
	{
		m_box.readfullbox(&m_stream, m_abort);
		m_box_data_read += 4;
		if (m_box.version > max_version) throw exception_io_data("Unsupported MP4 atom version");
	}
	void skip (t_filesize bytes)
	{
		m_stream.skip_object(bytes, m_abort);
		m_box_data_read += pfc::downcast_guarded<t_size>(bytes);
	}
	t_size get_remaining_data_size() 
	{
		return pfc::downcast_guarded<t_size>(m_box.get_data_size()) - m_box_data_read;
	}

	void readbox()
	{
		m_box_data_read = 0;
		m_box.readbox(&m_stream, m_stream.get_remaining(), m_abort);
	}
	void search_box(t_uint32 type, bool b_cache_data = false)
	{
		do
		{
			readbox();
			if (m_box.type != type)
				m_stream.skip_object(m_box.get_data_size(), m_abort);
			else
			{
				if (b_cache_data)
				{
					m_box.readdata(&m_stream, m_abort);
					m_data_ref_stream.set_data(m_box.m_data.get_ptr(), m_box.m_data.get_size());
				}
				break;
			}
		}
		while (m_box.size32 && m_stream.get_remaining());

		if (m_box.type != type) throw pfc::exception("Could not find box");
	}
	t_uint32 box_type() {return m_box.type;}
	bool next_box()
	{
		if (m_stream.get_remaining() < 8) return false;
		readbox();
		return true;
	}
	void skip_box()
	{
		m_stream.skip_object(m_box.get_data_size(), m_abort);
	}
	void read_string(pfc::string8 & p_out, t_size len)
	{
		//t_size len = m_box.get_data_size();//pfc::downcast_guarded<t_size>(m_stream.get_remaining());
		read(pfc::string_buffer(p_out, len), len);
	}
	template<typename T> 
	void read_bendian_t(T& p_object) 
	{
		m_stream.read_bendian_t(p_object, m_abort);
		m_box_data_read += sizeof(T);
	}

	t_size read(void * p_buffer,t_size p_bytes)
	{
		t_size ret = m_stream.read(p_buffer, p_bytes, m_abort);
		m_box_data_read += ret;
		return ret;
	}

	void seek_end() 
	{
		skip(get_remaining_data_size());
	}

	mp4_reader(file::ptr & p_file, abort_callback & p_abort) : m_stream(p_file.get_ptr(), p_file->get_remaining(p_abort)), m_abort(p_abort), m_box_data_read(0) {}; 
	mp4_reader(stream_reader * p_stream, t_filesize p_size, abort_callback & p_abort) : m_stream(p_stream, p_size), m_abort(p_abort), m_box_data_read(0) {}; 
	mp4_reader(mp4_reader & p_reader) : m_stream(0,0), m_abort(p_reader.m_abort), m_box_data_read(0)
	{
		t_uint8 * p_buffer = NULL;
		t_size size = pfc::downcast_guarded<t_size>(p_reader.get_remaining_data_size());//(p_reader.m_data_ref_stream.get_remaining());//(p_reader.get_remaining_data_size());//(p_reader.m_box.get_data_size());
		p_reader.m_data_ref_stream.read_nobuffer(p_buffer, size, m_abort);
		p_reader.m_stream.set_data(&p_reader.m_data_ref_stream, p_reader.m_data_ref_stream.get_remaining());
		m_data_ref_stream.set_data(p_buffer, size);
		m_stream.set_data(&m_data_ref_stream, size);
	};

private:
	fbh::StreamReaderMemblock m_data_ref_stream;
	abort_callback & m_abort;
	fbh::StreamReaderLimiter m_stream;
};

bool g_check_mp4_type(const char * path)
{
	return g_check_mp4_type(path, abort_callback_impl());
}

bool g_check_mp4_type(const char * path, abort_callback & p_abort)
{
	service_ptr_t<file> file;
	filesystem::g_open_read(file, path, p_abort);
	return g_check_mp4_type(file, p_abort);
}

bool g_get_gapless_mp4_apple_v2(service_ptr_t<file> p_file, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort)
{
	bool ret = false;

	bool b_delay = false, b_padding = false;

	try
	{
		mp4_reader boxroot(p_file, p_abort);
		boxroot.search_box('moov', true);

		mp4_reader boxmoov(boxroot);
		boxmoov.search_box('udta');

		mp4_reader boxudta(boxmoov);
		boxudta.search_box('meta');
		boxudta.readfullbox();

		mp4_reader boxmeta(boxudta);
		boxmeta.search_box('ilst');

		mp4_reader boxilst(boxmeta);
		boxilst.search_box('----');
		
		pfc::string8 mean, name, data;
		bool mean_valid=false, name_valid=false, data_valid=false;
		
		mp4_reader hyphenreader(boxilst);
		
		while (hyphenreader.next_box())
		{
			switch (hyphenreader.m_box.type)
			{
			case 'mean':
				{
					hyphenreader.readfullbox();
					hyphenreader.read_string(mean, pfc::downcast_guarded<t_size>(hyphenreader.m_box.get_data_size()-4));
					mean_valid = true;
				}
				break;
			case 'name':
				{
					hyphenreader.readfullbox();
					hyphenreader.read_string(name, pfc::downcast_guarded<t_size>(hyphenreader.m_box.get_data_size()-4));
					name_valid = true;
				}
				break;
			case 'data':
				{
					hyphenreader.readfullbox();
					hyphenreader.skip(4);
					hyphenreader.read_string(data, pfc::downcast_guarded<t_size>(hyphenreader.m_box.get_data_size()-8));
					data_valid = true;
				}
				break;
			default:
				hyphenreader.skip_box();
				break;
			};
		};

		if (mean_valid && data_valid && name_valid && !stricmp_utf8(name, "iTunSMPB"))
		{
			const char * ptrstart = data.get_ptr(), *ptr = ptrstart;

			while (*ptr == ' ') ptr++;

			ptrstart = ptr;
			while (*ptr && *ptr != ' ') ptr++;
			t_uint32 val = mmh::strtoul_n(ptrstart, t_size(ptr-ptrstart), 0x10);

			while (*ptr == ' ') ptr++;

			ptrstart = ptr;
			while (*ptr && *ptr != ' ') ptr++;
			b_delay = ptr>ptrstart;
			delay = mmh::strtoul_n(ptrstart, t_size(ptr-ptrstart), 0x10);

			while (*ptr == ' ') ptr++;

			ptrstart = ptr;
			while (*ptr && *ptr != ' ') ptr++;
			b_padding = ptr>ptrstart;
			padding = mmh::strtoul_n(ptrstart, t_size(ptr-ptrstart), 0x10);

			ret = b_padding && b_delay;
			return ret;
		}
	}
	catch (const pfc::exception &) {return false;}
	return ret;
}



bool g_get_gapless_mp4_nero(service_ptr_t<file> p_file, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort);
bool g_get_gapless_mp4_nero_v2(service_ptr_t<file> p_file, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort);

t_uint32 g_read_mp4_descr_length_v2(mp4_reader & p_reader)
{
    t_uint8 b = 0;
    t_uint8 numBytes = 0;
    t_uint32 length = 0;

    do
    {
		p_reader.read_bendian_t(b);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}

t_uint32 g_read_mp4_descr_length(stream_reader * p_file, abort_callback & p_abort)
{
    t_uint8 b = 0;
    t_uint8 numBytes = 0;
    t_uint32 length = 0;

    do
    {
		p_file->read_lendian_t(b, p_abort);
        numBytes++;
        length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && numBytes < 4);

    return length;
}

t_uint32 g_read_mp4_descr_length(service_ptr_t<file> p_file, abort_callback & p_abort)
{
	return g_read_mp4_descr_length(p_file.get_ptr(), p_abort);
}

t_uint32 g_convert_samplerate(t_uint32 val)
{
	switch (val)
	{
	case 0: return 96000;
	case 1: return 88200;
	case 2: return 64000;
	case 3: return 48000;
	case 4: return 44100;
	case 5: return 32000;
	case 6: return 24000;
	case 7: return 22050;
	case 8: return 16000;
	case 9: return 12000;
	case 10: return 11025;
	case 11: return 8000;
	case 12: return 7350;
	}
	return 0;
}

class mp4a_entry 
{
public:
	t_uint32 m_samplerate;
	bool m_samplerate_valid;
	bool m_frame_960;

	mp4a_entry() : m_samplerate(0), m_samplerate_valid(0), m_frame_960(false) {};
};

void g_read_atom_mp4a(mp4_reader & boxstsd, mp4a_entry & p_out)
{
	boxstsd.skip(28);

	mp4_reader boxmp4a(boxstsd);
	boxmp4a.search_box('esds');

	boxmp4a.skip(4);

	t_uint8 tag, tab;
	boxmp4a.read_bendian_t(tag);
	if (tag == 0x3)
	{
		g_read_mp4_descr_length_v2(boxmp4a);
		boxmp4a.skip(3);
	}
	else
		boxmp4a.skip(2);
	boxmp4a.read_bendian_t(tab);
	if (tab != 0x4)
		throw exception_io_unsupported_format();
	g_read_mp4_descr_length_v2(boxmp4a);
	boxmp4a.skip(1+4+4+4);
	boxmp4a.read_bendian_t(tab);
	if (tab != 0x5)
		throw exception_io_unsupported_format();
	t_uint32 len = g_read_mp4_descr_length_v2(boxmp4a);
	pfc::array_t<t_uint8> buff;
	buff.set_size(len);
	boxmp4a.read(buff.get_ptr(), len);
	t_uint8 * ptr = buff.get_ptr();
	t_uint32 freq;
	if (len >=2)
		freq = (((ptr[0] & ((1<<0)|(1<<1)|(1<<2))) << 1) | ((ptr[1] & (1<<7)) >>7) );
	else throw exception_io_unsupported_format();
	if (freq == 15)
	{
		if (len < 2 + 3)
			throw exception_io_unsupported_format();
		p_out.m_frame_960 = (ptr[4] & 1<<2) != 0;
		p_out.m_samplerate = (ptr[1] << 17) | (ptr[2] << 9) | (ptr[3] << 1) | ( (ptr[4] & (1<<7)) >>7); //works?
	}
	else
	{
		p_out.m_samplerate = g_convert_samplerate(freq);
		p_out.m_frame_960 = (ptr[1] & 1<<2) != 0;
	}
	p_out.m_samplerate_valid = true;

}

bool g_get_gapless_mp4_nero_v2(service_ptr_t<file> p_file, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort)
{
	bool ret = false;
	bool b_delay = false, b_padding = false;
	t_uint64 stts_total = 0;
	bool b_stts_total_valid = false;
	bool b_time_start_valid = false;
	t_uint64 time_start = 0;

	mp4a_entry mp4a;

	try
	{

		mp4_reader boxroot(p_file, p_abort);
		boxroot.search_box('moov', true);

		mp4_reader boxmoov(boxroot);
		while (boxmoov.next_box())
		{
			switch (boxmoov.m_box.type)
			{
			case 'trak':
				{
					mp4_reader boxtrak(boxmoov);
					boxtrak.search_box('mdia');

					mp4_reader boxmdia(boxtrak);
					boxmdia.search_box('minf');

					mp4_reader boxminf(boxmdia);
					boxminf.search_box('stbl');

					mp4_reader boxstbl(boxminf);

					while (boxstbl.next_box())
					{
						switch (boxstbl.m_box.type)
						{
						case 'stsd':
							{
								boxstbl.readfullbox();
								t_uint32 entry_count = 0;
								boxstbl.read_bendian_t(entry_count);
								//for (i=0; i<entry_count; i++)
								{

									mp4_reader boxstsd(boxstbl);
									boxstsd.search_box('mp4a');

									g_read_atom_mp4a(boxstsd, mp4a);
								}
							}
							break;
						case 'stts':
							{
								boxstbl.readfullbox();
								t_uint32 entry_count = 0, i;
								boxstbl.read_bendian_t(entry_count);
								for (i=0; i<entry_count; i++)
								{
									t_uint32 frame_num, frame_size;
									boxstbl.read_bendian_t(frame_num);
									boxstbl.read_bendian_t(frame_size);
									stts_total += frame_num * frame_size;
									b_stts_total_valid = true;
								}
							}
							break;
						default:
							boxstbl.skip_box();
							break;
						}
					};
				}
				break;
			case 'udta':
				{
					mp4_reader boxudta(boxmoov);
					boxudta.search_box('chpl');

					boxudta.skip(1+4);
					t_uint32 entry_count;
					boxudta.read_bendian_t(entry_count);
					if (entry_count)
						boxudta.read_bendian_t(time_start);
					b_time_start_valid = true;
				}
				break;
			default:
				boxmoov.skip_box();
				break;
			};
		};

	}
	catch (const pfc::exception &) {return false;}

	if (!b_time_start_valid || ! b_stts_total_valid || !mp4a.m_samplerate_valid)
		return false;
	delay = pfc::rint32(time_start * mp4a.m_samplerate / (double)(1000 * 10000));
	t_uint32 frame_size = mp4a.m_frame_960 ? 960 : 1024;
	padding = stts_total % (frame_size);
	if (padding) padding = frame_size - padding;
	return true;
}

class stts_entry
{
public:
	t_uint32 sample_count, sample_delta;

	stts_entry() : sample_count(0), sample_delta(0) {};
};
class stsz_t
{
public:
	t_uint32 sample_size, sample_count;
	pfc::array_t<t_uint32> sample_size_list;

	t_size get_count() {return sample_count;}

	t_uint32 operator [](t_size index) {return !sample_size && index < sample_size_list.get_count() ? sample_size_list[index] : sample_size;}
};

class stsc_entry
{
public:
	t_uint32 first_chunk, samples_per_chunk, sample_description_index;
};

class trak_t
{
public:
	stsz_t m_stsz;
	pfc::array_t<stts_entry> m_stts_entries;
	pfc::array_t<stsc_entry> m_stsc_entries;
	pfc::array_t<t_uint64> m_stco_entries;
	pfc::array_t<t_uint32> m_chap_ref_entries;
	mp4a_entry m_mp4a_entry;
	t_uint32 m_track_id, m_timescale;
	bool m_tx3g, m_jpeg;
	t_uint32 m_handler_type;

	trak_t() : m_tx3g(false),m_track_id(0),m_timescale(600),m_jpeg(false),m_handler_type(0) {};
};

class trak_list : public pfc::list_t<trak_t>
{
public:
	static int g_compare_track_id_value(const trak_t & p1, t_uint32 p2)
	{
		return pfc::compare_t(p1.m_track_id, p2);
	}
	bool find_by_track_id(t_uint32 track_id, t_size & index)
	{
		bool b_ret = false;
		for (t_size i = 0, count = get_count(); i<count; i++)
		{
			if (get_item(i).m_track_id == track_id)
			{
				b_ret = true;
				index = i;
				break;
			}
		}
		return b_ret;
	}
};

class mp4_info
{
public:
	trak_list trak_entries;
};

void g_get_mp4_info(service_ptr_t<file> p_file, mp4_info & p_info /*out*/, abort_callback & p_abort)
{

	mp4_reader boxroot(p_file, p_abort);
	boxroot.search_box('moov', true);


	trak_list & trak_entries = p_info.trak_entries;

	mp4_reader boxmoov(boxroot);
	while (boxmoov.next_box())
	{
		switch (boxmoov.m_box.type)
		{
		case 'trak':
			{
				trak_t trak_entry;

				mp4_reader boxtrak(boxmoov);
				while (boxtrak.next_box())
				{
					switch (boxtrak.box_type())
					{
					case 'tkhd':
						{
							boxtrak.readfullbox(1);
							if (boxtrak.m_box.version >= 1)
								boxtrak.skip(8 + 8);
							else
								boxtrak.skip(4 + 4);
							boxtrak.read_bendian_t(trak_entry.m_track_id);
							boxtrak.seek_end();
						};
						break;
					case 'mdia':
						{
							mp4_reader boxmdia(boxtrak);
							while (boxmdia.next_box())
							{
								switch (boxmdia.box_type())
								{
								case 'hdlr':
									{
										boxmdia.readfullbox(0);
										boxmdia.skip(4);
										boxmdia.read_bendian_t(trak_entry.m_handler_type);
										boxmdia.seek_end();
									};
									break;
								case 'mdhd':
									{
										boxmdia.readfullbox(1);
										if (boxmdia.m_box.version >= 1)
											boxmdia.skip(8 + 8);
										else
											boxmdia.skip(4 + 4);
										boxmdia.read_bendian_t(trak_entry.m_timescale);
										boxmdia.seek_end();
									}
									break;
								case 'minf':
									{

										mp4_reader boxminf(boxmdia);
										boxminf.search_box('stbl');

										mp4_reader boxstbl(boxminf);

										while (boxstbl.next_box())
										{
											switch (boxstbl.m_box.type)
											{
											case 'stsd':
												{
													boxstbl.readfullbox();
													t_uint32 entry_count = 0;
													boxstbl.read_bendian_t(entry_count);
													{
														mp4_reader boxstsd(boxstbl);

														while (boxstsd.next_box())
														{
															switch (boxstsd.m_box.type)
															{
															case 'tx3g':
																trak_entry.m_tx3g = true;
																boxstsd.seek_end();
																break;
															case 'jpeg':
																trak_entry.m_jpeg = true;
																boxstsd.seek_end();
																break;
															case 'mp4a':
																g_read_atom_mp4a(boxstsd, trak_entry.m_mp4a_entry);
																boxstsd.seek_end();
																break;
															default:
																boxstsd.skip_box();
																break;
															};
														};
													}
												}
												break;
											case 'stts':
												{
													boxstbl.readfullbox();
													t_uint32 entry_count = 0, i;
													boxstbl.read_bendian_t(entry_count);
													trak_entry.m_stts_entries.set_size(entry_count);
													for (i=0; i<entry_count; i++)
													{
														boxstbl.read_bendian_t(trak_entry.m_stts_entries[i].sample_count);
														boxstbl.read_bendian_t(trak_entry.m_stts_entries[i].sample_delta);
													}
												}
												break;
											case 'stsz':
												boxstbl.readfullbox();
												boxstbl.read_bendian_t(trak_entry.m_stsz.sample_size);
												boxstbl.read_bendian_t(trak_entry.m_stsz.sample_count);
												if (trak_entry.m_stsz.sample_size == 0)
												{
													t_uint32 i;
													trak_entry.m_stsz.sample_size_list.set_size(trak_entry.m_stsz.sample_count);
													for (i=0; i<trak_entry.m_stsz.sample_count; i++)
														boxstbl.read_bendian_t(trak_entry.m_stsz.sample_size_list[i]);
												}
												break;
											case 'stz2':
												{
													boxstbl.readfullbox();
													trak_entry.m_stsz.sample_size = 0;
													t_uint32 field_size;
													boxstbl.read_bendian_t(field_size);
													field_size = (field_size&0xFF);

													t_uint32 entry_count = 0, i;
													boxstbl.read_bendian_t(entry_count);
													trak_entry.m_stsz.sample_count = entry_count;
													trak_entry.m_stsz.sample_size_list.set_size(entry_count);
													for (i=0; i<entry_count; i++)
													{
														switch (field_size)
														{
														case 4:
															{
																t_uint8 value;
																boxstbl.read_bendian_t(value);
																trak_entry.m_stsz.sample_size_list[i] = (value & 0x7);
																if (i+1 < entry_count)
																	trak_entry.m_stsz.sample_size_list[i] = (value>>4);
																i++;
															}
															break;
														case 8:
															{
																t_uint8 value;
																boxstbl.read_bendian_t(value);
																trak_entry.m_stsz.sample_size_list[i] = (value);
															}
															break;
														case 16:
															{
																t_uint16 value;
																boxstbl.read_bendian_t(value);
																trak_entry.m_stsz.sample_size_list[i] = (value);
															}
															break;
														};
													}
												}
												break;
											case 'stsc':
												{
													boxstbl.readfullbox();
													t_uint32 entry_count = 0, i;
													boxstbl.read_bendian_t(entry_count);
													trak_entry.m_stsc_entries.set_size(entry_count);
													for (i=0; i<entry_count; i++)
													{
														boxstbl.read_bendian_t(trak_entry.m_stsc_entries[i].first_chunk);
														boxstbl.read_bendian_t(trak_entry.m_stsc_entries[i].samples_per_chunk);
														boxstbl.read_bendian_t(trak_entry.m_stsc_entries[i].sample_description_index);
													}
												}
												break;
											case 'stco':
											case 'co64':
												{
													boxstbl.readfullbox();
													t_uint32 entry_count = 0, i;
													boxstbl.read_bendian_t(entry_count);
													trak_entry.m_stco_entries.set_size(entry_count);
													for (i=0; i<entry_count; i++)
													{
														if (boxstbl.m_box.type == 'co64')
															boxstbl.read_bendian_t(trak_entry.m_stco_entries[i]);
														else
														{
															t_uint32 buffer;
															boxstbl.read_bendian_t(buffer);
															trak_entry.m_stco_entries[i] = buffer;
														}
													}
												}
												break;
											default:
												boxstbl.skip_box();
												break;
											}
										};
									}
									break;
								default:
									boxmdia.skip_box();
									break;
								}
							}
						}
						break;
					case 'tref':
						{
							mp4_reader boxtref(boxtrak);
							if (boxtref.next_box() && boxtref.box_type() == 'chap')
							{
								t_uint32 entry_count = boxtref.get_remaining_data_size()/4;
								trak_entry.m_chap_ref_entries.set_size(entry_count);
								for (t_size i=0; i<entry_count; i++)
									boxtref.read_bendian_t(trak_entry.m_chap_ref_entries[i]);
							}
						}
						break;
					default:
						boxtrak.skip_box();
						break;
					}
				};
				trak_entries.add_item(trak_entry);
			}
			break;
		default:
			boxmoov.skip_box();
			break;
		};
	};
}

bool g_get_itunes_chapters_mp4(service_ptr_t<file> p_file, itunesdb::chapter_list & p_chapter_list /*out*/, abort_callback & p_abort)
{
	bool ret = false;

	try
	{
		mp4_info p_mp4_info;
		g_get_mp4_info(p_file, p_mp4_info, p_abort);
		trak_list & trak_entries = p_mp4_info.trak_entries;

		for (t_size i=0, count = trak_entries.get_count(); i < count; i++)
		{
			if (/*trak_entries[i].m_mp4a_entry.m_samplerate_valid && */trak_entries[i].m_chap_ref_entries.get_count())
			{
				//t_uint32 samplerate = trak_entries[i].m_mp4a_entry.m_samplerate;

				for (t_size j=0, jcount = trak_entries[i].m_chap_ref_entries.get_count(); j < jcount; j++)
				{
					t_uint32 chapter_track_id = trak_entries[i].m_chap_ref_entries[j];
					t_size chapter_track_index;
					if (!trak_entries.find_by_track_id(chapter_track_id, chapter_track_index)/* && chapter_track_index != i && chapter_track_id != trak_entries[chapter_track_index].m_track_id*/)
						throw exception_io_data();
					{
						trak_t & p_chapter_track = trak_entries[chapter_track_index];
#if 0
						console::formatter() << "stsz";
						for (t_size k = 0, kcount = p_chapter_track.m_stsz.get_count(); k < kcount; k++)
							console::formatter() << p_chapter_track.m_stsz[k];

						console::formatter() << "stts";
						for (t_size k = 0, kcount = p_chapter_track.m_stts_entries.get_count(); k < kcount; k++)
							console::formatter() << p_chapter_track.m_stts_entries[k].sample_count << " : " <<  p_chapter_track.m_stts_entries[k].sample_delta;

						console::formatter() << "stsc";
						for (t_size k = 0, kcount = p_chapter_track.m_stsc_entries.get_count(); k < kcount; k++)
							console::formatter() << p_chapter_track.m_stsc_entries[k].first_chunk << " : " <<  p_chapter_track.m_stsc_entries[k].samples_per_chunk << " : " <<  p_chapter_track.m_stsc_entries[k].sample_description_index;

						console::formatter() << "stco";
						for (t_size k = 0, kcount = p_chapter_track.m_stco_entries.get_count(); k < kcount; k++)
							console::formatter() << p_chapter_track.m_stco_entries[k];
#endif
						//if (p_chapter_track.m_tx3g)
						{
							t_size chunkcount = p_chapter_track.m_stco_entries.get_count(), chunkindex = 0, sampleindex=0, sttsindex = 0, sttscount = p_chapter_track.m_stts_entries.get_count(), sttsptr=0;
							t_uint64 sample_delta = 0;
							for (t_size k = 0, kcount = p_chapter_track.m_stsc_entries.get_count(); k< kcount; k++)
							{
								stsc_entry & p_stsc = p_chapter_track.m_stsc_entries[k];
								chunkindex = p_stsc.first_chunk - 1;

								do 
								{
									if (chunkindex >= p_chapter_track.m_stco_entries.get_count())
										throw exception_io_data();
									p_file->seek(p_chapter_track.m_stco_entries[chunkindex], p_abort);
									for (t_size l=0,lcount=p_stsc.samples_per_chunk; l<lcount; l++)
									{
										//console::formatter() << "ci: " << chunkindex << " cc: " << chunkcount << " si: " << sampleindex << " sc: " << p_chapter_track.m_stsz.get_count()
										//	<< " sti: " << sttsindex << " stc: " << sttscount;
										if (sampleindex >= p_chapter_track.m_stsz.get_count())
											throw exception_io_data();
										if (sttsindex >= sttscount)
											throw exception_io_data();
										pfc::string8 text, url;

										t_size samplesize = p_chapter_track.m_stsz[sampleindex];
										pfc::array_staticsize_t<t_uint8> sampledata(samplesize);
										p_file->read(sampledata.get_ptr(), samplesize, p_abort);

										if (p_chapter_track.m_tx3g)
										{
											fbh::StreamReaderMemblock samplereader(sampledata);
											t_uint16 stringlen;
											samplereader.read_bendian_t(stringlen, p_abort);
											samplereader.read(pfc::string_buffer(text, stringlen), stringlen, p_abort);
											mp4_reader p_reader(&samplereader, samplereader.get_remaining(), p_abort);
											while (p_reader.next_box())
											{
												switch (p_reader.box_type())
												{
												case 'href':
													p_reader.skip(4);
													t_uint8 urllen;
													p_reader.read_bendian_t(urllen);
													p_reader.read(pfc::string_buffer(url, urllen), urllen);
													p_reader.seek_end();
													break;
												default:
													p_reader.skip_box();
													break;
												};
											};
										}
										t_size index_chapter;
										t_uint32 position = pfc::downcast_guarded<t_uint32>((sample_delta*1000/p_chapter_track.m_timescale));
										if (position == 0) position = 1;
										if (!p_chapter_list.bsearch_t(itunesdb::chapter_entry::g_compare_position_value, position, index_chapter))
											p_chapter_list.insert_item(itunesdb::chapter_entry(), index_chapter);
										itunesdb::chapter_entry & p_chapter =  p_chapter_list[index_chapter];
										p_chapter.m_start_position = position;
										if (p_chapter_track.m_tx3g)
										{
											if (j > 0)
											{
												p_chapter.m_url_title = text;
												p_chapter.m_url = url;
											}
											else
												p_chapter.m_title = text;
										}
										else if (p_chapter_track.m_jpeg)
										{
											p_chapter.m_ploc1 = 'trak';
											p_chapter.m_ploc2 = 13;
											p_chapter.m_ploc3 = 4;
											p_chapter.m_ploc4 = sampleindex;
											p_chapter.m_ploc_valid = true;
											p_chapter.m_image_data = sampledata;

											//file::ptr p_jpg;
											//filesystem::g_open_write_new(p_jpg, pfc::string8() << "I:\\chap" << sampleindex << ".jpg", p_abort);
											//p_jpg->write(p_chapter.m_image_data.get_ptr(), p_chapter.m_image_data.get_size(), p_abort);
											
										}
										//console::formatter() << (sample_delta*1000/p_chapter_track.m_timescale) << " : " << text << " : " << url;
										sample_delta += p_chapter_track.m_stts_entries[sttsindex].sample_delta;
										++sampleindex;
										if (++sttsptr == p_chapter_track.m_stts_entries[sttsindex].sample_count)
										{
											sttsptr = 0;
											++sttsindex;
										}
									}
									++chunkindex;
								}
								while ((k+1 < kcount && chunkindex + 1 < p_chapter_track.m_stsc_entries[k+1].first_chunk) || (k+1 == kcount && chunkindex < chunkcount));

							}
						}




						//trak_entries[chapter_track_index].
					}
				}
				p_chapter_list.m_hedr_2 = 7;
#if 0
				{
					for (t_size i = 0, count = p_chapter_list.get_count(); i<count; i++)
					{
						console::formatter() << p_chapter_list[i].m_start_position 
							<< " | " << p_chapter_list[i].m_title
							<< " | " << p_chapter_list[i].m_url_title
							<< " | " << p_chapter_list[i].m_url
							<< " | " << p_chapter_list[i].m_ploc1
							<< " | " << p_chapter_list[i].m_ploc2
							<< " | " << p_chapter_list[i].m_ploc3
							<< " | " << p_chapter_list[i].m_ploc4
							;
					}
					console::formatter() << p_chapter_list.m_hedr_1 << " | " << p_chapter_list.m_hedr_2;
				}
#endif
				ret = true;
				break;
			};
		}

	}
	catch (const pfc::exception &) {return false;}

	return ret;
}

bool g_check_mp4_type(service_ptr_t<file> p_file, abort_callback & p_abort)
{
	bool ret = false;

	try
	{
		mp4_info p_mp4_info;
		g_get_mp4_info(p_file, p_mp4_info, p_abort);
		bit_array_bittable mask(p_mp4_info.trak_entries.get_count());
		for (t_size i=0, count=p_mp4_info.trak_entries.get_count(); i<count; i++)
		{
			for (t_size j=0, jcount = p_mp4_info.trak_entries[i].m_chap_ref_entries.get_count(); j < jcount; j++)
			{
				t_uint32 chapter_track_id = p_mp4_info.trak_entries[i].m_chap_ref_entries[j];
				t_size chapter_track_index;
				if (p_mp4_info.trak_entries.find_by_track_id(chapter_track_id, chapter_track_index))
					mask.set(chapter_track_index, true);
			}
		}
		for (t_size i=0, count=p_mp4_info.trak_entries.get_count(); i<count; i++)
		{
			if (!mask[i] && p_mp4_info.trak_entries[i].m_handler_type == 'vide')
			{
				ret = true;
				break;
			}
		}
	}
	catch (const pfc::exception & ex) 
	{
		console::formatter() << "iPod manager: Error parsing MP4 file: " << ex.what();
		return false;
	}
	return ret;
}


bool g_get_gapless_mp4(const char * path, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort)
{
	service_ptr_t<file> file, fileCached;
	filesystem::g_open_read(file, path, p_abort);
	file_cached::g_create(fileCached, file, p_abort, 8*1024*1024);
	bool b_ret = g_get_gapless_mp4_apple_v2(fileCached, delay, padding, p_abort);
	if (!b_ret)
	{
		fileCached->seek(0, p_abort);
		b_ret = g_get_gapless_mp4_nero_v2(fileCached, delay, padding, p_abort);
	}
	return b_ret;
}

bool g_get_itunes_chapters_mp4(const char * path, itunesdb::chapter_list & p_out, abort_callback & p_abort)
{
	service_ptr_t<file> file, fileCached;
	filesystem::g_open_read(file, path, p_abort);
	file_cached::g_create(fileCached, file, p_abort, 8*1024*1024);
	return g_get_itunes_chapters_mp4(fileCached, p_out, p_abort);
}

bool g_get_gapless_mp4(const char * path, t_uint32 & delay, t_uint32 & padding)
{
	return g_get_gapless_mp4(path, delay, padding, abort_callback_impl());
}
