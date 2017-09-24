#ifndef _SCANNER_DOP_H_
#define _SCANNER_DOP_H_

#include "helpers.h"
#include "mobile_device_v2.h"
#include "reader.h"

int wcsicmp_partial(const WCHAR * p1,const WCHAR * p2,t_size num=~0);
int wcsicmp_ex(const wchar_t * p1,t_size p1_bytes,const wchar_t * p2,t_size p2_bytes);

namespace ipod_models
{
	enum t_model
	{
		ipod_1g,
		ipod_2g,
		ipod_3g,
		ipod_4g,
		ipod_4g_color,
		ipod_5g,
		ipod_5_5g,
		ipod_6g,
		ipod_6_1g,
		ipod_6_2g,
		ipod_mini_1g,
		ipod_mini_2g,
		ipod_nano_1g,
		ipod_nano_2g,
		ipod_nano_3g,
		ipod_nano_4g,
		ipod_nano_5g,
		ipod_nano_6g,
		ipod_nano_7g,
		ipod_shuffle_1g,
		ipod_shuffle_2g,
		ipod_shuffle_3g,
		ipod_shuffle_4g,
	};
	enum t_4g_model
	{
		ipod_4g_white,
		ipod_4g_u2,
	};
	enum t_4g_color_model
	{
		ipod_4g_color_white,
		ipod_4g_color_u2,
	};
	enum t_5g_model
	{
		ipod_5g_white,
		ipod_5g_black,
		ipod_5g_u2,
	};
	enum t_6xg_model
	{
		ipod_6xg_black,
		ipod_6xg_silver,
	};
	enum t_nano_1g_model
	{
		ipod_nano_1g_white,
		ipod_nano_1g_black,
	};
	enum t_nano_2g_model
	{
		ipod_nano_2g_black,
		ipod_nano_2g_pink,
		ipod_nano_2g_green,
		ipod_nano_2g_blue,
		ipod_nano_2g_silver,
		ipod_nano_2g_red,
	};
	enum t_nano_3g_model
	{
		ipod_nano_3g_pink,
		ipod_nano_3g_black,
		ipod_nano_3g_red,
		ipod_nano_3g_green,
		ipod_nano_3g_blue,
		ipod_nano_3g_silver,
	};
	enum t_nano_4g_model
	{
		ipod_nano_4g_black,
		ipod_nano_4g_red,
		ipod_nano_4g_yellow,
		ipod_nano_4g_green,
		ipod_nano_4g_orange,
		ipod_nano_4g_purple,
		ipod_nano_4g_pink,
		ipod_nano_4g_blue,
		ipod_nano_4g_silver,
	};
	enum t_nano_5g_model
	{
		ipod_nano_5g_silver,
		ipod_nano_5g_black,
		ipod_nano_5g_purple,
		ipod_nano_5g_blue,
		ipod_nano_5g_green,
		ipod_nano_5g_yellow,
		ipod_nano_5g_orange,
		ipod_nano_5g_red,
		ipod_nano_5g_pink,
	};
	enum t_nano_6g_model
	{
		ipod_nano_6g_silver,
		ipod_nano_6g_graphite,
		ipod_nano_6g_blue,
		ipod_nano_6g_green,
		ipod_nano_6g_orange,
		ipod_nano_6g_pink,
		ipod_nano_6g_red,
	};
	enum t_nano_7g_model
	{
		ipod_nano_7g_pink,
		ipod_nano_7g_yellow,
		ipod_nano_7g_blue,
		ipod_nano_7g_green,
		ipod_nano_7g_purple,
		ipod_nano_7g_silver,
		ipod_nano_7g_slate,
		ipod_nano_7g_red,
	};
	enum t_shuffle_2g_model
	{
		ipod_shuffle_2g_orange,
		ipod_shuffle_2g_pink,
		ipod_shuffle_2g_green,
		ipod_shuffle_2g_blue,
		ipod_shuffle_2g_silver,
		
		ipod_shuffle_25g_purple,
		ipod_shuffle_25g_green,
		ipod_shuffle_25g_blue,
		ipod_shuffle_25g_silver,

		ipod_shuffle_27g_purple,
		ipod_shuffle_27g_blue,
		ipod_shuffle_27g_green,
		ipod_shuffle_27g_red,
	};

	enum t_shuffle_3g_model
	{
		ipod_shuffle_3g_black,
		ipod_shuffle_3g_silver,
		ipod_shuffle_3g_green,
		ipod_shuffle_3g_blue,
		ipod_shuffle_3g_pink,
		ipod_shuffle_3g_aluminium,
	};
	enum t_shuffle_4g_model
	{
		ipod_shuffle_4g_silver,
		ipod_shuffle_4g_pink,
		ipod_shuffle_4g_orange,
		ipod_shuffle_4g_green,
		ipod_shuffle_4g_blue,
	};

	enum t_mini_1g_model
	{
		ipod_mini_1g_silver,
		ipod_mini_1g_green,
		ipod_mini_1g_pink,
		ipod_mini_1g_blue,
		ipod_mini_1g_gold,
	};

	enum t_mini_2g_model
	{
		ipod_mini_2g_silver,
		ipod_mini_2g_green,
		ipod_mini_2g_pink,
		ipod_mini_2g_blue,
	};

	struct ipod_model
	{
		t_uint32 model;
		t_uint32 submodel;
		ipod_model() : model(-1), submodel(0) {};
	};
}

void g_get_model_string(const ipod_models::ipod_model & model, pfc::string8 & p_out);

class ipod_info
{
public:
	bool family_valid;
	t_uint32 family_id;
	t_uint32 updater_family_id;
	bool board_valid;
	t_uint32 board_id;
	pfc::string8 serial;
	pfc::string8 firmware;
	t_uint32 battery_poll_interval;
	bool get_model(ipod_models::ipod_model & p_out);

	ipod_info() : family_valid(false), board_valid(false), board_id(0), family_id(0), updater_family_id(0), battery_poll_interval(0) {};
};

enum t_ipod_model
{
	ipod_3g=1,
	ipod_4g=2,
	ipod_4g_colour=3,
	ipod_mini=4,
	ipod_5g=5,
	ipod_nano=6,
	ipod_nano_2g=7,
	ipod_shuffle=8,
	ipod_shuffle_2g=9,
	ipod_6g=10,
	ipod_nano_3g=11,
	ipod_iphone=12,
	ipod_touch=13,
	ipod_touch_2g=14,
	ipod_iphone_3g=15,
	ipod_nano_4g=16,
	ipod_shuffle_3g=17,
	ipod_iphone_3g_s=18,
	ipod_nano_5g=19,
	ipod_touch_3g=20,
	ipad=21,
	ipod_iphone_4=22,
	ipod_shuffle_4g=23,
	ipod_nano_6g=24,
	ipod_touch_4g=25,
	ipod_iphone_4_verizon=26,
	ipod_ipad_2_1=27,
	ipod_ipad_2_2=28,
	ipod_ipad_2_3=29,
	ipod_nano_7g=30,
	ipod_unknown_1394=-1,
	ipod_unrecognised=-2, //new model
};

class artwork_format_t
{
public:
	t_uint32 m_format_id;
	t_uint32 m_render_height;
	t_uint32 m_render_width;
	bool m_align_row_bytes;
	t_size m_offset_alignment;
	t_uint32 m_row_byte_alignment;
	t_uint32 m_pixel_format;
	bool m_interlaced;
	t_uint32 m_back_colour;
	bool m_crop;
	t_size m_associated_format;
	t_size m_excluded_formats;
	bool m_alternate_pixel_order;
	bool m_rgb555;
	t_uint32 m_left_inset;
	t_uint32 m_top_inset;
	t_uint32 m_right_inset;
	t_uint32 m_bottom_inset;
	t_uint32 m_original_height;
	t_uint32 m_original_width;
	t_uint32 m_minimum_size;

	artwork_format_t()
		: m_format_id(NULL), m_render_height(NULL), m_render_width(NULL), m_align_row_bytes(true),
		m_interlaced(false), m_back_colour(0), m_crop(false), m_pixel_format('L565'),
		m_associated_format(0), m_offset_alignment(0), m_alternate_pixel_order(false), m_rgb555(false), m_excluded_formats(0),
		m_left_inset(0), m_top_inset(0), m_right_inset(0), m_bottom_inset(0), m_row_byte_alignment(4), m_original_height(0),
		m_original_width(0), m_minimum_size(0)
	{};

	t_size get_row_stride() const //bytes
	{
		t_size ret=0;
		if (m_pixel_format == 'jpeg')
			ret = 333;
		else
		{
			ret = 2*m_render_width;
			if (m_align_row_bytes && (ret%m_row_byte_alignment))
				ret+= (m_row_byte_alignment-(ret%m_row_byte_alignment));
		}
		return ret;
	}

	t_size get_raw_size() const
	{
		t_size ret=get_row_stride();
		ret *= m_render_height;
		return ret;
	}
	t_size get_size() const
	{
		t_size ret=get_raw_size();
		if (m_offset_alignment && (ret%m_offset_alignment))
		{
			ret += (m_offset_alignment - (ret%m_offset_alignment));
		}
		return ret;
	}
	t_size get_padding() const
	{
		t_size ret=0;
		if (m_offset_alignment)
		{
			t_size size = get_raw_size();
			if (size%m_offset_alignment)
			{
				ret = (m_offset_alignment - (size%m_offset_alignment));
			}
		}
		return ret;
	}

	static int g_compare_format_id_value(artwork_format_t const & p1, t_uint32 p2)
	{
		return pfc::compare_t(p1.m_format_id, p2);
	}
};

class device_properties_t
{
public:
	t_uint32 m_db_version;
	pfc::string8 m_FireWireGUID;
	bool m_SQLiteDB;
	bool m_ShadowDB;
	bool m_SupportsSoundCheck;
	bool m_Speakable;
	bool m_PlaylistFoldersSupported;
	t_uint32 m_SpeakableSampleRate;
	t_size m_ShadowDBVersion;
	t_size m_SQLMusicLibraryUserVersion;
	pfc::string_list_impl m_SQLMusicLibraryPostProcessCommands;

	pfc::list_t<artwork_format_t> m_artwork_formats, m_chapter_artwork_formats, m_image_formats;

	bool get_artwork_format_by_id(t_uint32 id, artwork_format_t & p_out)
	{
		for (t_size i = 0, count = m_artwork_formats.get_count(); i<count; i++)
			if (m_artwork_formats[i].m_format_id == id) {p_out = m_artwork_formats[i]; return true;}
		for (t_size i = 0, count = m_chapter_artwork_formats.get_count(); i<count; i++)
			if (m_chapter_artwork_formats[i].m_format_id == id) {p_out = m_chapter_artwork_formats[i]; return true;}
		for (t_size i = 0, count = m_image_formats.get_count(); i<count; i++)
			if (m_image_formats[i].m_format_id == id) {p_out = m_image_formats[i]; return true;}
		return false;
	}

	bool m_Initialised;

	device_properties_t() : m_db_version(NULL), m_SQLiteDB(false),
	m_ShadowDB(false), m_ShadowDBVersion(0), m_SupportsSoundCheck(false), m_SQLMusicLibraryUserVersion(NULL),
	m_Initialised (false), m_Speakable(false), m_SpeakableSampleRate(0), m_PlaylistFoldersSupported(false) {};
};

class drive_space_info_t
{
public:
	t_filesize
		m_capacity,
		m_freespace,
		m_blocksize;
	drive_space_info_t() : m_capacity(0), m_freespace(0), m_blocksize(0) {} ;
};

class ipod_write_action_v2_t;

class ipod_action_manager
{
	typedef ipod_write_action_v2_t ipod_action_t;
public:
	class track_action
	{
	public:
		track_action(const ipod_device_ptr_t & p_device, ipod_action_t & p_action);// : m_device(p_device), m_action(p_action) {m_device->m_action_manager.register_action(&m_action);}
		~track_action();// {m_device->m_action_manager.deregister_action(&m_action);}
	private:
		ipod_device_ptr_t m_device;
		ipod_action_t & m_action;
	};
	void register_action (ipod_action_t * p_action) {insync (m_sync); m_actions.add_item(p_action);}
	void deregister_action (ipod_action_t * p_action) {insync (m_sync); m_actions.remove_item(p_action);}

	void suspend_all();
	void resume_all();
	void abort_all();

private:
	critical_section m_sync;
	pfc::ptr_list_t<ipod_action_t> m_actions;
};

class ipod_device_t : public pfc::refcounted_object_root
{
public:
	typedef ipod_device_t self_t;
	typedef pfc::refcounted_object_ptr_t<self_t> ptr;

	char drive;
	t_ipod_model model;
	bool shuffle;
	bool mobile;
	pfc::string8 mobile_serial;
	mobile_device_handle::ptr mobile_device;
	pfc::string_simple_t<WCHAR> device_instance_path, volume_path, driver_symbolic_path;
	DEVINST instance;

	ipod_action_manager m_action_manager;

	critical_section m_database_sync;
	ipod::tasks::load_database_t m_database;

	device_properties_t m_device_properties;

	bool is_6g_format() const
	{
		return (m_device_properties.m_db_version >= 3 || model == ipod_6g || model == ipod_nano_3g || model == ipod_touch || model == ipod_iphone || model == ipod_nano_4g || model == ipod_touch_2g || model == ipod_iphone_3g);
	}

	bool supports_playlist_folders() const
	{
		return (m_device_properties.m_PlaylistFoldersSupported || model == ipod_6g || model == ipod_nano_3g || model == ipod_nano_4g || model == ipod_nano_5g);
	}
	void get_usbserial(pfc::string_base & p_out) const
	{
		pfc::string8 temp = pfc::stringcvt::string_utf8_from_wide(device_instance_path);
		t_size index = temp.find_last('\\');
		if (index != pfc_infinite)
		{
			index++;

			t_size index_end = temp.find_first('&', index);
			t_size count = index_end == pfc_infinite ? pfc_infinite : index_end - index;
			{
				p_out.set_string(temp.get_ptr()+index, count);
			}
		}
	}

	void get_firewireguid(pfc::string_base & p_out) const
	{
		if (!m_device_properties.m_FireWireGUID.is_empty())
			p_out = m_device_properties.m_FireWireGUID;
		else
			get_usbserial(p_out);
	}

	void get_deviceuniqueid(pfc::string_base & p_out) const
	{
		if (mobile)
			p_out = mobile_serial;
		else
			get_firewireguid(p_out);
	}

	void get_database_folder(pfc::string8 & p_out) const
	{
		if (mobile)
			p_out = "iTunes_Control";
		else
			p_out = "iPod_Control";
	}

	void get_database_path(pfc::string8 & p_out) const
	{
		pfc::string8 folder;
		get_database_folder(folder);

		p_out.reset();
		if (mobile)
			p_out << "applemobiledevice://" << mobile_serial << ":/" << folder;
		else
		{
			p_out << "file://";
			p_out.add_byte(drive);
			p_out <<  ":\\" << folder;
		}
	}

	void get_root_path(pfc::string8 & p_out) const
	{
		p_out.reset();
		if (mobile)
			p_out << "applemobiledevice://" << mobile_serial << ":/";
		else
		{
			p_out << "file://";
			p_out.add_byte(drive);
			p_out <<  ":\\";
		}
	}

	char get_path_separator() const
	{
		return mobile ? '/' : '\\';
	}

	const char * get_path_separator_ptr() const
	{
		return mobile ? "/" : "\\";
	}

	void do_before_sync()
	{
		if (mobile_device.is_valid())
		{
			//try {
			mobile_device->do_before_sync();
			//}
			//catch (const pfc::exception & ex)
			//{
			//	console::formatter() << "iPod manager: Apple Mobile Device: Warning: " << ex.what();
			//}
		}
	}

	void do_after_sync()
	{
		if (mobile_device.is_valid())
		{
			try {
			mobile_device->do_after_sync();
			}
			catch (const pfc::exception & ex)
			{
				console::formatter() << "iPod manager: Apple Mobile Device: Warning: " << ex.what();
			}
		}
	}

	void get_capacity_information (drive_space_info_t & p_info);

	ipod_device_t() : drive(0), model(ipod_unrecognised), shuffle(0), instance(NULL), mobile(false) {};
	ipod_device_t(char d, t_ipod_model m, bool s, const WCHAR * devid, const WCHAR * volume, const WCHAR * symbolicdriver, DEVINST inst, const device_properties_t & aw, bool p_mobile = false, const char * p_mobile_serial = "") : drive(d), model(m), shuffle(s), device_instance_path(devid),
	instance(inst), m_device_properties(aw), mobile(p_mobile), mobile_serial(p_mobile_serial), volume_path(volume), driver_symbolic_path(symbolicdriver) {};
};

class ipod_device_notification_t 
{
public:
	virtual void on_device_arrival(ipod_device_ptr_cref_t p_ipod) {};
	virtual void on_device_modified(ipod_device_ptr_cref_t p_ipod) {};
	virtual void on_device_removal(ipod_device_ptr_cref_t p_ipod) {};
	virtual void on_shutdown() {};
};

template <class receiver_t>
class ipod_device_notification_maintthread_t : public ipod_device_notification_t
{
public:
	class mainthread_on_device_arrival : public main_thread_callback
	{
	public:
		service_ptr_t<receiver_t> m_receiver;
		ipod_device_ptr_t m_ipod;
		void callback_run()
		{
			m_receiver->on_device_arrival(m_ipod);
		}
	};
	class mainthread_on_device_removal : public main_thread_callback
	{
	public:
		service_ptr_t<receiver_t> m_receiver;
		ipod_device_ptr_t m_ipod;
		void callback_run()
		{
			m_receiver->on_device_removal(m_ipod);
		}
	};
	class mainthread_on_device_modified : public main_thread_callback
	{
	public:
		service_ptr_t<receiver_t> m_receiver;
		ipod_device_ptr_t m_ipod;
		void callback_run()
		{
			m_receiver->on_device_modified(m_ipod);
		}
	};
	class mainthread_on_shutdown : public main_thread_callback
	{
	public:
		service_ptr_t<receiver_t> m_receiver;
		void callback_run()
		{
			m_receiver->on_shutdown();
		}
	};
	virtual void on_device_arrival(ipod_device_ptr_cref_t p_ipod) 
	{
		if (m_receiver.is_valid())
		{
			if (core_api::is_main_thread())
				m_receiver->on_device_arrival(p_ipod);
			else
			{
				mainthread_on_device_arrival * ptr = new service_impl_t<mainthread_on_device_arrival>;
				ptr->m_ipod = p_ipod;
				ptr->m_receiver=m_receiver;
				static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
			}
		}
	};
	virtual void on_device_removal(ipod_device_ptr_cref_t p_ipod) 
	{
		if (m_receiver.is_valid())
		{
			if (core_api::is_main_thread())
				m_receiver->on_device_removal(p_ipod);
			else
			{
				mainthread_on_device_removal * ptr = new service_impl_t<mainthread_on_device_removal>;
				ptr->m_ipod = p_ipod;
				ptr->m_receiver=m_receiver;
				static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
			}
		}
	};
	virtual void on_device_modified(ipod_device_ptr_cref_t p_ipod) 
	{
		if (m_receiver.is_valid())
		{
			if (core_api::is_main_thread())
				m_receiver->on_device_modified(p_ipod);
			else
			{
				mainthread_on_device_modified * ptr = new service_impl_t<mainthread_on_device_modified>;
				ptr->m_ipod = p_ipod;
				ptr->m_receiver=m_receiver;
				static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
			}
		}
	};
	virtual void on_shutdown()
	{
		if (m_receiver.is_valid())
		{
			mainthread_on_shutdown * ptr = new service_impl_t<mainthread_on_shutdown>;
			ptr->m_receiver=m_receiver;
			static_api_ptr_t<main_thread_callback_manager>()->add_callback(ptr);
		}
	};
	service_ptr_t<receiver_t> m_receiver;
};

class t_drive_manager
{
public:
	void register_callback (ipod_device_notification_t * ptr, bool b_calloninit = true)
	{
		insync(m_sync);
		m_callbacks.add_item(ptr);

		if (b_calloninit)
		{
			t_size index, drivecount=m_drives.get_count();
			for (index=0; index<drivecount; index++)
				ptr->on_device_arrival(m_drives[index]);
		}
	}
	void deregister_callback (ipod_device_notification_t * ptr)
	{
		insync(m_sync);
		m_callbacks.remove_item(ptr);
	}
	void on_device_modified(ipod_device_ptr_cref_t ptr, const ipod::tasks::load_database_t & p_new_database)
	{
		{
		insync (ptr->m_database_sync);
		ptr->m_database = p_new_database;
		}

		{
		insync(m_sync);
		t_size i, count=m_callbacks.get_count();
		for (i=0; i<count; i++)
			m_callbacks[i]->on_device_modified(ptr);
		}
	}
	void set_wnd(HWND wnd)
	{
		insync(m_sync);
		m_wnd=wnd;
	}

	void add_drive(ipod_device_ptr_cref_t p_ipod, abort_callback & p_abort)
	{
		console::formatter() << "iPod manager: Apple iPod/iPhone connected. Device Instance Path: " << Tu( p_ipod->device_instance_path.get_ptr() );
		insync(m_sync);
		t_size index = m_drives.add_item(p_ipod);

		try {
			m_drives[index]->m_database.run(m_drives[index], threaded_process_dummy_t(), p_abort, true);
		} catch (pfc::exception const & ex) 
		{
			m_drives[index]->m_database.m_tracks.remove_all();
			m_drives[index]->m_database.m_playlists.remove_all();
			m_drives[index]->m_database.m_handles.remove_all();
			if (m_drives[index]->m_database.m_library_playlist.is_valid())
			{
				m_drives[index]->m_database.m_library_playlist->name = ex.what();
			}
		};

		t_size i, count=m_callbacks.get_count();
		for (i=0; i<count; i++)
			m_callbacks[i]->on_device_arrival(m_drives[index]);
	}
	void remove_drives(DWORD rem)
	{
		insync(m_sync);
		t_size i, count=m_drives.get_count();
		for (i=count; i>0; i--)
			if (!m_drives[i-1]->mobile && (rem & 1<<(m_drives[i-1]->drive - 'A')))
			{
				console::formatter() << "iPod manager: Apple iPod/iPhone disconnected. Device Instance Path: " << Tu( m_drives[i-1]->device_instance_path.get_ptr() );
				t_size j, jcount=m_callbacks.get_count();
				for (j=0; j<jcount; j++)
					m_callbacks[j]->on_device_removal(m_drives[i-1]);
				m_drives.remove_by_idx(i-1);
			}
	}
	void remove_all_drives()
	{
		insync(m_sync);
		m_drives.remove_all();

		t_size i, count=m_callbacks.get_count();
		for (i=0; i<count; i++)
			m_callbacks[i]->on_shutdown();
	}
	void remove_all_mobile_devices()
	{
		{
			insync(m_sync);
			t_size i, count=m_drives.get_count();
			for (i=count; i>0; i--)
				if (m_drives[i-1]->mobile_device.is_valid())
				{
					t_size j, jcount=m_callbacks.get_count();
					for (j=0; j<jcount; j++)
						m_callbacks[j]->on_device_removal(m_drives[i-1]);
					m_drives.remove_by_idx(i-1);
				}
		}
	}
	void remove_by_mobile_device (const mobile_device_handle::ptr & ptr)
	{
		if (ptr.is_valid())
		{
			insync(m_sync);
			t_size i, count=m_drives.get_count();
			for (i=count; i>0; i--)
				if (m_drives[i-1]->mobile_device == ptr)
				{
					m_drives[i-1]->m_action_manager.abort_all();
					console::formatter() << "iPod manager: Apple iPod/iPhone disconnected. Device Instance Path: " << Tu( m_drives[i-1]->device_instance_path.get_ptr() );
					t_size j, jcount=m_callbacks.get_count();
					for (j=0; j<jcount; j++)
						m_callbacks[j]->on_device_removal(m_drives[i-1]);
					m_drives.remove_by_idx(i-1);
				}
		}
	}
	bool find_by_mobile_device_handle (am_device * handle, ipod_device_ptr_ref_t p_out)
	{
		insync(m_sync);
		for (t_size i = 0, count = m_drives.get_count(); i<count; i++)
		{
			if (m_drives[i]->mobile_device.is_valid() && m_drives[i]->mobile_device->m_device == handle)
			{
				p_out = m_drives[i];
				return true;
			}
		}
		return false;
	}
	void get_drives(pfc::list_base_t<ipod_device_ptr_t> & p_out)
	{
		p_out.remove_all();
		insync(m_sync);
		p_out.add_items(m_drives);
	}
	t_size get_drive_count()
	{
		insync(m_sync);
		return m_drives.get_count();
	}
	void set_pending_mobile_device_connection(bool b_val) { m_pending_mobile_device_connection = b_val; }
	bool get_pending_mobile_device_connection() { return m_pending_mobile_device_connection; }
	bool wait_for_initialisation(double secs)
	{
		return m_event_initialised.wait_for(secs);
	}
	HWND get_wnd()
	{
		insync(m_sync);
		return m_wnd;
	}
	t_drive_manager() : m_wnd(NULL), m_pending_mobile_device_connection(false)/*, m_ipod_mask(NULL)*/ {};
	~t_drive_manager()
	{
		insync(m_sync);
		m_drives.remove_all();
	}
	class win32_event_auto : public win32_event
	{
	public:
		win32_event_auto() {create(true, false);}
	} m_event_initialised;

private:
	pfc::ptr_list_t<ipod_device_notification_t> m_callbacks;
	HWND m_wnd;
	//DWORD m_ipod_mask;
	pfc::list_t<ipod_device_ptr_t> m_drives;
	//pfc::list_t<pfc::string8> m_pending_mobile_devices;
	volatile bool m_pending_mobile_device_connection;
	critical_section m_sync;
};

extern t_drive_manager g_drive_manager;

void g_get_device_xml(ipod_device_ptr_ref_t p_ipod, pfc::string8 & p_out);
void _g_get_device_xml(const char drive, pfc::string8 & p_out);
void g_get_device_info(const char * xml, ipod_info & info);
void g_get_artwork_info(const char * xml, device_properties_t & p_out);

void g_get_sysinfo(ipod_device_ptr_ref_t pod, pfc::array_t<t_uint8> & p_out, abort_callback & p_abort);
void g_get_device_info_from_sysinfo(const t_uint8 * sysinfo, t_size size, ipod_info & info);
void g_get_plist_cfobject (const char * ptr, cfobject::object_t::ptr_t & p_out);

struct ipod_battery_status
{
	bool m_charged;
	t_uint8 m_level;
	t_uint8 m_unk; 
	t_uint16 m_raw_data;
};

enum ipod_lowlevel_value_t
{
	ipod_lowlevel_volume_limit = 4,
	ipod_lowlevel_battery_status = 1,
};

t_uint16 get_ipod_lowlevel_value(ipod_device_ptr_ref_t p_ipod, short value);
void get_ipod_battery_status(ipod_device_ptr_ref_t p_ipod, ipod_battery_status & p_out);

class device_instance_info_t
{
public:
	pfc::string_simple_t<WCHAR> m_path;
	DEVINST m_handle;
	device_instance_info_t() : m_handle(NULL) {};
};

void enum_mobile_devices(pfc::list_t<device_instance_info_t> & p_out);
bool get_mobile_device_instance_by_serial(const char * serial, device_instance_info_t & p_out);
void g_get_mobile_device_properties_string (const ipod_device_ptr_t & p_ipod, pfc::string8 & p_out);
void g_get_checkpoint_artwork_formats(cfobject::object_t::ptr_t const & deviceInfo, device_properties_t & p_out);
void g_get_checkpoint_device_info(cfobject::object_t::ptr_t const & deviceInfo, device_properties_t & p_out);

class string_format_metadb_handle_for_progress : public pfc::string8
{
	titleformat_object::ptr to;
public:
	const char * run(const metadb_handle_ptr & ptr)
	{
		if (ptr.is_valid())
		{	
			if (!to.is_valid())
				static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "[$if2(%artist%,%show%) - ]%title%");
			ptr->format_title(NULL, *this, to, NULL);
		}
		else
			set_string("<invalid item>");
		return get_ptr();
	}
	string_format_metadb_handle_for_progress(const metadb_handle_ptr & ptr)
	{
		run(ptr);
	}
	string_format_metadb_handle_for_progress()
	{
	}
};

#endif