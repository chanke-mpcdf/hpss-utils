/*!\file
*\brief purge a file in HPSS from a given storagelevel (default=0).
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
do_hpss_purge(struct evbuffer *out_evb, char *escaped_path, int enclosed_json, unsigned32 storage_level)
{
	int rc = 0;
        int hfd = -1;
        u_signed64  bytes_purged = 0;
	if (serverInfo.LogLevel < LL_DEBUG) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: processing escaped_path %s\n", __FILE__,
			__LINE__, escaped_path);
	}

        /*
         * Open the HPSS file
         */

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
        rc = hpss_Purge(hfd, cast64m(0), cast64m(0), storage_level, BFS_PURGE_ALL,
                                   &bytes_purged);

	evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
		escaped_path);

	if (rc) {
		evbuffer_add_printf(out_evb, "\"errstr\" : \"Cannot purge\", ");
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\"", rc);
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_Purge(\"%s\") returned rc=%d\n", __FILE__,
			__LINE__, escaped_path, rc);
		rc = 400;
                goto end;
	} else { 
	evbuffer_add_printf(out_evb, "\"bytes_purged\" : \"%lu\"",
		bytes_purged);
                
        }
 end:
	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "}");
	} else { 
		evbuffer_add_printf(out_evb, ", ");
        }


	/*
	 * Close the files, if required
	 */
	if (hfd > 0) {
		hpss_Close(hfd);
	}
	return rc;
}

int
hpss_purge(struct evbuffer *out_evb, char *given_path, const char *flags,
	  unsigned32 storage_level)
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
	evbuffer_add_printf(out_evb, "\"action\" : \"purge\", ");
	if ((first_illegal_flag = check_given_flags("", flags))) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"22\", ");
		evbuffer_add_printf(out_evb,
			" \"errstr\" : \"Illegal flag %c given.\", ", first_illegal_flag);
		goto end;
	}
	rc = do_hpss_purge(out_evb, escaped_path, 0, storage_level);

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
