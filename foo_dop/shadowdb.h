#pragma once

namespace itunessd {
	class t_entry {
	public:
		t_uint32 unk1 : 24;
		t_uint32 starttime : 24;
		t_uint32 unk2 : 24;
		t_uint32 unk3 : 24; //start byte pos?
		t_uint32 stoptime : 24;
		t_uint32 unk4 : 24;
		t_uint32 unk5 : 24; //stop byte pos
		t_uint32 volume : 24;
		t_uint32 filetype : 24;
		t_uint32 unk6 : 24; //filename length? usually 200h = 512
		pfc::array_t<char> filename;
		t_uint32 unk7 : 24;
		t_uint32 unk8 : 24;
		t_uint32 unk9 : 24;
		bool unk10;
		bool dont_skip_in_shuffle;
		bool bookmarkable;
		bool unk11;
	};
	class writer : public stream_writer_mem {
	public:
		void write_int(t_uint32 value, abort_callback & p_abort)
		{
			t_uint32 temp = value;
			byte_order::order_native_to_be_t(temp);
			//if (temp & 0xff) throw pfc::exception_overflow(); //meh signed ints
			temp = temp >> 8;
			write(&temp, 3, p_abort);
		}
#if 0
		void write_string(WCHAR * str, t_size len1, abort_callback & p_abort) //len1 must be accurate!
		{
			t_size len = pfc::wcslen_max(str, len1);//save time
			for (; len; len--)
				byte_order::order_native_to_be_t(str[len - 1]);
			write(str, len1*sizeof(WCHAR), p_abort);
		}
#endif
	};
};

namespace iTunesSD2 {
	hm3(s, h, d, b);
	hm3(s, h, t, h);
	hm3(s, h, t, r);
	hm3(s, h, p, h);
	hm3(s, h, p, l);

	class writer : public stream_writer_mem {
	public:
		using stream_writer_mem::write;

		void write(const writer & p_source, abort_callback & p_abort)
		{
			write(p_source.get_ptr(), p_source.get_size(), p_abort);
		}
	};
}
