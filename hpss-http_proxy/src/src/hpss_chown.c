/*!\file 
*\brief change ownership of a file  
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_chown_payload {
	int uid;
	int gid;
	const char *flags;
};

/*
 * ! Actually doing the job * Execute hpss_Chown 
 */
int
do_hpss_chown(struct evbuffer *out_evb, char *escaped_path, void *payloadP,
	      int enclosed_json)
{
	struct hpss_chown_payload *payload;
	char owner_str[50];
	int rc = 0;

	payload = (struct hpss_chown_payload *)payloadP;

	snprintf(owner_str, 49, "%d:%d", payload->uid, payload->gid);
	rc = hpss_Chown(escaped_path, payload->uid, payload->gid);
	if (rc < 0)
		rc = -rc;
	if (serverInfo.LogLevel < LL_DEBUG || rc != 0)
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_Chown(\"%s\",%d,%d) returned rc=%d\n",
			__FILE__, __LINE__, escaped_path, payload->uid,
			payload->gid, rc);
	if (rc) {
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "{");
		}
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb,
				    "  \"errstr\" : \"Cannot chown\", ");
		evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"", rc);
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "}");
		} else {
			evbuffer_add_printf(out_evb, ", ");
		}

	}
	return rc;
}

int
hpss_chown(struct evbuffer *out_evb, char *given_path, const char *flags,
	   int uid, int gid)
{

	char *escaped_path;
	int rc = 0;
	struct hpss_chown_payload payload;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();

	escaped_path = escape_path(given_path, flags);
	payload.uid = uid;
	payload.gid = gid;
	payload.flags = flags;
	payload.flags = flags;

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
		fprintf(serverInfo.LogFile, "%s:%d:: uid=%d, gid=%d\n",
			__FILE__, __LINE__, uid, gid);
	}

	evbuffer_add_printf(out_evb, "{");

	rc = do_hpss_chown(out_evb, escaped_path, &payload, 0);

	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, " \"elapsed\" : \"%.3f\"}", elapsed);

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
