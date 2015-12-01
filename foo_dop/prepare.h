#ifndef _DOP_PREARE_H_
#define _DOP_PREARE_H_


namespace ipod
{
	namespace tasks
	{

		class preparer_t
		{
		public:
			virtual bool run(ipod_device_ptr_ref_t p_ipod, load_database_t & m_library, class database_writer_t & m_writer,threaded_process_v2_t & p_status,abort_callback & p_abort);
		};

	}
}


#endif //_DOP_PREARE_H_