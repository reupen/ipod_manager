#include "stdafx.h"

#include "gapless.h"

void ipod_scan_gapless::on_exit()
{
	if (!m_process.get_abort().is_aborting()/* && !m_failed*/)
	{
		if (m_scanner.m_errors.get_count())
		{
			results_viewer::g_run(L"Warnings - Determine gapless data", m_scanner.m_errors);
		}
	}
}
