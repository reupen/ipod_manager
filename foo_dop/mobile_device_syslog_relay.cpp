#include "main.h"

DWORD syslog_relay::on_thread()
{
	pfc::array_t<char> buffer;
	buffer.set_size(16383);
	while (true)
	{
		int ret = recv(m_socket, buffer.get_ptr(), 16383, NULL);
		if (ret == 0 || ret == SOCKET_ERROR) 
		{
			//console::formatter() << "iPod manager: Apple Mobile Device: Error receiving data (
			return 0;
		}
		bool b_data_received = false;
		insync(m_sync);
		for (t_size i=0; i<ret; ) 
		{
			t_size start = i;
			while (i<ret && buffer[i] != 0 && buffer[i] != '\r' && buffer[i] != '\n') i++;
			if (i>start)
			{
				m_buffer.add_string(buffer.get_ptr()+start, i-start);
				m_buffer.add_string("\r\n");
				b_data_received = true;
			}
			while (i<ret && (buffer[i] == '\r' || buffer[i] == '\n' || buffer[i] == 0)) i++;
			//if (i<ret && buffer[i] == 0) i++;
		}
		if (b_data_received)
		{
			for (t_size i=0, count = m_wnd_notify_list.get_count(); i<count; i++)
				PostMessage(m_wnd_notify_list[i], WM_USER+2, NULL, NULL);
		}
	}
}

void syslog_relay::initialise(SOCKET s)
{
	m_socket = s;
	create_thread();
}

void syslog_relay::deinitialise()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
	wait_for_and_release_thread();
}

void syslog_relay::get_syslog (pfc::string8 & p_out)
{
	insync(m_sync);
	p_out = m_buffer;
}

void syslog_relay::register_wnd (HWND wnd)
{
	insync(m_sync);
	m_wnd_notify_list.add_item(wnd);
}

void syslog_relay::deregister_wnd (HWND wnd)
{
	insync(m_sync);
	m_wnd_notify_list.remove_item(wnd);
}

syslog_relay::syslog_relay() : m_socket(INVALID_SOCKET) {};