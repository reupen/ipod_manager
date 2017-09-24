#include "stdafx.h"

#if 0
int zlib_handle::_deflateInit(z_streamp strm, int level)
{
	return _deflateInit_((strm), (level), ZLIB_VERSION, sizeof(z_stream));
}
int zlib_handle::_inflateInit(z_streamp strm)
{
	return _inflateInit_((strm), ZLIB_VERSION, sizeof(z_stream));
}
#endif