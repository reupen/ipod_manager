#ifndef _DOP_MAIN_H_
#define _DOP_MAIN_H_

#define _WIN32_WINNT 0x0601 // Windows 7
#define OEMRESOURCE

// Bizarrely, without this <dshow.h> will define NO_SHLWAPI_STRFCNS, which causes
// functions like StrCmpLogicalW to not be available if dshow.h is included 
// before shlwapi.h
#define NO_DSHOW_STRSAFE

// 'this' : used in base member initializer list
#pragma warning( disable : 4355 )

//#define LOAD_LIBRARY_INDICES
//#define PHOTO_BROWSER

#include <optional>
#include <regex>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <commctrl.h>
#include <windowsx.h>
#include <dshow.h>
#include <gdiplus.h>
#include <uxtheme.h>
#include <wincodec.h>

#include <dbt.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

// Speech API
#define _SAPI_VER 0x051

#include <sapi.h>
#include <sphelper.h>

// Media Foundation 
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>


#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"

#include "../mmh/stdafx.h"
#include "../ui_helpers/stdafx.h"
#include "../fbh/stdafx.h"
#include "../dop-sdk/dop.h"
#include "../columns_ui-sdk/ui_extension.h"

#include "../qedit/qedit.h"
#include "../sqlite/sqlite3.h"
#include "../zlib-1.2.5/zlib.h"
#include "../MobileDeviceSign/stdafx.h"

typedef pfc::refcounted_object_ptr_t<class ipod_device_t> ipod_device_ptr_t;
typedef ipod_device_ptr_t & ipod_device_ptr_ref_t;
typedef const ipod_device_ptr_t & ipod_device_ptr_cref_t;

#endif //_DOP_MAIN_H_