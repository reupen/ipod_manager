#ifndef _RESULTS_H_
#define _RESULTS_H_

namespace results_viewer
{
	class result_t
	{
	public:
		pfc::string8 m_message;
		metadb_handle_ptr m_handle;
		metadb_handle_ptr m_source_handle;
		result_t() {};
		result_t (const metadb_handle_ptr & ptr, const metadb_handle_ptr & ptr_source, const char * msg) 
			: m_handle(ptr), m_source_handle(ptr_source), m_message(msg) {};
	};
	void g_run(const TCHAR * p_title, const pfc::list_base_const_t<result_t> & results);
};

#endif //_RESULTS_H_