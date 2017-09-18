#include "main.h"

class filter_pin_enum //new interfaces
{
public:
	filter_pin_enum(IBaseFilter * pFilter)
	{
		mmh::ComPtr<IEnumPins> pPinEnum;
		mmh::ComPtr<IPin> pTemp;
		pFilter->EnumPins(pPinEnum.get_pp());
		while (S_OK == pPinEnum->Next(1, pTemp.get_pp(), NULL))
		{
			PIN_DIRECTION Dir;
			pTemp->QueryDirection(&Dir);
			if (Dir == PINDIR_INPUT)
				m_inputs.add_item(pTemp);
			else if (Dir == PINDIR_OUTPUT)
				m_outputs.add_item(pTemp);
		}
	}
	pfc::list_t< mmh::ComPtr <IPin> > m_inputs;
	pfc::list_t< mmh::ComPtr <IPin> > m_outputs;
};

void FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if (mt.cbFormat != 0)
	{
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL)
	{
		// Unecessary because pUnk should not be used, but safest.
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}


void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if (pmt != NULL)
	{
		FreeMediaType(*pmt); // See FreeMediaType for the implementation.
		CoTaskMemFree(pmt);
	}
}

void _check_hresult(HRESULT hr)
{
	if (FAILED(hr))
	{
		WCHAR buffer[MAX_ERROR_TEXT_LEN+1];
		pfc::string8 text;
		memset(buffer, 0, sizeof(buffer));
		DWORD err = AMGetErrorText(hr, buffer, MAX_ERROR_TEXT_LEN);
		if (!err)
			text << "Unknown error: " << pfc::format_hex(hr, 8) << "h";
		else
			text = pfc::stringcvt::string_utf8_from_wide(buffer);

		throw pfc::exception(text);
	}
}

void _check_hresult_mediafoundation(HRESULT hr)
{
	if (FAILED(hr))
	{
		pfc::string8 text;
		DWORD err = uFormatMessage(hr, text);
		if (!err)
			text << "Error " << pfc::format_hex(hr, 8) << "h";

		throw pfc::exception(text);
	}
}



void video_thumbailer_mediafoundation::ensure_initialised()
{
	if (!m_MFInitialised)
	{
		try 
		{
			load_libraries();
			HRESULT hr = m_MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
			_check_hresult_mediafoundation(hr);
			m_MFInitialised = true;
		} 
		catch (pfc::exception const &) 
		{
			cleanup_libraries();
			throw;
		}
	}
}
#define MDGPA2(x, y) m_##x = (x##PROC)GetProcAddress(y, #x)
#define SafeMDGPA2(x, y) if (!(MDGPA2(x,y))) throw pfc::exception(pfc::string8() << "Failed to locate function " << #x)
	void video_thumbailer_mediafoundation::load_libraries()
	{
		if (!(m_library_MFPLAT = LoadLibrary(L"MFPLAT.DLL")))
			throw pfc::exception(pfc::string8() << "Failed to load MFPLAT.DLL - " << format_win32_error(GetLastError()));
		if (!(m_library_MFREADWRITE = LoadLibrary(L"MFREADWRITE.DLL")))
			throw pfc::exception(pfc::string8() << "Failed to load MFREADWRITE.DLL - " << format_win32_error(GetLastError()));

		SafeMDGPA2(MFCreateAttributes, m_library_MFPLAT);
		SafeMDGPA2(MFCreateMediaType, m_library_MFPLAT);
		SafeMDGPA2(MFStartup, m_library_MFPLAT);
		SafeMDGPA2(MFShutdown, m_library_MFPLAT);
		SafeMDGPA2(MFCreateSourceReaderFromURL, m_library_MFREADWRITE);

	}
	void video_thumbailer_mediafoundation::cleanup_libraries()
	{
		if (m_library_MFREADWRITE)
		{
			FreeLibrary(m_library_MFREADWRITE);
			m_library_MFREADWRITE = NULL;
		}
		if (m_library_MFPLAT)
		{
			FreeLibrary(m_library_MFPLAT);
			m_library_MFPLAT = NULL;
		}
	}
	RECT video_thumbailer_mediafoundation::CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR)
	{
		// Start with a rectangle the same size as src, but offset to the origin (0,0).
		RECT rc = {0, 0, src.right - src.left, src.bottom - src.top};

		if ((srcPAR.Numerator != 1) || (srcPAR.Denominator != 1))
		{
			// Correct for the source's PAR.

			if (srcPAR.Numerator > srcPAR.Denominator)
			{
				// The source has "wide" pixels, so stretch the width.
				rc.right = MulDiv(rc.right, srcPAR.Numerator, srcPAR.Denominator);
			}
			else if (srcPAR.Numerator < srcPAR.Denominator)
			{
				// The source has "tall" pixels, so stretch the height.
				rc.bottom = MulDiv(rc.bottom, srcPAR.Denominator, srcPAR.Numerator);
			}
			// else: PAR is 1:1, which is a no-op.
		}
		return rc;
	}
	LONGLONG video_thumbailer_mediafoundation::GetDuration()
	{
		PROPVARIANT var;
		PropVariantInit(&var);
		LONGLONG ret = 0;

		HRESULT hr = S_OK;

		hr = m_pReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
		_check_hresult_mediafoundation(hr);

		assert(var.vt == VT_UI8);
		ret = var.hVal.QuadPart;

		PropVariantClear(&var);

		return ret;
	}

void video_thumbailer_mediafoundation::UpdateFormat()
{
	HRESULT hr = E_FAIL;
	mmh::ComPtr<IMFMediaType> pType;
	UINT32  width = 0, height = 0;
	LONG lStride = 0;
	MFRatio par = { 0 , 0 };

	GUID subtype = { 0 };

	// Get the media type from the stream.
	hr = m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, pType);
	_check_hresult_mediafoundation(hr);

	// Make sure it is a video format.
	hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
	if (subtype != MFVideoFormat_RGB32)
		_check_hresult_mediafoundation(E_UNEXPECTED);

	// Get the width and height
	hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
	_check_hresult_mediafoundation(hr);

	// Get the stride to find out if the bitmap is top-down or bottom-up.
	lStride = (LONG)MFGetAttributeUINT32(pType, MF_MT_DEFAULT_STRIDE, 1);

	m_format.bTopDown = (lStride > 0); 

	// Get the pixel aspect ratio. (This value might not be set.)
	hr = MFGetAttributeRatio(pType, MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&par.Numerator, (UINT32*)&par.Denominator);
	if (SUCCEEDED(hr) && (par.Denominator != par.Numerator))
	{
		RECT rcSrc = { 0, 0, width, height };
		m_format.rcPicture = CorrectAspectRatio(rcSrc, par);
	}
	else
	{
		// Either the PAR is not set (assume 1:1), or the PAR is set to 1:1.
		SetRect(&m_format.rcPicture, 0, 0, width, height);
	}

    m_format.imageWidthPels = width;
    m_format.imageHeightPels = height;
}

void video_thumbailer_mediafoundation::run(const char * path, album_art_data_ptr & p_out)
{
	TRACK_CALL_TEXT("video_thumbailer_mediafoundation::run");
	bool ret = false;
	HRESULT hr = S_OK;
	pfc::stringcvt::string_os_from_utf8 wpath(path);

	mmh::ComPtr<IMFAttributes> pAttributes;

	hr = m_MFCreateAttributes(pAttributes, 1);
	_check_hresult_mediafoundation(hr);

	hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
	_check_hresult_mediafoundation(hr);

	// Create the source reader from the URL.
	hr = m_MFCreateSourceReaderFromURL(wpath.get_ptr(), pAttributes, m_pReader);
	_check_hresult_mediafoundation(hr);

	// Attempt to find a video stream.

	mmh::ComPtr<IMFMediaType> pType;

	// Configure the source reader to give us progressive RGB32 frames.
	// The source reader will load the decoder if needed.

	hr = m_MFCreateMediaType(pType);
	_check_hresult_mediafoundation(hr);

	hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	_check_hresult_mediafoundation(hr);

	hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	_check_hresult_mediafoundation(hr);

	hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
	_check_hresult_mediafoundation(hr);

	// Ensure the stream is selected.
	hr = m_pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
	_check_hresult_mediafoundation(hr);

	pType.release();

	UpdateFormat();

    DWORD       dwFlags = 0;

    BYTE        *pBitmapData = NULL;    // Bitmap data
    DWORD       cbBitmapData = 0;       // Size of data, in bytes

    mmh::ComPtr<IMFMediaBuffer> pBuffer;
    mmh::ComPtr<IMFSample> pSample;

    {
        PROPVARIANT var;
        PropVariantInit(&var);

        var.vt = VT_I8;
        var.hVal.QuadPart = GetDuration()/10;

        hr = m_pReader->SetCurrentPosition(GUID_NULL, var);
    }

    while (1)
    {
        mmh::ComPtr<IMFSample> pSampleTmp;

        hr = m_pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, &dwFlags, NULL, pSampleTmp);
		_check_hresult_mediafoundation(hr);
    
        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }

        if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            // Type change. Get the new format.
			UpdateFormat();
        }

		if (pSampleTmp.is_valid())
		{
			// We got a sample. Hold onto it.

			pSample = pSampleTmp;
			break;
		}
    }

    if (pSample.is_valid())
    {
        UINT32 pitch = 4 * m_format.imageWidthPels; 

        // Get the bitmap data from the sample

        hr = pSample->ConvertToContiguousBuffer(pBuffer);
		_check_hresult_mediafoundation(hr);

        hr = pBuffer->Lock(&pBitmapData, NULL, &cbBitmapData);
		_check_hresult_mediafoundation(hr);

        assert(cbBitmapData == (pitch * m_format.imageHeightPels));

		long bpp = (32+7)/8;
		long pBufferSize = bpp * m_format.imageWidthPels;
		if (pBufferSize % 4)
			pBufferSize += 4-(pBufferSize % 4);
		pBufferSize *= m_format.imageHeightPels;

		pfc::array_t<t_uint8> bytes;
		bytes.set_size(cbBitmapData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) );
		bytes.fill_null();
		BITMAPFILEHEADER * pbfh = (BITMAPFILEHEADER*)bytes.get_ptr();
		BITMAPINFOHEADER * pbih = (BITMAPINFOHEADER*)(bytes.get_ptr()+sizeof(BITMAPFILEHEADER));
		pbfh->bfOffBits = sizeof (BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
		pbfh->bfSize = bytes.get_size();
		pbfh->bfType = 0x4D42;
		pbih->biSize = sizeof (BITMAPINFOHEADER);
		pbih->biWidth = m_format.imageWidthPels;
		pbih->biHeight = m_format.imageHeightPels * (m_format.bTopDown ? -1 : 1);
		pbih->biPlanes = 1;
		pbih->biBitCount = 32;
		pbih->biCompression = BI_RGB;

		t_uint8 * p_bits = bytes.get_ptr()+ sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		memcpy(p_bits, pBitmapData, cbBitmapData);
		p_out = album_art_data_impl::g_create(bytes.get_ptr(), bytes.get_size());

        pBuffer->Unlock();
    }
    else
    {
		throw pfc::exception("No sample decoded");
    }

}

bool video_thumbailer_t::create_video_thumbnail(const char * path, album_art_data_ptr & p_out)
{
	if (mmh::is_windows_vista_or_newer())
	{
		try 
		{
			m_vtmf.ensure_initialised();
			m_vtmf.run(path, p_out);
			return true;
		}
		catch (pfc::exception const &)
		{
			if (mmh::is_windows_7_or_newer()) throw;
		}
	}
	return create_video_thumbnail_directshow(path, p_out);
	//try
	//{
	//}
}

bool video_thumbailer_t::create_video_thumbnail_directshow(const char * path, album_art_data_ptr & p_out)
{
	TRACK_CALL_TEXT("video_thumbailer_t::create_video_thumbnail");
	bool ret = false;
	HRESULT hr = S_OK;
	pfc::stringcvt::string_os_from_utf8 wpath(path);

	//coinitialise_scope p_coinit(COINIT_MULTITHREADED);

	{
		{

			mmh::ComPtr<IGraphBuilder> pGraph;
			hr = pGraph.instantiate(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
			_check_hresult(hr);
			mmh::ComPtr<IBaseFilter> pSourceFilter;
			hr = pGraph->AddSourceFilter(wpath.get_ptr(), L"Source", pSourceFilter);
			_check_hresult(hr);

			mmh::ComPtr<IBaseFilter> pNullFilter;
			hr = pNullFilter.instantiate(CLSID_NullRenderer);
			_check_hresult(hr);
			hr = pGraph->AddFilter(pNullFilter, L"NullFilter");
			_check_hresult(hr);

			mmh::ComPtr<ISampleGrabber> pSampleGrabber;
			hr = pSampleGrabber.instantiate(CLSID_SampleGrabber);
			_check_hresult(hr);
			mmh::ComPtr<IBaseFilter> pSampleGrabberFilter = pSampleGrabber;
			hr = pGraph->AddFilter(pSampleGrabberFilter, L"SampleGrabber");
			_check_hresult(hr);

			AM_MEDIA_TYPE amt;
			memset(&amt, 0, sizeof(amt));
			amt.majortype = MEDIATYPE_Video;
			amt.subtype = MEDIASUBTYPE_RGB24;
			amt.formattype = FORMAT_VideoInfo;
			hr = pSampleGrabber->SetMediaType(&amt);
			_check_hresult(hr);

			{
				filter_pin_enum pinenum(pSourceFilter);
				filter_pin_enum pinenum2(pSampleGrabberFilter);
				filter_pin_enum pinenum3(pNullFilter);

				t_size i, count = pinenum.m_outputs.get_count();
				for (i=0; i<count; i++)
				{
					mmh::ComPtr<IEnumMediaTypes> pEnumMediaTypes;
					pinenum.m_outputs[i]->EnumMediaTypes(pEnumMediaTypes);
					AM_MEDIA_TYPE * pamt;
					bool b_video = false;
					while (S_OK == pEnumMediaTypes->Next(1, &pamt, NULL))
					{
						if (pamt->majortype == MEDIATYPE_Video)
						{
							b_video=true;

						}
						DeleteMediaType(pamt);
						if (b_video) break;
					}
					if (b_video)
					{
						if (pinenum2.m_inputs.get_count())
						{
							hr = pGraph->Connect(pinenum.m_outputs[i], pinenum2.m_inputs[0]);
							_check_hresult(hr);
						}
						if (pinenum2.m_outputs.get_count() && pinenum3.m_inputs.get_count())
						{
							hr = pGraph->Connect(pinenum2.m_outputs[0], pinenum3.m_inputs[0]);
							_check_hresult(hr);
						}
						break;
					}
				}
			}

			//mmh::ComPtr<IBasicAudio> pBasicAudio = pGraph;
			//mmh::ComPtr<IVideoWindow> pVideoWindow = pGraph;
			mmh::ComPtr<IMediaEventEx> pMediaEventEx = pGraph;
			mmh::ComPtr<IMediaSeeking> pMediaSeeking = pGraph;
			mmh::ComPtr<IMediaControl> pMediaControl = pGraph;
			mmh::ComPtr<IMediaFilter> pMediaFilter  = pGraph;

			if (!pMediaEventEx.is_valid() || !pMediaSeeking.is_valid() || !pMediaControl.is_valid() || !pMediaFilter.is_valid())
				throw pfc::exception("Failed to query IGraphBuilder interface");

			pMediaFilter->SetSyncSource(NULL);

			/*
			if (pBasicAudio.is_valid())
			hr = pBasicAudio->put_Volume(-10000);
			if (pVideoWindow.is_valid())
			hr = pVideoWindow->put_AutoShow(OAFALSE);
			*/

			hr = pSampleGrabber->SetOneShot(TRUE);
			_check_hresult(hr);
			hr = pSampleGrabber->SetBufferSamples(TRUE);
			_check_hresult(hr);

			LONGLONG duration = NULL;
			hr = pMediaSeeking->GetDuration(&duration);
			_check_hresult(hr);

			LONGLONG start = duration / 10;
			LONGLONG stop = duration;

			hr = pMediaSeeking->SetPositions(&start, AM_SEEKING_AbsolutePositioning, &stop, AM_SEEKING_AbsolutePositioning);
			_check_hresult(hr);

			hr = pMediaControl->Run();
			_check_hresult(hr);

			long evcode;
			hr = pMediaEventEx->WaitForCompletion(pfc_infinite, &evcode);
			_check_hresult(hr);

			VIDEOINFOHEADER vih;
			memset(&vih, 0, sizeof(vih));

			hr = pSampleGrabber->GetConnectedMediaType(&amt);
			_check_hresult(hr);

			if (amt.formattype == FORMAT_VideoInfo)
			{
				if (amt.cbFormat >= sizeof(VIDEOINFOHEADER))
				{
					VIDEOINFOHEADER *pvih = reinterpret_cast<VIDEOINFOHEADER*>(amt.pbFormat);
					{
						if (pvih->bmiHeader.biCompression == BI_RGB)
						{
							try
							{
								//void * p_data = NULL;
								//ret = CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&pvih->bmiHeader), DIB_RGB_COLORS, &p_data,0,0);

								long bpp = (pvih->bmiHeader.biBitCount+7)/8;
								long pBufferSize = bpp * pvih->bmiHeader.biWidth;
								if (pBufferSize % 4)
									pBufferSize += 4-(pBufferSize % 4);
								pBufferSize *= pvih->bmiHeader.biHeight;

								long pRequiredSize = NULL;
								hr = pSampleGrabber->GetCurrentBuffer(&pRequiredSize, NULL);
								if (hr == VFW_E_WRONG_STATE)
									throw pfc::exception("No video samples were decoded. This usually indicates a problem with an installed DirectShow filter.");
								_check_hresult(hr);
								pfc::array_t<t_uint8> bytes;
								bytes.set_size(pRequiredSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) );
								bytes.fill_null();
								BITMAPFILEHEADER * pbfh = (BITMAPFILEHEADER*)bytes.get_ptr();
								BITMAPINFOHEADER * pbih = (BITMAPINFOHEADER*)(bytes.get_ptr()+sizeof(BITMAPFILEHEADER));
								pbfh->bfOffBits = sizeof (BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
								pbfh->bfSize = bytes.get_size();
								pbfh->bfType = 0x4D42;
								*pbih = pvih->bmiHeader;
								t_uint8 * p_bits = bytes.get_ptr()+ sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

								hr = pSampleGrabber->GetCurrentBuffer(&pRequiredSize, (long*)(p_bits));
								_check_hresult(hr);
								p_out = album_art_data_impl::g_create(bytes.get_ptr(), bytes.get_size());
								ret=true;
								//memcpy(p_data, bytes.get_ptr(), min(pRequiredSize, pBufferSize));
							} catch (pfc::exception const &) {FreeMediaType(amt); throw;}
						}

					}

				}

			}
			FreeMediaType(amt);
		}
		//CoFreeUnusedLibrariesEx(0, NULL);
	}

	return ret;
}
