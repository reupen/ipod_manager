#include "main.h"

#include "ipod_manager.h"
#include "prepare.h"

namespace ipod
{
	namespace tasks
	{

		bool preparer_t::run(ipod_device_ptr_ref_t p_ipod, ipod::tasks::load_database_t & m_library, class ipod::tasks::database_writer_t & m_writer,threaded_process_v2_t & p_status,abort_callback & p_abort)
		{
			p_status.checkpoint();

			pfc::string8 base;
			p_ipod->get_database_path(base);
			//string_print_drive m_path(p_ipod->drive);
			pfc::string8 dir = base;
			if (!filesystem::g_exists( dir , p_abort))
				filesystem::g_create_directory(dir, p_abort);
			if (!filesystem::g_exists( (dir << p_ipod->get_path_separator_ptr() << "iTunes" ) , p_abort))
				filesystem::g_create_directory(dir, p_abort);

			p_status.checkpoint();

			if (
				!filesystem::g_exists( (pfc::string8() << dir << p_ipod->get_path_separator_ptr() << "iTunesCDB" ) , p_abort)
				&& !filesystem::g_exists( (pfc::string8() << dir << p_ipod->get_path_separator_ptr() << "iTunesDB" ) , p_abort)
				)
			{
				service_ptr_t<genrand_service> p_rand = genrand_service::g_create();
				p_rand->seed(GetTickCount());
				t_uint64 p1 = p_rand->genrand(MAXLONG);
				t_uint64 p2 = p_rand->genrand(MAXLONG);
				m_library.dbid = p1|(p2<<32);
				p1 = p_rand->genrand(MAXLONG);
				p2 = p_rand->genrand(MAXLONG);
				m_library.m_library_playlist->id = p1|(p2<<32);
				p1 = p_rand->genrand(MAXLONG);
				p2 = p_rand->genrand(MAXLONG);
				m_library.unk1_1 = p1|(p2<<32);
				if (!uGetUserName(m_library.m_library_playlist->name))
					m_library.m_library_playlist->name = "Unnamed";
				m_library.encoding = p_ipod->m_device_properties.m_SQLiteDB ? 1 : 0;
				m_library.format = p_ipod->m_device_properties.m_SQLiteDB ? 2 : 1;
				if (m_library.encoding)
				{
					zlib_stream zs;
				}
				p_status.checkpoint();
				//m_writer.run(m_path, m_library, p_status, p_abort);
				//p_status.update_progress_subpart_helper(3,3);
				return true;
			}
			return false;
		}



	}
}