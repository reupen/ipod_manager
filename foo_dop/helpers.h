#ifndef _DOP_HELPERS_H_
#define _DOP_HELPERS_H_

class thread_t
{
public:
	void create_thread()
	{
		if (!m_thread)
			m_thread = CreateThread(NULL, NULL, &g_threadproc, (LPVOID)this, NULL, NULL);
	}
	void on_destroy_thread()
	{
		if (m_thread)
		{
			WaitForSingleObject(m_thread,10*1000);
			CloseHandle(m_thread);
			m_thread = NULL;
		}
	}
	void release_thread()
	{
		if (m_thread)
		{
			CloseHandle(m_thread);
			m_thread = NULL;
		}
	}
	virtual DWORD on_thread()=0;
	thread_t() : m_thread(NULL) {};

	virtual ~thread_t() {};
private:
	static DWORD CALLBACK g_threadproc(LPVOID lpThreadParameter)
	{
		TRACK_CALL_TEXT("thread_t entry");
		thread_t * p_this = reinterpret_cast<thread_t*>(lpThreadParameter);
		return p_this->on_thread();
	}
	HANDLE m_thread;
};

class suspend_tracker {
public:
	suspend_tracker() : m_suspended(false) {
		m_event_suspend.create(true,true);
	}
	inline void suspend() {set_suspend_state(true);}
	inline void resume() {set_suspend_state(false);}

	void set_suspend_state(bool p_state) {m_suspended = p_state; m_event_suspend.set_state(!p_state);}

	bool is_suspended() const {return m_suspended;}

	abort_callback_event get_suspend_event() const {return m_event_suspend.get();}

private:
	suspend_tracker(const suspend_tracker &) {throw pfc::exception_not_implemented();}
	const suspend_tracker & operator=(const suspend_tracker&) {throw pfc::exception_not_implemented();}
	
	volatile bool m_suspended;
#ifdef WIN32
	win32_event m_event_suspend;
#endif
};

class NOVTABLE checkpoint_base
{
public:
	virtual void checkpoint() const = 0;
};

class threaded_process_v2_t 
	: public thread_t, public ui_helpers::container_window_v2, public app_close_blocking_task, public fbh::InitQuitDynamic, public checkpoint_base
{
public:
	enum {MSG_UPDATE_PROGRESS=WM_USER+2, MSG_REDRAW, MSG_END};
	enum {flag_show_progress_window = 1<<0, flag_show_abort = 1<<1, flag_show_text =1<<2, flag_position_bottom_right = 1<<3,
		flag_show_button = 1<<4,flag_progress_marquee=1<<5,flag_no_delay=1<<6,flag_block_app_close=1<<7};
	enum {progress_width=355};

	class detail_entry
	{
	public:
		pfc::string8 m_label, m_value;

		detail_entry() {};
		detail_entry(const char * p_label, const char * p_value) : m_label(p_label), m_value(p_value) {};
	};

	void query_task_name(pfc::string_base & out) {out = m_title;}
	void on_quit() {m_abort.abort(); on_destroy_thread();}
	//override me
	virtual void on_run() = 0;
	virtual void on_init() {};
	virtual void on_exit() {};
	void set_steps (const t_uint32 * ptr, t_size count)
	{
		m_steps.set_count(0);
		m_steps.append_fromptr(ptr, count);
	}
	t_uint32 get_steps_total()
	{
		return get_step_position(m_steps.get_count());
	}
	t_uint32 get_step_position(t_size index)
	{
		t_size i;t_uint32 ret = 0; t_size count=m_steps.get_count();
		for (i=0; i<index; i++)
			if (i<count)
				ret+= m_steps[i];
		return ret;
	}
	void run(HWND wnd);

	void advance_progresstep(t_size count = 1)
	{
		insync(m_sync);
		m_position+=count;
		PostMessage(get_wnd(), MSG_UPDATE_PROGRESS, MulDiv(get_step_position(m_position), progress_width, get_steps_total()), NULL);
	}
	void update_progress(t_size position)
	{
		insync(m_sync);
		m_position = position;
		PostMessage(get_wnd(), MSG_UPDATE_PROGRESS, MulDiv(get_step_position(position), progress_width, get_steps_total()), NULL);
	}
	void update_progress_subpart_helper(t_size position, t_size range)
	{
		insync(m_sync);
		//PostMessage(get_wnd(), MSG_UPDATE_PROGRESS, MulDiv(m_position*range+position, progress_width, m_range*range), NULL);
		PostMessage(get_wnd(), MSG_UPDATE_PROGRESS, MulDiv(get_step_position(m_position)*range+m_steps[m_position]*position, progress_width, get_steps_total()*range), NULL);
	}
	void update_progress_subpart_helper(t_size subposition)
	{
		insync(m_sync);
		PostMessage(get_wnd(), MSG_UPDATE_PROGRESS, MulDiv(get_step_position(m_position)+subposition, progress_width, get_steps_total()), NULL);
	}
	void update_text(const char * p_text)
	{
		insync(m_sync);
		m_text = p_text;
		m_detail_entries.set_size(0);
		PostMessage(get_wnd(), MSG_REDRAW, NULL, NULL);
	}
	template <typename TArray>
	void update_text_and_details(const char * p_text, TArray & entries)
	{
		insync(m_sync);
		m_text = p_text;
		m_detail_entries = entries;
		PostMessage(get_wnd(), MSG_REDRAW, NULL, NULL);
	}
	template <typename TArray>
	void update_detail_entries(TArray & entries)
	{
		insync(m_sync);
		m_detail_entries = entries;
		PostMessage(get_wnd(), MSG_REDRAW, NULL, NULL);
	}
	threaded_process_v2_t(const char * title, t_size flags = flag_show_progress_window, t_size range=1)
		: m_range(range), m_position(0), m_wnd_progress(NULL), m_title(title), m_flags(flags), m_wnd_button(NULL),
		m_titlecolour(NULL), m_textfont_height(0), m_titlefont_height(0), m_timer_active(false), m_window_cx(0), m_window_cy(0)
	{
		memset(&m_time_last_redraw, 0, sizeof(m_time_last_redraw));
		if (m_flags & flag_block_app_close)
		{
			try { static_api_ptr_t<app_close_blocking_task_manager>()->register_task(this); } catch(const exception_service_not_found &) {/*user runs pre-0.9.5.1*/}
		}
		fbh::InitQuitManager::s_register_instance(this);
	};

	~threaded_process_v2_t()
	{
		if (m_flags & flag_block_app_close)
		{
			try { static_api_ptr_t<app_close_blocking_task_manager>()->unregister_task(this); } catch(const exception_service_not_found &) {/*user runs pre-0.9.5.1*/}
		}
		fbh::InitQuitManager::s_deregister_instance(this);
	}
	
	abort_callback & get_abort() {return m_abort;}
	abort_callback_impl & get_abort_impl() {return m_abort;}
	suspend_tracker & get_suspend() {return m_suspend_tracker;}

	void checkpoint() const //not from main thread
	{
		m_abort.check();
		if (m_suspend_tracker.is_suspended())
		{
			HANDLE events[2] = {m_abort.get_abort_event(), m_suspend_tracker.get_suspend_event()};
			DWORD wr = WaitForMultipleObjectsEx(2, events, FALSE, INFINITE, FALSE);
			if (wr == WAIT_OBJECT_0)
				m_abort.check();
		}
	}

protected:
private:
	virtual DWORD on_thread()
	{
		on_run();
		insync(m_sync);
		PostMessage(get_wnd(), MSG_END, NULL, NULL);
		return 0;
	};

	virtual t_uint32 get_styles() const {return WS_POPUP | WS_CLIPSIBLINGS| WS_CLIPCHILDREN  | WS_CAPTION;}
	virtual t_uint32 get_ex_styles() const {return WS_EX_DLGMODALFRAME;}

	virtual t_uint32 get_class_styles() const {return CS_DBLCLKS;}

	virtual const GUID & get_class_guid() 
	{
		// {7231C657-C7A5-457e-BFBD-9F09EED69705}
		static const GUID guid = 
		{ 0x7231c657, 0xc7a5, 0x457e, { 0xbf, 0xbd, 0x9f, 0x9, 0xee, 0xd6, 0x97, 0x5 } };
		return guid;
	}

#if 0
	class_data & get_class_data() const 
	{
		__implement_get_class_data_ex(_T("Dop_Progress"), _T("Progress"), false, 0, WS_POPUP | WS_CLIPSIBLINGS| WS_CLIPCHILDREN  | WS_CAPTION, WS_EX_DLGMODALFRAME, 0);
	}
#endif
	void resize();
	void on_size(t_size cx, t_size cy);
	void refresh_title_font();
	virtual LRESULT on_message(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	//HWND m_wnd_caption;
	HWND m_wnd_progress, m_wnd_button;
	gdi_object_t<HFONT>::ptr_t m_textfont, m_titlefont;
	t_size m_textfont_height, m_titlefont_height;
	COLORREF m_titlecolour;
	t_size m_range, m_position;
	pfc::array_t<t_uint32> m_steps;
	critical_section m_sync;
	pfc::string8 m_text, m_title;
	pfc::array_t<detail_entry> m_detail_entries;
	t_size m_flags;
	LARGE_INTEGER m_time_last_redraw;
	bool m_timer_active;
	abort_callback_impl m_abort;
	suspend_tracker m_suspend_tracker;
	t_size m_window_cx, m_window_cy;
};


class threaded_process_dummy_t : public threaded_process_v2_t
{
	virtual void on_run(){};
public:
	threaded_process_dummy_t() : threaded_process_v2_t("", NULL) {t_uint32 dummy=1;set_steps(&dummy, 1);};
};

class coinitialise_scope
{
public:
	coinitialise_scope(DWORD mode) {m_hr = CoInitializeEx(NULL, mode);}
	~coinitialise_scope() {if (SUCCEEDED(m_hr)) CoUninitialize();}
private:
	HRESULT m_hr;
};

class video_thumbailer_mediafoundation
{
	struct FormatInfo
	{
		UINT32          imageWidthPels;
		UINT32          imageHeightPels;
		BOOL            bTopDown;
		RECT            rcPicture;    // Corrected for pixel aspect ratio

		FormatInfo() : imageWidthPels(0), imageHeightPels(0), bTopDown(FALSE)
		{
			SetRectEmpty(&rcPicture);
		}
	};
	//MFPLAT.DLL
	typedef HRESULT (STDAPICALLTYPE * MFCreateAttributesPROC)(IMFAttributes** ppMFAttributes, UINT32 cInitialSize);
	typedef HRESULT (STDAPICALLTYPE * MFCreateMediaTypePROC)(IMFMediaType** ppMFType);
	typedef HRESULT (STDAPICALLTYPE * MFStartupPROC)(ULONG Version, DWORD dwFlags);
	typedef HRESULT (STDAPICALLTYPE * MFShutdownPROC)();

	//MFREADWRITE.DLL
	typedef HRESULT (STDAPICALLTYPE * MFCreateSourceReaderFromURLPROC)(LPCWSTR pwszURL, IMFAttributes *pAttributes, IMFSourceReader **ppSourceReader);
public:
	video_thumbailer_mediafoundation() : m_library_MFPLAT(NULL), m_library_MFREADWRITE(NULL), m_MFCreateAttributes(NULL),
		m_MFCreateMediaType(NULL), m_MFStartup(NULL), m_MFShutdown(NULL), m_MFCreateSourceReaderFromURL(NULL), m_MFInitialised(false)
	{
	}
	void deinitialise()
	{
		TRACK_CALL_TEXT("video_thumbailer_mediafoundation::deinitialise");
		m_pReader.release();
		HRESULT hr = S_OK;
		if (m_MFInitialised)
			hr = m_MFShutdown();
		cleanup_libraries();
		m_MFInitialised = false;
	}
	void ensure_initialised();
	void load_libraries();
	void cleanup_libraries();
	void run (const char * path, album_art_data_ptr & p_out);
	RECT CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR);
	LONGLONG GetDuration();
	void UpdateFormat();
private:
	mmh::ComPtr<IMFSourceReader> m_pReader;
	FormatInfo      m_format;
	HINSTANCE m_library_MFPLAT, m_library_MFREADWRITE;
	MFCreateAttributesPROC m_MFCreateAttributes;
	MFCreateMediaTypePROC m_MFCreateMediaType;
	MFStartupPROC m_MFStartup;
	MFShutdownPROC m_MFShutdown;
	MFCreateSourceReaderFromURLPROC m_MFCreateSourceReaderFromURL;
	bool m_MFInitialised;
};

class video_thumbailer_t 
{
public:
	video_thumbailer_t() : m_coinit(COINIT_MULTITHREADED) {};
	~video_thumbailer_t() 
	{
		TRACK_CALL_TEXT("video_thumbailer_t::~video_thumbailer_t");
		m_vtmf.deinitialise();
		const t_size delay = 1000;
		CoFreeUnusedLibrariesEx(delay, NULL);
		Sleep(delay+100);
		CoFreeUnusedLibrariesEx(delay, NULL);
	}
	bool create_video_thumbnail(const char * path, album_art_data_ptr & p_out);
	bool create_video_thumbnail_directshow(const char * path, album_art_data_ptr & p_out);
	//bool create_video_thumbnail_mediafoundation(const char * path, album_art_data_ptr & p_out);
private:
	coinitialise_scope m_coinit;
	video_thumbailer_mediafoundation m_vtmf;
};

BOOL
FileTimeToLocalFileTime2(
    __in  CONST FILETIME *lpFileTime,
    __out LPFILETIME lpLocalFileTime
    );

BOOL
LocalFileTimeToFileTime2(
    __in  CONST FILETIME *lpLocalFileTime,
    __out LPFILETIME lpFileTime
    );

t_filetimestamp g_iso_timestamp_string_to_filetimestamp(const char * p_str, bool b_read_time = true);


#endif _DOP_HELPERS_H_