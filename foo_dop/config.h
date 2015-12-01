#ifndef _DOP_CONF_H_
#define _DOP_CONF_H_


class cfg_stringlist : public cfg_var, public pfc::string_list_impl
{
public:
	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
		t_uint32 n, m = pfc::downcast_guarded<t_uint32>(get_count());
		p_stream->write_lendian_t(m,p_abort);
		for(n=0;n<m;n++) p_stream->write_string(get_item(n),p_abort);
	}

	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) {
		t_uint32 n,count;
		p_stream->read_lendian_t(count,p_abort);
		pfc::string8_fast_aggressive temp;
		temp.prealloc(32);
		for(n=0;n<count;n++) 
		{
			p_stream->read_string(temp,p_abort);
			add_item(temp);
		}
	}

	bool have_item(const char * p_item) 
	{
		t_size i, count = get_count();
		for (i=0; i<count; i++)
			if (!stricmp_utf8(p_item, get_item(i))) return true;
		return false;
	}

public:
	cfg_stringlist(const GUID & p_guid) : cfg_var(p_guid) {}
};

namespace settings
{
	enum {replaygain_processing_none=0,replaygain_processing_scan_after_encoding, replaygain_processing_apply_before_encoding};

	extern cfg_bool use_ipod_sorting,
		sort_ipod_library,
		mobile_devices_enabled,
		numbers_last,
		use_fb2k_artwork,
		devices_panel_autosend,
		use_dummy_gapless_data,
		sync_playlists_imported,
		allow_sort_fields,
		sort_playlists;
	extern cfg_int_t<t_uint8> replaygain_processing_mode;
	extern cfg_string
		artist_mapping,
		album_artist_mapping,
		title_mapping,
		album_mapping,
		composer_mapping,
		sort_artist_mapping,
		sort_album_artist_mapping,
		sort_title_mapping,
		sort_album_mapping,
		sort_composer_mapping,
		genre_mapping,
		comment_mapping,
		conversion_command,
		conversion_extension,
		conversion_parameters,
		artwork_sources,
		compilation_mapping,
		ipod_library_sort_script,
		devices_panel_autosend_playlist,
		voiceover_title_mapping;
	extern cfg_int soundcheck_adjustment;
	extern cfg_uint conversion_custom_thread_count,date_added_mode,soundcheck_rgmode, conversion_bitrate_limit, active_encoder;
	extern cfg_bool sync_library,
		add_artwork,
		convert_files,
		add_gapless,
		conversion_use_custom_thread_count,
		quiet_sync,
		video_thumbnailer_enabled,
		sync_eject_when_done,
		conversion_use_bitrate_limit,
		encoder_imported;
	extern advconfig_checkbox_factory check_video;
	extern advconfig_integer_factory
		extra_filename_characters,
		reserved_diskspace;
	extern advconfig_string_factory conversion_temp_files_folder;
	extern cfg_stringlist sync_playlists;

	class conversion_preset_t
	{
	public:
		enum {
			bps_16 = 2,
			bps_24,
			bps_32,
		};
		pfc::string8 m_name, m_executable, m_parameters, m_file_extension;
		t_uint32 m_highest_bps_supported;	
		bool m_encoder_requires_accurate_length;
		conversion_preset_t() : m_highest_bps_supported(bps_32) {};
		conversion_preset_t(const char * p_name, const char * p_executable, const char * p_parameters,
			const char * p_file_extension, t_uint32 p_highest_bps_supported, bool b_encoder_requires_accurate_length = false)
			: m_name(p_name), m_executable(p_executable), m_parameters(p_parameters), 
			m_file_extension(p_file_extension), m_highest_bps_supported(p_highest_bps_supported),
			m_encoder_requires_accurate_length(b_encoder_requires_accurate_length)
		{};
		static int g_compare(const conversion_preset_t & p1, const conversion_preset_t & p2)
		{
			return CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, pfc::stringcvt::string_os_from_utf8(p1.m_name), -1, 
				pfc::stringcvt::string_os_from_utf8(p2.m_name), -1) - 2;
		}
		void get_command(pfc::string8 & p_out) const
		{
			p_out.reset();
			p_out << "\"" << m_executable << "\" " << m_parameters;
		}
		t_uint32 get_max_bps() const
		{
			if (m_highest_bps_supported == bps_16) return 16;
			else if (m_highest_bps_supported == bps_24) return 24;
			else return 32;
		}
	};
	
	class cfg_conversion_presets_t : public cfg_var, public pfc::list_t<conversion_preset_t>
	{
	private:
		enum {stream_version_current = 0};
		void get_data_raw(stream_writer * p_stream,abort_callback & p_abort) {
			t_uint32 n, m = pfc::downcast_guarded<t_uint32>(get_count());
			p_stream->write_lendian_t(t_uint32(stream_version_current),p_abort);
			p_stream->write_lendian_t(m,p_abort);
			for(n=0;n<m;n++) 
			{
				conversion_preset_t & p_item = (*this)[n];
				p_stream->write_string(p_item.m_name,p_abort);
				p_stream->write_string(p_item.m_executable,p_abort);
				p_stream->write_string(p_item.m_parameters,p_abort);
				p_stream->write_string(p_item.m_file_extension,p_abort);
				p_stream->write_lendian_t(p_item.m_highest_bps_supported,p_abort);
				p_stream->write_lendian_t(p_item.m_encoder_requires_accurate_length,p_abort);
			}
		}
		void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort) 
		{
			if (p_sizehint)
			{
				remove_all();
				t_uint32 n,count,version;
				p_stream->read_lendian_t(version,p_abort);
				if (version <= stream_version_current)
				{
					p_stream->read_lendian_t(count,p_abort);
					for(n=0;n<count;n++) 
					{
						conversion_preset_t p_item;
						p_stream->read_string(p_item.m_name,p_abort);
						p_stream->read_string(p_item.m_executable,p_abort);
						p_stream->read_string(p_item.m_parameters,p_abort);
						p_stream->read_string(p_item.m_file_extension,p_abort);
						p_stream->read_lendian_t(p_item.m_highest_bps_supported,p_abort);
						p_stream->read_lendian_t(p_item.m_encoder_requires_accurate_length,p_abort);
						add_item(p_item);
					}
				}
			}
		}
	public:
		void reset()
		{
			remove_all();
			add_item(conversion_preset_t("AAC (Nero)", "neroAacEnc.exe", "-q 0.40 -lc -ignorelength -if - -of %d", "m4a", conversion_preset_t::bps_32));
			add_item(conversion_preset_t("Apple Lossless (ffmpeg)", "ffmpeg.exe", "-i - -acodec alac %d", "m4a", conversion_preset_t::bps_16));
			//add_item(conversion_preset_t("Apple Lossless (QAAC)", "qaac.exe", "--no-optimize --alac --silent --ignorelength - -o %d", "m4a", conversion_preset_t::bps_24));
			add_item(conversion_preset_t("MP3 (LAME)", "lame.exe", "-S --noreplaygain -V 5 - %d", "mp3", conversion_preset_t::bps_24));
			//add_item(conversion_preset_t("Imported Settings", "lame.exe", "xxx", "mp3", conversion_preset_t::bps_32);
		}
		cfg_conversion_presets_t(const GUID & p_guid) : cfg_var(p_guid) {reset();};

	};

	extern cfg_conversion_presets_t encoder_list;
};

class preferences_tab {
public:
	virtual HWND create(HWND wnd) = 0;
	virtual const char * get_name() = 0;
	virtual bool get_help_url(pfc::string_base & p_out) { return true; }
};

void g_update_conversion_prefs_encoders();
void g_sort_converstion_encoders();

#endif //_DOP_CONF_H_
