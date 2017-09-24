#pragma once

#include "helpers.h"

namespace dopdb {
	// {B621A46D-0FA4-4c37-A6C2-6B1958318DBB}
	const GUID header =
	{ 0xb621a46d, 0xfa4, 0x4c37,{ 0xa6, 0xc2, 0x6b, 0x19, 0x58, 0x31, 0x8d, 0xbb } };

	enum root_items {
		t_root_track
	};

	enum track_items {
		t_track_id,
		t_track_location,
		t_track_original_path,
		t_track_last_known_path,
		t_track_original_filesize,
		t_track_original_timestamp,
		t_track_transcoded,
		t_track_dbid,
		t_track_original_subsong,
		t_track_artwork_size,
		t_track_artwork_sha1_hash,
	};

	class writer : public stream_writer_mem {
	public:
		template <class t_int>
		void write_element(t_uint32 id, t_int value, abort_callback & p_abort)
		{
			write_lendian_t(id, p_abort);
			write_lendian_t((t_uint32)sizeof(value), p_abort);
			write_lendian_t(value, p_abort);
		}
		void write_element(t_uint32 id, const char * value, abort_callback & p_abort)
		{
			write_lendian_t(id, p_abort);
			t_uint32 len = strlen(value);
			write_lendian_t(len, p_abort);
			write(value, len, p_abort);
		}
		void write_element(t_uint32 id, const void * value, t_uint32 size, abort_callback & p_abort)
		{
			write_lendian_t(id, p_abort);
			write_lendian_t(size, p_abort);
			write(value, size, p_abort);
		}
	};
};
