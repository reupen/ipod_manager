#include "main.h"

#include "sqlite.h"

#if 1

struct sqlite3_file_fb2k : public sqlite3_file
{
	file::ptr * m_file;
};


int xClose(sqlite3_file* p_file)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	if (p_fb2kfile->m_file)
	{
		delete p_fb2kfile->m_file;
		p_fb2kfile->m_file = NULL;
		return SQLITE_OK;
	}
	return SQLITE_IOERR_CLOSE;
}

int xRead(sqlite3_file* p_file, void* buff, int iAmt, sqlite3_int64 iOfst)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	if (p_fb2kfile->m_file)
	{
		try
		{
			t_size read = 0;
			try {
				if (iOfst < p_fb2kfile->m_file->get_ptr()->get_size_ex(abort_callback_dummy()))
				{
				p_fb2kfile->m_file->get_ptr()->seek(iOfst, abort_callback_dummy());
				read = p_fb2kfile->m_file->get_ptr()->read(buff, iAmt, abort_callback_dummy());
				}
			} catch (exception_io_seek_out_of_range const &) {};
			
			if (iAmt > 0 && read < (unsigned)iAmt)
			{
				memset((t_uint8*)buff + read, 0, iAmt-read);
				return SQLITE_IOERR_SHORT_READ;
			}
			return SQLITE_OK;
		} catch (pfc::exception const & ex) 
		{
			console::formatter() << "iPod manager: xRead error: " << ex.what();
		}
	}
	return SQLITE_IOERR_READ;
}

int xWrite(sqlite3_file* p_file, const void* buff, int iAmt, sqlite3_int64 iOfst)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	if (p_fb2kfile->m_file)
	{
		try
		{
			abort_callback_dummy noabort;
			if (iOfst > p_fb2kfile->m_file->get_ptr()->get_size_ex(noabort))
				p_fb2kfile->m_file->get_ptr()->resize(iOfst, noabort);
			p_fb2kfile->m_file->get_ptr()->seek(iOfst, noabort);
			p_fb2kfile->m_file->get_ptr()->write(buff, iAmt, noabort);
			return SQLITE_OK;
		} catch (pfc::exception const & ex) 
		{
			console::formatter() << "iPod manager: xWrite error: " << ex.what();
		}

	}
	return SQLITE_IOERR_WRITE;
}

int xTruncate(sqlite3_file* p_file, sqlite3_int64 size)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	if (p_fb2kfile->m_file)
	{
		try
		{
			p_fb2kfile->m_file->get_ptr()->resize(size, abort_callback_dummy());
			//p_fb2kfile->m_file->get_ptr()->seek(size, abort_callback_dummy());
			//p_fb2kfile->m_file->get_ptr()->set_eof(abort_callback_dummy());
			return SQLITE_OK;
		} catch (pfc::exception const & ex) 
		{
			console::formatter() << "iPod manager: xTruncate error: " << ex.what();
		}
	}
	return SQLITE_IOERR_TRUNCATE;
}
int xSync(sqlite3_file* p_file, int flags)
{
    return SQLITE_OK;
}
int xFileSize(sqlite3_file* p_file, sqlite3_int64 *pSize)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	if (p_fb2kfile->m_file)
	{
		try
		{
			*pSize = p_fb2kfile->m_file->get_ptr()->get_size_ex(abort_callback_dummy());
			return SQLITE_OK;
		} 
		catch (pfc::exception const & ex) 
		{
			console::formatter() << "iPod manager: xFileSize error: " << ex.what();
		}
	}
	return SQLITE_IOERR_FSTAT;
}
int xLock(sqlite3_file* p_file, int)
{
    return SQLITE_OK;
}
int xUnlock(sqlite3_file* p_file, int)
{
    return SQLITE_OK;
}
int xCheckReservedLock(sqlite3_file* p_file, int *pResOut)
{
	*pResOut = FALSE;
    return SQLITE_OK;
}
int xFileControl(sqlite3_file* p_file, int op, void *pArg)
{
	switch( op )
	{
	case SQLITE_FCNTL_LOCKSTATE: 
		{
			*(int*)pArg = NULL;
			return SQLITE_OK;
		}
	case SQLITE_LAST_ERRNO: 
		{
			*(int*)pArg = NULL;
			return SQLITE_OK;					
		}
	case SQLITE_FCNTL_SIZE_HINT:
		{
			sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
			if (p_fb2kfile->m_file)
			{
				try
				{
					sqlite3_int64 p_size = *(sqlite3_int64*)pArg;
					abort_callback_dummy p_abort;
					//if (p_size < p_fb2kfile->m_file->get_ptr()->get_size_ex(p_abort))
						p_fb2kfile->m_file->get_ptr()->resize(p_size, p_abort);
					return SQLITE_OK;
				} 
				catch (pfc::exception const & ex) 
				{
					console::formatter() << "iPod manager: xFileControl error: " << ex.what();
				}
			}
			return SQLITE_OK;
		}
	case SQLITE_FCNTL_SYNC_OMITTED:
		return SQLITE_OK;

	};

	{
		console::formatter() << "iPod manager: xFileControl error: " << "unknown op: " << op;
	}
	return SQLITE_ERROR;
}
int xSectorSize(sqlite3_file* p_file) {return 512;}
int xDeviceCharacteristics(sqlite3_file* p_file) {return 0;}


const sqlite3_io_methods xIoMethod = {
  1,                        /* iVersion */
  xClose,
  xRead,
  xWrite,
  xTruncate,
  xSync,
  xFileSize,
  xLock,
  xUnlock,
  xCheckReservedLock,
  xFileControl,
  xSectorSize,
  xDeviceCharacteristics
};











int xOpen(sqlite3_vfs*, const char *zName, sqlite3_file* p_file,int flags, int *pOutFlags)
{
	sqlite3_file_fb2k * p_fb2kfile = static_cast<sqlite3_file_fb2k*>(p_file);
	p_fb2kfile->pMethods = &xIoMethod;
	p_fb2kfile->m_file = NULL;
	filesystem::t_open_mode fb2kMode = filesystem::open_mode_read;
	pfc::string8 temp;
	if (!zName)
	{
		uGetTempPath(temp);
		temp;// << "\\";
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		srand(ft.dwLowDateTime);
		temp << (unsigned) ft.dwHighDateTime << (unsigned) ft.dwLowDateTime << rand() << ".dop.temp";
		zName = temp.get_ptr();
	}
	if (flags & SQLITE_OPEN_READONLY)
	{
		fb2kMode = filesystem::open_mode_read;
		if (pOutFlags)
			*pOutFlags = SQLITE_OPEN_READONLY;
	}
	else if (flags & SQLITE_OPEN_READWRITE)
	{
		fb2kMode = filesystem::open_mode_write_existing;
		if (pOutFlags)
			*pOutFlags = SQLITE_OPEN_READWRITE;
	}
	else if ( (flags & (SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE)) == (SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE) )
	{
		fb2kMode = filesystem::open_mode_write_new;
		*pOutFlags = SQLITE_OPEN_CREATE|SQLITE_OPEN_EXCLUSIVE;
	}
	else if (flags & (SQLITE_OPEN_CREATE))
	{
		fb2kMode = filesystem::open_mode_write_existing;
		*pOutFlags = SQLITE_OPEN_CREATE;
	}
	else return SQLITE_NOLFS;

	try
	{
		file::ptr ptr;
		try {
		filesystem::g_open(ptr, zName, fb2kMode, abort_callback_dummy());
		}
		catch (exception_io_not_found const &)
		{
			if (flags & (SQLITE_OPEN_CREATE))
			{
				filesystem::g_open_write_new(ptr, zName, abort_callback_dummy());
				if (pOutFlags)
					*pOutFlags |= SQLITE_OPEN_CREATE;
			}
			else throw;			
		}

		p_fb2kfile->m_file = new file::ptr(ptr.get_ptr());
	}
	catch (const exception_io & ex) 
	{
		console::formatter() << "iPod manager: xOpen error: " << ex.what();
		return SQLITE_IOERR;
	}

	return SQLITE_OK;
}

int xDelete(sqlite3_vfs*, const char *zName, int syncDir)
{
	try {
		filesystem::g_remove(zName, abort_callback_dummy());
	} 
	catch (exception_io_not_found const &) 
	{
		//console::formatter() << "iPod manager: xDelete error: " << ex.what();
		//return SQLITE_IOERR;
	}
	catch (pfc::exception const & ex) 
	{
		console::formatter() << "iPod manager: xDelete error: " << ex.what();
		return SQLITE_IOERR;
	}
	return SQLITE_OK;
}

int xAccess(sqlite3_vfs*, const char *zName, int flags, int *pResOut)
{
	*pResOut = FALSE;
	try {
		if (flags == SQLITE_ACCESS_READWRITE)
		{
			if (filesystem::g_exists_writeable(zName, abort_callback_dummy()))
			{
				*pResOut = TRUE;
			}
		}
		else// if (flags == SQLITE_ACCESS_READWRITE)
		{
			if (filesystem::g_exists(zName, abort_callback_dummy()))
			{
				*pResOut = TRUE;
			}
		}
	}
#if 0
	catch (exception_io_denied const &) 
	{
		if (flags == SQLITE_ACCESS_EXISTS)
			*pResOut = TRUE;
		else
			*pResOut = FALSE;
	}
#endif
	catch (pfc::exception const & ex) 
	{
		console::formatter() << "iPod manager: xAccess error: " << ex.what();
		return SQLITE_IOERR;
	}
	return SQLITE_OK;
}

int xFullPathname(sqlite3_vfs*, const char *zName, int nOut, char *zOut)
{
	try {
		pfc::string8 temp;
		filesystem::g_get_canonical_path(zName, temp);
		t_size len = temp.get_length();
		if (nOut < 0 || len + 1 > (t_size)nOut)
			return SQLITE_IOERR;
		memcpy(zOut, temp.get_ptr(), len+1);
	} catch (pfc::exception const & ex) 
	{
		console::formatter() << "iPod manager: xFullPathname error: " << ex.what();
		return SQLITE_IOERR;
	}
	return SQLITE_OK;
}
//void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
//void (*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
//void (*(*xDlSym)(sqlite3_vfs*,void*, const char *zSymbol))(void);
//void (*xDlClose)(sqlite3_vfs*, void*);

/*
** Write up to nBuf bytes of randomness into zBuf.
*/
static int xRandomness(sqlite3_vfs *pVfs, int nBuf, char *zBuf){
  int n = 0;
  if( sizeof(SYSTEMTIME)<=nBuf-n ){
    SYSTEMTIME x;
    GetSystemTime(&x);
    memcpy(&zBuf[n], &x, sizeof(x));
    n += sizeof(x);
  }
  if( sizeof(DWORD)<=nBuf-n ){
    DWORD pid = GetCurrentProcessId();
    memcpy(&zBuf[n], &pid, sizeof(pid));
    n += sizeof(pid);
  }
  if( sizeof(DWORD)<=nBuf-n ){
    DWORD cnt = GetTickCount();
    memcpy(&zBuf[n], &cnt, sizeof(cnt));
    n += sizeof(cnt);
  }
  if( sizeof(LARGE_INTEGER)<=nBuf-n ){
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    memcpy(&zBuf[n], &i, sizeof(i));
    n += sizeof(i);
  }
  return n;
}


/*
** Sleep for a little while.  Return the amount of time slept.
*/
static int xSleep(sqlite3_vfs *pVfs, int microsec){
  Sleep((microsec+999)/1000);
  return ((microsec+999)/1000)*1000;
}


/*
** Find the current time (in Universal Coordinated Time).  Write the
** current time and date as a Julian Day number into *prNow and
** return 0.  Return 1 if the time and date cannot be found.
*/
int xCurrentTime(sqlite3_vfs *pVfs, double *prNow){
  FILETIME ft;
  /* FILETIME structure is a 64-bit value representing the number of 
     100-nanosecond intervals since January 1, 1601 (= JD 2305813.5). 
  */
  sqlite3_int64 timeW;   /* Whole days */
  sqlite3_int64 timeF;   /* Fractional Days */

  /* Number of 100-nanosecond intervals in a single day */
  static const sqlite3_int64 ntuPerDay = 
      10000000*(sqlite3_int64)86400;

  /* Number of 100-nanosecond intervals in half of a day */
  static const sqlite3_int64 ntuPerHalfDay = 
      10000000*(sqlite3_int64)43200;

  /* 2^32 - to avoid use of LL and warnings in gcc */
  static const sqlite3_int64 max32BitValue = 
      (sqlite3_int64)2000000000 + (sqlite3_int64)2000000000 + (sqlite3_int64)294967296;

  GetSystemTimeAsFileTime( &ft );
  timeW = (((sqlite3_int64)ft.dwHighDateTime)*max32BitValue) + (sqlite3_int64)ft.dwLowDateTime;
  timeF = timeW % ntuPerDay;          /* fractional days (100-nanoseconds) */
  timeW = timeW / ntuPerDay;          /* whole days */
  timeW = timeW + 2305813;            /* add whole days (from 2305813.5) */
  timeF = timeF + ntuPerHalfDay;      /* add half a day (from 2305813.5) */
  timeW = timeW + (timeF/ntuPerDay);  /* add whole day if half day made one */
  timeF = timeF % ntuPerDay;          /* compute new fractional days */
  *prNow = (double)timeW + ((double)timeF / (double)ntuPerDay);
  return 0;
}

int xGetLastError(sqlite3_vfs*, int, char *) {return 0;}



void sqlite_autoinit::init()
{
	sqlite3_initialize();

	memset (&m_sqlite3_vfs, 0, sizeof(m_sqlite3_vfs));

#if 1
	m_sqlite3_vfs.iVersion = 1;
	m_sqlite3_vfs.szOsFile = sizeof(sqlite3_file_fb2k);
	m_sqlite3_vfs.mxPathname = 2048;
	m_sqlite3_vfs.zName = "FB2K_VFS";
	m_sqlite3_vfs.pAppData = NULL;
	m_sqlite3_vfs.xOpen = &xOpen;
	m_sqlite3_vfs.xDelete = &xDelete;
	m_sqlite3_vfs.xAccess = &xAccess;
	m_sqlite3_vfs.xFullPathname = &xFullPathname;
	m_sqlite3_vfs.xRandomness = &xRandomness;
	m_sqlite3_vfs.xSleep = &xSleep;
	m_sqlite3_vfs.xCurrentTime = &xCurrentTime;
	m_sqlite3_vfs.xGetLastError = &xGetLastError;

	sqlite3_vfs_register(&m_sqlite3_vfs, 1);
#endif
}

pfc::refcounter sqlite_autoinit::m_refcount;
sqlite3_vfs sqlite_autoinit::m_sqlite3_vfs;

//nCol: Num Cols
//azVals: values
//azCols: columns
int xCallback(void *pContext,int nCol,char **azVals,char **azCols)
{
	return SQLITE_OK;
}

int xCallbackEnum(void *pContext,int nCol,char **azVals,char **azCols)
{
	pfc::string_list_impl * p_out = reinterpret_cast<pfc::string_list_impl *>(pContext);
	if (nCol && *azVals)
		p_out->add_item(*azVals);
	return SQLITE_OK;
}

void sqlite_database::exec(const char * statement)
{
	char * pErrStr = NULL;
	pfc::string8 errCopy = "unknown error";
	int ret = sqlite3_exec(m_database, statement, NULL, NULL, &pErrStr);
	if (pErrStr)
	{
		errCopy = pErrStr;
		sqlite3_free(pErrStr);
	}
	if (SQLITE_OK != ret)
		throw pfc::exception(pfc::string8(errCopy));
}

void sqlite_database::enum_tables(pfc::string_list_impl & p_out)
{
	char * pErrStr = NULL;
	pfc::string8 errCopy = "unknown error";
	int ret = sqlite3_exec(m_database, "SELECT sql FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'", xCallbackEnum, &p_out, &pErrStr);
	if (pErrStr)
	{
		errCopy = pErrStr;
		sqlite3_free(pErrStr);
	}
	if (SQLITE_OK != ret)
		throw pfc::exception(pfc::string8(errCopy));
}
void sqlite_database::copy_table_structures(const sqlite_database::ptr & p_source)
{
	pfc::string_list_impl tables;
	p_source->enum_tables(tables);
	t_size i, count = tables.get_count();
	for (i=0; i<count; i++)
		exec(tables[i]);
}

#endif