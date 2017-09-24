#ifndef _PHOTODB_H_
#define _PHOTODB_H_

#include "itunesdb.h"

using namespace itunesdb;

class artwork_format_t;

namespace photodb
{
	namespace identifiers
	{
		const t_uint32
			dfhm = hm(d,f),
			dshm = hm(d,s),
			ilhm = hm(i,l),
			iihm = hm(i,i),
			inhm = hm(i,n),
			dohm = hm(d,o),
			alhm = hm(a,l),
			abhm = hm(a,b),
			aihm = hm(a,i),
			flhm = hm(f,l),
			fihm = hm(f,i);
	}
	class t_image_name
	{
	public:
		t_uint32 do_type;
		t_uint32 correlation_id;
		t_uint32 file_offset;
		t_uint32 file_size;
		t_int16 vertical_padding;
		t_int16 horizontal_padding;
		t_uint16 height;
		t_uint16 width;
		t_uint32 display_start_time_ms;
		t_uint32 file_size_2;

		pfc::string8 location;
		bool location_valid;

		static int g_compare_offset(const t_image_name & item1, const t_image_name & item2)
		{
			return pfc::compare_t(item1.file_offset, item2.file_offset);
		}

#if 0
		static int g_compare_format_id(const t_image_name & item1, const t_image_name & item2)
		{
			return pfc::compare_t(item1.correlation_id, item2.correlation_id);
		}
#endif

		t_image_name() : do_type(0), correlation_id(0), file_offset(0), file_size(0),
			vertical_padding(0), horizontal_padding(0), height(0), width(0), location_valid(false), file_size_2(0), display_start_time_ms(0)
		{};
	};

	class t_image_item
	{
	public:
		t_uint32 image_id;
		t_uint64 song_dbid;
		t_uint32 unk4;
		t_uint32 rating;
		t_uint32 unk6;
		t_uint32 original_date;
		t_uint32 digitised_date;
		t_uint32 source_image_size;
		t_uint32 unk7;
		t_uint32 refcount;
		t_uint32 unk8;

		pfc::string8 label;
		bool label_valid;

		pfc::list_t<t_image_name> image_names;

		bool find_image_name_by_format(t_uint32 format_id, t_size & index, t_uint32 timems = -1)
		{
			t_size i, count = image_names.get_count();
			for (i=0; i<count; i++)
			{
				if (image_names[i].correlation_id == format_id && (timems == -1 || timems == image_names[i].display_start_time_ms))
				{
					index = i;
					return true;
				}
			}
			return false;
		}

		static int g_compare_id(const t_image_item & item1, const t_image_item & item2)
		{
			return pfc::compare_t(item1.image_id, item2.image_id);
		}
		static int g_compare_dbid(const t_image_item & item1, const t_image_item & item2)
		{
			return pfc::compare_t(item1.song_dbid, item2.song_dbid);
		}

		t_image_item() : image_id(0), song_dbid(0), unk4(0), rating(0), unk6(0), original_date(0),
			digitised_date(0), source_image_size(0), label_valid(false), unk7(0), refcount(0), unk8(0)
		{};
	};

	class t_image_list : public pfc::list_t<t_image_item>
	{
	public:
	};

	class t_album_item
	{
	public:
		t_uint32 unk1;
		t_uint32 image_id;

		t_album_item() : unk1(0), image_id(0)
		{};
	};

	class t_photo_album
	{
	public:
		t_uint32 album_id;
		t_uint32 unk2;
		t_uint8 unk3;
		t_uint8 unk4;
		t_uint8 album_type;
		t_uint8 play_music;
		t_uint8 repeat;
		t_uint8 random;
		t_uint8 show_titles;
		t_uint8 transition_direction;
		t_uint32 slide_duration;
		t_uint32 transition_duration;
		t_uint32 unk7;
		t_uint32 unk8;
		t_uint64 song_dbid2;
		t_uint32 previous_album_id;

		pfc::string8 album_name;
		pfc::string8 transition_effect;

		bool album_name_valid;
		bool transition_effect_valid;

		pfc::list_t<t_album_item> album_items;

		t_photo_album() : album_id(0), unk2(0), unk3(0), unk4(0), album_type(0), play_music(0), repeat(0), random(0), 
			show_titles(0), transition_duration(0), unk7(0), unk8(0), song_dbid2(0), previous_album_id(0), album_name_valid(false),
			transition_effect_valid(false)
		{};
	};

	class t_photo_album_list : public pfc::list_t<t_photo_album>
	{
	public:
	};

	class t_file_item
	{
	public:
		t_uint32 unk1;
		t_uint32 correlation_id;
		t_uint32 file_size;

		t_file_item() : unk1(0), correlation_id(0), file_size(0)
		{};
	};

	class t_file_list : public pfc::list_t<t_file_item>
	{
	public:
	};

	class t_thumbinfo
	{
	public:
		pfc::string8 filename;
		t_filesize dataend;
		t_uint32 format_id;
		t_thumbinfo() : dataend(0), format_id(0) {};
		t_thumbinfo(const char * name, t_filesize end, t_uint32 id) : filename(name), dataend(end), format_id(id) {};
	};
	class t_thumb_info_list : public pfc::list_t< t_thumbinfo >
	{
	public:
		bool find_by_filename(const char * filename, t_size & index)
		{
			t_size i, count=get_count();
			for (i=0; i<count; i++)
			{
				if (!stricmp_utf8(get_item(i).filename, filename))
				{
					index = i;
					return true;
				}
			}
			return false;
		}
	};

	class t_datafile
	{
	public:
		t_uint32 unk1;
		t_uint32 unk2;
		t_uint32 unk3;
		t_uint32 next_ii_id;
		t_uint64 unk5;
		t_uint64 unk6;
		t_uint32 unk7; //artworkdb: 0, photodb: 2
		t_uint32 unk8;
		t_uint32 unk9;
		t_uint32 unk10;
		t_uint32 unk11;

		t_image_list image_list;
		t_photo_album_list photo_albums;
		t_file_list file_list;

		t_thumb_info_list thumb_info_list;

		t_uint32 t_datafile::get_next_ii_id();
		bool find_by_dbid(t_size & index, t_uint64 dbid);
		bool find_by_image_id(t_size & index, t_uint32 id);
		void find_empty_block(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, t_size padded_size, t_uint32 & offset, t_uint32 & fileno, const pfc::list_t<t_image_name> & pending_images, /*t_filesize & dataend,*/ abort_callback & p_abort);
		void truncate_thumb_file(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, const char * p_thumb, abort_callback & p_abort);
		void truncate_thumb_file(ipod_device_ptr_ref_t p_ipod, t_uint32 fmt_id, const char * p_thumb, abort_callback & p_abort);

		void __remove_by_index (ipod_device_ptr_ref_t p_ipod, t_size index, abort_callback & p_abort)
		{
			t_size i, count = image_list[index].image_names.get_count();
			for (i=count; i; i--)
			{
				pfc::string8 path = image_list[index].image_names[i-1].location;
				t_uint32 fmt_id = image_list[index].image_names[i-1].correlation_id;
				image_list[index].image_names.remove_by_idx(i-1);
				if (path.length())
					truncate_thumb_file(p_ipod, fmt_id, path, p_abort);
			}
			image_list.remove_by_idx(index);
		}
		//void add_artwork(ipod_device_ptr_ref_t p_ipod, t_uint64 dbid, const char * image_path, abort_callback & p_abort);
		//void add_artwork_v2(ipod_device_ptr_ref_t p_ipod, t_uint64 dbid, t_uint32 & mhii_id, const char * image_path, t_size count_alloc, abort_callback & p_abort);
		
		//returns false is existing entry was modified
		bool add_artwork_v3(ipod_device_ptr_ref_t p_ipod, t_uint32 mediatype, t_uint64 dbid, t_uint32 & mhii_id, const album_art_data_ptr & data, t_size count_alloc, abort_callback & p_abort, itunesdb::chapter_list * p_chapter_list = NULL);
		void replace_artwork(ipod_device_ptr_ref_t p_ipod, t_uint32 mediatype, t_uint64 dbid, t_uint32 mhii_id, const album_art_data_ptr & data, t_size count_alloc, abort_callback & p_abort);
		void finalise_add_artwork_v2(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort);
		void initialise_artworkdb(ipod_device_ptr_ref_t p_ipod);
		//void remove_by_track(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & p_library, t_size index);
		void remove_by_dbid(t_uint64 dbid);
		void remove_by_image_id(t_uint32 iiid);
		void remove_by_dbid_v2(ipod_device_ptr_ref_t p_ipod, t_uint64 dbid);

		void _add_image_name(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, bool b_new, Gdiplus::Bitmap& image, t_image_item & p_item, t_uint32 timems, t_size count_alloc, abort_callback & p_abort);

		t_datafile() : unk1(0), unk2(2), unk3(0), next_ii_id(0), unk5(0), unk6(0), unk7(0), unk8(0), unk9(0), unk10(0), unk11(0)
		{};
	};
	using namespace shareddb;

	class t_data_object
	{
	public:
		t_uint16 type;
		t_uint16 unk1;
		pfc::array_t<t_uint8> data;
		void get_string(pfc::string8 & p_out, abort_callback & p_abort);
		void get_inhm(t_image_name & p_out, abort_callback & p_abort);

		t_data_object() : type(0), unk1(0)
		{};
	};
	template <t_uint32 id>
	void read_header(fbh::StreamReaderMemblock * p_reader, t_header_marker<id> & p_out, abort_callback & p_abort)
	{
		p_reader->read_bendian_t(p_out.identifier, p_abort);
		p_out.verify_identifier();

		p_reader->read_lendian_t(p_out.header_size, p_abort);
		p_reader->read_lendian_t(p_out.section_size, p_abort);
		//p_out.data.set_size(p_out.header_size - sizeof(t_uint32)*3);
		//p_reader->read(p_out.data.get_ptr(), p_out.header_size - sizeof(t_uint32)*3, p_abort);
		p_out.data.m_size = p_out.header_size - sizeof(t_uint32)*3;
		p_reader->read_nobuffer(p_out.data.m_data, p_out.data.m_size, p_abort);
	}
	void read_do(stream_reader * p_reader, t_header_marker< identifiers::dohm > & p_header, t_data_object & p_out, abort_callback & p_abort);
	class reader
	{
		fbh::StreamReaderMemblock * m_file;

		template <t_uint32 id>
		void read_header(t_header_marker<id> & p_out, abort_callback & p_abort)
		{
			photodb::read_header(m_file, p_out, p_abort);
		}
		void read_ilhm(t_header_marker< identifiers::ilhm > & p_header, t_image_list & p_out, abort_callback & p_abort);
		void read_iihm(t_header_marker< identifiers::iihm > & p_header, t_image_item & p_out, abort_callback & p_abort);

		void read_alhm(t_header_marker< identifiers::alhm > & p_header, t_photo_album_list & p_out, abort_callback & p_abort);
		void read_abhm(t_header_marker< identifiers::abhm > & p_header, t_photo_album & p_out, abort_callback & p_abort);
		void read_aihm(t_header_marker< identifiers::aihm > & p_header, t_album_item & p_out, abort_callback & p_abort);

		void read_flhm(t_header_marker< identifiers::flhm > & p_header, t_file_list & p_out, abort_callback & p_abort);
		void read_fihm(t_header_marker< identifiers::fihm > & p_header, t_file_item & p_out, abort_callback & p_abort);

		void read_do(t_header_marker< identifiers::dohm > & p_header, t_data_object & p_out, abort_callback & p_abort)
		{
			photodb::read_do(m_file, p_header, p_out, p_abort);
		}

	public:
		void read_photodb(t_datafile & p_out, abort_callback & p_abort);

		reader(fbh::StreamReaderMemblock * p_file) 
			: m_file(p_file)
		{};
	};

	enum t_encoding
	{
		encoding_utf8=1,
		encoding_utf16le=2
	};

	class writer
	{
	public:
		void write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
			const void * p_data, t_size data_size, t_uint32 header_data, abort_callback & p_abort);
		void write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
			const void * p_data, t_size data_size, abort_callback & p_abort);
		void write_do (t_uint16 type, void *data, t_uint32 data_size, abort_callback & p_abort);
		void write_do_string (t_uint16 type, const char * str, t_encoding encoding, abort_callback & p_abort);
		void write_iihm(const t_image_item & in, abort_callback & p_abort);
		void write_inhm(const t_image_name & in, abort_callback & p_abort);
		void write_dshm_ilhm(const t_image_list & ii, abort_callback & p_abort);
		void write_dshm_alhm(const t_photo_album_list & ii, abort_callback & p_abort);

		void write_aihm(const t_album_item & ai, abort_callback & p_abort);
		void write_abhm(const t_photo_album & ab, abort_callback & p_abort);
		void write_fihm(const t_file_item & fi, abort_callback & p_abort);
		void write_dshm_flhm(const t_file_list & fl, abort_callback & p_abort);

		void write_dfhm(const t_datafile & df, abort_callback & p_abort);

		writer(stream_writer * p_writer) : m_writer(p_writer) {};
	private:
		stream_writer * m_writer;
	};

};

namespace bitmap_utils
{
	class bitmap_to_alternative_pixel_order_t {
		pfc::array_t<t_uint16> m_buffer;
		int m_src_stride;
		const t_uint16 * m_src;
		unsigned m_width;
		unsigned m_height;
	public:
		bitmap_to_alternative_pixel_order_t(const t_uint16 *src, unsigned width, unsigned height, int src_stride)
			: m_src_stride(src_stride), m_src(src), m_width(width), m_height(height) {
			pfc::fill_array_t(m_buffer, 0);
		}
		const t_uint8 * to_alternative_order();
		
		const t_uint8 * get_ptr() { return reinterpret_cast<t_uint8*>(m_buffer.get_ptr()); }
	private:
		void to_alternative_order(t_size src_offset, t_size dst_offset, int width, int height);
	};

	class bitmap_from_alternative_pixel_order_t {
		pfc::array_t<t_uint16> m_buffer;
		int m_dst_stride;
		const t_uint16 * m_src;
		unsigned m_width;
		unsigned m_height;
	public:
		bitmap_from_alternative_pixel_order_t(const t_uint16 *src, int width, int height, int dst_stride)
			: m_dst_stride(dst_stride), m_src(src), m_width(width), m_height(height) {
		}
		const t_uint8 * from_alternative_order();
		const t_uint8 * get_ptr() { return reinterpret_cast<t_uint8*>(m_buffer.get_ptr()); }
	private:
		void from_alternative_order(t_size dst_offset, t_size src_offset, int width, int height);
	};

	HBITMAP create_bitmap_from_uyvy(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height);
	HBITMAP create_bitmap_from_rgb565(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height, t_uint32 stride, bool b_rgb555 = false);
	HBITMAP create_bitmap_from_jpeg(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height);
};

#endif