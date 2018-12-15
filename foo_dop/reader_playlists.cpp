#include "stdafx.h"

#include "plist.h"

static const std::wregex file_name_regex(L"Playlist_[A-Z0-9]{1,16}.plist");

namespace ipod::tasks
{

DevicePlaylist read_plist_otg_playlist(const char* path, abort_callback & p_abort)
{
	file::ptr f;
	filesystem::g_open_read(f, path, p_abort);
	auto parser = PlistParserFromFile(path, p_abort);

	if (parser.m_root_object.is_empty())
		throw exception_io_unsupported_format();

	auto& dict = parser.m_root_object->get_value_strict<cfobject::objectType::kTagDictionary>();
	DevicePlaylist otg_playlist;

	otg_playlist.playlist_persistent_id = dict.get_child_int64_strict(L"playlistPersistentID");
	otg_playlist.saved_index = dict.get_child_int64_strict(L"savedIndex", 0);
	otg_playlist.name = pfc::stringcvt::string_utf8_from_wide(dict.get_child_string_strict(L"name"));
	otg_playlist.db_timestamp_mac_os_date = dict.get_child_int64_strict(L"dbTimestampMacOsDate");
	otg_playlist.playlist_deleted = dict.get_child_bool_strict(L"playlistDeleted", false);
	auto& pid_objects = dict.get_child_array_strict(L"trackPersistentIds");

	otg_playlist.track_persistent_ids.reserve(pid_objects.size());
	for (auto&& pid_object : pid_objects) {
		otg_playlist.track_persistent_ids.emplace_back(pid_object->get_value_strict<cfobject::objectType::kTagInt>());
	}

	return otg_playlist;
}

void load_database_t::load_device_playlists(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort)
{
	service_ptr_t<file> p_counts_file;

	pfc::string8 database_folder;
	p_ipod->get_database_path(database_folder);
	pfc::string8 path = database_folder << "/iTunes/";
	pfc::list_t<pfc::string_simple> file_paths;
	directory_callback_retrieveList callback(file_paths, true, false);
	pfc::string8 canonical_path;
	filesystem::g_get_canonical_path(path, canonical_path);
	filesystem::g_list_directory(canonical_path, callback, p_abort);

	std::vector<DevicePlaylist> read_playlists;

	for (size_t i{0}; i < file_paths.get_count(); ++i) {
		auto& file_path = file_paths[i];
		pfc::string_filename_ext name(file_path);
		pfc::stringcvt::string_wide_from_utf8 wname(name);

		if (std::regex_search(wname.get_ptr(), file_name_regex)) {
			read_playlists.emplace_back(read_plist_otg_playlist(file_path, p_abort));
			m_read_device_playlists.add_item(file_path);
		}
	}

	std::sort(
		read_playlists.begin(),
		read_playlists.end(),
		[](auto&& left, auto&& right) { return left.saved_index < right.saved_index; }
	);

	for (auto&& otg_playlist : read_playlists) {
		load_device_playlist(otg_playlist);
	}
}

void load_database_t::load_device_playlist(const DevicePlaylist& otg_playlist)
{
	if (otg_playlist.playlist_deleted) {
		size_t playlist_index;
		if (find_playlist_by_id(otg_playlist.playlist_persistent_id, playlist_index)) {
			m_playlists.remove_by_idx(playlist_index);
		}
		return;
	}

	const auto now = current_hfs_plus_timestamp();
	uint32_t index{};
	const bool is_new_playlist = !find_playlist_by_id(otg_playlist.playlist_persistent_id, index);
	auto playlist = is_new_playlist ? pfc::rcnew_t<itunesdb::t_playlist>() : m_playlists[index];
	playlist->id = static_cast<uint64_t>(otg_playlist.playlist_persistent_id);
	playlist->name = otg_playlist.name;
	if (is_new_playlist)
		playlist->timestamp = now;
	playlist->date_modified = now;

	playlist->items.remove_all();
	for (auto&& pid : otg_playlist.track_persistent_ids) {
		auto track = get_track_by_pid(pid);
		if (track.is_empty())
			continue;

		t_playlist_entry entry;
		entry.item_pid = pid;
		entry.timestamp = track->dateadded;
		entry.track_id = track->id;
		playlist->items.add_item(entry);
	}

	// In theory we would need to (re)generate VoiceOver files for playlist names, however
	// those are only for iPod shuffles.
	if (is_new_playlist) {
		m_playlists.add_item(playlist);
	}
}

void load_database_t::clean_up_device_playlists()
{
	abort_callback_dummy aborter;
	for (size_t i{0}; i < m_read_device_playlists.get_count(); ++i) {
		auto&& path = m_read_device_playlists[i];
		try {
			filesystem::g_remove(path, aborter);
		}
		catch (const exception_io&) {
			console::formatter formatter;
			formatter << "iPod manager: Failed to delete file: " << path;
		}
	}
}

} // namespace ipod::tasks
