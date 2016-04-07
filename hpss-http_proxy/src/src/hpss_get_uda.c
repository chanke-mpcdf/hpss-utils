/*!\file 
*\brief get all user defined attributes of a file
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_get_uda_payload {
	const char *flags;
};

/*
 * actual doing the job 
 */
int do_hpss_get_uda(struct evbuffer *out_evb,	/**< [in] ev ouput buffer */
		     char *escaped_path,
			/**<  [in] HPSS-path in ASCII encoding */
		     void *payloadP,
		    /**< [in] pointer to struct hpss_get_uda_payload */
		     int enclosed_json
		 /**< [in] flag if output as independent json-object */ )
{

	struct hpss_get_uda_payload *payload;
	int hpss_flags = 0;
	int rc = 0, i = 0;

	payload = (struct hpss_get_uda_payload *)payloadP;
	hpss_userattr_list_t uda_list;
	hpss_flags = XML_ATTR;
	rc = hpss_UserAttrListAttrs(escaped_path, &uda_list, hpss_flags);

	if (rc < 0)
		rc = -rc;
	if (serverInfo.LogLevel < LL_DEBUG || rc != 0)
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_get_uda(\"%s\", flags=%p) returned rc=%d\n",
			__FILE__, __LINE__, escaped_path, payload->flags, rc);

	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "{");
	}
	if (rc) {
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb,
				    "  \"errstr\" : \"Cannot get uda\"");
		evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\"", errno);
		rc = 400;
	} else {
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb, "\"uda\" : { ");
		for (i = 0; i < uda_list.len; i++) {
			/*
			 * cut the automatically inserted prefixes out of the result 
			 */
			evbuffer_add_printf(out_evb, "\"%s\" : \"%s\" ",
					    uda_list.Pair[i].Key +
					    strlen(UDA_HPSS_PREFIX) +
					    strlen(serverInfo.uda_prefix),
					    uda_list.Pair[i].Value);
			if (i < uda_list.len - 1)
				evbuffer_add_printf(out_evb, ", ");
			free(uda_list.Pair[i].Key);
			free(uda_list.Pair[i].Value);
		}
		evbuffer_add_printf(out_evb, "},");
	}

	if (enclosed_json) {
		evbuffer_add_printf(out_evb, "}");
	}
	free(uda_list.Pair);
	return rc;
}

int hpss_get_uda(struct evbuffer *out_evb, /**< [in]  ev output buffer */
		  char *given_path,
		      /**< [in] flags. Allowed here: "" */
		  const char *flags /**< [in] flags. Allowed here: "" */ )
{

	char *escaped_path;
	int rc = 0;
	struct hpss_get_uda_payload payload;
	double start, elapsed;

	/*
	 * initialize 
	 */
	start = double_time();
	escaped_path = escape_path(given_path, flags);
	payload.flags = flags;

	if (serverInfo.LogLevel < LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: Entering method with hpss_path=%s, flags=%s\n",
			__FILE__, __LINE__, escaped_path, flags);
	}

	evbuffer_add_printf(out_evb, "{");
	rc = do_hpss_get_uda(out_evb, escaped_path, &payload, 0);

	elapsed = double_time() - start;
	evbuffer_add_printf(out_evb, "\"elapsed\" : \"%.3f\"}", elapsed);

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
