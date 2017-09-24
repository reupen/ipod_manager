#include "main.h"

#include "sync.h"
#include "gapless.h"
#include "maintenance.h"
#include "remove_files.h"
#include "send_files.h"

void g_video_tagger_run(metadb_handle_list_cref p_handles);

bool g_test_handles_on_ipod (bool b_on_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & p_data)
{
	//pfc::hires_timer timer;
	//timer.start();

	struct string_ptr
	{
		const char * str;
		t_size size;
	};

	bool ret = false;

	pfc::list_t<ipod_device_ptr_t> drives;
	g_drive_manager.get_drives(drives);
	pfc::array_staticsize_t<pfc::string8> root_paths(drives.get_count());
	pfc::array_staticsize_t<string_ptr> root_paths_ptrs(drives.get_count());

	if (!b_on_ipod) return drives.get_count() > 0;

	t_size i, count = drives.get_count(), j, itemcount = p_data.get_count();
	for (i=0; i<count; i++)
	{
		drives[i]->get_root_path(root_paths[i]);
		root_paths_ptrs[i].str = root_paths[i];
		root_paths_ptrs[i].size = root_paths[i].get_length();
	}

	string_ptr * p_root_paths_ptrs = root_paths_ptrs.get_ptr();

	for (j=0; j<itemcount; j++)
	{
		metadb_handle_ptr temp;
		p_data.get_item_ex(temp, j);
		const char * path = temp->get_path();
		for (i=0; i<count; i++)
		{
			if (!stricmp_utf8_partial(p_root_paths_ptrs[i].str, path, p_root_paths_ptrs[i].size))
			{
				ret = true;
				break;
			}
		}
	}
	//console::formatter() << timer.query();

	return ret;
}

// {B4DA537A-1390-4fd6-9C1B-B1E7EF2CAE64}
const GUID g_guid_contextmenu_group_ipod = 
{ 0xb4da537a, 0x1390, 0x4fd6, { 0x9c, 0x1b, 0xb1, 0xe7, 0xef, 0x2c, 0xae, 0x64 } };

service_factory_single_t<contextmenu_group_popup_impl> g_contextmenu_group_ipod(g_guid_contextmenu_group_ipod, contextmenu_groups::root, "iPod");

class contextmenu_items_dop : public contextmenu_item_simple
{
private:
	
	virtual GUID get_parent() {return g_guid_contextmenu_group_ipod;}
	virtual t_enabled_state get_enabled_state(unsigned p_index) {return p_index == 2 ? DEFAULT_OFF : DEFAULT_ON;}
	virtual unsigned get_num_items() {return 6;}
	virtual void get_item_name(unsigned p_index,pfc::string_base & p_out)
	{
		if (p_index==0)
			p_out="Remove from iPod";
		else if (p_index==1)
			p_out="Send to iPod";
		else if (p_index==2)
			p_out="Sync with iPod";
		else if (p_index==3)
			p_out="Update metadata";
		else if (p_index==4)
			p_out="Update gapless info";
		else if (p_index==5)
			p_out="Update artwork";
	}
	virtual bool context_get_display(unsigned p_index,metadb_handle_list_cref p_data,pfc::string_base & p_out,unsigned & p_displayflags,const GUID & p_caller) {
		PFC_ASSERT(p_index>=0 && p_index<get_num_items());
		get_item_name(p_index,p_out);
		return g_test_handles_on_ipod((p_index == 0 || p_index == 3 || p_index == 4 || p_index == 5), p_data);
	}
	virtual void get_item_default_path(unsigned p_index,pfc::string_base & p_out) {p_out = "iPod";}
	virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
	{
		if (p_data.get_count())
		{
			if (p_index==0)
			{
				ipod_remove_files::g_run(core_api::get_main_window(),p_data);
			}
			else if (p_index==1)
			{
				ipod_send_files::g_run(core_api::get_main_window(),p_data);
			}
#if 1
			else if (p_index==4)
			{
				ipod_scan_gapless::g_run(core_api::get_main_window(),p_data);
			}
#endif
			else if (p_index==2)
			{
				ipod_sync::g_run(core_api::get_main_window(),p_data, pfc::list_t<ipod_sync::t_playlist>(), false) ;
			}
			else if (p_index==3)
			{
				ipod_rewrite_library_t::g_run(core_api::get_main_window(),p_data);
			}
			else if (p_index==5)
			{
				ipod_update_artwork_library_t::g_run(core_api::get_main_window(),p_data);
			}
		}
	}
	virtual GUID get_item_guid(unsigned p_index) 
	{
		if (p_index==0)
		{
			// {3B4AD922-084E-492b-BC6C-680809B969AB}
			static const GUID guid = 
			{ 0x3b4ad922, 0x84e, 0x492b, { 0xbc, 0x6c, 0x68, 0x8, 0x9, 0xb9, 0x69, 0xab } };
			return guid;
		}
		else if (p_index==1)
		{
			// {07510E0C-AB09-47a7-BA61-F61A05854E5A}
			static const GUID guid = 
			{ 0x7510e0c, 0xab09, 0x47a7, { 0xba, 0x61, 0xf6, 0x1a, 0x5, 0x85, 0x4e, 0x5a } };
			return guid;
		}
		else if (p_index==4)
		{
			// {CA9C9B63-7573-4900-BCE0-2E43A4DA19AC}
			static const GUID guid = 
			{ 0xca9c9b63, 0x7573, 0x4900, { 0xbc, 0xe0, 0x2e, 0x43, 0xa4, 0xda, 0x19, 0xac } };
			return guid;
		}
		else if (p_index==3)
		{
			// {ECBEEFB5-6FF7-4f19-B85A-75BEB13FAF9E}
			static const GUID guid = 
			{ 0xecbeefb5, 0x6ff7, 0x4f19, { 0xb8, 0x5a, 0x75, 0xbe, 0xb1, 0x3f, 0xaf, 0x9e } };
			return guid;
		}
		else if (p_index==2)
		{
			// {DEF6DC8E-5BF7-4d4f-94CB-9FBD314F738F}
			static const GUID guid = 
			{ 0xdef6dc8e, 0x5bf7, 0x4d4f, { 0x94, 0xcb, 0x9f, 0xbd, 0x31, 0x4f, 0x73, 0x8f } };
			return guid;
		}
		else if (p_index==5)
		{
			// {5E6F1363-2086-4e22-906A-AF99E32E897B}
			static const GUID guid = 
			{ 0x5e6f1363, 0x2086, 0x4e22, { 0x90, 0x6a, 0xaf, 0x99, 0xe3, 0x2e, 0x89, 0x7b } };
			return guid;
		}
		return pfc::guid_null;
	}
	virtual bool get_item_description(unsigned p_index,pfc::string_base & p_out) 
	{
		if (p_index==0)
			p_out="Removes the specified files from your iPod"; 
		else if (p_index==1)
			p_out="Sends the specified files to the iPod"; 
		else if (p_index==3)
			p_out="Updates metadata stored in the iPod database for the specified file(s)"; 
		else if (p_index==4)
			p_out="Determines gapless data for the selected files for gapless playback on the iPod."; 
		else if (p_index==2)
			p_out="Syncs with iPod with the specified files. Warning: This will erase files from your iPod."; 
		else if (p_index==5)
			p_out="Updates artwork stored in the iPod database for the specified file(s)"; 
		return true;
	}

};

class contextmenu_items_dop2 : public contextmenu_item_simple
{
private:
	
	virtual GUID get_parent() {return contextmenu_groups::tagging;}
	virtual double get_sort_priority() {return 5;}
	virtual t_enabled_state get_enabled_state(unsigned p_index) {return contextmenu_item::DEFAULT_ON;}
	virtual unsigned get_num_items() {return 1;}
	virtual void get_item_name(unsigned p_index,pfc::string_base & p_out)
	{
		if (p_index==0)
			p_out="iPod tag editor";
	}
	virtual bool context_get_display(unsigned p_index,metadb_handle_list_cref p_data,pfc::string_base & p_out,unsigned & p_displayflags,const GUID & p_caller) {
		PFC_ASSERT(p_index>=0 && p_index<get_num_items());
		get_item_name(p_index,p_out);
		return p_index==0;
	}
	virtual void get_item_default_path(unsigned p_index,pfc::string_base & p_out) {p_out = "Tagging";}
	virtual void context_command(unsigned p_index,const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const GUID& p_caller)
	{
		if (p_data.get_count())
		{
			if (p_index==0)
			{
				g_video_tagger_run(p_data);
			}
		}
	}
	virtual GUID get_item_guid(unsigned p_index) 
	{
		if (p_index==0)
		{
			// {EA914F0A-8DE7-44d6-98E7-C7A29AB0B6B3}
			static const GUID guid = 
			{ 0xea914f0a, 0x8de7, 0x44d6, { 0x98, 0xe7, 0xc7, 0xa2, 0x9a, 0xb0, 0xb6, 0xb3 } };
			return guid;
		}
		return pfc::guid_null;
	}
	virtual bool get_item_description(unsigned p_index,pfc::string_base & p_out) 
	{
		if (p_index==0)
			p_out="Opens the video tagger. Note: this does not update the iPod database."; 
		return true;
	}

};

contextmenu_item_factory_t<contextmenu_items_dop> g_contextmenu_items_dop;
contextmenu_item_factory_t<contextmenu_items_dop2> g_contextmenu_items_dop2;
