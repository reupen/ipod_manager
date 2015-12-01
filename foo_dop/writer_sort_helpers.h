#pragma once

bool g_get_sort_string_for_ipod(const char * str, pfc::string8 & p_out, bool ipod_sorting);
void g_get_sort_string_for_sorting(const char * str, pfc::string_simple_t<WCHAR> & p_out, bool ipod_sorting);
bool g_test_sort_string(const char * str);

class t_sort_entry
{
public:
	class string_valid_t : public pfc::string_simple_t<WCHAR>
	{
	public:
		bool m_valid;

		string_valid_t() : m_valid (false) {};
	};
	pfc::string_simple_t<WCHAR> artist;
	pfc::string_simple_t<WCHAR> title;
	pfc::string_simple_t<WCHAR> album;
	pfc::string_simple_t<WCHAR> genre;
	pfc::string_simple_t<WCHAR> composer;
	pfc::string_simple_t<WCHAR> show;
	pfc::string_simple_t<WCHAR> episode;
	pfc::string_simple_t<WCHAR> album_artist;
	string_valid_t sort_artist;
	string_valid_t sort_title;
	string_valid_t sort_album;
	string_valid_t sort_composer;
	string_valid_t sort_show;
	string_valid_t sort_episode;
	string_valid_t sort_album_artist;
	t_uint32 tracknumber;
	t_uint32 discnumber;
	t_uint32 episodenumber;
	t_uint32 seasonnumber;
	bool is_tv_show;

	//t_size index;

	t_sort_entry() : tracknumber(0), discnumber(0), episodenumber(0), seasonnumber(0), is_tv_show(false)/*, index(0)*/ {};

	static bool g_need_letter_table(t_size index)
	{
		return (index == library_index_types::title 
			|| index == library_index_types::album_disc_tracknumber_title
			|| index == library_index_types::composer_title
			|| index == library_index_types::artist_album_disc_tracknumber_title
			|| index == library_index_types::genre_artist_album_disc_tracknumber_title
			|| index == library_index_types::show_episode_1);
	}
	const WCHAR * get_letter_string(t_size index)
	{
		if (index == library_index_types::title) return sort_title.m_valid ? sort_title : title;
		if (index == library_index_types::album_disc_tracknumber_title) return sort_album.m_valid ? sort_album : album;
		if (index == library_index_types::composer_title) return sort_composer.m_valid ? sort_composer : composer;
		if (index == library_index_types::artist_album_disc_tracknumber_title) return sort_artist.m_valid ? sort_artist : artist;
		if (index == library_index_types::genre_artist_album_disc_tracknumber_title) return genre;
		if (index == library_index_types::show_episode_1) return sort_show.m_valid ? sort_show : show;
		return NULL;
	}


	void set_from_track(bool b_ipod_sorting, const t_track & track)
	{
		bool ipod_sorting = false;
		is_tv_show = (track.media_type & t_track::type_tv_show) != 0;
		if (track.artist_valid)
		{
			g_get_sort_string_for_sorting(track.artist, artist, ipod_sorting);
			if (track.sort_artist_valid)
			{
				g_get_sort_string_for_sorting(track.sort_artist, sort_artist, ipod_sorting);
				sort_artist.m_valid = true;
			}
		}

		if (track.album_artist_valid)
		{
			if (track.sort_album_artist_valid)
			{
				g_get_sort_string_for_sorting(track.sort_album_artist, sort_album_artist, ipod_sorting);
				sort_album_artist.m_valid= true;
			}
			g_get_sort_string_for_sorting(track.album_artist, album_artist, ipod_sorting);
		} else {album_artist = artist; sort_album_artist = sort_artist;}

		if (track.title_valid)
		{
			if (track.sort_title_valid)
			{
				g_get_sort_string_for_sorting(track.sort_title, sort_title, ipod_sorting);
				sort_title.m_valid = true;
			}
			g_get_sort_string_for_sorting(track.title, title, ipod_sorting);
		}

		if (track.album_valid)
		{
			if (track.sort_album_valid)
			{
				g_get_sort_string_for_sorting(track.sort_album, sort_album, ipod_sorting);
				sort_album.m_valid = true;
			}
			g_get_sort_string_for_sorting(track.album, album, ipod_sorting);
		}

		if (track.genre_valid)
		{
			g_get_sort_string_for_sorting(track.genre, genre, ipod_sorting);
		}

		if (track.composer_valid)
		{
			if (track.sort_composer_valid)
			{
				g_get_sort_string_for_sorting(track.sort_composer, sort_composer, ipod_sorting);
				sort_composer.m_valid = true;
			}
			g_get_sort_string_for_sorting(track.composer, composer, ipod_sorting);
		}

		if (track.show_valid)
		{
			if (track.sort_show_valid)
			{
				g_get_sort_string_for_sorting(track.sort_show, sort_show, ipod_sorting);
				sort_show.m_valid = true;
			}
			g_get_sort_string_for_sorting(track.show, show, ipod_sorting);
		}

		if (track.episode_valid)
		{
			g_get_sort_string_for_sorting(track.episode, episode, false);
		}

		seasonnumber = track.season_number;
		tracknumber = track.tracknumber;
		discnumber = track.discnumber;
		episodenumber = track.episode_number;
	}

	typedef int (__cdecl * t_comparefunc)(const t_sort_entry &, const t_sort_entry &);
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4, t_comparefunc p_compare5, t_comparefunc p_compare6>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		if (!ret)
			ret = p_compare5(item1, item2);
		if (!ret)
			ret = p_compare6(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4, t_comparefunc p_compare5>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		if (!ret)
			ret = p_compare5(item1, item2);
		return ret;
	}
};

#define compareFieldBody(name) \
	g_compare_sort_field ( item1.sort_##name##, item1.##name##, item2.sort_##name##, item2.##name##)

template <bool numbersLast>
class ipod_sort_helpers_t
{
public:
	static t_size g_get_first_character_index(const wchar_t * str)
	{
		const wchar_t * ptr = str;
		while (*ptr && (iswpunct(*ptr) || iswspace(*ptr))) ptr++;

		return ptr-str;
	}
	static wchar_t g_get_first_character(const wchar_t * str)
	{
		return str[g_get_first_character_index(str)];
	}
	static char g_get_first_character_ascii_upper(const wchar_t * str)
	{
		wchar_t upper = char_upper(g_get_first_character(str));

		pfc::string8 temp;

		return *mmh::g_convert_utf16_to_ascii(&upper, 1, temp);
	}
	static wchar_t g_get_first_character_upper(const wchar_t * str)
	{
		return uCharUpper(g_get_first_character(str));
	}
	static bool g_test_latin_number(wchar_t c)
	{
		return c >= '0' && c <= '9';
	}
	static bool g_test_first_character_latin_number(const wchar_t * str)
	{
		return g_test_latin_number(g_get_first_character(str));
	}
	static int g_compare_string_empties(const wchar_t * str1, const wchar_t * str2)
	{
		bool a = g_get_first_character(str1) !=0, b = g_get_first_character(str2) !=0;
		int ret = 0;
		if (a != b)
			ret = a ? 1 : -1;
		return ret;
	}
	static int g_compare_string_numbers(const wchar_t * str1, const wchar_t * str2)
	{
		bool a = iswalpha(g_get_first_character(str1)) !=0, b = iswalpha(g_get_first_character(str2)) !=0;
		int ret = 0;
		if (a != b)
			ret = a ? -1 : 1;
		return ret;
	}
	static int g_compare_string_empties(const wchar_t str1, const wchar_t str2)
	{
		bool a = str1 ==0, b = (str2) ==0;
		int ret = 0;
		if (a != b)
			ret = a ? 1 : -1;
		return ret;
	}
	static int g_compare_string_numbers(const wchar_t str1, const wchar_t str2)
	{
		bool a = iswdigit((str1)) !=0, b = iswdigit((str2)) !=0;
		int ret = 0;
		if (a != b)
			ret = a ? 1 : -1;
		return ret;
	}
	static int g_compare_string(const wchar_t * str1, const wchar_t * str2)
	{
		int ret = 0;
		if (numbersLast)
		{
			wchar_t firsta = g_get_first_character(str1), firstb= g_get_first_character(str2);
			ret = g_compare_string_empties(firsta, firstb);
			if (!ret)
				ret = g_compare_string_numbers(firsta, firstb);
		}
		if (!ret)
			//ret = lstrcmpi(str1, str2);
			ret = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, str1, -1, str2, -1) - 2;
		return ret;
	}

	static int g_compare_string(const pfc::string_simple_t<WCHAR> & str1, const pfc::string_simple_t<WCHAR> & str2)
	{
		return g_compare_string(str1.get_ptr(), str2.get_ptr());
	}

	static int g_compare_sort_field(const t_sort_entry::string_valid_t & str1sort, const pfc::string_simple_t<WCHAR> & str1,
		const t_sort_entry::string_valid_t & str2sort, const pfc::string_simple_t<WCHAR> & str2)
	{
		int ret = 0;
		bool b_sort = str1sort.m_valid || str2sort.m_valid;
		ret = g_compare_string(str1sort.m_valid ? str1sort.get_ptr() : str1.get_ptr(), str2sort.m_valid ? str2sort.get_ptr() : str2.get_ptr());
		if (ret == 0 && b_sort)
			ret = g_compare_string(str1.get_ptr(), str2.get_ptr());
		return ret;
	}
	static int g_compare_artist(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(artist);
	}
	static int g_compare_album_artist(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(album_artist);
	}
	static int g_compare_title(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(title);
	}
	static int g_compare_album(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(album);
	}
	static int g_compare_album_or_show(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return g_compare_sort_field ( 
			item1.is_tv_show ? item1.sort_show : item1.sort_album,
			item1.is_tv_show ? item1.show : item1.album, 
			item2.is_tv_show ? item2.sort_show : item2.sort_album, 
			item2.is_tv_show ? item2.show : item2.album
			);
	}
	static int g_compare_genre(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return g_compare_string(item1.genre, item2.genre);
	}
	static int g_compare_composer(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(composer);
	}
	template<typename t_item1, typename t_item2>
	static int g_compare_2(const t_item1 & p_item1, const t_item2 & p_item2)
	{
		int ret = pfc::compare_t(p_item1, p_item2);
		if (ret && (!p_item1 || !p_item2))
			return (p_item1) ? -1 : 1;
		return ret;
	}
	static int g_compare_tracknumber(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = pfc::compare_t(item1.discnumber, item2.discnumber);
		if (!ret) ret = pfc::compare_t(item1.tracknumber, item2.tracknumber);
		return ret;
	}
	static int g_compare_tracknumber2(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = pfc::compare_t(item1.discnumber, item2.discnumber);
		if (!ret) ret = g_compare_2(item1.tracknumber, item2.tracknumber);
		return ret;
	}
	static int g_compare_episodenumber(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = pfc::compare_t(item1.episodenumber, item2.episodenumber);
		//if (ret && (!item2.episodenumber || !item1.episodenumber))
		//	return item1.episodenumber ? -1 : 1;
		return ret;
	}
	static int g_compare_seasonnumber(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return pfc::compare_t(item1.seasonnumber, item2.seasonnumber);
	}
	static int g_compare_show(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		return compareFieldBody(show);
	}
	static int g_compare_show1(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = compareFieldBody(show);
		if (ret && (!(item1.show.length()) || !(item2.show.length())))
			return (item1.show.length()) ? -1 : 1;
		return ret;
	}
	typedef int (__cdecl * t_comparefunc)(const t_sort_entry &, const t_sort_entry &);
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4, t_comparefunc p_compare5, t_comparefunc p_compare6>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		if (!ret)
			ret = p_compare5(item1, item2);
		if (!ret)
			ret = p_compare6(item1, item2);
		return ret;
	}
	template <t_comparefunc p_compare1, t_comparefunc p_compare2, t_comparefunc p_compare3, t_comparefunc p_compare4, t_comparefunc p_compare5>
	static int g_compare_stack(/*t_compare p_compare1, t_compare p_compare2, t_compare p_compare3, */const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = p_compare1(item1, item2);
		if (!ret)
			ret = p_compare2(item1, item2);
		if (!ret)
			ret = p_compare3(item1, item2);
		if (!ret)
			ret = p_compare4(item1, item2);
		if (!ret)
			ret = p_compare5(item1, item2);
		return ret;
	}
	static int g_compare_episodeid(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		//		profiler(compeid);
		int ret = g_compare_string(item1.episode, item2.episode);

		if (ret)
		{
			int l1 = item1.episode.length();
			int l2 = item2.episode.length();
			if (!l1 || !l2)
				return l1 ? -1 : 1;
		}
		//else
		//	return pfc::compare_t(item1.index, item2.index);

		return ret;
	}
	static int g_compare_album_and_track(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = g_compare_album(item1, item2);
		if (!ret) ret = g_compare_tracknumber(item1, item2);
		return ret;
	}
	static int g_compare_album_and_track2(const t_sort_entry & item1, const t_sort_entry & item2)
	{
		int ret = g_compare_album(item1, item2);
		if (!ret) ret = g_compare_tracknumber2(item1, item2);
		return ret;
	}
};
