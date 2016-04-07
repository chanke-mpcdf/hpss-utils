/*!\file 
*\brief remove a file in HPSS
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_rm_payload {
	mode_t target_posix_mode;
	const char *flags;
};

/*
 * actual doing the job 
 */
int
do_hpss_rm(struct evbuffer *out_evb, char *escaped_path, void *payloadP,
	   int enclosed_json)
{
	struct hpss_rm_payload *payload;
	int rc = 0;
	const char *flags;

	payload = (struct hpss_rm_payload *)payloadP;
	flags = payload->flags;

	rc = hpss_Unlink(escaped_path);
	if (rc < 0)
		rc = -rc;
	if (rc == 21 && have_flag("r")) {
		rc = hpss_Rmdir((char *)escaped_path);
		if (rc < 0)
			rc = -rc;
	}
	if (serverInfo.LogLevel <= LL_DEBUG || rc != 0) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_rm(\"%s\") returned rc=%d payload=%p\n",
			__FILE__, __LINE__, escaped_path, rc, payload);
		fflush(serverInfo.LogFile);
	}
	return rc;
}

int hpss_rm(struct evbuffer *out_evb, char *given_path, const char *flags)
{

	char *escaped_path;
	int rc = 0;
	struct hpss_rm_payload payload;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);
	payload.flags = flags;

	evbuffer_add_printf(out_evb, "{");

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fflush(serverInfo.LogFile);
	}

	rc = do_hpss_rm(out_evb, escaped_path, &payload, 0);
	if (rc) {
		evbuffer_add_printf(out_evb, "\"errno\" : \"%d\", ", errno);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" : \"cannot remove '%s'\",",
				    escaped_path);
	}

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
	return rc;
}
