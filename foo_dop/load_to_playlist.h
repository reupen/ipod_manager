#ifndef _LOAD_TO_DOP_H_
#define _LOAD_TO_DOP_H_

#include "actions_base.h"

class ipod_load_library_v2_t : public ipod_action_v2_t
{
public:
	DOP_IPOD_ACTION_ENTRY(ipod_load_library_v2_t);

protected:
	virtual void on_run();
	virtual void on_exit();
	ipod_load_library_v2_t()
		: m_failed(false), ipod_action_v2_t("Load iPod Library")
	{};
	bool m_failed;
	metadb_handle_list m_handles;
};

#endif //_LOAD_TO_DOP_H_