#pragma once

namespace ipod
{
	namespace tasks
	{
		class drive_scanner_t
		{
		public:
			void run(threaded_process_v2_t & p_status,abort_callback & p_abort) 
			{
				p_status.checkpoint();
				p_status.update_text("Scanning for iPod");
				run();
			}
			void run();
			pfc::list_t<ipod_device_ptr_t> m_ipods;
			pfc::string8 m_failure_message;
		};
	};
};
