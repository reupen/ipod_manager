#include "..\sqlite\sqlite3.h"

class sqlite_autoinit
{
public:
	sqlite_autoinit()
	{
		if (++m_refcount == 1)
		{
			init();
		}
	}
	~sqlite_autoinit()
	{
		if (--m_refcount == 0)
		{
			deinit();
		}
	}
private:
	void init();
	void deinit()
	{
		sqlite3_shutdown();
	}
	static pfc::refcounter m_refcount;
	static sqlite3_vfs m_sqlite3_vfs;
};

class sqlite_database : public pfc::refcounted_object_root, sqlite_autoinit
{
public:
	typedef sqlite_database type;
	typedef pfc::refcounted_object_ptr_t<type> ptr;

	void exec(const char * statement);
	void enum_tables(pfc::string_list_impl & p_out);
	void copy_table_structures(const sqlite_database::ptr & p_source);

	~sqlite_database() {sqlite3_close(m_database); m_database = NULL;}

	sqlite_database(sqlite3 * p_database) : m_database(p_database) {};

	operator sqlite3 * () {return m_database;}
private:
	sqlite3 * m_database;
};

class sqlite_statement
{
public:
	sqlite_statement() : m_stmt(NULL), m_auto_bind_index(0) {};
	void prepare (sqlite_database::ptr const & db, const char * query, t_size query_len)
	{
		int ret = sqlite3_prepare_v2(*db, query, query_len, &m_stmt, NULL);
		if (SQLITE_OK != ret)
		{
			const char * err_text = sqlite3_errmsg(*db);
			pfc::string8 text;
			if (err_text) text << err_text;
			else text << "sqlite3_prepare_v2 returned: " << ret;
			throw pfc::exception(text);
		}
	}
	void finalise()
	{
		int ret = sqlite3_step(m_stmt);
		if (SQLITE_DONE != ret)
		{
			sqlite3_finalize(m_stmt);
			m_stmt = NULL;
			throw pfc::exception(pfc::string8 () << "sqlite3_step returned: " << ret);
		}
		ret = sqlite3_finalize(m_stmt);
		{m_stmt = NULL;}
		if (SQLITE_OK != ret)
		throw pfc::exception(pfc::string8 () << "sqlite3_finalize returned: " << ret);
	}
	void autobind_text(const char * text, t_size text_len, void(*pointer_type)(void*) = SQLITE_STATIC)
	{
		sqlite3_bind_text(m_stmt, ++m_auto_bind_index, text, text_len, pointer_type);
	}
	void autobind_text(const wchar_t * text, t_size text_len, void(*pointer_type)(void*) = SQLITE_STATIC)
	{
		sqlite3_bind_text16(m_stmt, ++m_auto_bind_index, text, text_len*2, pointer_type);
	}
	void autobind_blob(const void * data, t_size data_len, void(*pointer_type)(void*) = SQLITE_STATIC)
	{
		sqlite3_bind_blob(m_stmt, ++m_auto_bind_index, data, data_len, pointer_type);
	}

	~sqlite_statement() {if (m_stmt) {sqlite3_finalize(m_stmt);m_stmt=NULL;} }
private:
	sqlite3_stmt * m_stmt;
	t_size m_auto_bind_index;
};

class sqlite_handle : private sqlite_autoinit
{
public:
	void open(const char * path, sqlite_database::ptr & p_out)
	{
		//if (!stricmp_utf8_max(path, "file://", 7)) path += 7;
		sqlite3 * ptr = NULL;
		int ret = sqlite3_open(path, &ptr);
		if (SQLITE_OK == ret)
			p_out = new sqlite_database(ptr);
		else
			throw pfc::exception();
	}
	void open_ex(const char * path, int flags, sqlite_database::ptr & p_out)
	{
		//if (!stricmp_utf8_max(path, "file://", 7)) path += 7;
		sqlite3 * ptr = NULL;
		int ret = sqlite3_open_v2(path, &ptr, flags, /*NULL*/ "FB2K_VFS");
		if (SQLITE_OK == ret)
			p_out = new sqlite_database(ptr);
		else
			throw pfc::exception();
	}
};

