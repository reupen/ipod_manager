#ifndef _DOP_LOCK_H_
#define _DOP_LOCK_H_

class in_ipod_sync
{
public:
	in_ipod_sync()
	{
		m_eating = !g_busy;
		if (m_eating) g_busy = true;
	}
	~in_ipod_sync() {release();}
	static bool is_busy()  {return g_busy;}
	bool check_eating() const
	{
		if (!m_eating)
			fbh::show_info_box_threadsafe("Error", "iPod is busy", OIC_ERROR);
		return m_eating;
	}
	bool is_eating() const
	{
		return m_eating;
	}
	void release() {if (m_eating) g_busy=false; m_eating=false;}

private:
	bool m_eating;
	static bool g_busy;
};

#endif //_DOP_LOCK_H_