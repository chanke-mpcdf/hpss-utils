/*!\file 
*\brief do a purge_lock on a file in HPSS
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
do_hpss_purge_lock(struct evbuffer *out_evb, char *escaped_path, int enclosed_json, int unlock)
{
	int rc = 0;
        int hfd = -1;

	if (serverInfo.LogLevel < LL_DEBUG) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: processing escaped_path %s\n", __FILE__,
			__LINE__, escaped_path);
	}

	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "{");
	}

        hfd = hpss_Open(escaped_path, O_RDWR, 0, NULL, NULL, NULL);
        if (hfd < 0) {
                evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
                evbuffer_add_printf(out_evb,
                                    "\"errstr\" : \"cannot open hpss-path %s.\", ",
                                    escaped_path);
                goto end;
        }

        if (unlock) {
		fprintf(serverInfo.LogFile, "UNLOCKING\n");
		rc = hpss_PurgeLock(hfd, PURGE_UNLOCK);
	} else {
		fprintf(serverInfo.LogFile, "LOCKING\n");
		rc = hpss_PurgeLock(hfd, PURGE_LOCK);
	}

	evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
			    escaped_path);
	if (rc) {
                if (unlock) {
   			evbuffer_add_printf(out_evb, "\"errstr\" : \"Cannot unlock purge_lock\", ");
                } else {
   			evbuffer_add_printf(out_evb, "\"errstr\" : \"Cannot purge_lock\", ");
                }
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\"", errno);
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_purge_lock(\"%s\") returned rc=%d\n", __FILE__,
			__LINE__, escaped_path, rc);
                goto end;
	} 

 end:
        hpss_Close(hfd);
	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "}");
	}
	return rc;
}

int
hpss_purge_lock(struct evbuffer *out_evb, char *given_path, const char *flags,
	  int max_recursion_level, int older, int newer)
{

	char *escaped_path;
	int rc = 0;
	char first_illegal_flag;
	double start, elapsed;
        int unlock = 0;

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
	evbuffer_add_printf(out_evb, "\"action\" : \"purge_lock\", ");
	if ((first_illegal_flag = check_given_flags("u", flags))) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"22\", ");
		evbuffer_add_printf(out_evb,
			" \"errstr\" : \"Illegal flag %c given.\", ", first_illegal_flag);
		goto end;
	}
        if (have_flag("u")) {
		unlock = 1;
	}
	rc = do_hpss_purge_lock(out_evb, escaped_path, 0, unlock);

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
