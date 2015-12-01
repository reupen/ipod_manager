namespace dop 
{

	/** 
	* Provides control methods for the iPod manager component (foo_dop)
	*/
	class ipod_manager_control_t : public service_base
	{
	public:
		/**
		* iPod manager API version history:
		* 
		* 1: >= version 0.6.4.1
		* 0: <  version 0.6.4.1
		*
		*/
		enum api_version_t {api_version_current = 1};

		struct version_data_t 
		{
			t_uint32 m_major, 
				m_minor1,
				m_minor2,
				m_minor3;
		};

		/** 
		* Retrieves API version of the installed iPod manager component 
		*
		* \See api_version_t
		*/
		virtual api_version_t get_api_version() = 0;

		/**
		* Gets version of installed iPod manager component 
		*/
		virtual void get_component_version (version_data_t & p_out) = 0;

		/**
		* Gets version of installed iPod manager component, without failing if this service is not available
		*/
		static t_uint32 g_get_api_version()
		{
			t_uint32 ret = 0;
			try
			{
				ret = standard_api_create_t<ipod_manager_control_t>()->get_api_version();
			}
			catch (exception_service_not_found const &) {};

			return ret;
		}

		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(ipod_manager_control_t);
	};

}