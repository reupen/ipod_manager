namespace dop
{
	enum media_type_t
	{
		/** subtype unused */
		mediatype_unknown, 
		/** subtype one of audio_subtype_t */
		mediatype_audio,   
		/** subtype one of video_subtype_t */
		mediatype_video    
	};
	enum audio_subtype_t
	{
		mediasubtype_song,
		mediasubtype_podcast,
		mediasubtype_audiobook,
	};
	enum video_subtype_t
	{
		mediasubtype_movie,
		mediasubtype_music_video,
		mediasubtype_tv_show,
		mediasubtype_video_podcast,
	};
	class playbackdata_playcount_t
	{
	public:
		bool m_valid;
		/** New/additional plays only! */
		t_size m_playcount;
		t_filetimestamp m_lastplayed_timestamp;
		playbackdata_playcount_t()
			: m_valid(false), m_playcount(0), m_lastplayed_timestamp(filetimestamp_invalid)
		{};
	};
	class playbackdata_skipcount_t
	{
	public:
		bool m_valid;
		/** New/additional skips only! */
		t_size m_skipcount;
		t_filetimestamp m_lastskipped_timestamp;
		playbackdata_skipcount_t()
			: m_valid(false), m_skipcount(0), m_lastskipped_timestamp(filetimestamp_invalid)
		{};
	};
	class playbackdata_rating_t
	{
	public:
		bool m_valid;
		/** Range: 0-100 */
		t_size m_rating;

		playbackdata_rating_t()
			: m_valid(false)
		{};

		/** Returns a rating in the range 0-5 */
		t_uint32 get_rating_scaled() { return m_rating/20; }
	};
	class playback_data_t
	{
	public:
		/** See media_type_t */
		t_size m_mediatype;
		/** See xxx_subtype_t */
		t_size m_mediasubtype;
		/**
		*   Handle to track on portable device
		*   Any metadata (if available) will already be loaded.
		*   Do NOT do any I/O on the file!
		*/
		metadb_handle_ptr m_track_on_device;
		/**
		*   Handle to track on local computer.
		*   May be invalid if not available
		*   May point to non-existant file (file was moved or is on another computer)
		*   Metadata not guaranteed to be loaded. Do a load info first if you need metadata.
		*/
		metadb_handle_ptr m_track_local;
		playbackdata_playcount_t m_playcountdata;
		playbackdata_skipcount_t m_skipcountdata;
		playbackdata_rating_t m_ratingdata;
		playback_data_t() : m_mediatype(mediatype_unknown), m_mediasubtype(NULL) {};
	};

	/**
	* \Deprecated as of API version 1.
	*
	* Called from main thread only
	*
	* \See portable_device_playbackdata_callback_v2
	*/
	class portable_device_playbackdata_callback : public service_base
	{
	public:
		typedef portable_device_playbackdata_callback self_t;
		typedef service_factory_single_t<self_t> factory;
		
		/**
		*   Called when playback data is being flushed from a portable device
		*   
		*	\param [in]		p_device_identifier		Identifies the type of device. \See dop::device_identifiers
		*	\param [in]		p_device_name			The personalised name of the device (e.g. "xxx xxx's iPod").
		*	\param [in]		p_playback_data			List of playback data changes for each track on the device
		*/
		virtual void on_playback_data(const GUID & p_device_identifier, const char * p_device_name, const pfc::list_base_const_t<const playback_data_t *> & p_playback_data) = 0;

		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(portable_device_playbackdata_callback);
	};

	/** 
	* Used to notify callback caller when processing of callback data had concluded
	*
	* Keep a reference to the object whilst your processing is in progress, and release it when complete.
	*/
	class portable_device_playbackdata_notifier_t : public service_base
	{
		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(portable_device_playbackdata_notifier_t);
	};

	/**
	* Called from main thread only
	*
	* Requires API version 1 or newer.
	*/
	class portable_device_playbackdata_callback_v2 : public service_base
	{
	public:
		typedef portable_device_playbackdata_callback_v2 self_t;
		typedef service_factory_single_t<self_t> factory_t;

		/**
		*   Retreives the name of your task which may be displayed to the user
		*   
		*	\param [out]	p_name					Receives name of your task. 
		*											Examples: "Updating media library with plays and ratings...",
		*											"Uploading plays to last.fm..."
		*/
		virtual void get_task_name(pfc::string_base & p_name) const = 0;
		
		/**
		*   Called when playback data is being flushed from a portable device
		*   
		*	\param [in]		p_device_identifier		Identifies the type of device. \See dop::device_identifiers
		*	\param [in]		p_device_name			The personalised name of the device (e.g. "xxx xxx's iPod").
		*	\param [in]		p_playback_data			List of playback data changes for each track on the device
		*	\param [in]		p_completion_notify		Notifies caller when you have finished your processing
		*/
		virtual void on_playback_data(const GUID & p_device_identifier, const char * p_device_name, 
			const pfc::list_base_const_t<const playback_data_t *> & p_playback_data,
			const portable_device_playbackdata_notifier_t::ptr & p_completion_notify) = 0;

		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(portable_device_playbackdata_callback_v2);
	};
}