/*!\file 
*\brief delete a user defined attribute on a file
*/

/*
 * helper 
 */
#include "util.h"
#include "hpss_methods.h"

struct hpss_del_uda_payload {
	hpss_userattr_list_t uda_list;
	const char *flags;
};

/*
 * ! Actually doing the job. * Execute hpss_UserAttrSetAttrs 
 */
int do_hpss_del_uda(struct evbuffer *out_evb, /**<[in] ev buffer */
		    char *escaped_path,
			/**<[in] HPSS-path in ASCII encoding */
		    void *payloadP,
		    /**<[in] pointer to struct hpss_del_uda_payload */
		    int enclosed_json
		/**<[in] flag if output as independent json-object */ )
{

	struct hpss_del_uda_payload *payload;
	int rc = 0;

	payload = (struct hpss_del_uda_payload *)payloadP;

	rc = hpss_UserAttrDeleteAttrs(escaped_path, &payload->uda_list, NULL);
	if (rc < 0)
		rc = -rc;
	if (serverInfo.LogLevel < LL_DEBUG || rc != 0)
		fprintf(serverInfo.LogFile,
			"%s:%d:: hpss_UserAttrDeleteAttrs(\"%s\",\"%s\") returned  rc=%d\n",
			__FILE__, __LINE__, escaped_path,
			payload->uda_list.Pair[0].Key, rc);
	if (rc) {
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "{");
		}
		evbuffer_add_printf(out_evb, "\"path\" : \"%s\", ",
				    escaped_path);
		evbuffer_add_printf(out_evb,
				    "  \"errstr\" : \"Cannot delete uda\", ");
		evbuffer_add_printf(out_evb, "  \"errno\" : \"%d\", ", errno);
		if (enclosed_json) {
			evbuffer_add_printf(out_evb, "}");
		}
		rc = 400;
	}

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile,
			"%s:%d:: do_hpss_del_uda returning %d\n", __FILE__,
			__LINE__, rc);
		fflush(serverInfo.LogFile);
	}
	return rc;
}

/*!delete a UDA by key. */
int hpss_del_uda(struct evbuffer *out_evb, /**<[in] ev buffer */
		 char *given_path,
		      /**< [in] path to work on */
		 const char *flags,
		       /**< [in] flags. Allowed here: "" */
		 char *key /**< [in] key of uda to delete */ )
{

	char *escaped_path;
	int rc = 0, i = 0;
	struct hpss_del_uda_payload payload;
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

	payload.uda_list.len = 1;
	payload.uda_list.Pair =
	    calloc(payload.uda_list.len, sizeof(hpss_userattr_t));
	if (serverInfo.LogLevel < LL_TRACE)
		fprintf(serverInfo.LogFile, "%s:%d::got key = %s\n", __FILE__,
			__LINE__, key);

	/*
	 * we only take absolute XPath values (=Key)
	 */
	evbuffer_add_printf(out_evb, "{");
	evbuffer_add_printf(out_evb, "\"action\" : \"hpss_del_uda\", ");
	if (key[0] != '/') {
		if (serverInfo.LogLevel < LL_INFO)
			fprintf(serverInfo.LogFile,
				"%s:%d:: key %s does not start with a \"/\". Bailing out.\n",
				__FILE__, __LINE__, key);
		evbuffer_add_printf(out_evb,
				    " \"errno\" : \"%d\", \"errstr\" : \"key %s does not start with a /\", ",
				    -rc, key);
	}
	/*
	 * Keys in HPSS must start with /hpss 
	 */
	payload.uda_list.Pair[0].Key =
	    (char *)malloc(strlen(UDA_HPSS_PREFIX) +
			   strlen(serverInfo.uda_prefix) + strlen(key) + 1);
	strcpy(payload.uda_list.Pair[0].Key, UDA_HPSS_PREFIX);
	strcat(payload.uda_list.Pair[0].Key, serverInfo.uda_prefix);
	strcat(payload.uda_list.Pair[0].Key, key);

	rc = do_hpss_del_uda(out_evb, escaped_path, &payload, 0);

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

	/*
	 * free uda_list 
	 */
	for (i = 0; i < payload.uda_list.len; i++) {
		free(payload.uda_list.Pair[i].Key);
	}
	free(payload.uda_list.Pair);

	return rc;
}
