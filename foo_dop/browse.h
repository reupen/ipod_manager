#ifndef _DOP_BROWSE_H_
#define _DOP_BROWSE_H_

//int ListView_InsertColumnText(HWND wnd_lv, UINT index, TCHAR * text, int cx);
//LRESULT ListView_InsertItemText(HWND wnd_lv, UINT item, UINT subitem, TCHAR * text, bool b_set= false, LPARAM lp = 0);
//LRESULT ListView_InsertItemText(HWND wnd_lv, UINT item, UINT subitem, const char * text, bool b_set= false, LPARAM lp = 0);

class t_browser : public ipod_action_v2_t
{
	virtual void on_run();
	virtual void on_exit();
public:
	DOP_IPOD_ACTION_ENTRY(t_browser);
	bool m_debug;
private:
	t_browser()
		: m_failed(false), ipod_action_v2_t("Browse iPod", 
		threaded_process_v2_t::flag_show_text|threaded_process_v2_t::flag_show_progress_window|threaded_process_v2_t::flag_show_button, 
		true)
	{
		m_debug = (GetKeyState(VK_SHIFT) & KF_UP) != 0;
	};
	bool m_failed;

	//service_ptr_t<t_browser> m_this;

};

void g_playlist_get_tracks (const pfc::rcptr_t< itunesdb::t_playlist > & p_playlist, const ipod::tasks::load_database_t & p_library,  pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > & p_out, metadb_handle_list_t<pfc::alloc_fast_aggressive> & p_handles);

class tree_builder_base_t
{
public:
	tree_builder_base_t( const ipod::tasks::load_database_t & p_library, const pfc::rcptr_t< itunesdb::t_playlist> & root_playlist,
		const pfc::list_base_t< pfc::rcptr_t< itunesdb::t_playlist> > & playlists = pfc::list_t< pfc::rcptr_t<itunesdb::t_playlist> >())
		: m_root_playlist(root_playlist), m_library(p_library) {m_playlists_to_process.add_items(playlists);};

	virtual HTREEITEM insert_item_in_tree(HWND wnd_tree, const pfc::rcptr_t<itunesdb::t_playlist> & p_playlist, HTREEITEM ti_parent)=0;
	void run (HWND wnd_tree, HTREEITEM ti_parent = TVI_ROOT)
	{
		if (m_root_playlist.is_valid())
		{
			ti_parent = insert_item_in_tree(wnd_tree, m_root_playlist, TVI_ROOT);
		}

		while (m_playlists_to_process.get_count())
		{
			pfc::rcptr_t<itunesdb::t_playlist> p_playlist = m_playlists_to_process[0];
			m_playlists_to_process.remove_by_idx(0);

			HTREEITEM ti = insert_item_in_tree(wnd_tree, p_playlist, ti_parent);

			if (p_playlist->folder_flag)
				__add_folder_playlists_recur(wnd_tree, ti, p_playlist->id);
		}
	}
	void __add_folder_playlists_recur (HWND wnd_tree, HTREEITEM ti_parent, t_uint64 folderid)
	{
		class folders_to_process_t
		{
		public:
			HTREEITEM ti;
			t_uint64 id;
		};
		
		pfc::list_t<folders_to_process_t> folders_to_process;
		t_size i, count = m_playlists_to_process.get_count();
		bit_array_bittable mask(count);
		for (i=0; i<count; i++)
		{
			if (m_playlists_to_process[i]->parentid == folderid)
			{
				mask.set(i,true);

				HTREEITEM ti = insert_item_in_tree(wnd_tree, m_playlists_to_process[i], ti_parent); 
				if (m_playlists_to_process[i]->folder_flag)
				{
					folders_to_process_t temp;
					temp.ti = ti;
					temp.id = m_playlists_to_process[i]->id;
					folders_to_process.add_item(temp);
				}
			}
		}
		m_playlists_to_process.remove_mask(mask);
		t_size j, count_folders = folders_to_process.get_count();
		for (j=0; j<count_folders; j++)
		{
			__add_folder_playlists_recur(wnd_tree, folders_to_process[j].ti, folders_to_process[j].id);
		}
	}
protected:
	const ipod::tasks::load_database_t & m_library;
private:
	pfc::list_t< pfc::rcptr_t< itunesdb::t_playlist> > m_playlists_to_process;
	pfc::rcptr_t< itunesdb::t_playlist > m_root_playlist;
};

//#define PHOTO_BROWSER

#include "photo_browser.h"

#endif