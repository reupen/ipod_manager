#include "main.h"

dop::ipod_manager_control_t::version_data_t g_version = {0, 6, 9, 8};

/** Declare some information about our component */
DECLARE_COMPONENT_VERSION_COPY("iPod manager",
pfc::string8()
	<< g_version.m_major << "."
	<< g_version.m_minor1 << "."
	<< g_version.m_minor2 << "."
	<< g_version.m_minor3
	<< ""
	,
"compiled: " __DATE__ "\n"
"written by musicmusic\n"
"Copyright (C) 2006-2011\n\n"
"zlib: Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler"
);

class ipod_manager_control : public dop::ipod_manager_control_t
{
	virtual api_version_t get_api_version() {return dop::ipod_manager_control_t::api_version_current;}
	virtual void get_component_version (dop::ipod_manager_control_t::version_data_t & p_out) { p_out = g_version; }
};

service_factory_single_t<ipod_manager_control> g_ipod_manager_control;