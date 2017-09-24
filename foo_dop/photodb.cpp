#include "stdafx.h"

#include "ipod_manager.h"

extern bool g_Gdiplus_initialised;

	void g_create_IStream_from_datablock(const void * p_data, t_size p_size, mmh::ComPtr<IStream> & p_out)
	{
		HGLOBAL gd = GlobalAlloc(GMEM_FIXED|GMEM_MOVEABLE, p_size);
		if (gd == NULL)
			throw exception_win32(GetLastError());
		void * p_dataptr = GlobalLock(gd);
		if (p_dataptr == NULL)
			throw exception_win32(GetLastError());

		memcpy(p_dataptr, p_data, p_size);
		GlobalUnlock(gd);

		HRESULT hr = CreateStreamOnHGlobal (gd, TRUE, p_out);
		if (FAILED(hr))
			throw exception_win32(hr);
	}

	template <typename TArray>
	void g_create_IStream_from_datablock(const TArray & p_data, mmh::ComPtr<IStream> & p_out)
	{
		g_create_IStream_from_datablock(p_data.get_ptr(), p_data.get_size(), p_out);
	}


enum t_bitmap_format
{
	format_uyvy,
	format_rgb565_le,
	format_rgb565_be,
	format_rgb565_be_90,
};

bool g_exists_directory(const TCHAR * path_win32)
{
	DWORD att = GetFileAttributes(path_win32);
	return (att != INVALID_FILE_ATTRIBUTES && (att & FILE_ATTRIBUTE_DIRECTORY));	
}

bool g_exists_directory(const char * path_win32)
{
	return g_exists_directory(pfc::stringcvt::string_os_from_utf8(path_win32).get_ptr());
}

namespace photodb
{
	t_uint32 g_translate_associated_format(t_uint64 assoc_format)
	{
		t_uint32 ret = 0;
		if (assoc_format == 0)
			ret = (~0);
		if ( assoc_format & 1 )
		  ret |= 0xFFFFF39D;
		if ( assoc_format & 2 )
		  ret |= 0xC62;
		if ( assoc_format & 4 )
		  ret |= 2;
		if ( assoc_format & 8 )
		  ret |= 0x20;
		if ( assoc_format & 0x10 )
		  ret |= 0x40;
		if ( assoc_format & 0x40 )
		  ret |= 0x400;
		if ( assoc_format & 0x80 )
		  ret |= 0x800;
		if ( assoc_format & 0x100 )
		  ret |= 4;
		if ( assoc_format & 0x200 )
		  ret |= 0x8000;
		if ( assoc_format & 0x400 )
		  ret |= 8;
		if ( assoc_format & 0x800 )
		  ret |= 0x100000;
		if ( assoc_format & 0x1000 )
		  ret |= 0x200000;
		if ( assoc_format & 0x2000 )
		  ret |= 0x400000;
		return ret;
	}
	t_uint32 t_datafile::get_next_ii_id()
	{
		mmh::Permutation permutation;
		t_size count = image_list.get_count();
		permutation.set_size(count);

		mmh::sort_get_permutation(image_list, permutation, t_image_item::g_compare_id, false);

		return count ? image_list[permutation[count-1]].image_id +1 : 0x64;
	}
	bool t_datafile::find_by_dbid(t_size & index, t_uint64 dbid)
	{
		/*pfc::array_t<t_size> permutation;
		t_size count = image_list.get_count();
		permutation.set_size(count);
		order_helper::g_fill(permutation);
		g_sort_get_permutation(image_list, permutation, t_image_item::g_compare_id);*/
		t_size i, count = image_list.get_count();
		for (i=0; i<count; i++)
		{
			if (image_list[i].song_dbid == dbid)
			{
				index=i;
				return true;
			}
		}
		return false;
	}
	bool t_datafile::find_by_image_id(t_size & index, t_uint32 id)
	{
		/*pfc::array_t<t_size> permutation;
		t_size count = image_list.get_count();
		permutation.set_size(count);
		order_helper::g_fill(permutation);
		g_sort_get_permutation(image_list, permutation, t_image_item::g_compare_id);*/
		t_size i, count = image_list.get_count();
		for (i=0; i<count; i++)
		{
			if (image_list[i].image_id == id)
			{
				index=i;
				return true;
			}
		}
		return false;
	}
	void t_datafile::truncate_thumb_file(ipod_device_ptr_ref_t p_ipod, t_uint32 fmt_id, const char * p_thumb, abort_callback & p_abort)
	{
		t_size j, jcount = p_ipod->m_device_properties.m_artwork_formats.get_count();
		for (j=0; j<jcount; j++)
			if (p_ipod->m_device_properties.m_artwork_formats[j].m_format_id == fmt_id)
			{
				truncate_thumb_file(p_ipod, p_ipod->m_device_properties.m_artwork_formats[j], p_thumb, p_abort);
				break;
			}

	}
	void t_datafile::truncate_thumb_file(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, const char * p_thumb, abort_callback & p_abort)
	{
		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork";// << p_ipod->get_path_separator_ptr();

		t_uint32 correlation_id=fmt.m_format_id;
		t_uint32 size=fmt.get_size();

		{
			pfc::string8 location = path, temp = p_thumb;
			temp.replace_byte(':',p_ipod->get_path_separator());

			location << temp;

			pfc::list_t<t_image_name> list;
			t_size i, count = image_list.get_count(), j;
			for (i=0; i<count; i++)
			{
				t_size ncount = image_list[i].image_names.get_count();
				for (j=0; j<ncount; j++)
					if (!stricmp_utf8(image_list[i].image_names[j].location, p_thumb))
						list.add_item(image_list[i].image_names[j]);
			}
			list.sort_t(t_image_name::g_compare_offset);

			t_filestats stats;
			stats.m_size = 0;
			bool dummy;

			try {
			if (filesystem::g_exists(location, p_abort))
				filesystem::g_get_stats(location, stats, dummy, p_abort);

			t_filesize data_end = 0;

			count = list.get_count();

			if (count)
			{
				i = count-1;
				data_end = list[i].file_offset + fmt.get_size();
			}
			if (data_end < stats.m_size)
			{
				file::ptr file;
				filesystem::g_open(file, location, filesystem::open_mode_write_existing, p_abort);
				file->truncate(data_end, p_abort);

				t_size k, kcount = file_list.get_count();
				for (k=0; k<kcount; k++)
					if (file_list[k].correlation_id == fmt.m_format_id)
						file_list[k].file_size = data_end;
			}
			} catch (exception_io const &) {};

		}
	}
	void t_datafile::find_empty_block(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, t_size padded_size, t_uint32 & offset, t_uint32 & fileno, const pfc::list_t<t_image_name> & pending_images, /*t_filesize & dataend,*/ abort_callback & p_abort)
	{
		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork" << p_ipod->get_path_separator_ptr();
		t_size fno = 1;
		t_uint32 correlation_id=fmt.m_format_id;
		t_uint32 size=padded_size;//fmt.get_size();
		//dataend = 0;

		do
		{
			pfc::string8 location = path, filename, temp;
			temp << "F" << correlation_id << "_" << fno << ".ithmb";
			location << temp;
			filename << ":" << temp;

			pfc::list_t<t_image_name> list;
			t_size i, count = image_list.get_count(), j;
			for (i=0; i<count; i++)
			{
				t_size ncount = image_list[i].image_names.get_count();
				for (j=0; j<ncount; j++)
					if (!stricmp_utf8(image_list[i].image_names[j].location, filename))
						list.add_item(image_list[i].image_names[j]);
			}

			for (t_size j=0, jcount=pending_images.get_count(); j<jcount; j++)
				if (!stricmp_utf8(pending_images[j].location, filename))
					list.add_item(pending_images[j]);

			list.sort_t(t_image_name::g_compare_offset);

			t_filestats stats;
			stats.m_size = 0;
			bool dummy;
			//console::formatter() << location;
			if (filesystem::g_exists(location, p_abort))
				filesystem::g_get_stats(location, stats, dummy, p_abort);

			t_filesize data_end = stats.m_size;

			count = list.get_count();
			//dataend = count ? list[count-1].file_offset + list[count-1].file_size : 0;
			if (count)
			{
				for (i=0; i<count; i++)
				{
					t_uint32 end = list[i].file_offset + list[i].file_size;
					if (fmt.m_offset_alignment && (end%fmt.m_offset_alignment))
						end += fmt.m_offset_alignment - (end%fmt.m_offset_alignment);
					if (i+1 >= count)
					{
						data_end = end;
					}
					else if (end + size < list[i+1].file_offset)
					{
						if (end + size > 500*1024*1024) break;
						offset = end;
						fileno = fno;
						return;
					}

				}
			}
			if (fmt.m_offset_alignment && (data_end%fmt.m_offset_alignment))
				data_end += fmt.m_offset_alignment - (data_end%fmt.m_offset_alignment);
			if (data_end + size <= 500*1024*1024)
			{
				offset = (t_uint32) data_end;
				fileno = fno;
				return;
			}
		}
		while (++fno < 10);
		throw pfc::exception("too many ithmb files.");
	}

	void t_datafile::initialise_artworkdb(ipod_device_ptr_ref_t p_ipod)
	{
		next_ii_id = 0x64;
		unk7 = 0;
		//pfc::ptr_list_t<const t_bitmap_info> bitmaps;
		//g_get_artwork_bitmaplist(p_ipod, bitmaps);
		pfc::dynamic_assert(file_list.get_count() == 0, "file_list.count != 0");
		t_size i, count = p_ipod->m_device_properties.m_artwork_formats.get_count();
		for (i=0; i<count; i++)
		{
			//if (p_ipod->m_device_properties.m_artwork_formats[i].m_associated_format == 0)
			{
				t_file_item item;
				item.correlation_id = p_ipod->m_device_properties.m_artwork_formats[i].m_format_id;
				item.file_size = p_ipod->m_device_properties.m_artwork_formats[i].get_size();
				file_list.add_item(item);
			}
		}

	}
	void t_datafile::remove_by_image_id(t_uint32 iiid)
	{
		t_size i = image_list.get_count();
		for (; i; i--)
			if (image_list[i-1].image_id == iiid)
			{
				image_list.remove_by_idx(i-1);
				break;
			}
	}
	void t_datafile::remove_by_dbid(t_uint64 dbid)
	{
		t_size i = image_list.get_count();
		for (; i; i--)
			if (image_list[i-1].song_dbid == dbid) image_list.remove_by_idx(i-1);
	}
	void t_datafile::remove_by_dbid_v2(ipod_device_ptr_ref_t p_ipod, t_uint64 dbid)
	{
		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork";
		abort_callback_impl p_abort;

		t_size index, z = image_list.get_count();
		bool b_found = false;
		for (index=0; index<z; index++)
			if (image_list[index].song_dbid == dbid) 
			{
				b_found = true;
				break;
			}

		if (!b_found) return;

		photodb::t_image_item & image = image_list[index];

		t_size k, count = image.image_names.get_count();

		for (k=0; k<count; k++)
		{
			pfc::list_t<t_image_name> list;
			t_size i, icount = image_list.get_count(), j;
			for (i=0; i<icount; i++)
			{
				t_size ncount = image_list[i].image_names.get_count();
				for (j=0; j<ncount; j++)
					if (!stricmp_utf8(image_list[i].image_names[j].location, image.image_names[k].location))
						list.add_item(image_list[i].image_names[j]);
			}
			list.sort_t(t_image_name::g_compare_offset);

			pfc::string8 file_open = image.image_names[k].location, location_open = path;
			file_open.replace_byte(':', p_ipod->get_path_separator());
			location_open << file_open;

			t_size l = list.get_count();
			if (l)
			{
				l--;

				if (image.image_names[k].file_size != list[l].file_size)
					throw exception_io_unsupported_format("Failed to remove artwork: Malformed artwork database");

				pfc::array_t<t_uint8> data;
				data.set_count(list[l].file_size);

				service_ptr_t<file> p_file;
				filesystem::g_open(p_file, location_open, filesystem::open_mode_write_existing, p_abort);
				p_file->seek(list[l].file_offset, p_abort);
				p_file->read(data.get_ptr(), list[l].file_size, p_abort);

				p_file->seek(image.image_names[k].file_offset, p_abort);
				p_file->write(data.get_ptr(), list[l].file_size, p_abort);
				list[l].file_offset = image.image_names[k].file_offset;
				p_file->truncate(list[l].file_offset, p_abort);
			}
			

		}
		image_list.remove_by_idx(index);
	}
	void g_check_gdiplus_ret(Gdiplus::Status ret)
	{
		if (ret != Gdiplus::Ok)
			throw pfc::exception(pfc::string8() << "Gdiplus error (" << ret << ")");
	}
	void g_check_gdiplus_ret(Gdiplus::Status ret, const char * func)
	{
		if (ret != Gdiplus::Ok)
			throw pfc::exception(pfc::string8() << "Gdiplus error (function: " << func << ", code: " << ret << ")");
	}

	void t_datafile::finalise_add_artwork_v2(ipod_device_ptr_ref_t p_ipod, abort_callback & p_abort)
	{
		t_size i, count = thumb_info_list.get_count();
		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork";
		for (i=0; i<count; i++)
		{
			pfc::string8 temp = thumb_info_list[i].filename, openpath = path;
			temp.replace_byte(':', p_ipod->get_path_separator());
			openpath << temp;
			service_ptr_t<file> p_file;
			try {
				filesystem::g_open(p_file, openpath, filesystem::open_mode_write_existing, p_abort);
				p_file->truncate(thumb_info_list[i].dataend, p_abort);

				t_size k, kcount = file_list.get_count();
				for (k=0; k<kcount; k++)
					if (file_list[k].correlation_id == thumb_info_list[i].format_id)
						file_list[k].file_size = thumb_info_list[i].dataend;
			}
			catch (const exception_io &) {};
		}
		thumb_info_list.remove_all();
	}

	class formatted_image
	{
	public:
		pfc::array_t<t_uint8> data;
		INT x, y;
		UINT cx, cy;
		t_size m_data_size;
		t_size m_padded_size;
	};

	void _check_hresult (HRESULT hr) {if (FAILED(hr)) throw pfc::exception(pfc::string8() << "WIC error: " << format_win32_error(hr));}

	Gdiplus::PixelFormat g_get_gdiplus_pixelformat(t_uint32 apple_pixel_format)
	{
		switch (apple_pixel_format)
		{
		case 'L555':
			return PixelFormat16bppRGB555;
		case 'L565':
		default:
			return PixelFormat16bppRGB565;
		case 'jpeg':
			return PixelFormat24bppRGB;
		};
	}
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
		UINT  num = 0;          // number of image encoders
		UINT  size = 0;         // size of the image encoder array in bytes


		Gdiplus::GetImageEncodersSize(&num, &size);
		if(size == 0)
			return -1;  // Failure

		pfc::array_staticsize_t<t_uint8> buffer(size);
		Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)buffer.get_ptr();

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for(UINT j = 0; j < num; ++j)
		{
			if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				return j;  // Success
			}    
		}

		return -1;  // Failure
	}
	void g_format_image (Gdiplus::Bitmap & image, const artwork_format_t & fmt, formatted_image & p_out)
	{
		const t_uint32 height = fmt.m_render_height - fmt.m_top_inset - fmt.m_bottom_inset,
			width = fmt.m_render_width - fmt.m_left_inset - fmt.m_right_inset;

		t_uint32 source_width = image.GetWidth(), source_height = image.GetHeight();

		double ar_source = (double)source_width / (double)source_height;
		double ar_dest = (double)width / (double)height;

		INT x=0, y=0; UINT cx=width, cy=height;

		if (fmt.m_crop)
		{
			if (ar_dest > ar_source)
				cy = (INT)floor((double)width / ar_source);
			else if (ar_dest < ar_source)
				cx = (INT)floor((double)height * ar_source);
		}
		else
		{
			if (ar_dest < ar_source)
				cy = (INT)floor((double)width / ar_source);
			else if (ar_dest > ar_source)
				cx = (INT)floor((double)height * ar_source);
			if ( (height - cy) % 2) cy++;
			if ( (width - cx) % 2) cx++;
		}

		x += int(width-cx)/2;
		y += int(height-cy)/2;

		int safe_x = max(x,0), safe_y = max(y,0);
		unsigned safe_cx = min(cx,width), safe_cy = min(cy,height);

		x += fmt.m_left_inset;
		y += fmt.m_top_inset;

		safe_x += fmt.m_left_inset;
		safe_y += fmt.m_top_inset;

		Gdiplus::PixelFormat PixelFormat = g_get_gdiplus_pixelformat(fmt.m_pixel_format);

		bool b_rgb_555 = fmt.m_rgb555;
		Gdiplus::Bitmap image2(fmt.m_render_width, fmt.m_render_height, PixelFormat);
		g_check_gdiplus_ret(image2.GetLastStatus(), "Gdiplus::Bitmap::c'tor");
		{
			Gdiplus::Graphics graphics(&image2);
			g_check_gdiplus_ret(graphics.GetLastStatus(), "Gdiplus::Graphics::c'tor");
			g_check_gdiplus_ret(graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality), "Gdiplus::Graphics::SetPixelOffsetMode");
			g_check_gdiplus_ret(graphics.SetInterpolationMode(Gdiplus::InterpolationModeBicubic), "Gdiplus::Graphics::SetInterpolationMode");
			Gdiplus::SolidBrush br(Gdiplus::Color(LOBYTE(HIWORD(fmt.m_back_colour)), HIBYTE(LOWORD(fmt.m_back_colour)), LOBYTE(LOWORD(fmt.m_back_colour))));
			Gdiplus::Rect rc(0, 0, image2.GetWidth(), image2.GetHeight());
			g_check_gdiplus_ret(graphics.FillRectangle(&br, rc), "Gdiplus::Graphics::FillRectangle");
			//g_check_gdiplus_ret(graphics.DrawImage(&image3, Gdiplus::Rect(x, y, cx, cy), 1, 1, image3.GetWidth()-2, image3.GetHeight()-2, Gdiplus::UnitPixel ), "Gdiplus::Graphics::DrawImage");
			Gdiplus::ImageAttributes imageAttributes;
			imageAttributes.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
			g_check_gdiplus_ret(graphics.DrawImage(&image, Gdiplus::Rect(x, y, cx, cy), 0, 0, image.GetWidth(), image.GetHeight(), Gdiplus::UnitPixel, &imageAttributes ), "Gdiplus::Graphics::DrawImage");
			//console::formatter() << x << " " << y << " " << cx << " " << cy;
		}

		if (fmt.m_pixel_format == 'jpeg')
		{
			CLSID encoderClsid;
			Gdiplus::EncoderParameters encoderParameters;
			ULONG quality;

			mmh::ComPtr<IStream> pStream;
			_check_hresult(CreateStreamOnHGlobal (NULL, TRUE, pStream));

			if (GetEncoderClsid(L"image/jpeg", &encoderClsid) < 0)
				throw pfc::exception("Gdiplus: Error retrieving JPEG encoder CLSID");

			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
			encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;

			quality = 95;
			encoderParameters.Parameter[0].Value = &quality;
			g_check_gdiplus_ret(image2.Save(pStream, &encoderClsid, &encoderParameters), "Image::Save");

			STATSTG sstg;
			memset(&sstg, 0, sizeof(sstg));
			_check_hresult(pStream->Stat(&sstg, STATFLAG_NONAME));

			LARGE_INTEGER li;
			li.QuadPart = 0;
			_check_hresult(pStream->Seek(li, STREAM_SEEK_SET, NULL));

			p_out.data.set_size(pfc::downcast_guarded<t_size>(sstg.cbSize.QuadPart));
			if (pStream->Read(p_out.data.get_ptr(), p_out.data.get_size(), NULL) != S_OK) //S_FALSE returned if not all read
				throw pfc::exception("IStream::Read error");

			p_out.m_data_size = p_out.data.get_size();
			p_out.m_padded_size = p_out.m_data_size;
		}
		else
		{
			p_out.m_data_size = fmt.get_raw_size();
			p_out.m_padded_size = fmt.get_size();

			p_out.data.set_size (fmt.get_raw_size());
			p_out.data.fill_null();

			Gdiplus::BitmapData GdiBitmapdata;
			Gdiplus::Rect rc(0, 0, image2.GetWidth(), image2.GetHeight());
			g_check_gdiplus_ret(image2.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat, &GdiBitmapdata), "Gdiplus::Bitmap::LockBits");

			const t_uint8 * ptr= (t_uint8*)GdiBitmapdata.Scan0;
			t_uint8 * dest_ptr = p_out.data.get_ptr();
			t_size j;

			pfc::array_t<t_uint8> buffer;

			if (b_rgb_555)
			{
				buffer.set_size(GdiBitmapdata.Stride*GdiBitmapdata.Height);
				buffer.fill_null();
				t_uint8 * buffer_ptr = buffer.get_ptr();

				t_size k, height_limit = min(GdiBitmapdata.Height, (safe_y + safe_cy) ), width_limit = min( (safe_x+safe_cx) ,GdiBitmapdata.Width);
				for (j=0; j<GdiBitmapdata.Height; j++)
				{
					for (k=0; k<GdiBitmapdata.Width; k++)
					{
						t_uint16 * pixel = (t_uint16*)(&ptr[j*GdiBitmapdata.Stride + k*2]);
						t_uint16 * dest_pixel = (t_uint16*)(&buffer_ptr[j*GdiBitmapdata.Stride + k*2]);
						if ((int)j >= safe_y && j<height_limit && (int)k >= safe_x && k < width_limit)
							*dest_pixel = ( (*pixel) | 1<<15 );
						else
						{
							*dest_pixel = *pixel;
						}
					}
				}
				ptr = buffer_ptr;
			}

			bitmap_utils::bitmap_to_alternative_pixel_order_t reordered((t_uint16*)ptr, GdiBitmapdata.Width, GdiBitmapdata.Height, GdiBitmapdata.Stride / 2);
			if (fmt.m_alternate_pixel_order)
			{
				ptr = reordered.to_alternative_order();
			}

			//if (b_rgb_555)
			{
				t_size k;
				for (j=0; j<GdiBitmapdata.Height; j++)
				{
					for (k=0; k<GdiBitmapdata.Width; k++)
					{
						t_uint16 * source_pixel = (t_uint16*)(&ptr[j*GdiBitmapdata.Stride + k*2]);
						t_uint16 * dest_pixel = (t_uint16*)(dest_ptr + fmt.get_row_stride()*j + k*2);
						*dest_pixel = *source_pixel;
					}
				}
			}

			g_check_gdiplus_ret(image2.UnlockBits(&GdiBitmapdata), "Gdiplus::Bitmap::UnlockBits");
		}

		p_out.x = safe_x;
		p_out.y = safe_y;
		p_out.cx = safe_cx;
		p_out.cy = safe_cy;
	}


	void t_datafile::_add_image_name(ipod_device_ptr_ref_t p_ipod, const artwork_format_t & fmt, bool b_new, Gdiplus::Bitmap& image, t_image_item & item, t_uint32 timems, t_size count_alloc, abort_callback & p_abort)
	{
		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork";

		t_size image_name_index = pfc_infinite;
		bool b_image_name_new = true;

		if (!b_new && item.find_image_name_by_format(fmt.m_format_id, image_name_index, timems))
			b_image_name_new = false;

		formatted_image fi;
		g_format_image(image, fmt, fi);
		//pImageFormatter.FormatImage(fmt, fi);

		t_image_name in_new;
		t_image_name & in = b_image_name_new ? in_new : item.image_names[image_name_index];

		t_uint32 offset;

		pfc::string8 location = path, filename, ipod_location;
		if (b_image_name_new || in.file_size_2 != fi.m_padded_size)
		{
			t_size fileno;
			find_empty_block(p_ipod, fmt, fi.m_padded_size, offset, fileno, item.image_names, p_abort);
			filename << "F" << fmt.m_format_id << "_" << fileno << ".ithmb";
			location << p_ipod->get_path_separator_ptr() << filename; 
			ipod_location << ":" << filename;
		}
		else
		{
			offset = in.file_offset;
			ipod_location = in.location;
			if (ipod_location.length())
				filename = &ipod_location[1];
			filename.replace_byte(':', p_ipod->get_path_separator()); //"just in case"
			location << p_ipod->get_path_separator_ptr() << filename;
		}

		service_ptr_t<file> file;
		//console::formatter() << location;
		{
			pfc::string8 dir = path;
			//dir << "iPod_Control\\Artwork";
			if (!filesystem::g_exists(dir, p_abort))
				filesystem::g_create_directory(dir, p_abort);
		}
		bool b_exists = filesystem::g_exists(location, p_abort);
		//console::formatter() << b_exists ? "exisst" : "noexst";
		filesystem::g_open(file, location, b_exists ? filesystem::open_mode_write_existing : filesystem::open_mode_write_new, p_abort);
		t_size thumb_index = 0;
		bool b_have_thumb = thumb_info_list.find_by_filename(ipod_location, thumb_index);
		if (!b_have_thumb)
		{
			t_filesize fsize = file->get_size_ex(p_abort);
			t_thumbinfo ti(ipod_location, fsize, fmt.m_format_id);
			try
			{
				ti.dataend = fsize;
				fsize += count_alloc*fmt.get_size(); //20*1024*1024;
				if (fmt.m_offset_alignment && (fsize%fmt.m_offset_alignment))
					fsize += fmt.m_offset_alignment - (fsize%fmt.m_offset_alignment);
				file->resize(fsize, p_abort);
				Sleep(50);
			}
			catch (const exception_io_device_full &) {};
			thumb_index = thumb_info_list.add_item(ti);
		}
		if (offset > file->get_size_ex(p_abort))
			file->resize(offset, p_abort);
		if (offset + fi.m_padded_size > thumb_info_list[thumb_index].dataend)
			thumb_info_list[thumb_index].dataend = offset+fi.m_padded_size;
		file->seek(offset, p_abort);

		file->write(fi.data.get_ptr(), fi.data.get_size(), p_abort);

		t_size post_padding = fmt.get_padding();
		if (post_padding)
		{
			pfc::array_t<t_uint8> paddingdata;
			paddingdata.set_size(post_padding);
			paddingdata.fill_null();
			file->write(paddingdata.get_ptr(), post_padding, p_abort);
		}
		if (file->get_size_ex(p_abort) == thumb_info_list[thumb_index].dataend)
			thumb_info_list.remove_by_idx(thumb_index);


		in.do_type = 2;
		in.file_size = fi.m_data_size;
		in.file_size_2 = fi.m_padded_size;
		in.correlation_id = fmt.m_format_id;

		in.height = fi.cy + fi.y; //bottom
		in.width = fi.cx + fi.x; //right
		in.vertical_padding = fi.y; //top
		in.horizontal_padding = fi.x; //left
		in.file_offset = offset;

		in.display_start_time_ms = timems;

		in.location_valid = true;
		in.location.reset();
		in.location << ":" << filename;
		if (b_image_name_new)
			item.image_names.add_item(in);
	}

	bool t_datafile::add_artwork_v3(ipod_device_ptr_ref_t p_ipod, t_uint32 mediatype, t_uint64 dbid, t_uint32 & mhii_id, const album_art_data_ptr & data, t_size count_alloc, abort_callback & p_abort, itunesdb::chapter_list * p_chapter_list)
	{
		//pfc::ptr_list_t<const t_bitmap_info> bitmaps;
		//g_get_artwork_bitmaplist(p_ipod, bitmaps);
		if (p_ipod->is_6g_format())
			count_alloc = (count_alloc+9)/10;

		pfc::string8 path;
		p_ipod->get_database_path(path);
		path << p_ipod->get_path_separator_ptr() << "Artwork";
		bool b_new = true;

		if (g_Gdiplus_initialised)
		{
			t_size index = pfc_infinite;
			if ( (mhii_id && find_by_image_id(index, mhii_id)) || (!p_ipod->is_6g_format() && find_by_dbid(index, dbid)))
				b_new = false;

			t_image_item item_new;
			t_image_item & item = b_new ? item_new : image_list[index];

			if (b_new)
			{
				item.image_id = get_next_ii_id();
				next_ii_id++;
				item.song_dbid = dbid;
			}

			item.source_image_size = (t_uint32)data->get_size();

			mmh::ComPtr<IStream> pStream;
			g_create_IStream_from_datablock(*data, pStream);

			Gdiplus::Bitmap image(pStream);
			pStream.release();

			g_check_gdiplus_ret(image.GetLastStatus(), "Gdiplus::Bitmap::c'tor");
			bool b_have_chapter_zero_ms = false;
			if (p_chapter_list)
			{
				for (t_size c=0, ccount = p_chapter_list->get_count(); c<ccount; c++)
				{
					if ((*p_chapter_list)[c].m_image_data.get_size())
					{
						mmh::ComPtr<IStream> pStreamChap;
						g_create_IStream_from_datablock((*p_chapter_list)[c].m_image_data, pStreamChap);

						Gdiplus::Bitmap imagec(pStreamChap);
						pStreamChap.release();
						g_check_gdiplus_ret(imagec.GetLastStatus(), "Gdiplus::Bitmap::c'tor");

						t_uint32 position = (*p_chapter_list)[c].m_start_position;
						if (position == 1) {position = 0; b_have_chapter_zero_ms=true;}
						for (t_size i=0, count = p_ipod->m_device_properties.m_chapter_artwork_formats.get_count(); i<count; i++)
						{			
							const artwork_format_t & fmt = p_ipod->m_device_properties.m_chapter_artwork_formats[i];
							if ( (mediatype & g_translate_associated_format(fmt.m_associated_format)) && (mediatype & fmt.m_excluded_formats) == 0)
							{
								_add_image_name(p_ipod, fmt, b_new, imagec, item, position, count_alloc*10, p_abort);
							}
						}
					}
				}
			}
			for (t_size i=0, count = p_ipod->m_device_properties.m_artwork_formats.get_count(); i<count; i++)
			{
				
				const artwork_format_t & fmt = p_ipod->m_device_properties.m_artwork_formats[i];
				if ( (mediatype & g_translate_associated_format(fmt.m_associated_format)) && (mediatype & fmt.m_excluded_formats) == 0)
				{
					_add_image_name(p_ipod, fmt, b_new, image, item, b_have_chapter_zero_ms ? -1 : 0, count_alloc, p_abort);
				}
			}
			//remove_by_dbid(dbid);
			if (b_new)
				image_list.add_item(item);
			mhii_id = item.image_id;			
		}
		else throw pfc::exception_bug_check();

		return b_new;

	}
	void reader::read_ilhm(t_header_marker< identifiers::ilhm > & p_header, t_image_list & p_out, abort_callback & p_abort)
	{
		t_size i;
		for (i=0; i<p_header.section_size; i++)
		{
			t_header_marker< identifiers::iihm > header;
			read_header(header, p_abort);
			t_image_item temp;
			read_iihm(header, temp, p_abort);
			p_out.add_item(temp);
		}
	}
	void reader::read_iihm(t_header_marker< identifiers::iihm > & p_header, t_image_item & p_out, abort_callback & p_abort)
	{
		t_uint32 do_count;
		itunesdb::stream_reader_memblock_ref_dop data(p_header.data.get_ptr(), p_header.data.get_size());
		data.read_lendian_t(do_count, p_abort);
		data.read_lendian_t(p_out.image_id, p_abort);
		data.read_lendian_t(p_out.song_dbid, p_abort);
		data.read_lendian_t(p_out.unk4, p_abort);
		data.read_lendian_t(p_out.rating, p_abort);
		data.read_lendian_t(p_out.unk6, p_abort);
		data.read_lendian_t(p_out.original_date, p_abort);
		data.read_lendian_t(p_out.digitised_date, p_abort);
		data.read_lendian_t(p_out.source_image_size, p_abort);
		data.read_lendian_t(p_out.unk7, p_abort);
		data.read_lendian_t(p_out.refcount, p_abort);
		data.read_lendian_t(p_out.unk8, p_abort); //usually 1

		t_size i;
		for (i=0; i<do_count; i++)
		{
			t_data_object dobj;
			t_header_marker< identifiers::dohm > header;
			read_header(header, p_abort);
			read_do(header, dobj, p_abort);
			switch (dobj.type)
			{
			case 2:
			case 5:
				{
					t_image_name temp;
					dobj.get_inhm(temp, p_abort);
					p_out.image_names.add_item(temp);
				}
				break;
			case 1:
				dobj.get_string(p_out.label, p_abort);
				p_out.label_valid = true;
				break;
			};
		}
	}

	void reader::read_alhm(t_header_marker< identifiers::alhm > & p_header, t_photo_album_list & p_out, abort_callback & p_abort)
	{
		t_size i;
		for (i=0; i<p_header.section_size; i++)
		{
			t_header_marker< identifiers::abhm > header;
			read_header(header, p_abort);
			t_photo_album temp;
			read_abhm(header, temp, p_abort);
			p_out.add_item(temp);
		}
	}
	void reader::read_abhm(t_header_marker< identifiers::abhm > & p_header, t_photo_album & p_out, abort_callback & p_abort)
	{
		t_uint32 do_count, ai_count;
		itunesdb::stream_reader_memblock_ref_dop data(p_header.data.get_ptr(), p_header.data.get_size());
		data.read_lendian_t(do_count, p_abort);
		data.read_lendian_t(ai_count, p_abort);
		data.read_lendian_t(p_out.album_id, p_abort);
		data.read_lendian_t(p_out.unk2, p_abort);
		data.read_lendian_t(p_out.unk3, p_abort);
		data.read_lendian_t(p_out.unk4, p_abort);
		data.read_lendian_t(p_out.album_type, p_abort);
		data.read_lendian_t(p_out.play_music, p_abort);
		data.read_lendian_t(p_out.repeat, p_abort);
		data.read_lendian_t(p_out.random, p_abort);
		data.read_lendian_t(p_out.show_titles, p_abort);
		data.read_lendian_t(p_out.transition_direction, p_abort);
		data.read_lendian_t(p_out.slide_duration, p_abort);
		data.read_lendian_t(p_out.transition_duration, p_abort);
		data.read_lendian_t(p_out.unk7, p_abort);
		data.read_lendian_t(p_out.unk8, p_abort);
		data.read_lendian_t(p_out.song_dbid2, p_abort);
		data.read_lendian_t(p_out.previous_album_id, p_abort);

		t_size i;
		for (i=0; i<do_count; i++)
		{
			t_data_object dobj;
			t_header_marker< identifiers::dohm > header;
			read_header(header, p_abort);
			read_do(header, dobj, p_abort);
			switch (dobj.type)
			{
			case 1:
				dobj.get_string(p_out.album_name, p_abort);
				p_out.album_name_valid = true;
				break;
			case 2:
				dobj.get_string(p_out.transition_effect, p_abort);
				p_out.transition_effect_valid = true;
				break;
			};
		}
		for (i=0; i<ai_count; i++)
		{
			t_header_marker< identifiers::aihm > header;
			read_header(header, p_abort);
			t_album_item temp;
			read_aihm(header, temp, p_abort);
			p_out.album_items.add_item(temp);
		}
	}
	void reader::read_aihm(t_header_marker< identifiers::aihm > & p_header, t_album_item & p_out, abort_callback & p_abort)
	{
		//t_uint32 do_count;
		itunesdb::stream_reader_memblock_ref_dop data(p_header.data.get_ptr(), p_header.data.get_size());
		data.read_lendian_t(p_out.unk1, p_abort);
		data.read_lendian_t(p_out.image_id, p_abort);
		m_file->skip_object(p_header.section_size-p_header.header_size, p_abort);
	}

	void reader::read_flhm(t_header_marker< identifiers::flhm > & p_header, t_file_list & p_out, abort_callback & p_abort)
	{
		t_size i;
		for (i=0; i<p_header.section_size; i++)
		{
			t_header_marker< identifiers::fihm > header;
			read_header(header, p_abort);
			t_file_item temp;
			read_fihm(header, temp, p_abort);
			p_out.add_item(temp);
		}
	}
	void reader::read_fihm(t_header_marker< identifiers::fihm > & p_header, t_file_item & p_out, abort_callback & p_abort)
	{
		itunesdb::stream_reader_memblock_ref_dop data(p_header.data.get_ptr(), p_header.data.get_size());
		data.read_lendian_t(p_out.unk1, p_abort);
		data.read_lendian_t(p_out.correlation_id, p_abort);
		data.read_lendian_t(p_out.file_size, p_abort);
		m_file->skip_object(p_header.section_size-p_header.header_size, p_abort);
	}

	void reader::read_photodb(t_datafile & p_out, abort_callback & p_abort)
	{
		t_header_marker< identifiers::dfhm > dfhm;
		read_header(dfhm, p_abort);
		itunesdb::stream_reader_memblock_ref_dop p_dfhm(dfhm.data.get_ptr(), dfhm.data.get_size());
		t_uint32 ds_count;
		p_dfhm.read_lendian_t(p_out.unk1, p_abort);
		p_dfhm.read_lendian_t(p_out.unk2, p_abort);
		p_dfhm.read_lendian_t(ds_count, p_abort);
		p_dfhm.read_lendian_t(p_out.unk3, p_abort);
		p_dfhm.read_lendian_t(p_out.next_ii_id, p_abort);
		p_dfhm.read_lendian_t(p_out.unk5, p_abort);
		p_dfhm.read_lendian_t(p_out.unk6, p_abort);
		p_dfhm.read_lendian_t(p_out.unk7, p_abort);
		p_dfhm.read_lendian_t(p_out.unk8, p_abort);
		p_dfhm.read_lendian_t(p_out.unk9, p_abort);
		p_dfhm.read_lendian_t(p_out.unk10, p_abort);
		p_dfhm.read_lendian_t(p_out.unk11, p_abort);
		t_size i;
		for (i=0; i<ds_count; i++)
		{
			t_header_marker< identifiers::dshm > dshm;
			read_header(dshm, p_abort);
			itunesdb::stream_reader_memblock_ref_dop p_dshm(dshm.data.get_ptr(), dshm.data.get_size());
			t_uint32 type;
			p_dshm.read_lendian_t(type, p_abort);
			switch (type)
			{
			case 1:
				{
				t_header_marker< identifiers::ilhm > ilhm;
				read_header(ilhm, p_abort);
				read_ilhm(ilhm, p_out.image_list, p_abort);;
				}
				break;
			case 2:
				{
				t_header_marker< identifiers::alhm > alhm;
				read_header(alhm, p_abort);
				read_alhm(alhm, p_out.photo_albums, p_abort);;
				}
				break;
			case 3:
				{
				t_header_marker< identifiers::flhm > flhm;
				read_header(flhm, p_abort);
				read_flhm(flhm, p_out.file_list, p_abort);;
				}
				break;
			default:
				m_file->skip_object(dshm.section_size-dshm.header_size, p_abort);
				break;
			};
		}
	}
	void read_do(stream_reader * p_reader, t_header_marker< identifiers::dohm > & dohm, t_data_object & p_out, abort_callback & p_abort)
	{
		itunesdb::stream_reader_memblock_ref_dop p_data(dohm.data.get_ptr(), dohm.data.get_size());
		p_data.read_lendian_t(p_out.type, p_abort);
		p_data.read_lendian_t(p_out.unk1, p_abort);
		p_out.data.set_size(dohm.section_size - dohm.header_size);
		p_reader->read(p_out.data.get_ptr(), dohm.section_size - dohm.header_size, p_abort);
	}
	void t_data_object::get_inhm(t_image_name & p_out, abort_callback & p_abort)
	{
		p_out.do_type = type;
		t_header_marker< identifiers::inhm > inhm;
		itunesdb::stream_reader_memblock_ref_dop p_dohm_data(data.get_ptr(), data.get_size());
		read_header(&p_dohm_data, inhm, p_abort);

		t_uint32 do_count;
		itunesdb::stream_reader_memblock_ref_dop p_inhm(inhm.data.get_ptr(), inhm.data.get_size());
		p_inhm.read_lendian_t(do_count, p_abort);
		p_inhm.read_lendian_t(p_out.correlation_id, p_abort);
		p_inhm.read_lendian_t(p_out.file_offset, p_abort);
		p_inhm.read_lendian_t(p_out.file_size, p_abort);
		p_inhm.read_lendian_t(p_out.vertical_padding, p_abort);
		p_inhm.read_lendian_t(p_out.horizontal_padding, p_abort);
		p_inhm.read_lendian_t(p_out.height, p_abort);
		p_inhm.read_lendian_t(p_out.width, p_abort);
		try
		{
			p_inhm.read_lendian_t(p_out.display_start_time_ms, p_abort);
			p_inhm.read_lendian_t(p_out.file_size_2, p_abort);
		}
		catch (const exception_io_data_truncation &) 
		{
			p_out.file_size_2 = p_out.file_size;
		};
		//unk 32-bit
		//unk filesize2

		t_size i;
		for (i=0; i<do_count; i++)
		{
			t_data_object dobj;
			t_header_marker< identifiers::dohm > header;
			read_header(&p_dohm_data, header, p_abort);
			read_do(&p_dohm_data, header, dobj, p_abort);
			switch (dobj.type)
			{
			case 3:
				dobj.get_string(p_out.location, p_abort);
				p_out.location_valid = true;
				break;
			};
		}
	}
	void t_data_object::get_string(pfc::string8 & p_out, abort_callback & p_abort)
	{
		t_uint32 length, encoding, unk;

		itunesdb::stream_reader_memblock_ref_dop p_dohm_data(data.get_ptr(), data.get_size());
		p_dohm_data.read_lendian_t(length, p_abort);
		p_dohm_data.read_lendian_t(encoding, p_abort);
		p_dohm_data.read_lendian_t(unk, p_abort);

		if (encoding==0 || encoding==1)
		{
			char * buff = p_out.lock_buffer(length);
			p_dohm_data.read(buff, length, p_abort);
			p_out.unlock_buffer();
		}
		else if (encoding==2)
		{
			pfc::array_t<WCHAR> buff;
			buff.set_size(length/2 + 1);
			buff.fill_null();
			p_dohm_data.read(buff.get_ptr(), length, p_abort);
			p_out = pfc::stringcvt::string_utf8_from_wide(buff.get_ptr());
		}
	}
	void writer::write_do (t_uint16 type, void *data, t_uint32 data_size, abort_callback & p_abort)
	{
		//assert (data_size%4 == 0);
		t_uint8 padding = (data_size%4) ? (4-data_size%4) : 0;
		stream_writer_mem hmdata;
		hmdata.write_lendian_t(type, p_abort);
		hmdata.write_lendian_t(t_uint8(0), p_abort);
		hmdata.write_lendian_t(padding, p_abort);//nose
		hmdata.write_lendian_t(t_uint32(0), p_abort);
		hmdata.write_lendian_t(t_uint32(0), p_abort);
		write_section(identifiers::dohm, hmdata.get_ptr(), hmdata.get_size(), data, data_size, 3*sizeof(t_uint32)+hmdata.get_size() + data_size + padding, p_abort);
		t_uint32 dummy = 0;
		m_writer->write(&dummy, padding, p_abort);
	}
	void writer::write_do_string (t_uint16 type, const char * str, t_encoding encoding, abort_callback & p_abort)
	{
		stream_writer_mem data;
		if (encoding == encoding_utf8)
		{
			t_uint32 len = strlen(str);
			data.write_lendian_t(len, p_abort);
			data.write_lendian_t(t_uint32(1), p_abort);
			data.write_lendian_t(t_uint32(0), p_abort);
			data.write(str, len, p_abort);
			//t_uint32 dummy = 0;
			//if (len%4)
				//hmdata.write(&dummy, 4-len%4, p_abort);
		}
		else
		{
			pfc::stringcvt::string_wide_from_utf8 utf16(str);
			t_uint32 len = utf16.length()*sizeof(WCHAR);
			data.write_lendian_t(len, p_abort);
			data.write_lendian_t(t_uint32(2), p_abort);
			data.write_lendian_t(t_uint32(0), p_abort);
			data.write(utf16.get_ptr(), len, p_abort); //endianness
			//t_uint32 dummy = 0;
			//if (len%4)
			//	hmdata.write(&dummy, 4-len%4, p_abort);
		}
		write_do(type, data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_inhm(const t_image_name & in, abort_callback & p_abort)
	{
		t_uint32 do_count = 0;
		if (in.location_valid) do_count++;

		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(do_count, p_abort); //12
		hmdata.write_lendian_t(in.correlation_id, p_abort); //16
		hmdata.write_lendian_t(in.file_offset, p_abort); //20
		hmdata.write_lendian_t(in.file_size, p_abort); //24 
		hmdata.write_lendian_t(in.vertical_padding, p_abort); //28
		hmdata.write_lendian_t(in.horizontal_padding, p_abort); //30
		hmdata.write_lendian_t(in.height, p_abort); //32
		hmdata.write_lendian_t(in.width, p_abort); //34
		hmdata.write_lendian_t(in.display_start_time_ms, p_abort); //36
		hmdata.write_lendian_t(in.file_size_2, p_abort); //40

		t_uint32 padding = 8;
		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		writer w(&data);

		if (in.location_valid)
			w.write_do_string(3, in.location, encoding_utf16le, p_abort);

		write_section(identifiers::inhm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_iihm(const t_image_item & ii, abort_callback & p_abort)
	{
		t_uint32 in_count = ii.image_names.get_count();
		t_uint32 do_count = in_count;
		if (ii.label_valid) do_count++;
		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(do_count, p_abort); //12
		hmdata.write_lendian_t(ii.image_id, p_abort); //16
		hmdata.write_lendian_t(ii.song_dbid, p_abort); //20
		hmdata.write_lendian_t(ii.unk4, p_abort); //28
		hmdata.write_lendian_t(ii.rating, p_abort); //32
		hmdata.write_lendian_t(ii.unk6, p_abort); //36
		hmdata.write_lendian_t(ii.original_date, p_abort); //40
		hmdata.write_lendian_t(ii.digitised_date, p_abort); //44
		hmdata.write_lendian_t(ii.source_image_size, p_abort); //48
		hmdata.write_lendian_t(ii.unk7, p_abort); //52
		hmdata.write_lendian_t(ii.refcount, p_abort); //56
		hmdata.write_lendian_t(ii.unk8, p_abort); //60

		t_uint32 padding = 22;
		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		writer dw(&data);

		t_size i;
		for (i=0; i<in_count; i++)
		{
			stream_writer_mem indata;
			writer inw(&indata);
			inw.write_inhm(ii.image_names[i], p_abort);
			dw.write_do(ii.image_names[i].do_type, indata.get_ptr(), indata.get_size(), p_abort);
		}

		if (ii.label_valid)
			dw.write_do_string(1, ii.label, encoding_utf8, p_abort);

		write_section(identifiers::iihm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_aihm(const t_album_item & ai, abort_callback & p_abort)
	{
		t_uint32 do_count = 0;
		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(do_count, p_abort);
		hmdata.write_lendian_t(ai.image_id, p_abort);

		t_uint32 padding = 5;
		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::aihm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_abhm(const t_photo_album & ab, abort_callback & p_abort)
	{
		t_uint32 ai_count = ab.album_items.get_count();
		t_uint32 do_count = 0;
		if (ab.album_name_valid) 
			do_count++;
		if (ab.transition_effect_valid) 
			do_count++;
		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(do_count, p_abort);
		hmdata.write_lendian_t(ai_count, p_abort);
		hmdata.write_lendian_t(ab.album_id, p_abort);
		hmdata.write_lendian_t(ab.unk2, p_abort);
		hmdata.write_lendian_t(ab.unk3, p_abort);
		hmdata.write_lendian_t(ab.unk4, p_abort);
		hmdata.write_lendian_t(ab.album_type, p_abort);
		hmdata.write_lendian_t(ab.play_music, p_abort);
		hmdata.write_lendian_t(ab.repeat, p_abort);
		hmdata.write_lendian_t(ab.random, p_abort);
		hmdata.write_lendian_t(ab.show_titles, p_abort);
		hmdata.write_lendian_t(ab.transition_direction, p_abort);
		hmdata.write_lendian_t(ab.slide_duration, p_abort);
		hmdata.write_lendian_t(ab.transition_duration, p_abort);
		hmdata.write_lendian_t(ab.unk7, p_abort);
		hmdata.write_lendian_t(ab.unk8, p_abort);
		hmdata.write_lendian_t(ab.song_dbid2, p_abort);
		hmdata.write_lendian_t(ab.previous_album_id, p_abort);

		t_uint32 padding = 21;
		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		writer dw(&data);

		if (ab.album_name_valid) 
			dw.write_do_string(1, ab.album_name, encoding_utf8, p_abort);
		if (ab.transition_effect_valid) 
			dw.write_do_string(1, ab.transition_effect, encoding_utf8, p_abort);

		t_size i;
		for (i=0; i<ai_count; i++)
		{
			dw.write_aihm(ab.album_items[i], p_abort);
		}
		write_section(identifiers::abhm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_dshm_ilhm(const t_image_list & il, abort_callback & p_abort)
	{
		stream_writer_mem ilhmdata, ildata, dohmdata, dodata;
		t_uint32 padding = 20, count_ii=il.get_count();
		for (;padding;padding--)
			ilhmdata.write_lendian_t(t_uint32(0), p_abort);

		writer ilw(&ildata);

		t_size i;
		for (i=0; i<count_ii; i++)
			ilw.write_iihm(il[i], p_abort);

		writer dow(&dodata);

		dow.write_section(identifiers::ilhm, ilhmdata.get_ptr(), ilhmdata.get_size(), ildata.get_ptr(), ildata.get_size(), count_ii, p_abort);

		dohmdata.write_lendian_t(t_uint32(1), p_abort);
		padding = 20;
		for (;padding;padding--)
			dohmdata.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::dshm, dohmdata.get_ptr(), dohmdata.get_size(), dodata.get_ptr(), dodata.get_size(), p_abort);
	}
	void writer::write_dshm_alhm(const t_photo_album_list & al, abort_callback & p_abort)
	{
		stream_writer_mem alhmdata, aldata, dohmdata, dodata;
		t_uint32 padding = 20, count_ai=al.get_count();
		for (;padding;padding--)
			alhmdata.write_lendian_t(t_uint32(0), p_abort);

		writer alw(&aldata);

		t_size i;
		for (i=0; i<count_ai; i++)
			alw.write_abhm(al[i], p_abort);

		writer dow(&dodata);

		dow.write_section(identifiers::alhm, alhmdata.get_ptr(), alhmdata.get_size(), aldata.get_ptr(), aldata.get_size(), count_ai, p_abort);

		dohmdata.write_lendian_t(t_uint32(2), p_abort);
		padding = 20;
		for (;padding;padding--)
			dohmdata.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::dshm, dohmdata.get_ptr(), dohmdata.get_size(), dodata.get_ptr(), dodata.get_size(), p_abort);
	}
	void writer::write_fihm(const t_file_item & fi, abort_callback & p_abort)
	{
		t_uint32 do_count = 0;
		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(do_count, p_abort);
		hmdata.write_lendian_t(fi.correlation_id, p_abort);
		hmdata.write_lendian_t(fi.file_size, p_abort);

		t_uint32 padding = 25;
		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::fihm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_dshm_flhm(const t_file_list & fl, abort_callback & p_abort)
	{
		stream_writer_mem flhmdata, fldata, dohmdata, dodata;
		t_uint32 padding = 20, count_fi=fl.get_count();
		for (;padding;padding--)
			flhmdata.write_lendian_t(t_uint32(0), p_abort);

		writer flw(&fldata);

		t_size i;
		for (i=0; i<count_fi; i++)
			flw.write_fihm(fl[i], p_abort);

		writer dow(&dodata);

		dow.write_section(identifiers::flhm, flhmdata.get_ptr(), flhmdata.get_size(), fldata.get_ptr(), fldata.get_size(), count_fi, p_abort);

		dohmdata.write_lendian_t(t_uint32(3), p_abort);
		padding = 20;
		for (;padding;padding--)
			dohmdata.write_lendian_t(t_uint32(0), p_abort);

		write_section(identifiers::dshm, dohmdata.get_ptr(), dohmdata.get_size(), dodata.get_ptr(), dodata.get_size(), p_abort);
	}
	void writer::write_dfhm(const t_datafile & df, abort_callback & p_abort)
	{
		stream_writer_mem hmdata, data;
		hmdata.write_lendian_t(df.unk1, p_abort);
		hmdata.write_lendian_t(t_uint32(2), p_abort);
		hmdata.write_lendian_t(t_uint32(3), p_abort);
		hmdata.write_lendian_t(df.unk3, p_abort);
		hmdata.write_lendian_t(df.next_ii_id, p_abort);
		hmdata.write_lendian_t(df.unk5, p_abort);
		hmdata.write_lendian_t(df.unk6, p_abort);
		hmdata.write_lendian_t(df.unk7, p_abort);
		hmdata.write_lendian_t(df.unk8, p_abort);
		hmdata.write_lendian_t(df.unk9, p_abort);
		hmdata.write_lendian_t(df.unk10, p_abort);
		hmdata.write_lendian_t(df.unk11, p_abort);

		t_uint32 padding = 16;

		for (;padding;padding--)
			hmdata.write_lendian_t(t_uint32(0), p_abort);

		writer w(&data);
		w.write_dshm_ilhm(df.image_list, p_abort);
		w.write_dshm_alhm(df.photo_albums, p_abort);
		w.write_dshm_flhm(df.file_list, p_abort);

		write_section(identifiers::dfhm, hmdata.get_ptr(), hmdata.get_size(), data.get_ptr(), data.get_size(), p_abort);
	}
	void writer::write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
		const void * p_data, t_size data_size, t_uint32 header_data, abort_callback & p_abort)
	{
		m_writer->write_bendian_t(identifier, p_abort);

		m_writer->write_lendian_t(t_uint32(header_size) + 4*3, p_abort);
		m_writer->write_lendian_t(header_data, p_abort);
		m_writer->write(p_header, header_size, p_abort);
		m_writer->write(p_data, data_size, p_abort);
	}
	void writer::write_section(t_uint32 identifier, const void * p_header, t_size header_size, 
		const void * p_data, t_size data_size, abort_callback & p_abort)
	{
		write_section(identifier, p_header, header_size, p_data, data_size, (t_uint32)data_size + t_uint32(header_size) + 4*3, p_abort);
	}

}

namespace bitmap_utils
{
#define clipcolour(a) \
	( (a < 0) ? 0 : (a>255 ? 255 : a))
	void convert_yuv_to_rgb(t_uint32 Y, t_uint32 U, t_uint32 V, t_uint32 & rgb)
	{
		int R, G, B;

		int C = Y - 16;
		int D = U - 128;
		int E = V - 128;

		R = clipcolour(( 298 * C           + 409 * E + 128) >> 8);
		G = clipcolour(( 298 * C - 100 * D - 208 * E + 128) >> 8);
		B = clipcolour(( 298 * C + 516 * D           + 128) >> 8);

		rgb = RGB(R,G,B);
	}
	void convert_uyvy_to_rgb32(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height, pfc::array_t<t_uint8> & p_out)
	{
		p_out.set_size(width * height * 4);
		t_uint8 * ptr = p_out.get_ptr();

		const t_uint32 * src = (const t_uint32 *)data;
		t_uint32 * dst = (t_uint32 *)ptr;

		t_size i, j;
		for (i=0; i<height/2; i++)
		{
			for (j=0; j<width; j++)
			{
				t_uint32 tx = (j >= width/2 ? j-width/2 : j)*2;
				t_uint32 ty = (i >= height/4 ? i - height/4 : i)*4;
				if (i >= height/4) ty++;
				if (j >= width/2) ty+=2;

				t_int32 y1=(src[i*width + j]>>8 & 0xff);
				t_int32 y2=(src[i*width + j]>>24 & 0xff);
				t_int32 u=(src[i*width + j]>>0 & 0xff);
				t_int32 v=(src[i*width + j]>>16 & 0xff);
				if ((height-ty-1)*width+tx < width*height)
					convert_yuv_to_rgb(y1, u, v, dst[(height-ty-1)*width+tx]);
				else console::formatter() << "iPod manager: y overflow";
				if ((height-ty-1)*width+tx+1 < width*height)
					convert_yuv_to_rgb(y2, u, v, dst[(height-ty-1)*width+tx+1]);
				else console::formatter() << "iPod manager: x overflow";
			}
		}
		
	}
	HBITMAP create_bitmap_from_uyvy(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height)
	{
		if (size < width*height*2) return NULL;
		pfc::array_t<t_uint8> rgbdata;
		convert_uyvy_to_rgb32(data, size, width, height, rgbdata);

		BITMAPV5HEADER bmh;
		memset(&bmh, 0, sizeof(bmh));
		bmh.bV5Size = sizeof(bmh);
		bmh.bV5Width = width;
		bmh.bV5Height = height;
		bmh.bV5Planes = 1;
		bmh.bV5BitCount = 32;
		bmh.bV5Compression = BI_RGB;
		//bmh.bV5CSType = LCS_sRGB;

		BITMAPINFO bi;
		memset(&bi, 0, sizeof(bi));
		bi.bmiHeader.biSize = sizeof (bi.bmiHeader);
		bi.bmiHeader.biWidth = bmh.bV5Width;
		bi.bmiHeader.biHeight = bmh.bV5Height;
		bi.bmiHeader.biPlanes = bmh.bV5Planes;
		bi.bmiHeader.biBitCount = bmh.bV5BitCount;
		bi.bmiHeader.biCompression = bmh.bV5Compression;

		HDC dc = GetDC(0);

		HBITMAP ret = CreateDIBitmap(dc, (LPBITMAPINFOHEADER)&bmh, CBM_INIT, rgbdata.get_ptr(), &bi, NULL);
		ReleaseDC(0, dc);
		return ret;
	}
	HBITMAP create_bitmap_from_jpeg(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height)
	{
		mmh::ComPtr<IStream> pStream;
		g_create_IStream_from_datablock(data, size, pStream);

		Gdiplus::Bitmap image(pStream);
		pStream.release();
		HBITMAP ret = 0;
		image.GetHBITMAP(Gdiplus::Color(255,0,0), &ret);
		return ret;
	}
	HBITMAP create_bitmap_from_rgb565(const t_uint8 * data, t_size size, t_uint32 width, t_uint32 height, t_uint32 stride, bool b_rgb555)
	{
		if (size < width*height*2) return NULL;
		BITMAPV5HEADER bmh;
		memset(&bmh, 0, sizeof(bmh));
		bmh.bV5Size = sizeof(bmh);
		bmh.bV5Width = width;
		bmh.bV5Height = height;
		bmh.bV5Planes = 1;
		bmh.bV5BitCount = 16;
		bmh.bV5Compression = BI_BITFIELDS;
		//bmh.bV5CSType = LCS_sRGB;

		struct BITMAPINFOBITFIELD {
		BITMAPINFOHEADER    bmiHeader;
		DWORD             bmiBitMasks[3];
		}
		bi;
		memset(&bi, 0, sizeof(bi));
		bi.bmiHeader.biSize = sizeof (bi.bmiHeader);
		bi.bmiHeader.biWidth = bmh.bV5Width;
		bi.bmiHeader.biHeight = -bmh.bV5Height;
		bi.bmiHeader.biPlanes = bmh.bV5Planes;
		bi.bmiHeader.biBitCount = bmh.bV5BitCount;
		bi.bmiHeader.biCompression = bmh.bV5Compression;
		bi.bmiBitMasks[2] = 1<<0|1<<1|1<<2|1<<3|1<<4;
		if (b_rgb555)
		{
		bi.bmiBitMasks[1] = 1<<5|1<<6|1<<7|1<<8|1<<9;
		bi.bmiBitMasks[0] = 1<<10|1<<11|1<<12|1<<13|1<<14;
		} 
		else 
		{
		bi.bmiBitMasks[1] = 1<<5|1<<6|1<<7|1<<8|1<<9|1<<10;
		bi.bmiBitMasks[0] = 1<<11|1<<12|1<<13|1<<14|1<<15;
		}

		HDC dc = GetDC(0);

		t_size deststride = width*2;
		if (deststride%4) deststride += (4-(deststride%4));

		pfc::array_staticsize_t<t_uint8> buffer(deststride*height);
		for (t_size i = 0; i < height; i++)
			memcpy(buffer.get_ptr()+i*deststride, data+i*stride, width*2);

		HBITMAP ret = CreateDIBitmap(dc, (LPBITMAPINFOHEADER)&bmh, CBM_INIT, buffer.get_ptr(), (LPBITMAPINFO)&bi, NULL);
		ReleaseDC(0, dc);
		return ret;
	}

	/*
	01
	23

	p(0) = 0
	p(1) = 2
	p(2) = 1
	p(3) = 3
	*/

	const t_uint8 * bitmap_to_alternative_pixel_order_t::to_alternative_order() {
		m_buffer.set_size(m_width*m_height);
		m_buffer.fill_null();
		to_alternative_order(0, 0, m_width, m_height);
		return get_ptr();
	}

	void bitmap_to_alternative_pixel_order_t::to_alternative_order(t_size src_offset, t_size dst_offset, int width, int height) {
		if (width == 1)
		{
			m_buffer[dst_offset] = m_src[src_offset];
		}
		else
		{
			width /= 2;
			height /= 2;
			to_alternative_order(src_offset,
				dst_offset,
				width,
				height
				);
			to_alternative_order(src_offset + height*m_src_stride,
				dst_offset + width * height * 1,
				width,
				height
				);
			to_alternative_order(src_offset + width,
				dst_offset + width * height * 2,
				width,
				height
				);
			to_alternative_order(src_offset + height*m_src_stride + width,
				dst_offset + width * height * 3,
				width,
				height
				);
		}
	}

	const t_uint8 * bitmap_from_alternative_pixel_order_t::from_alternative_order() {
		m_buffer.set_size(m_width*m_height);
		m_buffer.fill_null();
		from_alternative_order(0, 0, m_width, m_height);
		return get_ptr();
	}

	void bitmap_from_alternative_pixel_order_t::from_alternative_order(t_size dst_offset, t_size src_offset, int width, int height) {
		if (width == 1)
		{
			m_buffer[dst_offset] = m_src[src_offset];
		}
		else
		{
			width /= 2;
			height /= 2;
			from_alternative_order(dst_offset,
				src_offset,
				width,
				height
				);
			from_alternative_order(dst_offset + height*m_dst_stride,
				src_offset + width * height * 1,
				width,
				height
				);
			from_alternative_order(dst_offset + width,
				src_offset + width * height * 2,
				width,
				height
				);
			from_alternative_order(dst_offset + height*m_dst_stride + width,
				src_offset + width * height * 3,
				width,
				height
				);
		}
	}

};
