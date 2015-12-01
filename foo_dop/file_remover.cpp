#include "main.h"

void ipod_file_remover::run (ipod_device_ptr_ref_t p_ipod, const pfc::list_base_const_t<metadb_handle_ptr> & items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	////p_status.update_progress_subpart_helper(0);
	//p_status.force_update();
	
	t_size i, count = items.get_count();

	pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(2);
	string_format_metadb_handle_for_progress track_formatter;
	progress_details[0].m_label = "Item:";
	progress_details[1].m_label = "Remaining:";
	progress_details[0].m_value = "Processing...";
	progress_details[1].m_value = pfc::string8() << count;
	p_status.update_text_and_details("Removing files", progress_details);

	{

		t_size j, count_playlists = p_library.m_playlists.get_size();
		for (i=0; i<count; i++)
		{
			progress_details[0].m_value = track_formatter.run(items[i]);
			progress_details[1].m_value = pfc::string8() << count-i-1;
			p_status.update_detail_entries(progress_details);
			try
			{
				metadb_handle_ptr ptr;
				pfc::string8 can, speakableTracks;
				p_ipod->get_root_path(can);
				p_ipod->get_database_path(speakableTracks);
				speakableTracks << p_ipod->get_path_separator_ptr() << "Speakable" << p_ipod->get_path_separator_ptr() << "Tracks" << p_ipod->get_path_separator_ptr();
				if (!stricmp_utf8_max(items[i]->get_path(), can, can.length()))
				{
					t_size index;
					index = p_library.m_handles.find_item(items[i]);
					if (index != pfc_infinite)
					{
						if (p_library.m_tracks[index]->dshm_type_6)
							throw pfc::exception("Cannot remove tracks downloaded on-device. Please use the device to remove these.");
						try 
						{
							if (p_ipod->m_device_properties.m_Speakable)
							{
								filesystem::g_remove(pfc::string8() << speakableTracks << pfc::format_hex(p_library.m_tracks[index]->pid, 16) << ".wav", abort_callback_dummy());
							}
						}
						catch (exception_io_not_found const &) {};
						try 
						{
							filesystem::g_remove(items[i]->get_path(), abort_callback_dummy());
						}
						catch (exception_io_not_found)
						{
							for (j=0; j<count_playlists; j++)
								p_library.m_playlists[j]->remove_track_by_id(p_library.m_tracks[index]->id);
							p_library.m_library_playlist->remove_track_by_id(p_library.m_tracks[index]->id);
							p_library.remove_artwork(p_ipod, p_library.m_tracks[index]);
							p_library.m_tracks.remove_by_idx(index);
							p_library.m_handles.remove_by_idx(index);
							m_deleted_items.add_item(items[i]->get_path());
							throw;
						}
						for (j=0; j<count_playlists; j++)
							p_library.m_playlists[j]->remove_track_by_id(p_library.m_tracks[index]->id);
						p_library.m_library_playlist->remove_track_by_id(p_library.m_tracks[index]->id);
						p_library.remove_artwork(p_ipod, p_library.m_tracks[index]);
						p_library.m_tracks.remove_by_idx(index);
						p_library.m_handles.remove_by_idx(index);
						m_deleted_items.add_item(items[i]->get_path());
					}
					else throw pfc::exception("Failed to find file in database");
				}
				else throw pfc::exception("File is not located on iPod");
			}
			catch (exception_aborted &) {throw;}
			catch (const pfc::exception & ex)
			{
				m_errors.add_item(results_viewer::result_t(items[i], metadb_handle_ptr(), pfc::string8() << "Failed to remove file: " << ex.what()));
				//pfc::string8_fast_aggressive err; err << "Failed to remove file : " <<  items[i]->get_path() << "; Reason: " <<ex.what();
				//m_error_list.add_item(err);
				//console::print(err);
			}

			p_status.checkpoint();

			p_status.update_progress_subpart_helper(i,count);
		}
		p_library.repopulate_albumlist();
	}
}

void ipod_file_remover::run (ipod_device_ptr_ref_t p_ipod, const bool * items, ipod::tasks::load_database_t & p_library, threaded_process_v2_t & p_status, abort_callback & p_abort)
{
	p_status.update_text("Removing files");
	//p_status.update_progress_subpart_helper(0);
	//p_status.force_update();

	pfc::string8 speakableTracks;
	p_ipod->get_database_path(speakableTracks);
	speakableTracks  << p_ipod->get_path_separator_ptr() << "Speakable" << p_ipod->get_path_separator_ptr() << "Tracks" << p_ipod->get_path_separator_ptr();
	t_size i, count = p_library.m_tracks.get_count(), countFilesToRemove = 0;

	for (i=count; i; i--)
		if (items[i-1]) ++countFilesToRemove;

	pfc::array_staticsize_t<threaded_process_v2_t::detail_entry> progress_details(2);
	string_format_metadb_handle_for_progress track_formatter;
	progress_details[0].m_label = "Item:";
	progress_details[1].m_label = "Remaining:";
	progress_details[0].m_value = "Processing...";
	progress_details[1].m_value = pfc::string8() << countFilesToRemove;
	p_status.update_text_and_details("Removing files", progress_details);

	{

		t_size j, count_playlists = p_library.m_playlists.get_size();
		for (i=count; i; i--)
		{
			metadb_handle_ptr temp = p_library.m_handles[i-1];
			try
			{
				{
					if (items[i-1])
					{
						progress_details[0].m_value = track_formatter.run(p_library.m_handles[i-1]);
						progress_details[1].m_value = pfc::string8() << countFilesToRemove;
						p_status.update_detail_entries(progress_details);

						--countFilesToRemove;

						if (p_library.m_tracks[i-1]->dshm_type_6)
							throw pfc::exception("Cannot remove tracks downloaded on-device. Please use the device to remove these.");
						try 
						{
							if (p_ipod->m_device_properties.m_Speakable)
							{
								filesystem::g_remove(pfc::string8() << speakableTracks << pfc::format_hex(p_library.m_tracks[i-1]->pid, 16) << ".wav", abort_callback_dummy());
							}
						}
						catch (exception_io_not_found const &) {};
						try 
						{
							filesystem::g_remove(p_library.m_handles[i-1]->get_path(), abort_callback_dummy());
						}
						catch (exception_io_not_found)
						{
							for (j=0; j<count_playlists; j++)
								p_library.m_playlists[j]->remove_track_by_id(p_library.m_tracks[i-1]->id);
							p_library.m_library_playlist->remove_track_by_id(p_library.m_tracks[i-1]->id);
							p_library.remove_artwork(p_ipod, p_library.m_tracks[i-1]);
							p_library.m_tracks.remove_by_idx(i-1);
							p_library.m_handles.remove_by_idx(i-1);
							m_deleted_items.add_item(temp->get_path());
							throw;
						}
						for (j=0; j<count_playlists; j++)
							p_library.m_playlists[j]->remove_track_by_id(p_library.m_tracks[i-1]->id);
						p_library.m_library_playlist->remove_track_by_id(p_library.m_tracks[i-1]->id);
						p_library.remove_artwork(p_ipod, p_library.m_tracks[i-1]);
						p_library.m_tracks.remove_by_idx(i-1);
						p_library.m_handles.remove_by_idx(i-1);
						m_deleted_items.add_item(temp->get_path());
					}
					//else throw pfc::exception("Failed to find file in database");
				}
			}
			catch (exception_aborted &) {throw;}
			catch (const pfc::exception & ex)
			{
				m_errors.add_item(results_viewer::result_t(temp, metadb_handle_ptr(), pfc::string8() << "Failed to remove file: " << ex.what()));
				//pfc::string8_fast_aggressive err; err << "Failed to remove file : " <<  temp->get_path() << "; Reason: " <<ex.what();
				//m_error_list.add_item(err);
				//console::print(err);
			}

			p_status.checkpoint();
			p_status.update_progress_subpart_helper(count-i,count);
		}
		p_library.repopulate_albumlist();
	}
}
