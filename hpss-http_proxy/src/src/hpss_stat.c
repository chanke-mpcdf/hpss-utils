/*!\file 
*\brief do a stat on a file or dir in HPSS
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

/*
 * actually doing the job 
 */
int
do_hpss_stat(struct evbuffer *out_evb, char *escaped_path, int enclosed_json)
{
	int rc = 0;
	char s64_str[256];
	hpss_stat_t stat_info;

	if (serverInfo.LogLevel < LL_DEBUG) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: processing escaped_path %s\n", __FILE__,
			__LINE__, escaped_path);
	}
	rc = hpss_Stat(escaped_path, &stat_info);

	if (rc) {
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "{");
		}
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb, "\"errstr\" : \"Cannot stat\", ");
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\"", errno);
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "}");
		}
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_stat(\"%s\") returned rc=%d\n", __FILE__,
			__LINE__, escaped_path, rc);
		rc = 400;
		goto end;
	}

	evbuffer_add_printf(out_evb, "\"device\" : \"%d\", ", stat_info.st_dev);
	u64_to_decchar(stat_info.st_ino, s64_str);
	evbuffer_add_printf(out_evb, "\"inode\" : \"%s\", ", s64_str);
	evbuffer_add_printf(out_evb, "\"links\" : \"%d\", ",
			    stat_info.st_nlink);
	evbuffer_add_printf(out_evb, "\"flags\" : \"%d\", ", stat_info.st_flag);
	evbuffer_add_printf(out_evb, "\"uid\" : \"%d\", ", stat_info.st_uid);
	evbuffer_add_printf(out_evb, "\"gid\" : \"%d\", ", stat_info.st_gid);
	evbuffer_add_printf(out_evb, "\"rdevice\" : \"%d\", ",
			    stat_info.st_rdev);
	u64_to_decchar(stat_info.st_ssize, s64_str);
	evbuffer_add_printf(out_evb, "\"ssize\" : \"%s\", ", s64_str);
	ctime_r((time_t *) & stat_info.hpss_st_atime, s64_str);
	s64_str[strlen(s64_str) - 1] = '\0';
	evbuffer_add_printf(out_evb, "\"access time\" : \"%s\", ", s64_str);
	ctime_r((time_t *) & stat_info.hpss_st_mtime, s64_str);
	s64_str[strlen(s64_str) - 1] = '\0';
	evbuffer_add_printf(out_evb, "\"modify time\" : \"%s\", ", s64_str);
	ctime_r((time_t *) & stat_info.hpss_st_ctime, s64_str);
	s64_str[strlen(s64_str) - 1] = '\0';
	evbuffer_add_printf(out_evb, "\"creation time\" : \"%s\", ", s64_str);
	evbuffer_add_printf(out_evb, "\"blksize\" : \"%d\", ",
			    stat_info.st_blksize);
	evbuffer_add_printf(out_evb, "\"blocks\" : \"%d\", ",
			    stat_info.st_blocks);
	evbuffer_add_printf(out_evb, "\"vfstype\" : \"%d\", ",
			    stat_info.st_vfstype);
	evbuffer_add_printf(out_evb, "\"vfs\" : \"%d\", ", stat_info.st_vfs);
	evbuffer_add_printf(out_evb, "\"vnode type\" : \"%d\", ",
			    stat_info.st_type);
	evbuffer_add_printf(out_evb, "\"gen\" : \"%d\", ", stat_info.st_gen);
	u64_to_decchar(stat_info.st_size, s64_str);
	evbuffer_add_printf(out_evb, "\"size\" : \"%s\", ", s64_str);
	evbuffer_add_printf(out_evb, "\"mode\" : \"%d\", ", stat_info.st_mode);

 end:
	;
	return rc;
}

int
hpss_stat(struct evbuffer *out_evb, char *given_path, const char *flags,
	  int max_recursion_level, int older, int newer)
{

	char *escaped_path;
	int rc = 0;
	char first_illegal_flag;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"stat\", ");
	if ((first_illegal_flag = check_given_flags("", flags))) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"22\", ");
		evbuffer_add_printf(out_evb,
			" \"errstr\" : \"Illegal flag %c given.\", ", first_illegal_flag);
		goto end;
	}
	rc = do_hpss_stat(out_evb, escaped_path, 0);

end:
	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\" }", elapsed);

	if (serverInfo.LogLevel < LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving with rc=%d, elapsed=%3.3f\n", __FILE__,
			__LINE__, rc, elapsed);
	fflush(serverInfo.LogFile);

	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);

	return rc;
}
