#ifndef _DOP_MAIN_H_
#define _DOP_MAIN_H_

#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define OEMRESOURCE
// Bizarrely, without this <dshow.h> will define NO_SHLWAPI_STRFCNS, which causes
// functions like StrCmpLogicalW to not be available if dshow.h is included 
// before shlwapi.h
#define NO_DSHOW_STRSAFE

// 'this' : used in base member initializer list
#pragma warning( disable : 4355 )

//#define LOAD_LIBRARY_INDICES
//#define PHOTO_BROWSER

#include <winsock2.h>
#include <ws2tcpip.h>
#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#include <dshow.h>
#include "..\qedit\qedit.h"
#include <commctrl.h>
#include <windowsx.h>
#include <Dbt.h>
#include <gdiplus.h>

#define WINVERBACKUP WINVER
#undef WINVER
#define WINVER _WIN32_WINNT_WIN7

// Media Foundation 
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#undef WINVER
#define WINVER WINVERBACKUP
#undef WINVERBACKUP

#include <uxtheme.h>
#include <Wincodec.h>
#include "../mmh/stdafx.h"
#include "../ui_helpers/stdafx.h"
#include "../fbh/stdafx.h"
#include "../dop-sdk/dop.h"
#include "../columns_ui-sdk/ui_extension.h"
#include "resource.h"

class stream_writer_mem : public stream_writer, public pfc::array_t<t_uint8, pfc::alloc_fast_aggressive>
{
public:
	stream_writer_mem() {};
	void write(const void * p_buffer, t_size p_bytes, abort_callback & p_abort)
	{
		append_fromptr((t_uint8*)p_buffer, p_bytes);
	}
};

#include "zlib.h"
#include "speech.h"
#include "iPhoneCalc.h"

#include "cfobject.h"
#include "mobile_device.h"
#include "mobile_device_v2.h"

#include "helpers.h"
#include "results.h"

#include "sqlite.h"
#include "../MobileDeviceSign/stdafx.h"



class NOVTABLE mainmenu_command_t
{
public:
	virtual const GUID & get_guid() const=0;
	virtual void get_name(pfc::string_base & p_out)const = 0;
	virtual bool get_description(pfc::string_base & p_out) const = 0;
	virtual bool get_display(pfc::string_base & p_text,t_uint32 & p_flags) const 
	{
		p_flags = 0;
		get_name(p_text);
		return true;
	}
	virtual void execute(service_ptr_t<service_base> p_callback) const = 0;
};


typedef pfc::refcounted_object_ptr_t<class ipod_device_t> ipod_device_ptr_t;
typedef ipod_device_ptr_t & ipod_device_ptr_ref_t;
typedef const ipod_device_ptr_t & ipod_device_ptr_cref_t;

#include "config.h"
#include "config_database.h"
#include "config_features.h"
#include "config_conversion.h"
#include "config_behaviour.h"
#include "config_ios.h"
#include "itunesdb.h"
#include "dopdb.h"
#include "shadowdb.h"
#include "chapter.h"
#include "photodb.h"
#include "bplist.h"

using namespace itunesdb;

#include "lock.h"

#include "resource.h"

#include "mp4.h"

#include "reader.h"
#include "ipod_manager.h"
#include "plist.h"
#include "ipod_scanner.h"
#include "prepare.h"
#include "writer_sort_helpers.h"
#include "writer.h"
#include "file_adder.h"
#include "file_adder_conversion.h"
#include "file_remover.h"
#include "sync_logic.h"
#include "gapless_scanner.h"


#endif //_DOP_MAIN_H_