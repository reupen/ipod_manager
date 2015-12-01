#include "main.h"

namespace ipod
{
	namespace tasks
	{
		void drive_scanner_t::run()
		{
			g_drive_manager.m_event_initialised.wait_for(5);
			g_drive_manager.get_drives(m_ipods);
			if (!m_ipods.get_count())
				throw pfc::exception("No iPod found");
			//if (m_ipods[0]->m_device_properties.m_SQLiteDB)
			//	throw pfc::exception("SQLite DB not supported");
		}
	}
}
