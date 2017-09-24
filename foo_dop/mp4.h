#ifndef _MP4_DOP_H_
#define _MP4_DOP_H_

#include "itunesdb.h"

bool g_check_mp4_type(const char * path);
bool g_check_mp4_type(const char * path, abort_callback & p_abort);
bool g_check_mp4_type(service_ptr_t<file> p_file, abort_callback & p_abort);

//t_uint32 g_get_gapless_sync_frame_mp3(const char * path, abort_callback & p_abort);
//t_uint32 g_get_gapless_sync_frame_mp3(service_ptr_t<file> p_file, abort_callback & p_abort);

t_filesize g_get_gapless_sync_frame_mp3_v2(const char * path, abort_callback & p_abort);
t_filesize g_get_gapless_sync_frame_mp3_v2(service_ptr_t<file> p_file, abort_callback & p_abort);
bool g_get_gapless_mp4(const char * path, t_uint32 & delay, t_uint32 & padding, abort_callback & p_abort);
bool g_get_gapless_mp4(const char * path, t_uint32 & delay, t_uint32 & padding);
bool g_get_itunes_chapters_mp4(const char * path, itunesdb::chapter_list & p_out, abort_callback & p_abort);
#endif