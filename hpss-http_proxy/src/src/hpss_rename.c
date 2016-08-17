/*!\file 
*\brief rename a file in HPSS
*/

/*
* helper 
*/
#include "util.h"
#include "hpss_methods.h"

int
hpss_rename(struct evbuffer *out_evb, char *given_path, const char *flags,
	    char *new_path)
{

	char *escaped_path, *escaped_new_path;
	int rc;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();

	/*
	 * escape pathes 
	 */
	escaped_path = escape_path(given_path, flags);
	escaped_new_path = escape_path(new_path, flags);

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s\n", __FILE__,
			__LINE__, escaped_path);
		fprintf(serverInfo.LogFile, "%s:%d:: new_path=%s\n", __FILE__,
			__LINE__, escaped_new_path);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"hpss_link\", ");
	evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ", escaped_path);
	evbuffer_add_printf(out_evb, "\"new_path\" : \"%s\", ",
			    escaped_new_path);

	rc = hpss_Rename(escaped_path, escaped_new_path);
	if (serverInfo.LogLevel < LL_TRACE || rc < 0)
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_Rename(\"%s\", \"%s\") returns %d\n",
			__FILE__, __LINE__, escaped_path, escaped_new_path,
			-rc);
	if (rc < 0) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"rc=%d\", ", rc);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"cannot rename '%s' to '%s' \", ",
				    escaped_path, escaped_new_path);
	}

	elapsed = double_time() - start;

	if (serverInfo.LogLevel < LL_TRACE)
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving with rc=%d, elapsed=%3.3f\n", __FILE__,
			__LINE__, rc, elapsed);
	fflush(serverInfo.LogFile);

	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\" }", elapsed);

	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);
	if (escaped_new_path != new_path)
		free(escaped_new_path);
	return rc;
}
