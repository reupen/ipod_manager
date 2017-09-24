#include "stdafx.h"

#include "config.h"
#include "config_conversion.h"
#include "config_database.h"
#include "config_features.h"
#include "config_behaviour.h"
#include "config_ios.h"
#include "resource.h"


namespace settings
{
	namespace guids
	{
		// {5417B159-87B3-4be0-BCF8-456D72416800}
		const GUID use_ipod_sorting = 
		{ 0x5417b159, 0x87b3, 0x4be0, { 0xbc, 0xf8, 0x45, 0x6d, 0x72, 0x41, 0x68, 0x0 } };
		// {98D35366-9435-41af-B764-0EC8DC24506F}
		const GUID artist_mapping = 
		{ 0x98d35366, 0x9435, 0x41af, { 0xb7, 0x64, 0xe, 0xc8, 0xdc, 0x24, 0x50, 0x6f } };
		// {D6F31591-0BA6-4084-BA7E-A197F11E89C6}
		const GUID album_artist_mapping = 
		{ 0xd6f31591, 0xba6, 0x4084, { 0xba, 0x7e, 0xa1, 0x97, 0xf1, 0x1e, 0x89, 0xc6 } };
		// {68E11518-B578-44b1-B5F3-416B2401C36B}
		const GUID title_mapping = 
		{ 0x68e11518, 0xb578, 0x44b1, { 0xb5, 0xf3, 0x41, 0x6b, 0x24, 0x1, 0xc3, 0x6b } };
		// {557540D8-FE09-4708-820A-7EAFB56CEFCE}
		const GUID album_mapping = 
		{ 0x557540d8, 0xfe09, 0x4708, { 0x82, 0xa, 0x7e, 0xaf, 0xb5, 0x6c, 0xef, 0xce } };
		// {8FCC8FAA-EAF7-4bcb-AA30-C82193E9B236}
		const GUID genre_mapping = 
		{ 0x8fcc8faa, 0xeaf7, 0x4bcb, { 0xaa, 0x30, 0xc8, 0x21, 0x93, 0xe9, 0xb2, 0x36 } };
		// {5E541A59-CB5A-4e8a-AF7A-698885EE4884}
		const GUID composer_mapping = 
		{ 0x5e541a59, 0xcb5a, 0x4e8a, { 0xaf, 0x7a, 0x69, 0x88, 0x85, 0xee, 0x48, 0x84 } };
		// {BCF2ED9D-0F08-46de-BE95-BDA29D39E85E}
		const GUID soundcheck_adjustment = 
		{ 0xbcf2ed9d, 0xf08, 0x46de, { 0xbe, 0x95, 0xbd, 0xa2, 0x9d, 0x39, 0xe8, 0x5e } };
		// {CE281DAC-7293-4b8e-B0F3-E3242E11F611}
		static const GUID soundcheck_rgmode = 
		{ 0xce281dac, 0x7293, 0x4b8e, { 0xb0, 0xf3, 0xe3, 0x24, 0x2e, 0x11, 0xf6, 0x11 } };
		// {B9725A47-DD6C-4299-95FD-3F5092C7EDF8}
		const GUID conversion_command = 
		{ 0xb9725a47, 0xdd6c, 0x4299, { 0x95, 0xfd, 0x3f, 0x50, 0x92, 0xc7, 0xed, 0xf8 } };
		// {F999631C-72BB-43bc-9EE1-14265164619A}
		const GUID conversion_extension = 
		{ 0xf999631c, 0x72bb, 0x43bc, { 0x9e, 0xe1, 0x14, 0x26, 0x51, 0x64, 0x61, 0x9a } };
		// {C6F38BEA-3428-4ece-A49E-8C9B15EAC83D}
		const GUID sync_playlists = 
		{ 0xc6f38bea, 0x3428, 0x4ece, { 0xa4, 0x9e, 0x8c, 0x9b, 0x15, 0xea, 0xc8, 0x3d } };
		// {B9423456-8186-4d25-9B06-98CDDFDA1DC1}
		const GUID sync_library = 
		{ 0xb9423456, 0x8186, 0x4d25, { 0x9b, 0x6, 0x98, 0xcd, 0xdf, 0xda, 0x1d, 0xc1 } };
		// {E308CC48-B78A-468d-9810-4EBFE506771E}
		const GUID check_video = 
		{ 0xe308cc48, 0xb78a, 0x468d, { 0x98, 0x10, 0x4e, 0xbf, 0xe5, 0x6, 0x77, 0x1e } };
		// {AE232993-6EC8-47d3-878F-18A70E2CFCCE}
		const GUID extra_filename_characters = 
		{ 0xae232993, 0x6ec8, 0x47d3, { 0x87, 0x8f, 0x18, 0xa7, 0xe, 0x2c, 0xfc, 0xce } };
		// {BF20E95E-555E-4fd2-A0F1-7C8ACAFA66C7}
		const GUID advconfig_ipodbranch = 
		{ 0xbf20e95e, 0x555e, 0x4fd2, { 0xa0, 0xf1, 0x7c, 0x8a, 0xca, 0xfa, 0x66, 0xc7 } };
		// {8A99520C-6ECA-4f00-B786-2031ACEEDE55}
		const GUID artwork_sources = 
		{ 0x8a99520c, 0x6eca, 0x4f00, { 0xb7, 0x86, 0x20, 0x31, 0xac, 0xee, 0xde, 0x55 } };
		// {A4C90336-B7FD-4a9d-85B5-4D67CA7FDB46}
		const GUID add_artwork = 
		{ 0xa4c90336, 0xb7fd, 0x4a9d, { 0x85, 0xb5, 0x4d, 0x67, 0xca, 0x7f, 0xdb, 0x46 } };
		// {8FCE3573-92F3-4e83-8756-E2656E23775A}
		const GUID add_gapless = 
		{ 0x8fce3573, 0x92f3, 0x4e83, { 0x87, 0x56, 0xe2, 0x65, 0x6e, 0x23, 0x77, 0x5a } };
		// {86F331DD-C9EB-4f90-B89F-05662F87CE6D}
		const GUID convert_files = 
		{ 0x86f331dd, 0xc9eb, 0x4f90, { 0xb8, 0x9f, 0x5, 0x66, 0x2f, 0x87, 0xce, 0x6d } };
		// {6034422B-F70A-4c4a-90F7-90698BDB3561}
		const GUID compilation_mapping = 
		{ 0x6034422b, 0xf70a, 0x4c4a, { 0x90, 0xf7, 0x90, 0x69, 0x8b, 0xdb, 0x35, 0x61 } };
		// {353BB8F6-2A8D-4146-896B-42C79B238E91}
		const GUID sort_ipod_library = 
		{ 0x353bb8f6, 0x2a8d, 0x4146, { 0x89, 0x6b, 0x42, 0xc7, 0x9b, 0x23, 0x8e, 0x91 } };
		// {2E7D4C13-D41B-412f-BBD9-0CBB4AD77668}
		const GUID ipod_library_sort_script = 
		{ 0x2e7d4c13, 0xd41b, 0x412f, { 0xbb, 0xd9, 0xc, 0xbb, 0x4a, 0xd7, 0x76, 0x68 } };
		// {311DFC38-1E2C-479c-8AE6-012B9259811C}
		const GUID mobile_devices_enabled = 
		{ 0x311dfc38, 0x1e2c, 0x479c, { 0x8a, 0xe6, 0x1, 0x2b, 0x92, 0x59, 0x81, 0x1c } };
		// {BBB9BD1D-C5F6-48e1-B06A-F32CD4FC167C}
		const GUID numbers_last = 
		{ 0xbbb9bd1d, 0xc5f6, 0x48e1, { 0xb0, 0x6a, 0xf3, 0x2c, 0xd4, 0xfc, 0x16, 0x7c } };
		// {60D5E1E2-6813-4aa0-94C9-C54EA2A86E5E}
		const GUID use_fb2k_artwork = 
		{ 0x60d5e1e2, 0x6813, 0x4aa0, { 0x94, 0xc9, 0xc5, 0x4e, 0xa2, 0xa8, 0x6e, 0x5e } };
		// {BAB0BFCA-A4E2-49a6-B93F-CFE65B7388AF}
		const GUID devices_panel_autosend = 
		{ 0xbab0bfca, 0xa4e2, 0x49a6, { 0xb9, 0x3f, 0xcf, 0xe6, 0x5b, 0x73, 0x88, 0xaf } };
		// {F6E9A0D1-0F9B-44bc-8FF7-32F5D535746C}
		const GUID devices_panel_autosend_playlist = 
		{ 0xf6e9a0d1, 0xf9b, 0x44bc, { 0x8f, 0xf7, 0x32, 0xf5, 0xd5, 0x35, 0x74, 0x6c } };
		// {5526C8B2-94D6-424d-84C6-3750218606B4}
		const GUID conversion_use_custom_thread_count = 
		{ 0x5526c8b2, 0x94d6, 0x424d, { 0x84, 0xc6, 0x37, 0x50, 0x21, 0x86, 0x6, 0xb4 } };
		// {A144A02E-7514-4490-95C1-C365C0F211F1}
		const GUID conversion_custom_thread_count = 
		{ 0xa144a02e, 0x7514, 0x4490, { 0x95, 0xc1, 0xc3, 0x65, 0xc0, 0xf2, 0x11, 0xf1 } };
		// {59C8456A-638E-4cd7-8A0C-D16209CFF361}
		const GUID replaygain_processing_mode = 
		{ 0x59c8456a, 0x638e, 0x4cd7, { 0x8a, 0xc, 0xd1, 0x62, 0x9, 0xcf, 0xf3, 0x61 } };
		// {2623694B-8688-44d7-9209-ACBA4D75132C}
		const GUID date_added_mode = 
		{ 0x2623694b, 0x8688, 0x44d7, { 0x92, 0x9, 0xac, 0xba, 0x4d, 0x75, 0x13, 0x2c } };
		// {65DD5796-D646-4f86-B4C6-B4BC9CBC74DF}
		const GUID use_dummy_gapless_data = 
		{ 0x65dd5796, 0xd646, 0x4f86, { 0xb4, 0xc6, 0xb4, 0xbc, 0x9c, 0xbc, 0x74, 0xdf } };
		// {B1DA879B-CA0D-430f-A111-6167B85FB0D8}
		const GUID quiet_sync = 
		{ 0xb1da879b, 0xca0d, 0x430f, { 0xa1, 0x11, 0x61, 0x67, 0xb8, 0x5f, 0xb0, 0xd8 } };
		// {860CF4E5-C51E-417e-A0F5-D7FDB2C7A37C}
		const GUID sync_playlists_imported = 
		{ 0x860cf4e5, 0xc51e, 0x417e, { 0xa0, 0xf5, 0xd7, 0xfd, 0xb2, 0xc7, 0xa3, 0x7c } };
		// {3C5C6E24-E3D1-4e7b-B1EB-DC22A6D333C0}
		const GUID conversion_parameters = 
		{ 0x3c5c6e24, 0xe3d1, 0x4e7b, { 0xb1, 0xeb, 0xdc, 0x22, 0xa6, 0xd3, 0x33, 0xc0 } };
		// {94E25866-C24A-4ec1-943B-F48354E7D8A6}
		const GUID video_thumbnailer_enabled = 
		{ 0x94e25866, 0xc24a, 0x4ec1, { 0x94, 0x3b, 0xf4, 0x83, 0x54, 0xe7, 0xd8, 0xa6 } };
		// {FDE9EAA8-444D-4683-AD32-EAFE9D351052}
		const GUID reserved_diskspace = 
		{ 0xfde9eaa8, 0x444d, 0x4683, { 0xad, 0x32, 0xea, 0xfe, 0x9d, 0x35, 0x10, 0x52 } };
		// {B489E1F4-1477-4a97-B23F-805023F382DC}
		const GUID sync_eject_when_done = 
		{ 0xb489e1f4, 0x1477, 0x4a97, { 0xb2, 0x3f, 0x80, 0x50, 0x23, 0xf3, 0x82, 0xdc } };
		// {8B18BFA4-CF0F-4a58-8364-4B1B8E73E75E}
		const GUID conversion_use_bitrate_limit = 
		{ 0x8b18bfa4, 0xcf0f, 0x4a58, { 0x83, 0x64, 0x4b, 0x1b, 0x8e, 0x73, 0xe7, 0x5e } };
		// {3E63007C-E135-4619-B85F-0C5734F67BA7}
		const GUID conversion_bitrate_limit = 
		{ 0x3e63007c, 0xe135, 0x4619, { 0xb8, 0x5f, 0xc, 0x57, 0x34, 0xf6, 0x7b, 0xa7 } };
		// {D075A397-1320-48b8-8F49-4893BD0A25F1}
		const GUID comment_mapping = 
		{ 0xd075a397, 0x1320, 0x48b8, { 0x8f, 0x49, 0x48, 0x93, 0xbd, 0xa, 0x25, 0xf1 } };
		// {B1A80346-4574-4f87-8A1E-679072CD45CC}
		const GUID sort_artist_mapping = 
		{ 0xb1a80346, 0x4574, 0x4f87, { 0x8a, 0x1e, 0x67, 0x90, 0x72, 0xcd, 0x45, 0xcc } };
		// {06C6BFE9-F8D7-46df-B538-C427BD9C11A7}
		const GUID sort_album_artist_mapping = 
		{ 0x6c6bfe9, 0xf8d7, 0x46df, { 0xb5, 0x38, 0xc4, 0x27, 0xbd, 0x9c, 0x11, 0xa7 } };
		// {DCB43749-1AA8-47c8-B95A-1540AA2CC444}
		const GUID sort_album_mapping = 
		{ 0xdcb43749, 0x1aa8, 0x47c8, { 0xb9, 0x5a, 0x15, 0x40, 0xaa, 0x2c, 0xc4, 0x44 } };
		// {2B13563B-24C7-4f4b-AD69-48DBB61D7A20}
		const GUID sort_title_mapping = 
		{ 0x2b13563b, 0x24c7, 0x4f4b, { 0xad, 0x69, 0x48, 0xdb, 0xb6, 0x1d, 0x7a, 0x20 } };
		// {985983F5-ADE8-440f-BBBA-9CADECA09DA4}
		const GUID sort_composer_mapping = 
		{ 0x985983f5, 0xade8, 0x440f, { 0xbb, 0xba, 0x9c, 0xad, 0xec, 0xa0, 0x9d, 0xa4 } };
		// {CD3B34C2-AA2E-4c21-90DF-B414A9AA194D}
		const GUID allow_sort_fields = 
		{ 0xcd3b34c2, 0xaa2e, 0x4c21, { 0x90, 0xdf, 0xb4, 0x14, 0xa9, 0xaa, 0x19, 0x4d } };
		// {AC326636-E158-44c0-B1B5-C797D014390A}
		const GUID voiceover_title_mapping = 
		{ 0xac326636, 0xe158, 0x44c0, { 0xb1, 0xb5, 0xc7, 0x97, 0xd0, 0x14, 0x39, 0xa } };
		// {DD7F369B-B6F7-4fb7-884B-12C48E056761}
		const GUID sort_playlists = 
		{ 0xdd7f369b, 0xb6f7, 0x4fb7, { 0x88, 0x4b, 0x12, 0xc4, 0x8e, 0x5, 0x67, 0x61 } };
		// {3A217F13-F163-4ae1-BF34-93EE1EAF562C}
		const GUID encoder_list = 
		{ 0x3a217f13, 0xf163, 0x4ae1, { 0xbf, 0x34, 0x93, 0xee, 0x1e, 0xaf, 0x56, 0x2c } };
		// {655FF18A-69FA-45ee-9A12-3E08BF29D543}
		const GUID active_encoder = 
		{ 0x655ff18a, 0x69fa, 0x45ee, { 0x9a, 0x12, 0x3e, 0x8, 0xbf, 0x29, 0xd5, 0x43 } };
		// {2AD27553-117E-4aa9-A67E-1EF653C68684}
		const GUID encoder_imported = 
		{ 0x2ad27553, 0x117e, 0x4aa9, { 0xa6, 0x7e, 0x1e, 0xf6, 0x53, 0xc6, 0x86, 0x84 } };
		// {E3DCF37A-315F-4ce2-92AE-8BD30C454DB1}
		const GUID conversion_temp_files_folder = 
		{ 0xe3dcf37a, 0x315f, 0x4ce2, { 0x92, 0xae, 0x8b, 0xd3, 0xc, 0x45, 0x4d, 0xb1 } };

	}
	cfg_bool sort_playlists(guids::sort_playlists, true);
	cfg_bool conversion_use_bitrate_limit(guids::conversion_use_bitrate_limit, false);
	cfg_uint conversion_bitrate_limit(guids::conversion_bitrate_limit, 320);
	cfg_bool sort_ipod_library(guids::sort_ipod_library, true);
	cfg_bool use_fb2k_artwork(guids::use_fb2k_artwork, true);
	cfg_bool conversion_use_custom_thread_count(guids::conversion_use_custom_thread_count, false);
	cfg_uint conversion_custom_thread_count(guids::conversion_custom_thread_count, 1);
	cfg_uint date_added_mode(guids::date_added_mode, 0);
	cfg_string ipod_library_sort_script (guids::ipod_library_sort_script, "%album artist% - %album% - %disc% - %tracknumber% - %title%");
	cfg_bool use_ipod_sorting(guids::use_ipod_sorting, true);
	cfg_bool numbers_last(guids::numbers_last, true);
	cfg_bool devices_panel_autosend(guids::devices_panel_autosend, true);
	cfg_int_t<t_uint8> replaygain_processing_mode(guids::replaygain_processing_mode, 1);
	cfg_bool use_dummy_gapless_data(guids::use_dummy_gapless_data, true);
	cfg_bool quiet_sync(guids::quiet_sync, false);
	cfg_bool video_thumbnailer_enabled(guids::video_thumbnailer_enabled, true);
	cfg_bool sync_eject_when_done(guids::sync_eject_when_done, false);
	cfg_string devices_panel_autosend_playlist (guids::devices_panel_autosend_playlist, "iPod View");
	//cfg_bool check_video(guids::check_video, true);
	cfg_string artist_mapping (guids::artist_mapping, "");
	cfg_string album_artist_mapping (guids::album_artist_mapping, "");
	cfg_string album_mapping (guids::album_mapping, "");
	cfg_string title_mapping (guids::title_mapping, "");
	cfg_string composer_mapping (guids::composer_mapping, "");

	cfg_string sort_artist_mapping (guids::sort_artist_mapping, "");
	cfg_string sort_album_artist_mapping (guids::sort_album_artist_mapping, "");
	cfg_string sort_album_mapping (guids::sort_album_mapping, "");
	cfg_string sort_title_mapping (guids::sort_title_mapping, "");
	cfg_string sort_composer_mapping (guids::sort_composer_mapping, "");

	cfg_string genre_mapping (guids::genre_mapping, "");
	cfg_string compilation_mapping (guids::compilation_mapping, "$if($stricmp($meta(album artist),various artists),1,%ipod_compilation%)");
	cfg_string comment_mapping (guids::comment_mapping, "");
	cfg_string voiceover_title_mapping (guids::voiceover_title_mapping, "[%artist% - ]$if2(%title%,Unknown title)");
	cfg_string conversion_command (guids::conversion_command, "neroAacEnc.exe");
	cfg_string conversion_parameters (guids::conversion_parameters, "-q 0.40 -lc -ignorelength -if - -of %d");
	cfg_string conversion_extension (guids::conversion_extension, "m4a");
	cfg_string artwork_sources (guids::artwork_sources, "cover");
	cfg_int soundcheck_adjustment (guids::soundcheck_adjustment, 0);
	cfg_uint soundcheck_rgmode (guids::soundcheck_rgmode, 1);
	cfg_bool sync_library (guids::sync_library, true);
	cfg_bool add_artwork (guids::add_artwork, true);
	cfg_bool add_gapless (guids::add_gapless, true);
	cfg_bool convert_files (guids::convert_files, false);
	cfg_bool mobile_devices_enabled (guids::mobile_devices_enabled, false);
	cfg_bool sync_playlists_imported (guids::sync_playlists_imported, false);
	cfg_stringlist sync_playlists(guids::sync_playlists);
	cfg_bool allow_sort_fields (guids::allow_sort_fields, false);
	advconfig_branch_factory ipodbranch("iPod Manager", guids::advconfig_ipodbranch, advconfig_entry::guid_branch_tools, 0);
	advconfig_checkbox_factory check_video("Scan MP4s for video content", settings::guids::check_video, guids::advconfig_ipodbranch, 0, true); 
	advconfig_integer_factory extra_filename_characters("Number of extra filename characters allowed (the iPod wll not play files with paths over a certain length)", settings::guids::extra_filename_characters, guids::advconfig_ipodbranch, 0, 4, 0, 0x1000); 
	advconfig_integer_factory reserved_diskspace("Reserved disk space (thousandths of total capacity)", settings::guids::reserved_diskspace, guids::advconfig_ipodbranch, 0, 5, 0, 1000); 
	advconfig_string_factory conversion_temp_files_folder("Conversion temporary files storage folder path (folder must exist; leave blank for the default path)", settings::guids::conversion_temp_files_folder, guids::advconfig_ipodbranch, 6, ""); 
	cfg_conversion_presets_t encoder_list(guids::encoder_list);
	cfg_bool encoder_imported (guids::encoder_imported, false);
	cfg_uint active_encoder (guids::active_encoder, 0);
};





t_config_tab1 g_tab1;
t_config_tab2 g_tab2;
t_config_tab3 g_tab3;
t_config_tab4 g_tab4;
t_config_tab_conversion g_tab2a;

preferences_tab * g_tabs[] = {&g_tab1,&g_tab2a,&g_tab2,&g_tab4,&g_tab3};

// {7F06EE12-BD38-4640-9AAD-66978D689240}
static const GUID guid_child = 
{ 0x7f06ee12, 0xbd38, 0x4640, { 0x9a, 0xad, 0x66, 0x97, 0x8d, 0x68, 0x92, 0x40 } };

void g_update_conversion_prefs_encoders()
{
	g_tab2a.repopulate_encoder_list();
}

void g_sort_converstion_encoders()
{
	g_encoder_manager.sort_encoder_list();
	g_update_conversion_prefs_encoders();
}


cfg_int_t<t_uint32> cfg_child(guid_child,0);

class t_config_dop : public preferences_page
{
public:
	HWND child;
private:


	void make_child(HWND wnd)
	{
		//HWND wnd_destroy = child;
		if (child)
		{
			ShowWindow(child, SW_HIDE);
			DestroyWindow(child);
			child=0;
		}

		HWND wnd_tab = GetDlgItem(wnd, IDC_TAB1);
		
		RECT tab;
		
		GetWindowRect(wnd_tab,&tab);
		MapWindowPoints(HWND_DESKTOP, wnd, (LPPOINT)&tab, 2);
		
		TabCtrl_AdjustRect(wnd_tab,FALSE,&tab);
		
		unsigned count = tabsize(g_tabs);
		if (cfg_child >= count) cfg_child = 0;

		if (cfg_child < count && cfg_child >= 0)
		{
			child = g_tabs[cfg_child]->create(wnd);
		}
	
		//SetWindowPos(wnd_tab,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		
		if (child) 
		{
			//uxtheme_ptr uxtheme_api;
			//if (uxtheme_handle::g_create(uxtheme_api))
			{
				EnableThemeDialogTexture(child, ETDT_ENABLETAB);
			}
		}

		SetWindowPos(child, 0, tab.left, tab.top, tab.right-tab.left, tab.bottom-tab.top, SWP_NOZORDER);
		SetWindowPos(wnd_tab,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		
		ShowWindow(child, SW_SHOWNORMAL);
	}

	static BOOL CALLBACK g_DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		t_config_dop * p_this = NULL;
		switch(msg)
		{
		case WM_INITDIALOG:
			SetWindowLongPtr(wnd, DWL_USER, lp);
			p_this = reinterpret_cast<t_config_dop*>(lp);
			break;
		default:
			p_this = reinterpret_cast<t_config_dop*>(GetWindowLongPtr(wnd, DWL_USER));
			break;
		}
		if (p_this)
			return p_this->DialogProc(wnd, msg, wp, lp);
		return FALSE;
	}

	BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				HWND wnd_tab = GetDlgItem(wnd, IDC_TAB1);
				//uSendMessage(wnd_tab, TCM_SETMINTABWIDTH, 0, 35);
				unsigned n, count = tabsize(g_tabs);
				for (n=0; n<count; n++)
				{
					uTabCtrl_InsertItemText(wnd_tab, n, g_tabs[n]->get_name());
				}
				TabCtrl_SetCurSel(wnd_tab, cfg_child);
				make_child(wnd);
			}
			break;
		case WM_DESTROY:
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lp)->idFrom)
			{
			case IDC_TAB1:
				switch (((LPNMHDR)lp)->code)
				{
				case TCN_SELCHANGE:
					{
						cfg_child = TabCtrl_GetCurSel(GetDlgItem(wnd, IDC_TAB1));
						make_child(wnd);
					}
					break;
				}
				break;
			}
			break;
		
			
			case WM_PARENTNOTIFY:
				switch(wp)
				{
				case WM_DESTROY:
					{
						if (child && (HWND)lp == child) child = 0;
					}
					break;	
				}
				break;
		}
		return 0;
	}

public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_HOST,parent,g_DialogProc, (LPARAM)this);
	}
	const char * get_name() {return "iPod Manager";}
	static const GUID guid;

	virtual GUID get_guid()
	{
		return guid;
	}
	virtual GUID get_parent_guid() {return preferences_page::guid_tools;}
	virtual bool reset_query()	{return false;}
	virtual void reset() 
	{
		settings::use_ipod_sorting = true;
	};
	virtual bool get_help_url(pfc::string_base & p_out)
	{
		p_out = "http://yuo.be/wiki/dop:dop";
		if (cfg_child < tabsize(g_tabs) && cfg_child >= 0)
		{
			g_tabs[cfg_child]->get_help_url(p_out);
		}
		return true;
	}
	t_config_dop() : child(NULL) {};
};


// {772A1918-0590-4195-B5D2-85AEC218D279}
const GUID t_config_dop::guid = 
{ 0x772a1918, 0x590, 0x4195, { 0xb5, 0xd2, 0x85, 0xae, 0xc2, 0x18, 0xd2, 0x79 } };


preferences_page_factory_t<t_config_dop> g_conf;


