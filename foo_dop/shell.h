#pragma once

class directory_callback_simple : public directory_callback
{
	struct t_entry
	{
		pfc::string_simple m_path;
		t_filestats m_stats;
		bool m_is_dir;
		t_entry(const char * p_path, const t_filestats & p_stats, bool is_dir) : m_path(p_path), m_stats(p_stats), m_is_dir(is_dir) {}
	};

	pfc::list_t<pfc::rcptr_t<t_entry> > m_data;

	static int sortfunc(const pfc::rcptr_t<const t_entry> & p1, const pfc::rcptr_t<const t_entry> & p2) 
	{
		int ret = -pfc::compare_t(p1->m_is_dir, p2->m_is_dir);
		if (!ret) ret = lstrcmpi(pfc::stringcvt::string_wide_from_utf8(pfc::string_filename(p1->m_path)),
			pfc::stringcvt::string_wide_from_utf8(pfc::string_filename(p2->m_path)));
		return ret;
	}
public:
	bool on_entry(filesystem * owner,abort_callback & p_abort,const char * url,bool is_subdirectory,const t_filestats & p_stats)
	{
		p_abort.check_e();
		m_data.add_item(pfc::rcnew_t<t_entry>(url,p_stats,is_subdirectory));
		return true;
	}

	t_size get_count() {return m_data.get_count();}
	const char * operator[](t_size n) const {return m_data[n]->m_path;}
	const t_entry & get_item(t_size n) const {return *m_data[n];}
	const t_filestats & get_item_stats(t_size n) const {return m_data[n]->m_stats;}
	void sort() {m_data.sort_t(sortfunc);}
	void reset() {m_data.remove_all();}
};

class shell_window : public ui_helpers::popup_container_window
{
public:
	typedef pfc::refcounted_object_ptr_t<shell_window> ptr;
	enum {MSG_ON_DIRECTORY_THREAD_END = WM_USER + 2, MSG_REFRESH};
	class item 
	{
	public:
		item(const char * p_path, bool b_is_dir, bool b_is_virtual = false)
			: m_path(p_path), m_is_dir(b_is_dir), m_is_virtual(b_is_virtual) {};
		item() {};
		pfc::string8 m_path;
		bool m_is_dir;
		bool m_is_virtual;
	};
private:
	class directory_reader_thread : public mmh::Thread
	{
	public:
		directory_reader_thread() : m_wnd(NULL) {};
		void reset () {m_data.reset(); m_directory.reset();m_abort.reset();m_wnd=NULL;}
		void start (HWND wnd, const char * p_directory)
		{
			wait_for_and_release_thread();
			reset();
			m_wnd = wnd;
			m_directory = p_directory;
			create_thread();
		}
		void abort() {m_abort.abort();}
		DWORD on_thread()
		{
			m_error.reset();
			try {
			filesystem::g_list_directory(m_directory, m_data, m_abort);
			} catch (pfc::exception const & ex) {m_error = ex.what();};
			
			m_data.sort();
			if (!m_abort.is_aborting())
				PostMessage(m_wnd, MSG_ON_DIRECTORY_THREAD_END, NULL, NULL);
			return 0;
		}
		directory_callback_simple m_data;
		pfc::string8 m_directory;
		pfc::string8 m_error;
	private:
		abort_callback_impl m_abort;
		HWND m_wnd;
	};
	virtual t_uint32 get_styles() const {return style_popup_default;}
	virtual t_uint32 get_ex_styles() const {return ex_style_popup_default;}
	virtual const GUID & get_class_guid() 
	{
		// {D1005CA7-EB56-4dcf-BF00-0A20B72AA8F6}
		static const GUID g_guid = 
		{ 0xd1005ca7, 0xeb56, 0x4dcf, { 0xbf, 0x0, 0xa, 0x20, 0xb7, 0x2a, 0xa8, 0xf6 } };
		return g_guid;
	};
	const char * get_window_title() {return "iPod File System Explorer";}
	LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
	void on_size(unsigned cx, unsigned cy);
	void refresh (const pfc::string8 & p_path);
	void populate ();
	void disable ();
	void enable ();

	//uih::ListView m_items_view;
	pfc::string8 m_directory;
	pfc::list_t<item> m_items;
	HWND m_wnd_items_view;
	directory_reader_thread m_directory_reader;
	ptr m_this;
};


