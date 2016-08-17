/*!\file 
*\brief change unix permissions of a file 
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_chmod_payload {
	mode_t target_posix_mode;
	const char *flags;
	const char *mode_str;
};

/*
 * actual doing the job 
 */
int
do_hpss_chmod(struct evbuffer *out_evb, char *escaped_path, void *payloadP,
	      int enclosed_json)
{
	struct hpss_chmod_payload *payload;
	int rc = 0;

	payload = (struct hpss_chmod_payload *)payloadP;

	rc = hpss_Chmod(escaped_path, payload->target_posix_mode);
	if (rc < 0)
		rc = -rc;
	if (serverInfo.LogLevel <= LL_TRACE || rc != 0) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_Chmod(\"%s\",\"%s\") returned rc=%d \n",
			__FILE__, __LINE__, escaped_path, payload->mode_str,
			rc);
		fflush(serverInfo.LogFile);
	}
	if (rc) {
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "{");
		}
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb,
				    "  \"errstr\" : \"Cannot chmod\", ");
		evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"", rc);
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "}");
		} else {
			evbuffer_add_printf(out_evb, ", ");
		}
		rc = 400;
		goto end;
	}

 end:
	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: do_hpss_chmod returning %d\n", __FILE__,
			__LINE__, rc);
		fflush(serverInfo.LogFile);
	}
	return rc;
}

int
hpss_chmod(struct evbuffer *out_evb, char *given_path, const char *flags,
	   int max_recursion_level, int older, int newer, char *mode_str)
{

	char *escaped_path;
	int rc = 0;
	char first_illegal_flag;
	struct hpss_chmod_payload payload;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);
	payload.target_posix_mode = get_posix_mode(mode_str);
	payload.mode_str = mode_str;
	payload.flags = flags;

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fprintf(serverInfo.LogFile,
			"%s:%d:: mode_str = %s, posix_mode =%o\n", __FILE__,
			__LINE__, mode_str, payload.target_posix_mode);
	}

	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"chmod\", ");

	if ((first_illegal_flag = check_given_flags("", flags))) {
		evbuffer_add_printf(out_evb, " \"errno\" : \"22\", ");
		evbuffer_add_printf(out_evb,
			" \"errstr\" : \"Illegal flag %c given.\", ", first_illegal_flag);
		goto end;
	}
	rc = do_hpss_chmod(out_evb, escaped_path, &payload, 0);

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

	return rc;
}
