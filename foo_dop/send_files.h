#ifndef _SEND_DOP_H_
#define _SEND_DOP_H_

class ipod_send_files : public ipod_write_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY_PARAM(ipod_send_files);

	ipod_add_files m_adder;
	t_field_mappings m_field_mappings;
	ipod::tasks::check_files_in_library_t m_checker;
	virtual void on_run();
	void on_exit();
private:
	ipod_send_files(const pfc::list_base_const_t<metadb_handle_ptr> & items)
		: m_failed(false), m_items(items), ipod_write_action_v2_t("Send Files to iPod")
	{};
	bool m_failed;
	metadb_handle_list m_items;
};

#endif //_SEND_DOP_H_