/*!file
*\brief create a hard or soft link in HPSS
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

int
hpss_link(struct evbuffer *out_evb, char *given_path, const char *flags,
	  char *dest_path)
{

	int rc;
	double start, elapsed;
	char *escaped_path, *escaped_dest_path;

	/*
	 * initialize 
	 */
	start = double_time();

	/*
	 * escape pathes 
	 */
	escaped_path = escape_path(given_path, flags);
	escaped_dest_path = escape_path(dest_path, flags);

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fprintf(serverInfo.LogFile, "%s:%d:: dest_path=%s\n", __FILE__,
			__LINE__, escaped_dest_path);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"link\", ");
	evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ", escaped_path);

	if (!have_flag("s") && !have_flag("h")) {
		rc = 2;
		evbuffer_add_printf(out_evb,
				    "  \"errstr\" : \"One of flags -h or -s required.\"");
		evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"", rc);
		goto end;
	}

	if (have_flag("h")) {
		if ((rc = hpss_Link(escaped_path, escaped_dest_path)) < 0) {
			rc = -rc;
			evbuffer_add_printf(out_evb,
					    "  \"errstr\" : \"hpss error cannot create hardlink  %s pointing to %s.\", ",
					    escaped_path, dest_path);
			evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"",
					    rc);
			fprintf(serverInfo.LogFile,
				"%s:%d:: hpss_Link(\"%s\",\"%s\") returned rc=%d\n",
				__FILE__, __LINE__, escaped_path,
				escaped_dest_path, -rc);
			goto end;
		}
	} else {
		if ((rc = hpss_Symlink(escaped_path, escaped_dest_path)) < 0) {
			rc = -rc;
			evbuffer_add_printf(out_evb,
					    "  \"errstr\" : \"hpss error cannot create symbolic link  %s pointing to %s.\", ",
					    escaped_path, dest_path);
			evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"",
					    rc);
			fprintf(serverInfo.LogFile,
				"%s:%d:: hpss_Symlink(\"%s\",\"%s\") returned rc=%d\n",
				__FILE__, __LINE__, escaped_path,
				escaped_dest_path, -rc);
			goto end;
		}
	}
end:
	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\" }", elapsed);

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: leaving with rc=%d, elapsed=%3.3f\n", __FILE__,
			__LINE__, rc, elapsed);
		fflush(serverInfo.LogFile);
	}

	/*
	 * free if we do have an escaped path 
	 */
	if (escaped_path != given_path)
		free(escaped_path);

	if (escaped_dest_path != dest_path)
		free(escaped_dest_path);

	fflush(serverInfo.LogFile);

	return -rc;
}
