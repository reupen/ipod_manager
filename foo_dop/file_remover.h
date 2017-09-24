#pragma once

#include "reader.h"

class ipod_file_remover
{
public:
	void run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort);
	void run (ipod_device_ptr_ref_t p_ipod, const bool * items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort);
	//pfc::string_list_impl m_error_list;
	pfc::list_t<results_viewer::result_t> m_errors;
	pfc::string_list_impl m_deleted_items;
};
