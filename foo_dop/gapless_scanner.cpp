#include "stdafx.h"

#include "gapless_scanner.h"
#include "mp4.h"

namespace ipod
{
	namespace tasks
	{

		void gapless_scanner_t::run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort)
		{
			p_status.update_text("Scanning for gapless info");
			run2(p_ipod, items, p_library, p_mappings, p_status, p_abort, 0, items.get_count());
		}

		void gapless_scanner_t::g_scan_gapless(const file_info * pinfo, itunesdb::t_track & p_track, const metadb_handle_ptr & ptr, bool b_use_dummy, abort_callback & p_abort)
		{
			if (!p_track.gapless_heuristic_info)
			{
				file_info_impl _info;
				pfc::string_extension ext(ptr->get_path());
				bool mp4 = !stricmp_utf8(ext, "mp4") || !stricmp_utf8(ext, "m4a") || !stricmp_utf8(ext, "m4b");
				if (stricmp_utf8(ext, "mp3") && !mp4)
					throw exception_dop_gapless_unsupported_format();
				else 
				{
					if (mp4)
					{
						t_uint32 delay=NULL, padding=NULL;
						if (g_get_gapless_mp4(ptr->get_path(), delay, padding, p_abort))
						{
							p_track.gapless_encoding_delay = delay;
							p_track.gapless_encoding_drain = padding;
							p_track.gapless_heuristic_info = 0x1;
						}
						else if (b_use_dummy)
						{
							p_track.gapless_encoding_delay = 1;
							p_track.gapless_encoding_drain = 0;
							p_track.gapless_heuristic_info = 0x2000003;
						}
						else throw exception_dop_gapless_no_gapless_data_found();
					}
					else
					{
						if (!pinfo)
						{
							if (ptr->get_info_async(_info))
								pinfo = &_info;
						}
						bool b_have_accurate = pinfo && pinfo->info_exists("enc_delay") && pinfo->info_exists("enc_padding");
						if (b_have_accurate || b_use_dummy)
						{
							t_filesize sync = g_get_gapless_sync_frame_mp3_v2(ptr->get_path(), p_abort);
							if (sync)
							{
								p_track.resync_frame_offset = sync;
								if (b_have_accurate)
								{
									p_track.gapless_encoding_delay = (t_uint32)pinfo->info_get_int("enc_delay");
									p_track.gapless_encoding_drain = (t_uint32)pinfo->info_get_int("enc_padding");
									p_track.gapless_heuristic_info = 0x1;
								}
								else
								{
									p_track.gapless_encoding_delay = 1;
									p_track.gapless_encoding_drain = 0;
									p_track.gapless_heuristic_info = 0x2000003;
								}
							}
							else
								throw exception_dop_gapless_corrupt();
						}
						else throw exception_dop_gapless_no_gapless_data_found();
					}
				}
			}
			else throw exception_dop_gapless_already_has_gapless_data();
		}

		void gapless_scanner_t::run2 (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort, t_uint32 progress_start, t_uint32 progress_count)
		{
			p_status.update_progress_subpart_helper(progress_start,progress_count);

			t_size i, count = items.get_count();

			{
				for (i=0; i<count; i++)
				{
					t_size index = p_library.m_handles.find_item(items[i]);
					try
					{
						t_size index = p_library.m_handles.find_item(items[i]);
						if (index != pfc_infinite)
						{
							g_scan_gapless(NULL, *p_library.m_tracks[index], items[i], p_mappings.use_dummy_gapless_data, p_abort);
						}
						else throw exception_dop_gapless_not_on_device();
					}
					catch (const pfc::exception & ex)
					{
						m_errors.add_item(results_viewer::result_t(items[i], index != pfc_infinite ? p_library.m_tracks[index]->create_source_handle() : metadb_handle_ptr(), pfc::string8() << "Failed to add gapless data for file: " << ex.what()));
					}

					p_status.update_progress_subpart_helper(progress_start+i+1,progress_count);
				}
			}
		}

	}
}
