#include "stdafx.h"

#include "smart_playlist_processor.h"
#include "browse.h"

template<typename t_item1, typename t_item2>
//pfc::rcptr_t <itunesdb::t_track>
static int g_compare_rcptr(const pfc::rcptr_t< t_item1 > & p_item1, const pfc::rcptr_t<t_item2> & p_item2) {
	return pfc::compare_t(&*p_item1, &*p_item2);
}

static int g_compare_track_ptr (const pfc::rcptr_t<t_track> & item1, const pfc::rcptr_t<t_track> &item2)
{
	return pfc::compare_t(&(*item1),&(*item2)); //BLAH
}

namespace ipod
{
namespace smart_playlist
{
	enum string_operators_t
	{
		string_is = (1<<24)|(1<<0),
		string_is_not = (1<<24)|(1<<0)|(1<<25),
		string_contains =(1<<24)|(1<<1),
		string_does_not_contain =(1<<24)|(1<<1)|(1<<25),
		string_begins_with =(1<<24)|(1<<2),
		string_ends_with =(1<<24)|(1<<3),
	};
	enum integer_operators_t
	{
		integer_is = (0<<24)|(1<<0),
		integer_is_not = (0<<24)|(1<<0)|(1<<25),
		integer_is_greater_than = (0<<24)|(1<<4),
		//integer_is greater than or equal to = (0<<24)|(1<<5),
		integer_is_less_than = (0<<24)|(1<<6),
		//integer_is less than or equal to = (0<<24)|(1<<7),
		integer_is_between = (0<<24)|(1<<8),
	};
	enum timestamp_operators_t
	{
		timestamp_is = (0<<24)|(1<<0),
		timestamp_is_not = (0<<24)|(1<<0)|(1<<25),
		timestamp_is_greater_than = (0<<24)|(1<<4),
		//integer_is greater than or equal to = (0<<24)|(1<<5),
		timestamp_is_less_than = (0<<24)|(1<<6),
		//integer_is less than or equal to = (0<<24)|(1<<7),
		timestamp_is_between = (0<<24)|(1<<8),
		timestamp_is_in_the_last = (0<<24)|(1<<9),
		timestamp_is_not_in_the_last = (0<<24)|(1<<9)|(1<<25),
	};
	bool g_test_track_generic_string(const char * str, const itunesdb::t_smart_playlist_rule & rule)
	{
		pfc::stringcvt::string_utf8_from_wide value(rule.string.get_ptr());
		if (rule.action == string_is)
			return !stricmp_utf8(str, value.get_ptr());
		else if (rule.action == string_is_not)
			return 0 != stricmp_utf8(str, value.get_ptr());
		else if (rule.action == string_contains)
			return 0 != strstr(string_lower(str), string_lower(value.get_ptr()));
		else if (rule.action == string_does_not_contain)
			return 0 == strstr(string_lower(str), string_lower(value.get_ptr()));
		else if (rule.action == string_begins_with)
			return !stricmp_utf8_partial(str, value.get_ptr());
		else if (rule.action == string_ends_with)
		{
			t_size len1 = strlen(str), len2=value.length();
			if (len2 <= len1)
				return !stricmp_utf8_partial(str+len1-len2, value.get_ptr());
			else
				return false;
		}
		return false;
	}
	bool g_test_track_generic_integer(t_uint64 value, const itunesdb::t_smart_playlist_rule & rule)
	{
		if (rule.action == integer_is)
			return rule.from_value*rule.from_units == value;
		else if (rule.action == integer_is_not)
			return rule.from_value*rule.from_units != value;
		else if (rule.action == integer_is_greater_than)
			return rule.from_value*rule.from_units < value;
		else if (rule.action == integer_is_less_than)
			return rule.from_value*rule.from_units > value;
		else if (rule.action == integer_is_between)
			return rule.from_value*rule.from_units <= value && value <= rule.to_value*rule.to_units;
		return false;
	}
	bool g_test_track_generic_bitmask(t_uint64 value, const itunesdb::t_smart_playlist_rule & rule)
	{
		if (rule.action == 0x00000400)
			return (value & rule.from_value) != 0;
		return false;
	}
	bool g_test_track_generic_timestamp(t_int64 value, const itunesdb::t_smart_playlist_rule & rule, t_uint32 currenttime)
	{
		t_int64 value_from_adjusted = value;// &~ rule.from_units;
		t_int64 value_to_adjusted = value;// &~ rule.to_units;
		if (rule.action == integer_is)
			return rule.from_value*rule.from_units == value_from_adjusted;
		else if (rule.action == timestamp_is_not)
			return rule.from_value*rule.from_units != value_from_adjusted;
		else if (rule.action == timestamp_is_greater_than)
			return rule.from_value*rule.from_units < value_from_adjusted;
		else if (rule.action == timestamp_is_less_than)
			return rule.from_value*rule.from_units > value_from_adjusted;
		else if (rule.action == timestamp_is_between)
			return rule.from_value*rule.from_units <= value_from_adjusted && value_to_adjusted <= rule.to_value*rule.to_units;
		else if (rule.action == timestamp_is_in_the_last)
			return rule.from_date*rule.from_units + currenttime/*_from_adjusted*/ < value;
		else if (rule.action == timestamp_is_not_in_the_last)
			return rule.from_date*rule.from_units + currenttime/*_from_adjusted*/ >= value;
		return false;
	}
	bool generator_t::g_test_track(const pfc::rcptr_t<t_track> & track, const itunesdb::t_smart_playlist_rule & rule)
	{
		switch (rule.field)
		{
		case itunesdb::smart_playlist_fields::album:
			return g_test_track_generic_string(track->album, rule);
		case itunesdb::smart_playlist_fields::album_artist:
			return g_test_track_generic_string(track->album_artist, rule);
		case itunesdb::smart_playlist_fields::artist:
			return g_test_track_generic_string(track->artist, rule);
		case itunesdb::smart_playlist_fields::bitrate:
			return g_test_track_generic_integer(track->bitrate, rule);
		case itunesdb::smart_playlist_fields::bpm:
			return g_test_track_generic_integer(track->bpm, rule);
		case itunesdb::smart_playlist_fields::category:
			return g_test_track_generic_string(track->category, rule);
		case itunesdb::smart_playlist_fields::comment:
			return g_test_track_generic_string(track->comment, rule);
		case itunesdb::smart_playlist_fields::compilation:
			return g_test_track_generic_integer(track->is_compilation, rule);
		case itunesdb::smart_playlist_fields::composer:
			return g_test_track_generic_string(track->composer, rule);
		case itunesdb::smart_playlist_fields::date_added:
			return g_test_track_generic_timestamp(track->dateadded, rule, m_timestamp);
		case itunesdb::smart_playlist_fields::date_modified:
			return g_test_track_generic_timestamp(track->lastmodifiedtime, rule, m_timestamp);
		case itunesdb::smart_playlist_fields::description:
			return g_test_track_generic_string(track->description, rule);
		case itunesdb::smart_playlist_fields::disc_number:
			return g_test_track_generic_integer(track->discnumber, rule);
		case itunesdb::smart_playlist_fields::genre:
			return g_test_track_generic_string(track->genre, rule);
		case itunesdb::smart_playlist_fields::grouping:
			return g_test_track_generic_string(track->grouping, rule);
		case itunesdb::smart_playlist_fields::kind:
			return g_test_track_generic_string(track->filetype, rule);
		case itunesdb::smart_playlist_fields::last_played:
			return g_test_track_generic_timestamp(track->lastplayedtime, rule, m_timestamp);
		case itunesdb::smart_playlist_fields::last_skipped:
			return g_test_track_generic_timestamp(track->last_skipped, rule, m_timestamp);
		case itunesdb::smart_playlist_fields::play_count:
			return g_test_track_generic_integer(track->play_count_user, rule);
		case itunesdb::smart_playlist_fields::podcast:
			return g_test_track_generic_integer(track->podcast_flag, rule);
		case itunesdb::smart_playlist_fields::rating:
			return g_test_track_generic_integer(track->rating, rule);
		case itunesdb::smart_playlist_fields::sample_rate:
			return g_test_track_generic_integer(track->samplerate / 0x10000, rule);
		case itunesdb::smart_playlist_fields::season_number:
			return g_test_track_generic_integer(track->season_number, rule);
		case itunesdb::smart_playlist_fields::size:
			return g_test_track_generic_integer(track->file_size_32, rule);
		case itunesdb::smart_playlist_fields::skip_count:
			return g_test_track_generic_integer(track->skip_count_user, rule);
		case itunesdb::smart_playlist_fields::sort_album:
			return g_test_track_generic_string(track->sort_album, rule);
		case itunesdb::smart_playlist_fields::sort_album_artist:
			return g_test_track_generic_string(track->sort_album_artist, rule);
		case itunesdb::smart_playlist_fields::sort_artist:
			return g_test_track_generic_string(track->sort_artist, rule);
		case itunesdb::smart_playlist_fields::sort_composer:
			return g_test_track_generic_string(track->sort_composer, rule);
		case itunesdb::smart_playlist_fields::sort_show:
			return g_test_track_generic_string(track->sort_show, rule);
		case itunesdb::smart_playlist_fields::sort_title:
			return g_test_track_generic_string(track->sort_title, rule);
		case itunesdb::smart_playlist_fields::time:
			return g_test_track_generic_integer(track->length, rule);
		case itunesdb::smart_playlist_fields::title:
			return g_test_track_generic_string(track->title, rule);
		case itunesdb::smart_playlist_fields::track_number:
			return g_test_track_generic_integer(track->tracknumber, rule);
		case itunesdb::smart_playlist_fields::tv_show:
			return g_test_track_generic_string(track->show, rule);
		case itunesdb::smart_playlist_fields::video_kind:
			return g_test_track_generic_bitmask(track->media_type, rule);
		case itunesdb::smart_playlist_fields::year:
			return g_test_track_generic_integer(track->year, rule);
		default:
			return false;
		};
	}

	enum limit_sort_values_t
	{
		random = 2,
		album = 4,
		artist = 5,
		genre = 7,
		title = 3,
		date_added_descending = 0x10,
		play_count_descending = 0x14,
		last_played_descending = 0x15,
		rating = 0x17,
	};

		void generator_t::run (const itunesdb::t_smart_playlist_data & p_data, const itunesdb::t_smart_playlist_rules & p_rules)
		{
			if (p_data.check_rules)
				process_rules(p_rules);
			else
				m_tracks = m_library.m_tracks;
			if (p_data.match_checked_only)
			{
				pfc::array_t<bool> mask;
				mask.set_count(m_tracks.get_count());
				t_size i, count = m_tracks.get_count();
				for (i=0; i<count; i++)
					mask[i] = !m_tracks[i]->is_user_disabled;
				m_tracks.remove_mask(mask.get_ptr());
			}
			if (p_data.check_limits)
				process_limits(p_data);
		}

		void generator_t::sort(t_uint32 order, bool b_desc)
		{
			mmh::Permutation permutation(m_tracks.get_count());

			switch (order)
			{
			case itunesdb::playlist_sort_orders::manual:
				break;
			case itunesdb::playlist_sort_orders::random:
				break;
			case itunesdb::playlist_sort_orders::title:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_title, b_desc);
				break;
			case itunesdb::playlist_sort_orders::album:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_album, b_desc);
				break;
			case itunesdb::playlist_sort_orders::artist:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_artist, b_desc);
				break;
			case itunesdb::playlist_sort_orders::bitrate:
				break;
			case itunesdb::playlist_sort_orders::genre:
				break;
			case itunesdb::playlist_sort_orders::kind:
				break;
			case itunesdb::playlist_sort_orders::date_modified:
				break;
			case itunesdb::playlist_sort_orders::track_number:
				break;
			case itunesdb::playlist_sort_orders::size:
				break;
			case itunesdb::playlist_sort_orders::time:
				break;
			case itunesdb::playlist_sort_orders::year:
				break;
			case itunesdb::playlist_sort_orders::sample_rate:
				break;
			case itunesdb::playlist_sort_orders::comment:
				break;
			case itunesdb::playlist_sort_orders::date_added:
				break;
			case itunesdb::playlist_sort_orders::equalizer:
				break;
			case itunesdb::playlist_sort_orders::composer:
				break;
			case itunesdb::playlist_sort_orders::unk2:
				break;
			case itunesdb::playlist_sort_orders::play_count:
				break;
			case itunesdb::playlist_sort_orders::last_played:
				break;
			case itunesdb::playlist_sort_orders::disc_number:
				break;
			case itunesdb::playlist_sort_orders::rating:
				break;
			case itunesdb::playlist_sort_orders::release_date:
				break;
			case itunesdb::playlist_sort_orders::bpm:
				break;
			case itunesdb::playlist_sort_orders::grouping:
				break;
			case itunesdb::playlist_sort_orders::category:
				break;
			case itunesdb::playlist_sort_orders::description:
				break;
			case itunesdb::playlist_sort_orders::show:
				break;
			case itunesdb::playlist_sort_orders::season:
				break;
			case itunesdb::playlist_sort_orders::episode_number:
				break;
			}

			m_tracks.reorder(permutation.data());
		}

		void generator_t::process_limits (const itunesdb::t_smart_playlist_data & p_data)
		{
			mmh::Permutation permutation(m_tracks.get_count());

			switch (p_data.limit_sort)
			{
			case random:
				{
					pfc::array_t<t_size> random;
					random.set_count(m_tracks.get_count());
					genrand_service::ptr api = genrand_service::g_create();
					api->seed(GetTickCount());

					t_size i, count = random.get_count();
					for (i=0; i<count; i++)
						random[i] = api->genrand(pfc_infinite);

					mmh::sort_get_permutation(random.get_ptr(), permutation, pfc::compare_t<t_size, t_size>, false);
				}
				break;
			case album:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_album, p_data.reverse_limit_sort != 0);
				break;
			case artist:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_artist, p_data.reverse_limit_sort != 0);
				break;
			case genre:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_genre, p_data.reverse_limit_sort != 0);
				break;
			case title:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_title, p_data.reverse_limit_sort != 0);
				break;
			case date_added_descending:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_date_added_descending, p_data.reverse_limit_sort != 0);
				break;
			case play_count_descending:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_play_count_descending, p_data.reverse_limit_sort != 0);
				break;
			case last_played_descending:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_last_played_descending, p_data.reverse_limit_sort != 0);
				break;
			case rating:
				mmh::sort_get_permutation(m_tracks.get_ptr(), permutation, t_track::g_compare_rating_descending, p_data.reverse_limit_sort != 0);
				break;
			}

			m_tracks.reorder(permutation.data());

			switch (p_data.limit_type)
			{
			case 3:
				m_tracks.truncate(p_data.limit_value);
				break;
			case 1:
			case 4:
				{
					t_size unit = 60*1000;
					if (p_data.limit_type == 4)
						unit *= 60;
					t_size limit = unit * p_data.limit_value;
					t_size counter=0, cumulative=0;
					while (counter < m_tracks.get_count())
					{
						cumulative += m_tracks[counter]->length;
						if (cumulative > limit)
							break;
						counter++;
					}
					m_tracks.truncate(counter);
				}
				break;
			case 2:
			case 5:
				{
					t_size unit = 1024*1024;
					if (p_data.limit_type == 5)
						unit *= 1024;
					t_size limit = unit * p_data.limit_value;
					t_size counter=0, cumulative=0;
					while (counter < m_tracks.get_count())
					{
						cumulative += m_tracks[counter]->file_size_32;
						if (cumulative > limit)
							break;
						counter++;
					}
					m_tracks.truncate(counter);
				}
				break;
			};
		}

		void generator_t::process_rules (const itunesdb::t_smart_playlist_rules & p_rules)
		{
			bool b_or = p_rules.rule_operator != 0;
			t_size i, count = p_rules.rules.get_count();
			for (i=0; i<count; i++)
			{
				if (p_rules.rules[i].field == smart_playlist_fields::playlist)
				{
					t_size index;
					if (m_library.find_playlist_by_id(p_rules.rules[i].from_value, index))
					{
						bool negate = p_rules.rules[i].action == ((0<<24)|(1<<0)|(1<<25));
						pfc::list_t <pfc::rcptr_t <itunesdb::t_track>, pfc::alloc_fast_aggressive > tracksPlaylist, tracksToAdd;
						g_playlist_get_tracks(m_library.m_playlists[index], m_library, tracksPlaylist, metadb_handle_list_t<pfc::alloc_fast_aggressive> ());
						if (negate)
						{
							tracksToAdd = m_library.m_tracks;
							pfc::array_t<bool> mask;
							mask.set_count(tracksToAdd.get_count());
							mask.fill(false);
							for (t_size j=0; j<tracksPlaylist.get_count(); j++)
							{
								t_size index = tracksToAdd.find_item(tracksPlaylist[j]);
								if (index != pfc_infinite) mask[index] = true;
							}
							tracksToAdd.remove_mask(mask.get_ptr());
						}
						else
							tracksToAdd = tracksPlaylist;
						if (b_or || i==0)
						{
							m_tracks.add_items(tracksToAdd);
						}
						else
						{
							t_size j, jcount = tracksToAdd.get_count();
							pfc::array_t<bool> mask;
							mask.set_count(m_tracks.get_count());
							mask.fill(true);

							for (j=0; j<jcount; j++)
							{
								t_size index_track;
								if (pfc_infinite != (index_track = m_tracks.find_item(tracksToAdd[j])))
									mask[index_track] = false;
							}
							m_tracks.remove_mask(mask.get_ptr());
						}
					}
				}
				else
				{
					pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > tracks (b_or || i==0 ? m_library.m_tracks : m_tracks);
					if (!b_or)
						m_tracks.remove_all();
					process_tracks (tracks, p_rules.rules[i]);
				}
				{
					mmh::remove_duplicates(m_tracks, g_compare_track_ptr);
				}
			}
		}


		void generator_t::process_tracks (const pfc::list_t< pfc::rcptr_t<itunesdb::t_track>, pfc::alloc_fast_aggressive > & p_tracks, const itunesdb::t_smart_playlist_rule & p_rule)
		{
			t_size i, count = p_tracks.get_count();
			for (i=0; i<count; i++)
			{
				if (g_test_track(p_tracks[i], p_rule))
					m_tracks.add_item(p_tracks[i]);
			}
		}
};
};