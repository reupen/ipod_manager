#pragma once

#include "reader.h"

namespace ipod
{
	namespace tasks
	{
		class gapless_scanner_t
		{
		public:
			void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort);
			void run2 (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, const t_field_mappings & p_mappings, threaded_process_v2_t & p_status, abort_callback & p_abort, t_uint32 progress_start, t_uint32 progress_count);
			PFC_DECLARE_EXCEPTION(exception_dop_gapless, pfc::exception, "Generic error");
			PFC_DECLARE_EXCEPTION(exception_dop_gapless_already_has_gapless_data, exception_dop_gapless, "Already has gapless data");
			PFC_DECLARE_EXCEPTION(exception_dop_gapless_no_gapless_data_found, exception_dop_gapless, "No gapless data found");
			PFC_DECLARE_EXCEPTION(exception_dop_gapless_unsupported_format, exception_dop_gapless, "File format not supported");
			PFC_DECLARE_EXCEPTION(exception_dop_gapless_not_on_device, exception_dop_gapless, "File not on iPod");
			PFC_DECLARE_EXCEPTION(exception_dop_gapless_corrupt, exception_dop_gapless, "Error parsing file. File may be corrupt");
			static void g_scan_gapless(const file_info * pinfo, itunesdb::t_track & p_track, const metadb_handle_ptr & ptr, bool b_use_dummy, abort_callback & p_abort);

			pfc::list_t<results_viewer::result_t> m_errors;
		};
	}
}
