#include "main.h"

#include "file_adder_conversion.h"
#include "speech.h"

const speech_map speech_map_list[] = 
{
	speech_map("ft.","featuring"),
	speech_map("feat.","featuring"),
	speech_map("feat:","featuring"),
	speech_map("II","2"),
	speech_map("III","3"),
	speech_map("IV","4"),  
	speech_map("$hort","Short"),
	speech_map("10cc","10 cc"),
	speech_map("112","One Twelve"),  
	speech_map("2na","Tuna"),  
	speech_map("2Pac","Two-Pock"),
	//speech_map("40oz","40 ounce"), "American vernacular" says wikipedia
	//speech_map("40oz.","40 ounce"),  
	speech_map("Collide0Scope","kaleidoscope"),
	speech_map("India.Arie","India Arie"),
	speech_map("LvUrFR3NZ","Love your friends"),
	speech_map("M!ssundaztood","Misunderstood"),
	speech_map("M!ch!gan","Michigan"),
	speech_map("Ma$e","Mace"),
	speech_map("M+M's","M & M's"),
	speech_map("E=MC","E equals M C Squared"),
	speech_map("E=MC2","E equals M C Squared"),
	speech_map("N2Deep","Inn too deep"),
	speech_map("N9ne","Nine"),
	speech_map("P$C","Pimp Squad Clique"),
	speech_map("p!nk","Pink"),
	speech_map("Rnw@y","Run away"),
	speech_map("will.i.am","Will I Am"),
	speech_map("AppleTV","apple TV"),
	speech_map("iPod","eye pod"),
	speech_map("iTunes","eye tunes"),
	speech_map("iPhone","eye phone"),
	speech_map("iLife","eye life"),
	speech_map("iMix","eye mix"),
	speech_map("iMac","eye Mac")
};

speech_string_preprocessor::speech_string_preprocessor()
{
	m_maps.add_items(speech_map_list);
	m_maps.sort_t(speech_map::g_compare);
}


template<class TBase = IStream>
class IStream_memblock_v2 : public TBase
{
	long refcount;
	pfc::array_t<t_uint8> m_data;
	t_size m_position;
public:

	const void * get_ptr() const {return m_data.get_ptr();} ;
	t_size get_size() const {return m_data.get_size();};

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void ** ppvObject)
	{
		if (ppvObject==0) return E_NOINTERFACE;
		else if (iid == IID_IUnknown) {AddRef();*ppvObject = (IUnknown*)this;return S_OK;}
		else if (iid == IID_IStream) {AddRef();*ppvObject = (IStream*)this;return S_OK;}
		else if (iid == IID_ISequentialStream) {AddRef();*ppvObject = (ISequentialStream*)this;return S_OK;}
		else {*ppvObject = NULL; return E_NOINTERFACE;}
	}
	virtual ULONG STDMETHODCALLTYPE AddRef() {return InterlockedIncrement(&refcount);}
	virtual ULONG STDMETHODCALLTYPE Release()
	{
		LONG rv = InterlockedDecrement(&refcount);
		if (!rv) delete this;
		return rv;
	}

	virtual HRESULT STDMETHODCALLTYPE Seek( 
		LARGE_INTEGER dlibMove,
		DWORD dwOrigin,
		ULARGE_INTEGER *plibNewPosition)
	{
		if (dwOrigin == STREAM_SEEK_CUR)
		{
			if (dlibMove.QuadPart + (LONGLONG)m_position < 0
				|| dlibMove.QuadPart + (LONGLONG)m_position > MAXDWORD)
				return STG_E_INVALIDFUNCTION;
			m_position += (t_size)dlibMove.QuadPart; //Cast should be OK by if condition
		}
		else if (dwOrigin == STREAM_SEEK_END)
		{
			if (dlibMove.QuadPart + (LONGLONG)m_data.get_size() < 0)
				return STG_E_INVALIDFUNCTION;
			m_position = m_data.get_size() - (t_size)dlibMove.QuadPart; //Cast should be OK by if condition
		}
		else if (dwOrigin == STREAM_SEEK_SET)
		{
			if ((ULONGLONG)dlibMove.QuadPart > MAXDWORD)
				return STG_E_INVALIDFUNCTION;
			m_position = (t_size)dlibMove.QuadPart; //Cast should be OK by if condition
		}
		else
			return STG_E_INVALIDFUNCTION;
		if (plibNewPosition)
			plibNewPosition->QuadPart = m_position;
		return S_OK;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
		/* [length_is][size_is][out] */ void *pv,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG *pcbRead)
	{
		if (m_position > m_data.get_size())
			return STG_E_INVALIDFUNCTION;
		t_size read = min (cb, m_data.get_size() - m_position);
		memcpy(pv, &m_data.get_ptr()[m_position], read);
		m_position += read;
		if (pcbRead)
			*pcbRead = read;
		return S_OK;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
		/* [size_is][in] */ const void *pv,
		/* [in] */ ULONG cb,
		/* [out] */ ULONG *pcbWritten)
	{
		t_size old_size = m_data.get_size(), new_size = m_position + cb;
		if (new_size > old_size)
		{
			m_data.grow_size(new_size);
			memset(&m_data.get_ptr()[old_size], 0,new_size-new_size);
		}
		memcpy(&m_data.get_ptr()[m_position], pv, cb);
		m_position += cb;
		if (pcbWritten)
			*pcbWritten = cb;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetSize( 
		ULARGE_INTEGER libNewSize)
	{
		if (libNewSize.QuadPart > MAXDWORD)
			return STG_E_INVALIDFUNCTION;
		m_data.set_size((t_size)libNewSize.QuadPart);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CopyTo( 
		/* [unique][in] */ IStream *pstm,
		/* [in] */ ULARGE_INTEGER cb,
		/* [out] */ ULARGE_INTEGER *pcbRead,
		/* [out] */ ULARGE_INTEGER *pcbWritten)
	{
		if (cb.QuadPart > m_data.get_size() - m_position)
			return STG_E_INVALIDFUNCTION;
		t_size read = min ((t_size)cb.QuadPart, m_data.get_size() - m_position);
		ULONG pwritten = NULL;
		pstm->Write(&m_data.get_ptr()[m_position], read, &pwritten);
		m_position += read;
		if (pcbRead)
			pcbRead->QuadPart = read;
		if (pcbWritten)
			pcbWritten->QuadPart = pwritten;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Commit( 
		/* [in] */ DWORD grfCommitFlags)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Revert( void)
	{
		return E_FAIL;
	}

	virtual HRESULT STDMETHODCALLTYPE LockRegion( 
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Stat( 
		/* [out] */ __RPC__out STATSTG *pstatstg,
		/* [in] */ DWORD grfStatFlag)
	{
		memset(pstatstg, 0, sizeof(STATSTG));
		pstatstg->cbSize.QuadPart = m_data.get_size();
		pstatstg->type = STGTY_STREAM;
		pstatstg->pwcsName = NULL;
		/*if (!(grfStatFlag & STATFLAG_NONAME))
		{
		if (pstatstg->pwcsName = (LPOLESTR)CoTaskMemAlloc(1*2))
		wcscpy_s(pstatstg->pwcsName, 5, L"AB.jpg");
		}*/
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone( 
		/* [out] */ __RPC__deref_out_opt IStream **ppstm)
	{
		*ppstm = new IStream_memblock_v2<IStream>(m_data.get_ptr(), m_data.get_size());
		return S_OK;
	}

	IStream_memblock_v2(const t_uint8 * p_data, t_size size) : refcount(0), m_position(0) {m_data.append_fromptr(p_data, size);};
	IStream_memblock_v2() : refcount(0), m_position(0) {};
};

class ISpStreamFormat_memblock : public IStream_memblock_v2<ISpStreamFormat>, public CSpStreamFormat
{
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void ** ppvObject)
	{
		if (ppvObject==0) return E_NOINTERFACE;
		else if (iid == IID_ISpStreamFormat) {AddRef();*ppvObject = (ISpStreamFormat*)this;return S_OK;}
		else return IStream_memblock_v2<ISpStreamFormat>::QueryInterface(iid, ppvObject);
	}
	HRESULT STDMETHODCALLTYPE GetFormat( 
		__RPC__in GUID *pguidFormatId,
		__RPC__deref_in_opt WAVEFORMATEX **ppCoMemWaveFormatEx)
	{
		*pguidFormatId = m_guidFormatId;
		if (ppCoMemWaveFormatEx)
		{
			(*ppCoMemWaveFormatEx) = (LPWAVEFORMATEX)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
			(**ppCoMemWaveFormatEx) = *m_pCoMemWaveFormatEx;
		}
		return S_OK;

	}

	//ISpStreamFormat_memblock() : IStream_memblock_v2<ISpStreamFormat>(0, 0) {};
};

void sapi::run_mapped (const char * text, unsigned samplerate, const char * path)
{
	pfc::string8 mapped_text;
	m_preprocessor.run(text, mapped_text);
	run(mapped_text, samplerate, path);
}

void sapi::run (const char * text, unsigned samplerate, const char * path)
{
	HRESULT hr = E_FAIL;

	ISpStreamFormat_memblock * pSpStreamFormat_memblock = new ISpStreamFormat_memblock;
	mmh::ComPtr<ISpStreamFormat> pSpStreamFormat = pSpStreamFormat_memblock, pCurSpStreamFormat;

#if 0
	CSpStreamFormat	cAudioFmt;
	mmh::ComPtr<ISpStream> cpStream;

	hr = cAudioFmt.AssignFormat(SPSF_16kHz16BitMono);
	_check_hresult(hr);

	hr = SPBindToFile( pfc::stringcvt::string_wide_from_utf8(path),  SPFM_CREATE_ALWAYS, cpStream.get_pp(), &cAudioFmt.FormatId(), cAudioFmt.WaveFormatExPtr());
	_check_hresult(hr);
#endif

	hr = m_SpVoice->GetOutputStream(pCurSpStreamFormat.get_pp());
	_check_hresult(hr);

	GUID fmt; WAVEFORMATEX * wfe = NULL;
	hr = pCurSpStreamFormat->GetFormat(&fmt, &wfe);
	_check_hresult(hr);
	if (wfe == NULL)
		_check_hresult(E_FAIL);
	if (wfe->nChannels > 1)
	{
		wfe->nBlockAlign /= wfe->nChannels;
		wfe->nAvgBytesPerSec/= wfe->nChannels;
		wfe->nChannels = 1;
	}
	hr = pSpStreamFormat_memblock->AssignFormat(wfe);
	if (wfe) CoTaskMemFree(wfe);
	_check_hresult(hr);

#if 0
	hr = m_SpVoice->SetOutput( cpStream, FALSE );
#else
	hr = m_SpVoice->SetOutput( pSpStreamFormat, FALSE );
#endif
	_check_hresult(hr);

	hr = m_SpVoice->Speak( pfc::stringcvt::string_wide_from_utf8(text),  SPF_DEFAULT, NULL );
	_check_hresult(hr);

#if 0
	hr = cpStream->Close();
	return;
#endif

	const WAVEFORMATEX * pwfex = pSpStreamFormat_memblock->WaveFormatExPtr();
	if (pwfex == NULL)
		_check_hresult(E_FAIL);

	{
		static_api_ptr_t<audio_postprocessor> processor;
		dsp::ptr resampler;
		dsp_chunk_list_impl resampler_chunks;
		audio_chunk_impl chunk;
		mem_block_container_impl_t<pfc::alloc_fast_aggressive> chunk2;
		pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> finalOutStream;

		if (!resampler_entry::g_create(resampler, chunk.get_sample_rate(), samplerate, 1.0))
			throw pfc::exception( pfc::string8() << "Could not create resampler (" << chunk.get_sample_rate() << " Hz -> " << samplerate << " Hz)");

		chunk.set_data_fixedpoint(pSpStreamFormat_memblock->get_ptr(), pSpStreamFormat_memblock->get_size(), pwfex->nSamplesPerSec, pwfex->nChannels, pwfex->wBitsPerSample, 1);
		resampler_chunks.add_chunk(&chunk);
		resampler->run(&resampler_chunks, metadb_handle_ptr(), dsp::FLUSH);

		t_riff_header riff;
		riff.id0 = 'R'|'I'<<8|'F'<<16|'F'<<24;
		riff.id1 = 'f'|'m'<<8|'t'<<16|' '<<24;
		riff.type0 = 'W'|'A'<<8|'V'<<16|'E'<<24;
		riff.id2 = 'd'|'a'<<8|'t'<<16|'a'<<24;
		riff.headersize = sizeof(riff.header);

		riff.header.wFormatTag=WAVE_FORMAT_PCM;

		riff.header.nSamplesPerSec = samplerate;//pwfex->nSamplesPerSec;
		riff.header.nChannels = pwfex->nChannels;
		riff.header.wBitsPerSample = pwfex->wBitsPerSample;
		riff.header.nBlockAlign=(riff.header.nChannels*riff.header.wBitsPerSample )/8;
		riff.header.nAvgBytesPerSec =(riff.header.nBlockAlign*riff.header.nSamplesPerSec );

		riff.datasize = (unsigned)chunk.get_sample_count()*riff.header.nBlockAlign;
		riff.filesize = riff.datasize + sizeof(riff);

		abort_callback_impl p_abort;
		file::ptr p_file;
		filesystem::g_open_write_new(p_file, path, p_abort);
		t_size i, count = resampler_chunks.get_count();
		for (i=0; i<count; i++)
		{
			audio_chunk * pChunk = resampler_chunks.get_item(i);
			if (pChunk)
			{
				processor->run(*pChunk, chunk2, 16, 16, false, 1.0);
				finalOutStream.append_fromptr((t_uint8*)chunk2.get_ptr(), chunk2.get_size());
			}
		}
		resampler_chunks.remove_all();
		riff.datasize = (unsigned)finalOutStream.get_size();
		riff.filesize = riff.datasize + sizeof(riff);
		p_file->write(&riff, sizeof(riff), p_abort);
		p_file->write(finalOutStream.get_ptr(), finalOutStream.get_size(), p_abort);
	}

}
