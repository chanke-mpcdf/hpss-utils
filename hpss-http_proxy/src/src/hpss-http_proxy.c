/*!file
*\brief main http-server
*/

#include <getopt.h>

#include "util.h"
#include "hpss_methods.h"

char uri_root[512];

/*
 * ! \brief list of methods which need RW access * * The preceeding and
 * trailing comma are required for the correct parsing. 
 */
char *rw_actions =
    ",chmod,chown,del_uda,link,mkdir,put_from_proxy,rename,rm,set_uda,purge,purge_lock,";
/*
 * ! \brief list of all exported methods * * The preceeding and trailing
 * comma are required for the correct parsing. 
 */
char *all_actions =
    ",chmod,chown,del_uda,get_to_proxy,get_storage_info,get_udas,link,ls,mkdir,put_from_proxy,rename,rm,set_uda,stat,stage,purge,purge_lock,get_to_client,";

/*
 * ! \brief check given actions 
 */
void check_and_init_actions(char **serverInfo_actions, char *given_actions)
{
	char *ptr = NULL, *tmp_str = NULL;
	char comma_action[MAX_QS_VALUE_LEN + 4];
	*serverInfo_actions = malloc(strlen(given_actions) + 4);
	strcpy(*serverInfo_actions, ",");
	strcat(*serverInfo_actions, given_actions);
	strcat(*serverInfo_actions, ",");
	/*
	 * strtok destroys string, so make a copy of it 
	 */
	tmp_str = malloc(strlen(given_actions) + 4);
	strcpy(tmp_str, *serverInfo_actions);
	/*
	 * need to check that given actions are in all_actions 
	 */
	ptr = strtok(tmp_str, ",");
	while (ptr != NULL) {
		if (strlen(ptr) > MAX_QS_KEY_LEN - 1) {
			fprintf(serverInfo.LogFile,
				"Action list contains illegal action %s (name too long)\n",
				ptr);
			exit(1);
		}
		strcpy(comma_action, ",");
		strcat(comma_action, ptr);
		strcat(comma_action, ",");
		if (strstr(all_actions, comma_action) == NULL) {
			fprintf(serverInfo.LogFile,
				"Action list contains illegal action %s (not available)\n",
				ptr);
			exit(1);
		}
		ptr = strtok(NULL, ",");
	}
	free(tmp_str);
}

/*
 * check that a qs-param has only chars of a safe range 
 */
int
is_unsafe_param(struct evbuffer *out_evb, const char *key, const char *value)
{
	const char *p;
	for (p = &value[0]; *p; p++) {
		if (*p < MIN_ALLOWED_CHAR || *p > MAX_ALLOWED_CHAR) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: CRIT: qs-param %s contains illegal character of ord=%d!\n",
				__FILE__, __LINE__, key, *p);
			fflush(serverInfo.LogFile);
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"param %s contains illegal char(s)\", ",
					    key);
			evbuffer_add_printf(out_evb, "\"errno\" : \"1\" }");
		LOG_RETURN_INT(1)}
	}
LOG_RETURN_INT(0)}

/*
 * ! return HTTP-Code 200 if a (mandatory) variable was not included in
 * the command. 
 */
int check_qs_param(struct evbuffer *out_evb, const char *key, const char *value)
{
	if (serverInfo.LogLevel <= LL_DEBUG)
		fprintf(serverInfo.LogFile,
			"check_qs_param: checking key=%s, value=%s\n", key,
			value);
	if (strlen(key) >= MAX_QS_KEY_LEN) {
		evbuffer_add_printf(out_evb,
				    "{ \"errstr\" : \"parameter-name  too long\", \"errno\" : \"1\"}");
	LOG_RETURN_INT(1)}
	if (!value) {
		evbuffer_add_printf(out_evb,
				    "{ \"errstr\" : \"no %s-parameter given\", \"errno\" : \"2\"}",
				    key);
	LOG_RETURN_INT(2)} else {
		if (strlen(value) >= MAX_QS_VALUE_LEN) {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"value of %s too long\", \"errno\" : \"2\"}",
					    key);
		LOG_RETURN_INT(1)}
		if (strlen(value) == 0) {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"%s-parameter empty\", \"errno\" : \"2\"}",
					    key);
		LOG_RETURN_INT(1)}
	}

	/*
	 * check for ascii 
	 */
	if (is_unsafe_param(out_evb, key, value)) {
	LOG_RETURN_INT(1)}
LOG_RETURN_INT(0)}

    /*
     * ! \brief safely convert a string into an int. * Return
     * HTTP-Error 200 on failure. 
     */
int
check_conversion_to_int(struct evbuffer *out_evb,
			const char *param_buf, char *end_ptr, const char *param)
{
	if (errno) {
		evbuffer_add_printf(out_evb, "{ \"param\" : \"%s\", ", param);
		evbuffer_add_printf(out_evb, "\"value\" : \"%s\", ", param_buf);
		evbuffer_add_printf(out_evb,
				    "\"errstr\" :  \"error converting to int\", \"errno\" : \"%d\" }",
				    errno);
		if (serverInfo.LogLevel <= LL_ERROR) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: strtol of param %s, value %s failed with errno %d.\n",
				__FILE__, __LINE__, param, param_buf, errno);
			fflush(serverInfo.LogFile);
		}
		return 1;
	}
	if (param_buf == end_ptr) {
		evbuffer_add_printf(out_evb, "{ \"param\" : \"%s\", ", param);
		evbuffer_add_printf(out_evb, " \"value\" : \"%s\", ",
				    param_buf);
		evbuffer_add_printf(out_evb,
				    " \"errstr\" : \"no digits found\", \"errno\" : \"1\" }");
		if (serverInfo.LogLevel <= LL_ERROR) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: strtol of param %s, value %s failed with errno %d.\n",
				__FILE__, __LINE__, param, param_buf, errno);
			fflush(serverInfo.LogFile);
		}
		return 1;
	}
	if (param_buf != '\0' && *end_ptr == '\0') {
		return 0;
	} else {
		evbuffer_add_printf(out_evb, "{ \"param\" : \"%s\", ", param);
		evbuffer_add_printf(out_evb, "\"value\" : \"%s\", ", param_buf);
		evbuffer_add_printf(out_evb,
				    " \"errstr\" : \"Further characters after number: %s\", \"errno\" : \"1\" }",
				    end_ptr);
		if (serverInfo.LogLevel <= LL_ERROR) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: strtol of param %s, value %s failed with errno %d.\n",
				__FILE__, __LINE__, param, param_buf, errno);
			fflush(serverInfo.LogFile);
		}
		return 1;
	}
}

  /*
   * This callback gets invoked when we get any http request that
   * doesn't match any other callback.  Like any evhttp server callback, 
   * it has a simple job: it must eventually call evhttp_send_error() or 
   * evhttp_send_reply(). 
   */
static void dispatcher(struct evhttp_request *req, void *arg)
{
	struct evbuffer *out_evb = NULL;
	const char *qs = NULL;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	struct evkeyvalq query_str_kvq;
	const char *path;
	char *decoded_path;
	const char *action = NULL, *flags = NULL, *qs_param_buf = NULL;
	char mode_str[MAX_QS_VALUE_LEN];
	char other_path[MAX_QS_VALUE_LEN];
	char param_buf[MAX_QS_VALUE_LEN];
	char param_buf_2[MAX_QS_VALUE_LEN];
	char comma_action[MAX_QS_KEY_LEN + 2];
	char *end_ptr;
	int depth;
	int rc;
	long older_than, newer_than;
	time_t now;

	time(&now);

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/*
	 * This holds the content we're sending. 
	 */
	out_evb = evbuffer_new();

	if (serverInfo.LogLevel <= LL_DEBUG)
		fprintf(serverInfo.LogFile, "Got a GET request for <%s>\n",
			uri);

	/*
	 * Decode the URI 
	 */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		if (serverInfo.LogLevel <= LL_ERROR)
			fprintf(serverInfo.LogFile,
				"It's not a good URI. Sending BADREQUEST\n");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/*
	 * Let's see what path the user asked for. 
	 */
	path = evhttp_uri_get_path(decoded);
	if (!path)
		path = "/";

	/*
	 * We need to decode it, to see what path the user really wanted. 
	 */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	if (serverInfo.LogLevel <= LL_DEBUG)
		fprintf(serverInfo.LogFile, "Got path %s\n", decoded_path);

	/*
	 * parse query_string 
	 */
	qs = evhttp_uri_get_query(decoded);
	if (qs) {
		if (evhttp_parse_query_str(qs, &query_str_kvq)) {
			if (serverInfo.LogLevel <= LL_ERROR)
				fprintf(serverInfo.LogFile,
					"Parsing query string failed.\n");
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"cannot parse query string.\", \"errno\" : \"1\"}");
			goto send_reply;
		};
		action = evhttp_find_header(&query_str_kvq, "action");
		flags = evhttp_find_header(&query_str_kvq, "flags");
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile, "%s:%d:: got action %s\n",
				__FILE__, __LINE__, action);
			fprintf(serverInfo.LogFile, "%s:%d:: got flags %s\n",
				__FILE__, __LINE__, flags);
		}
	} else {
		fprintf(serverInfo.LogFile, "Parsing query string failed.\n");
		evbuffer_add_printf(out_evb,
				    "{ \"errstr\" : \"got no query string.\", \"errno\" : \"1\"}");
		goto send_reply;
	}

	if (serverInfo.LogLevel <= LL_DEBUG)
		fprintf(serverInfo.LogFile, "Got query_string %s\n", qs);

	if (check_qs_param(out_evb, "action", action))
		goto send_reply;

	strcpy(comma_action, ",");
	strcat(comma_action, action);
	strcat(comma_action, ",");

	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile, "%s:%d:: got comma_action %s\n",
			__FILE__, __LINE__, comma_action);
		fprintf(serverInfo.LogFile, "%s:%d:: allowed_actions %s\n",
			__FILE__, __LINE__, serverInfo.allowed_actions);
		fprintf(serverInfo.LogFile, "%s:%d:: forbidden_actions %s\n",
			__FILE__, __LINE__, serverInfo.forbidden_actions);
		fflush(serverInfo.LogFile);
	}

	/*
	 * return HTTP-Code 200 and rc=1 if we call an illegal action 
	 */
	if ((serverInfo.forbidden_actions
	     && (strstr(serverInfo.forbidden_actions, comma_action) !=
		 NULL)) || (serverInfo.allowed_actions
			    &&
			    (strstr
			     (serverInfo.allowed_actions,
			      comma_action) == NULL))) {
		if (serverInfo.LogLevel <= LL_ERROR) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: denying action %s.\n", __FILE__,
				__LINE__, action);
			fflush(serverInfo.LogFile);
		}
		evbuffer_add_printf(out_evb,
				    "{ \"errstr\" : \"action '%s' not allowed\", ",
				    action);
		evbuffer_add_printf(out_evb, "\"errno\" : \"13\" }");
		goto send_reply;
	}

	/*
	 * flags parameter 
	 */
	qs_param_buf = evhttp_find_header(&query_str_kvq, "flags");
	if (qs_param_buf) {
		if (check_qs_param(out_evb, "flags", flags))
			goto send_reply;
	} else {
		flags = "";
	}

	/*
	 * get optional parameters 
	 */
	qs_param_buf = evhttp_find_header(&query_str_kvq, "depth");
	if (qs_param_buf) {
		if (check_qs_param(out_evb, "depth", qs_param_buf))
			goto send_reply;
		errno = 0;
		depth = strtol(qs_param_buf, &end_ptr, 10);
		if (check_conversion_to_int
		    (out_evb, qs_param_buf, end_ptr, "depth"))
			goto send_reply;
	} else {
		depth = -1;
	}

	qs_param_buf = evhttp_find_header(&query_str_kvq, "older_than");
	if (qs_param_buf) {
		if (check_qs_param(out_evb, "older_than", qs_param_buf))
			goto send_reply;
		errno = 0;
		older_than = strtol(qs_param_buf, &end_ptr, 10);
		if (check_conversion_to_int
		    (out_evb, qs_param_buf, end_ptr, "depth"))
			goto send_reply;
	} else {
		older_than = -1;
	}

	qs_param_buf = evhttp_find_header(&query_str_kvq, "newer_than");
	if (qs_param_buf) {
		if (check_qs_param(out_evb, "newer_than", qs_param_buf))
			goto send_reply;
		errno = 0;
		newer_than = strtol(qs_param_buf, &end_ptr, 10);
		if (check_conversion_to_int
		    (out_evb, qs_param_buf, end_ptr, "depth"))
			goto send_reply;
	} else {
		newer_than = -1;
	}

	if (older_than < newer_than) {
		evbuffer_add_printf(out_evb,
				    "{ \"errstr\" : \"parameter newer_than must be smaller than parameter older_than\", \"errno\" : \"1\" ");
		evbuffer_add_printf(out_evb, "\"newer_than\" : \"%ld\", ",
				    newer_than);
		evbuffer_add_printf(out_evb, "\"older_than\" : \"%ld\" }",
				    older_than);
		goto send_reply;
	}

	qs_param_buf = evhttp_find_header(&query_str_kvq, "mode_str");
	if (qs_param_buf) {
		if (check_qs_param(out_evb, "mode_str", qs_param_buf))
			goto send_reply;
		strncpy(mode_str, qs_param_buf, MAX_QS_VALUE_LEN);
	} else {
		strcpy(mode_str, "rwxr-xr-x");
	}

	/*
	 * ignore all other QS-parameters, should we check for them ?
	 */

	/*
	 * dispatch to libhpss 
	 */

	if (!strcmp(action, "ls")) {
		rc = hpss_ls(out_evb, (char *)path, flags, depth, older_than,
			     newer_than);
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: got rc=%d from hpss_ls\n", __FILE__,
				__LINE__, rc);
			fflush(serverInfo.LogFile);
		}
		goto send_reply;
	}

	if (!strcmp(action, "mkdir")) {
		rc = hpss_mkdir(out_evb, (char *)path, flags, mode_str);
		if (serverInfo.LogLevel <= LL_TRACE) {
			fprintf(serverInfo.LogFile,
				"%s:%d:: leaving with rc=%d\n", __FILE__,
				__LINE__, rc);
			fflush(serverInfo.LogFile);
		}
		goto send_reply;
	}

	if (!strcmp(action, "rm")) {
		rc = hpss_rm(out_evb, (char *)path, flags);
		goto send_reply;
	}

	if (!strcmp(action, "put_from_proxy")) {
		int cos_id = 0;

		qs_param_buf = evhttp_find_header(&query_str_kvq, "local_path");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "local_path", qs_param_buf))
				goto send_reply;
			strncpy(other_path, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter local_path missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		qs_param_buf = evhttp_find_header(&query_str_kvq, "cos_id");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "cos_id", qs_param_buf))
				goto send_reply;
			cos_id = strtol(qs_param_buf, &end_ptr, 10);
			if (check_conversion_to_int
			    (out_evb, qs_param_buf, end_ptr, "cos_id"))
				goto send_reply;
		} else {
			cos_id = 0;
		}
		rc = hpss_put_from_proxy(out_evb, (char *)path, flags, other_path,
				    mode_str, cos_id);
		goto send_reply;
	}
	if (!strcmp(action, "chmod")) {
		rc = hpss_chmod(out_evb, (char *)path, flags, depth,
				older_than, newer_than, mode_str);
		goto send_reply;
	}
	if (!strcmp(action, "chown")) {
		int uid, gid;
		qs_param_buf = evhttp_find_header(&query_str_kvq, "uid");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "uid", qs_param_buf))
				goto send_reply;
			uid = strtol(qs_param_buf, &end_ptr, 10);
			if (check_conversion_to_int
			    (out_evb, qs_param_buf, end_ptr, "uid"))
				goto send_reply;
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter uid missing\", \"errno\" : \"2\" ");
                      goto send_reply;
		}
		qs_param_buf = evhttp_find_header(&query_str_kvq, "gid");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "gid", qs_param_buf))
				goto send_reply;
			gid = strtol(qs_param_buf, &end_ptr, 10);
			if (check_conversion_to_int
			    (out_evb, qs_param_buf, end_ptr, "gid"))
				goto send_reply;
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter gid missing\", \"errno\" : \"2\" ");
                      goto send_reply;
		}
		rc = hpss_chown(out_evb, (char *)path, flags, uid, gid);
		goto send_reply;
	}
	if (!strcmp(action, "stat")) {
		rc = hpss_stat(out_evb, (char *)path, flags, depth,
			       older_than, newer_than);
		goto send_reply;
	}
	if (!strcmp(action, "stage")) {
		int storage_level = 0;
                qs_param_buf = evhttp_find_header(&query_str_kvq, "storage_level");
                if (qs_param_buf) {
                        if (check_qs_param(out_evb, "storage_level", qs_param_buf))
                                goto send_reply;
                        storage_level = strtol(qs_param_buf, &end_ptr, 10);
                        if (check_conversion_to_int
                            (out_evb, qs_param_buf, end_ptr, "storage_level"))
                                goto send_reply;
                } else {
                        storage_level = 0;
                }

		rc = hpss_stage(out_evb, (char *)path, flags, storage_level);
		goto send_reply;
	}
	if (!strcmp(action, "purge_lock")) {
		rc = hpss_purge_lock(out_evb, (char *)path, flags, depth,
                               older_than, newer_than);
		goto send_reply;
	}
	if (!strcmp(action, "purge")) {
		int storage_level = 0;
                qs_param_buf = evhttp_find_header(&query_str_kvq, "storage_level");
                if (qs_param_buf) {
                        if (check_qs_param(out_evb, "storage_level", qs_param_buf))
                                goto send_reply;
                        storage_level = strtol(qs_param_buf, &end_ptr, 10);
                        if (check_conversion_to_int
                            (out_evb, qs_param_buf, end_ptr, "storage_level"))
                                goto send_reply;
                } else {
                        storage_level = 0;
                }

		rc = hpss_purge(out_evb, (char *)path, flags, storage_level);
		goto send_reply;
	}
	if (!strcmp(action, "link")) {
		qs_param_buf = evhttp_find_header(&query_str_kvq, "dest_path");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "dest_path", qs_param_buf))
				goto send_reply;
			strncpy(other_path, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter dest_path missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		rc = hpss_link(out_evb, (char *)path, flags, other_path);
		goto send_reply;
	}

	if (!strcmp(action, "rename")) {
		qs_param_buf = evhttp_find_header(&query_str_kvq, "new_path");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "new_path", qs_param_buf))
				goto send_reply;
			strncpy(other_path, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter new_path missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		rc = hpss_rename(out_evb, (char *)path, flags, other_path);
		goto send_reply;
	}

	if (!strcmp(action, "get_storage_info")) {
		rc = hpss_get_storage_info(out_evb, (char *)path, flags);
		goto send_reply;
	}

	if (!strcmp(action, "set_uda")) {
		qs_param_buf = evhttp_find_header(&query_str_kvq, "key");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "key", qs_param_buf))
				goto send_reply;
			strncpy(param_buf, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter key missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		qs_param_buf = evhttp_find_header(&query_str_kvq, "value");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "value", qs_param_buf))
				goto send_reply;
			strncpy(param_buf_2, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter value missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		rc = hpss_set_uda(out_evb, (char *)path, flags, param_buf,
				  param_buf_2);
		goto send_reply;
	}

	if (!strcmp(action, "get_udas")) {
		rc = hpss_get_udas(out_evb, (char *)path, flags);
		goto send_reply;
	}

	if (!strcmp(action, "del_uda")) {
		qs_param_buf = evhttp_find_header(&query_str_kvq, "key");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "key", qs_param_buf))
				goto send_reply;
			strncpy(param_buf, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter key missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}

		rc = hpss_del_uda(out_evb, (char *)path, flags, param_buf);
		goto send_reply;
	}

	if (!strcmp(action, "get_to_proxy")) {

		qs_param_buf = evhttp_find_header(&query_str_kvq, "local_path");
		if (qs_param_buf) {
			if (check_qs_param(out_evb, "local_path", qs_param_buf))
				goto send_reply;
			strncpy(other_path, qs_param_buf, MAX_QS_VALUE_LEN);
		} else {
			evbuffer_add_printf(out_evb,
					    "{ \"errstr\" : \"parameter new_path missing\", \"errno\" : \"2\" ");
			goto send_reply;
		}
		rc = hpss_get_to_proxy(out_evb, (char *)path, flags,
				       other_path, mode_str);
		goto send_reply;
	}

	if (!strcmp(action, "get_to_client")) {

		rc = hpss_get_to_client(out_evb, (char *)path, flags);
                evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
                evhttp_send_reply(req, 200, "OK", out_evb);
                goto done;
	}
	/*
	 * if we reach this here, we got some unknown action 
	 */
	evhttp_send_error(req, HTTP_BADREQUEST, 0);

 send_reply:
	if (serverInfo.LogLevel <= LL_TRACE) {
		fprintf(serverInfo.LogFile, "Sending reply across the wire.\n");
	}
	evhttp_add_header(evhttp_request_get_output_headers(req),
			  "Content-Type", "application/json");
	evbuffer_add_printf(out_evb, "\n");
	evhttp_send_reply(req, 200, "OK", out_evb);
	goto done;
 err:
	evhttp_send_error(req, 404, "Document was not found");
 done:
	fflush(serverInfo.LogFile);
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (out_evb)
		evbuffer_free(out_evb);
}

  /*
   * ! \brief print out usage line
   */
void usage()
{
	fprintf(stderr,
		"Usage: hpss-http_proxy -u <username> -t <path to keytab> -m <krb5|unix> -p port [-l <numeric loglevel> -L <path to logfile>]\n");
	fprintf(stderr, "further options:\n");
	fprintf(stderr, "-U encode UTF-8\n");
	fprintf(stderr, "-a csv-list of allowed actions\n");
	fprintf(stderr, "-f csv-list of forbidden actions \n");
	fprintf(stderr, "-R disable modifying access (run in READONLY)\n");
	fprintf(stderr, "-P prefix to use for all UDAs (e.g. MPCDF)\n");
        fprintf(stderr, "available actions: %s\n", all_actions);
	return;
}

int main(int argc, char **argv)
{
	struct event_base *base;
	struct evhttp *http;
	struct evhttp_bound_socket *handle;

	unsigned short port = 0;
	int c;
	char *UserName;
	char *PathToKeytab = NULL;
	char *AuthMech = NULL;
	char *PathToLogFile = NULL;
	char *allowed_actions = NULL, *forbidden_actions = NULL;

        /*
         * setup defaults
         */
        serverInfo.LogLevel = LL_INFO;

	/*
	 * command line parsing 
	 */
	while ((c = getopt(argc, argv, "a:f:u:t:m:p:l:L:P:h?URs")) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			exit(0);
		case 'u':
			UserName = optarg;
			break;
		case 'U':
			serverInfo.do_encode = 1;
			break;
		case 't':
			PathToKeytab = optarg;
			break;
		case 'm':
			AuthMech = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'l':
			serverInfo.LogLevel = atoi(optarg);
			break;
		case 'L':
			PathToLogFile = optarg;
			break;
		case 'a':
			allowed_actions = optarg;
			break;
		case 'f':
			forbidden_actions = optarg;
			break;
		case 'R':
			forbidden_actions = rw_actions;
			break;
		case 's':
			fprintf(stdout,
				"hpss-http_proxy: available actions: %s\n",
				all_actions);
			exit(1);
			break;
		case 'P':
			serverInfo.uda_prefix = optarg;
			break;
		default:
			fprintf(stderr, "hpss-http_proxy: unkown option: %c\n",
				c);
			exit(1);
		}
	}

	if (port <= 0) {
		fprintf(stderr, "hpss-http_proxy: invalid or no port given.\n");
		usage();
		exit(1);
	}

	if (!UserName) {
		fprintf(stderr, "hpss-http_proxy: UserName required.\n");
		exit(1);
	}

	if (!PathToKeytab) {
		fprintf(stderr, "hpss-http_proxy: path to keytab required.\n");
		exit(1);
	}
	if (!AuthMech) {
		fprintf(stderr, "hpss-http_proxy: AuthMech (unix|krb5) required.\n");
		exit(1);
	}
	if (!PathToLogFile) {
		serverInfo.LogFile = stdout;
	} else {
		serverInfo.LogFile = fopen(PathToLogFile, "a");
		if (serverInfo.LogFile == NULL) {
			fprintf(stderr, "Cannot open logfile %s. Dying.\n",
				PathToLogFile);
			exit(errno);
		}
	}

	if (allowed_actions && forbidden_actions) {
		fprintf(stderr,
			"hpss-http_proxy: cannot set both allowed and forbidden action list\n");
		exit(1);
	}
	if (allowed_actions) {
		check_and_init_actions(&serverInfo.allowed_actions,
				       allowed_actions);
	}
	if (forbidden_actions) {
		check_and_init_actions(&serverInfo.forbidden_actions,
				       forbidden_actions);
	}
	if (!serverInfo.uda_prefix) {
		serverInfo.uda_prefix = malloc(sizeof(char) * 2);
		strcpy(serverInfo.uda_prefix, "");
	} else {
		if (serverInfo.uda_prefix[strlen(serverInfo.uda_prefix) - 1] ==
		    '/') {
			fprintf(stderr,
				"hpss-http_proxy: uda-prefix  must not end in a '/'.\n");
			exit(1);
		}
	}
	if (serverInfo.LogLevel <= LL_CRIT)
		fprintf(serverInfo.LogFile, "starting...\n");
	if (serverInfo.LogLevel <= LL_INFO) {
		fprintf(serverInfo.LogFile, "allowed_actions: %s\n",
			allowed_actions);
		fprintf(serverInfo.LogFile, "forbidden_actions: %s\n",
			forbidden_actions);
		fprintf(serverInfo.LogFile, "listening on port: %d\n",
			port);
	}
	fflush(serverInfo.LogFile);

	/*
	 * authenticate to HPSS 
	 */
	authenticate(UserName, PathToKeytab, AuthMech);

	/*
	 * startup libevent 
	 */

	base = event_base_new();
	if (!base) {
		fprintf(serverInfo.LogFile,
			"Couldn't create an event_base: exiting\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Create a new evhttp object to handle requests. 
	 */
	http = evhttp_new(base);
	if (!http) {
		fprintf(serverInfo.LogFile,
			"couldn't create evhttp. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	evhttp_set_gencb(http, dispatcher, NULL);

	/*
	 * Now we tell the evhttp what port to listen on 
	 */
	handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
	if (!handle) {
		fprintf(serverInfo.LogFile,
			"couldn't bind to port %d. Exiting.\n", (int)port);
		exit(EXIT_FAILURE);
	}

	event_base_dispatch(base);
	return 0;
}
